# encoding: utf-8
# 2009 © Václav Šmilauer <eudoxos@arcig.cz>
"""
Creating packings and filling volumes defined by boundary representation or constructive solid geometry.

For examples, see
	- scripts/test/gts-horse.py
	- scripts/test/gts-operators.py
	- scripts/test/gts-random-pack-obb.py
	- scripts/test/gts-random-pack.py
	- scripts/test/pack-cloud.py
	- scripts/test/pack-predicates.py
	- scripts/test/regular-sphere-pack.py
"""

import itertools,warnings
from numpy import arange
from math import sqrt
from yade import utils

from miniWm3Wrap import *
from yade import *

## compatibility hack for python 2.5 (21/8/2009)
## can be safely removed at some point
if 'product' not in dir(itertools):
	def product(*args, **kwds):
		"http://docs.python.org/library/itertools.html#itertools.product"
		pools = map(tuple, args) * kwds.get('repeat', 1); result = [[]]
		for pool in pools: result = [x+[y] for x in result for y in pool]
		for prod in result: yield tuple(prod)
	itertools.product=product

# for now skip the import, but try in inGtsSurface constructor again, to give error if we really use it
try:
	import gts
except ImportError: pass

# make c++ predicates available in this module
from _packPredicates import * ## imported in randomDensePack as well
# import SpherePack
from _packSpheres import *
from _packObb import *


class inGtsSurface_py(Predicate):
	"""This class was re-implemented in c++, but should stay here to serve as reference for implementing
	Predicates in pure python code. C++ allows us to play dirty tricks in GTS which are not accessible
	through pygts itself; the performance penalty of pygts comes from fact that if constructs and destructs
	bb tree for the surface at every invocation of gts.Point().is_inside(). That is cached in the c++ code,
	provided that the surface is not manipulated with during lifetime of the object (user's responsibility).

	---
	
	Predicate for GTS surfaces. Constructed using an already existing surfaces, which must be closed.

		import gts
		surf=gts.read(open('horse.gts'))
		inGtsSurface(surf)

	Note: padding is optionally supported by testing 6 points along the axes in the pad distance. This
	must be enabled in the ctor by saying doSlowPad=True. If it is not enabled and pad is not zero,
	warning is issued.
	"""
	def __init__(self,surf,noPad=False):
		# call base class ctor; necessary for virtual methods to work as expected.
		# see comments in _packPredicates.cpp for struct PredicateWrap.
		super(inGtsSurface,self).__init__()
		if not surf.is_closed(): raise RuntimeError("Surface for inGtsSurface predicate must be closed.")
		self.surf=surf
		self.noPad=noPad
		inf=float('inf')
		mn,mx=[inf,inf,inf],[-inf,-inf,-inf]
		for v in surf.vertices():
			c=v.coords()
			mn,mx=[min(mn[i],c[i]) for i in 0,1,2],[max(mx[i],c[i]) for i in 0,1,2]
		self.mn,self.mx=tuple(mn),tuple(mx)
		import gts
	def aabb(self): return self.mn,self.mx
	def __call__(self,_pt,pad=0.):
		p=gts.Point(*_pt)
		if self.noPad:
			if pad!=0: warnings.warn("Padding disabled in ctor, using 0 instead.")
			return p.is_inside(self.surf)
		pp=[gts.Point(_pt[0]-pad,_pt[1],_pt[2]),gts.Point(_pt[0]+pad,_pt[1],_pt[2]),gts.Point(_pt[0],_pt[1]-pad,_pt[2]),gts.Point(_pt[0],_pt[1]+pad,_pt[2]),gts.Point(_pt[0],_pt[1],_pt[2]-pad),gts.Point(_pt[0],_pt[1],_pt[2]+pad)]
		return p.is_inside(self.surf) and pp[0].is_inside(self.surf) and pp[1].is_inside(self.surf) and pp[2].is_inside(self.surf) and pp[3].is_inside(self.surf) and pp[4].is_inside(self.surf) and pp[5].is_inside(self.surf)

class inSpace(Predicate):
	"""Predicate returning True for any points, with infinite bounding box."""
	def __init__(self, _center=Vector3().ZERO): self._center=_center
	def aabb(self):
		inf=float('inf'); return Vector3(-inf,-inf,-inf),Vector3(inf,inf,inf)
	def center(self): return self._center
	def dim(self):
		inf=float('inf'); return Vector3(inf,inf,inf)
	def __call__(self,pt,pad): return True

#####
## surface construction and manipulation
#####

def gtsSurface2Facets(surf,**kw):
	"""Construct facets from given GTS surface. **kw is passed to utils.facet."""
	return [utils.facet([v.coords() for v in face.vertices()],**kw) for face in surf]

def sweptPolylines2gtsSurface(pts,threshold=0,capStart=False,capEnd=False):
	"""Create swept suface (as GTS triangulation) given same-length sequences of points (as 3-tuples).

	If threshold is given (>0), then

	* degenerate faces (with edges shorter than threshold) will not be created
	* gts.Surface().cleanup(threshold) will be called before returning, which merges vertices mutually
		closer than threshold. In case your pts are closed (last point concident with the first one)
		this will the surface strip of triangles. If you additionally have capStart==True and capEnd==True,
		the surface will be closed.

	Note: capStart and capEnd make the most naive polygon triangulation (diagonals) and will perhaps fail
	for non-convex sections.
	"""
	if not len(set([len(pts1) for pts1 in pts]))==1: raise RuntimeError("Polylines must be all of the same length!")
	vtxs=[[gts.Vertex(x,y,z) for x,y,z in pts1] for pts1 in pts]
	sectEdges=[[gts.Edge(vtx[i],vtx[i+1]) for i in xrange(0,len(vtx)-1)] for vtx in vtxs]
	interSectEdges=[[] for i in range(0,len(vtxs)-1)]
	for i in range(0,len(vtxs)-1):
		for j in range(0,len(vtxs[i])):
			interSectEdges[i].append(gts.Edge(vtxs[i][j],vtxs[i+1][j]))
			if j<len(vtxs[i])-1: interSectEdges[i].append(gts.Edge(vtxs[i][j],vtxs[i+1][j+1]))
	if threshold>0: # replace edges of zero length with None; their faces will be skipped
		def fixEdges(edges):
			for i,e in enumerate(edges):
				if (Vector3(e.v1.x,e.v1.y,e.v1.z)-Vector3(e.v2.x,e.v2.y,e.v2.z)).Length()<threshold: edges[i]=None
		for ee in sectEdges: fixEdges(ee)
		for ee in interSectEdges: fixEdges(ee)
	surf=gts.Surface()
	for i in range(0,len(vtxs)-1):
		for j in range(0,len(vtxs[i])-1):
			ee1=interSectEdges[i][2*j+1],sectEdges[i+1][j],interSectEdges[i][2*j]
			ee2=sectEdges[i][j],interSectEdges[i][2*j+2],interSectEdges[i][2*j+1]
			if None not in ee1: surf.add(gts.Face(interSectEdges[i][2*j+1],sectEdges[i+1][j],interSectEdges[i][2*j]))
			if None not in ee2: surf.add(gts.Face(sectEdges[i][j],interSectEdges[i][2*j+2],interSectEdges[i][2*j+1]))
	def doCap(vtx,edg,start):
		ret=[]
		eFan=[edg[0]]+[gts.Edge(vtx[i],vtx[0]) for i in range(2,len(vtx))]
		for i in range(1,len(edg)):
			ret+=[gts.Face(eFan[i-1],eFan[i],edg[i]) if start else gts.Face(eFan[i-1],edg[i],eFan[i])]
		return ret
	caps=[]
	if capStart: caps+=doCap(vtxs[0],sectEdges[0],start=True)
	if capEnd: caps+=doCap(vtxs[-1],sectEdges[-1],start=False)
	for cap in caps: surf.add(cap)
	if threshold>0: surf.cleanup(threshold)
	return surf

def gtsSurfaceBestFitOBB(surf):
	"""Return (Vector3 center, Vector3 halfSize, Quaternion orientation) describing
	best-fit oriented bounding box (OBB) for the given surface. See cloudBestFitOBB
	for details."""
	pts=[Vector3(v.x,v.y,v.z) for v in surf.vertices()]
	return cloudBestFitOBB(tuple(pts))

def revolutionSurfaceMeridians(sects,angles,origin=Vector3().ZERO,orientation=Quaternion().IDENTITY):
	"""Revolution surface given sequences of 2d points and sequence of corresponding angles,
	returning sequences of 3d points representing meridian sections of the revolution surface.
	The 2d sections are turned around z-axis, but they can be transformed
	using the origin and orientation arguments to give arbitrary orientation."""
	import math
	def toGlobal(x,y,z):
		return tuple(origin+orientation*(Vector3(x,y,z)))
	return [[toGlobal(x2d*math.cos(angles[i]),x2d*math.sin(angles[i]),y2d) for x2d,y2d in sects[i]] for i in range(0,len(sects))]

########
## packing generators
########


def regularOrtho(predicate,radius,gap,**kw):
	"""Return set of spheres in regular orthogonal grid, clipped inside solid given by predicate.
	Created spheres will have given radius and will be separated by gap space."""
	ret=[]
	mn,mx=predicate.aabb()
	if(max([mx[i]-mn[i] for i in 0,1,2])==float('inf')): raise ValueError("AABB of the predicate must not be infinite (didn't you use union | instead of intersection & for unbounded predicate such as notInNotch?");
	xx,yy,zz=[arange(mn[i]+radius,mx[i]-radius,2*radius+gap) for i in 0,1,2]
	for xyz in itertools.product(xx,yy,zz):
		if predicate(xyz,radius): ret+=[utils.sphere(xyz,radius=radius,**kw)]
	return ret

def regularHexa(predicate,radius,gap,**kw):
	"""Return set of spheres in regular hexagonal grid, clipped inside solid given by predicate.
	Created spheres will have given radius and will be separated by gap space."""
	ret=[]
	a=2*radius+gap
	h=a*sqrt(3)/2.
	mn,mx=predicate.aabb()
	dim=[mx[i]-mn[i] for i in 0,1,2]
	if(max(dim)==float('inf')): raise ValueError("AABB of the predicate must not be infinite (didn't you use union | instead of intersection & for unbounded predicate such as notInNotch?");
	ii,jj,kk=[range(0,int(dim[0]/a)+1),range(0,int(dim[1]/h)+1),range(0,int(dim[2]/h)+1)]
	for i,j,k in itertools.product(ii,jj,kk):
		x,y,z=mn[0]+radius+i*a,mn[1]+radius+j*h,mn[2]+radius+k*h
		if j%2==0: x+= a/2. if k%2==0 else -a/2.
		if k%2!=0: x+=a/2.; y+=h/2.
		if predicate((x,y,z),radius): ret+=[utils.sphere((x,y,z),radius=radius,**kw)]
	return ret

def filterSpherePack(predicate,spherePack,**kw):
	"""Using given SpherePack instance, return spheres the satisfy predicate.
	The packing will be recentered to match the predicate and warning is given if the predicate
	is larger than the packing."""
	mn,mx=predicate.aabb()
	dimP,centP=predicate.dim(),predicate.center()
	dimS,centS=spherePack.dim(),spherePack.center()
	if dimP[0]>dimS[0] or dimP[1]>dimS[1] or dimP[2]>dimS[2]: warnings.warn("Packing's dimension (%s) doesn't fully contain dimension of the predicate (%s)."%(dimS,dimP))
	spherePack.translate(centP-centS)
	ret=[]
	for s in spherePack:
		if predicate(s[0],s[1]): ret+=[utils.sphere(s[0],radius=s[1],**kw)]
	return ret

def randomDensePack(predicate,radius,material=0,dim=None,cropLayers=0,rRelFuzz=0.,spheresInCell=0,memoizeDb=None,useOBB=True,memoDbg=False):
	"""Generator of random dense packing with given geometry properties, using TriaxialTest (aperiodic)
	or PeriIsoCompressor (periodic). The priodicity depens on whether	the spheresInCell parameter is given.

	*O.switchScene()* magic is used to have clean simulation for TriaxialTest without deleting the original simulation.
	This function therefore should never run in parallel with some code accessing your simulation.

	:param predicate: solid-defining predicate for which we generate packing
	:param spheresInCell: if given, the packing will be periodic, with given number of spheres in the periodic cell.
	:param radius: mean radius of spheres
	:param rRelFuzz: relative fuzz of the radius -- e.g. radius=10, rRelFuzz=.2, then spheres will have radii 10 ± ½(10*.2)).
		0 by default, meaning all spheres will have exactly the same radius.
	:param cropLayers: (aperiodic only) how many layers of spheres will be added to the computed dimension of the box so that there no
		(or not so much, at least) boundary effects at the boundaries of the predicate.
	:param dim: dimension of the packing, to override dimensions of the predicate (if it is infinite, for instance)
	:param memoizeDb: name of sqlite database (existent or nonexistent) to find an already generated packing or to store
		the packing that will be generated, if not found (the technique of caching results of expensive computations
		is known as memoization). Fuzzy matching is used to select suitable candidate -- packing will be scaled, rRelFuzz
		and dimensions compared. Packing that are too small are dictarded. From the remaining candidate, the one with the
		least number spheres will be loaded and returned.
	:param useOBB: effective only if a inGtsSurface predicate is given. If true (default), oriented bounding box will be
		computed first; it can reduce substantially number of spheres for the triaxial compression (like 10× depending on
		how much asymmetric the body is), see scripts/test/gts-triax-pack-obb.py.
	:param memoDbg: show packigns that are considered and reasons why they are rejected/accepted

	:return: SpherePack object with spheres, filtered by the predicate.
	"""
	import sqlite3, os.path, cPickle, time, sys, _packPredicates
	from yade import log
	from math import pi
	wantPeri=(spheresInCell>0)
	if 'inGtsSurface' in dir(_packPredicates) and type(predicate)==inGtsSurface and useOBB:
		center,dim,orientation=gtsSurfaceBestFitOBB(predicate.surf)
		print "Best-fit oriented-bounding-box computed for GTS surface, orientation is",orientation
		dim*=2 # gtsSurfaceBestFitOBB returns halfSize
	else:
		if not dim: dim=predicate.dim()
		if max(dim)==float('inf'): raise RuntimeError("Infinite predicate and no dimension of packing requested.")
		center=predicate.center()
		orientation=None
	if not wantPeri: fullDim=tuple([dim[i]+4*cropLayers*radius for i in 0,1,2])
	else:
		# compute cell dimensions now, as they will be compared to ones stored in the db
		# they have to be adjusted to not make the cell to small WRT particle radius
		fullDim=dim
		cloudPorosity=0.25 # assume this number for the initial cloud (can be underestimated)
		beta,gamma=fullDim[1]/fullDim[0],fullDim[2]/fullDim[0] # ratios β=y₀/x₀, γ=z₀/x₀
		N100=spheresInCell/cloudPorosity # number of spheres for cell being filled by spheres without porosity
		x1=radius*(1/(beta*gamma)*N100*(4/3.)*pi)**(1/3.)
		y1,z1=beta*x1,gamma*x1; vol0=x1*y1*z1
		maxR=radius*(1+rRelFuzz)
		x1=max(x1,8*maxR); y1=max(y1,8*maxR); z1=max(z1,8*maxR); vol1=x1*y1*z1
		N100*=vol1/vol0 # volume might have been increased, increase number of spheres to keep porosity the same
	if(memoizeDb and os.path.exists(memoizeDb)):
		if memoDbg:
			def memoDbgMsg(s): print s
		else:
			def memoDbgMsg(s): pass
		# find suitable packing and return it directly
		conn=sqlite3.connect(memoizeDb); c=conn.cursor();
		try:
			c.execute('select radius,rRelFuzz,dimx,dimy,dimz,N,timestamp,periodic from packings order by N')
		except sqlite3.OperationalError:
			raise RuntimeError("ERROR: database `"+memoizeDb+"' not compatible with randomDensePack (corrupt, deprecated format or not a db created by randomDensePack)")
		for row in c:
			R,rDev,X,Y,Z,NN,timestamp,isPeri=row[0:8]; scale=radius/R
			rDev*=scale; X*=scale; Y*=scale; Z*=scale
			memoDbgMsg("Considering packing (radius=%g±%g,N=%g,dim=%g×%g×%g,%s,scale=%g), created %s"%(R,.5*rDev,NN,X,Y,Z,"periodic" if isPeri else "non-periodic",scale,time.asctime(time.gmtime(timestamp))))
			if (rRelFuzz==0 and rDev!=0) or (rRelFuzz==0 and rDev!=0) or (rRelFuzz!=0 and abs((rDev-rRelFuzz)/rRelFuzz)>1e-2): memoDbgMsg("REJECT: radius fuzz differs too much (%g, %g desired)"%(rDev,rRelFuzz)); continue # radius fuzz differs too much
			if isPeri and wantPeri:
				if spheresInCell>NN: memoDbgMsg("REJECT: Number of spheres in the packing too small"); continue
				if abs((y1/x1)/(Y/X)-1)>0.3 or abs((z1/x1)/(Z/X)-1)>0.3: memoDbgMsg("REJECT: proportions (y/x=%g, z/x=%g) differ too much from what is desired (%g, %g)."%(Y/X,Z/X,y1/x1,z1/x1)); continue
			else:
				if (X<fullDim[0] or Y<fullDim[1] or Z<fullDim[2]): memoDbgMsg("REJECT: not large enough"); continue # not large enough
			memoDbgMsg("ACCEPTED");
			print "Found suitable packing in %s (radius=%g±%g,N=%g,dim=%g×%g×%g,%s,scale=%g), created %s"%(memoizeDb,R,rDev,NN,X,Y,Z,"periodic" if isPeri else "non-periodic",scale,time.asctime(time.gmtime(timestamp)))
			c.execute('select pack from packings where timestamp=?',(timestamp,))
			sp=SpherePack(cPickle.loads(str(c.fetchone()[0])))
			if isPeri and wantPeri:
				sp.cellSize=(X,Y,Z); sp.cellFill(Vector3(fullDim[0],fullDim[1],fullDim[2])); sp.cellSize=(0,0,0) # resetting cellSize avoids warning when rotating
			sp.scale(scale);
			if orientation: sp.rotate(*orientation.ToAxisAngle())
			return filterSpherePack(predicate,sp,material=material)
		print "No suitable packing in database found, running",'PERIODIC compression' if wantPeri else 'triaxial'
		sys.stdout.flush()
	O.switchScene(); O.resetThisScene() ### !!
	if wantPeri:
		# x1,y1,z1 already computed above
		sp=SpherePack()
		O.periodicCell=((0,0,0),(x1,y1,z1))
		#print cloudPorosity,beta,gamma,N100,x1,y1,z1,O.periodicCell
		#print x1,y1,z1,radius,rRelFuzz
		num=sp.makeCloud(O.periodicCell[0],O.periodicCell[1],radius,rRelFuzz,spheresInCell,True)
		O.engines=[BexResetter(),BoundingVolumeMetaEngine([InteractingSphere2AABB()]),InsertionSortCollider(nBins=5,sweepLength=.05*radius),InteractionDispatchers([ef2_Sphere_Sphere_Dem3DofGeom()],[SimpleElasticRelationships()],[Law2_Dem3Dof_Elastic_Elastic()]),PeriIsoCompressor(charLen=radius/5.,stresses=[100e9,1e8],maxUnbalanced=1e-2,doneHook='O.pause();',globalUpdateInt=5,keepProportions=True),NewtonsDampedLaw(damping=.6)]
		O.materials.append(GranularMat(young=30e9,frictionAngle=.5,poisson=.3,density=1e3))
		for s in sp: O.bodies.append(utils.sphere(s[0],s[1]))
		O.dt=utils.PWaveTimeStep()
		O.run(); O.wait()
		sp=SpherePack(); sp.fromSimulation()
		#print 'Resulting cellSize',sp.cellSize,'proportions',sp.cellSize[1]/sp.cellSize[0],sp.cellSize[2]/sp.cellSize[0]
		# repetition to the required cell size will be done below, after memoizing the result
	else:
		assumedFinalDensity=0.6
		V=(4/3)*pi*radius**3; N=assumedFinalDensity*fullDim[0]*fullDim[1]*fullDim[2]/V;
		TriaxialTest(
			numberOfGrains=int(N),radiusMean=radius,radiusStdDev=rRelFuzz,
			# upperCorner is just size ratio, if radiusMean is specified
			upperCorner=fullDim,
			## no need to touch any the following
			noFiles=True,lowerCorner=[0,0,0],sigmaIsoCompaction=1e7,sigmaLateralConfinement=1e3,StabilityCriterion=.05,strainRate=.2,fast=True,thickness=-1,maxWallVelocity=.1,wallOversizeFactor=1.5,autoUnload=True,autoCompressionActivation=False).load()
		log.setLevel('TriaxialCompressionEngine',log.WARN)
		O.run(); O.wait()
		sp=SpherePack(); sp.fromSimulation()
	O.switchScene() ### !!
	if(memoizeDb):
		if os.path.exists(memoizeDb):
			conn=sqlite3.connect(memoizeDb)
		else:
			conn=sqlite3.connect(memoizeDb)
			c=conn.cursor()
			c.execute('create table packings (radius real, rRelFuzz real, dimx real, dimy real, dimz real, N integer, timestamp real, periodic integer, pack blob)')
		c=conn.cursor()
		packBlob=buffer(cPickle.dumps(sp.toList_pointsAsTuples(),cPickle.HIGHEST_PROTOCOL))
		packDim=sp.cellSize if wantPeri else fullDim
		c.execute('insert into packings values (?,?,?,?,?,?,?,?,?)',(radius,rRelFuzz,packDim[0],packDim[1],packDim[2],len(sp),time.time(),wantPeri,packBlob,))
		c.close()
		conn.commit()
		print "Packing saved to the database",memoizeDb
	if wantPeri: sp.cellFill(Vector3(fullDim[0],fullDim[1],fullDim[2]))
	if orientation:
		sp.cellSize=(0,0,0); # reset periodicity to avoid warning when rotating periodic packing
		sp.rotate(*orientation.ToAxisAngle())
	return filterSpherePack(predicate,sp,material=material)

# compatibility with the deprecated name, can be removed in the future
def triaxialPack(*args,**kw):
	import warnings; warnings.warn("pack.triaxialPack was renamed to pack.randomDensePack, update your code!",DeprecationWarning,stacklevel=2);
	return randomDensePack(*args,**kw)


