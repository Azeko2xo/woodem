# encoding: utf-8
# 2009 © Václav Šmilauer <eudoxos@arcig.cz>

"""
Creating packings and filling volumes defined by boundary representation or constructive solid geometry.


.. warning:: Broken links ahead:

For examples, see

* :woosrc:`scripts/test/gts-operators.py`
* :woosrc:`scripts/test/gts-random-pack-obb.py`
* :woosrc:`scripts/test/gts-random-pack.py`
* :woosrc:`scripts/test/pack-cloud.py`
* :woosrc:`scripts/test/pack-predicates.py`
* :woosrc:`examples/packs/packs.py`
* :woosrc:`examples/gts-horse/gts-horse.py`
* :woosrc:`examples/WireMatPM/wirepackings.py`
"""

import itertools,warnings,os
from numpy import arange
from math import sqrt
from woo import utils
import numpy

from minieigen import *
from woo import *
import woo
import woo.dem

def ShapePack_fromSimulation(sp,S):
	import warnings
	warnings.warn('ShapePack.fromSimulation is deprecated, use ShapePack.fromDem instead')
	return sp.fromDem(scene=S,dem=S.dem,mask=0,skipUnsupported=True)
woo.dem.ShapePack.fromSimulation=ShapePack_fromSimulation


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
try: import gts
except ImportError: pass

# make c++ predicates available in this module
from _packPredicates import * ## imported in randomDensePack as well
# import SpherePack
from _packSpheres import *
from _packObb import *
import woo
_docInlineModules=(woo._packPredicates,woo._packSpheres,woo._packObb)

##
# extend _packSphere.SpherePack c++ class by these methods
##
def SpherePack_fromSimulation(self,scene):
	ur"""Reset this SpherePack object and initialize it from the current simulation; only spherical particles are taken in account. Periodic boundary conditions are supported, but the hSize matrix must be diagonal. Clumps are supported."""
	self.reset()
	import woo.dem
	for p in scene.dem.par:
		if p.shape.__class__!=woo.dem.Sphere: continue
		if p.shape.nodes[0].dem.clumped: continue # those will be added as clumps in the next block
		self.add(p.pos,p.shape.radius)
	# handle clumps here
	clumpId=0
	for n in scene.dem.nodes:
		if not n.dem.clump: continue
		someOk=False # prevent adding only a part of the clump to the packing
		for nn in n.dem.nodes: # get particles belonging to clumped nodes
			for p in nn.dem.parRef:
				if p.shape.__class__!=woo.dem.Sphere:
					if someOk: raise RuntimError("A clump with mixed sphere/non-sphere shapes was encountered, which is not supported by SpherePack.fromSimulation");
					continue
				assert p.shape.nodes[0].dem.clumped
				self.add(p.pos,p.shape.radius,clumpId=clumpId),
				someOk=True
		clumpId+=1
	if scene.periodic:
		h=scene.cell.hSize
		if h-h.transpose()!=Matrix3.Zero: raise RuntimeError("Only box-shaped (no shear) periodic boundary conditions can be represented with a pack.SpherePack object")
		self.cellSize=h.diagonal()

SpherePack.fromSimulation=SpherePack_fromSimulation

def SpherePack_toSimulation(self,scene,rot=Matrix3.Identity,**kw):
	ur"""Append spheres directly to the simulation. In addition calling :obj:`woo.dem.BodyContainer.append`,
this method also appropriately sets periodic cell information of the simulation.

	>>> from woo import pack; from math import *; from woo.dem import *; from woo.core import *
	>>> sp=pack.SpherePack()

Create random periodic packing with 20 spheres:

	>>> sp.makeCloud((0,0,0),(5,5,5),rMean=.5,rRelFuzz=.5,periodic=True,num=20)
	20

Virgin simulation is aperiodic:

	>>> scene=Scene(fields=[DemField()])
	>>> scene.periodic
	False

Add generated packing to the simulation, rotated by 45° along +z

	>>> sp.toSimulation(scene,rot=Quaternion((0,0,1),pi/4),color=0)
	[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]

Periodic properties are transferred to the simulation correctly, including rotation:

	>>> scene.periodic
	True
	>>> scene.cell.size
	Vector3(5,5,5)
	>>> scene.cell.hSize
	Matrix3(3.5355339059327373,-3.5355339059327378,0, 3.5355339059327378,3.5355339059327373,0, 0,0,5)

The current state (even if rotated) is taken as mechanically undeformed, i.e. with identity transformation:

	>>> scene.cell.trsf
	Matrix3(1,0,0, 0,1,0, 0,0,1)

:param Quaternion/Matrix3 rot: rotation of the packing, which will be applied on spheres and will be used to set :obj:`woo.core.Cell.trsf` as well.
:param kw: passed to :obj:`woo.utils.sphere`
:return: list of body ids added (like :obj:`woo.dem.BodyContainer.append`)
"""
	if isinstance(rot,Quaternion): rot=rot.toRotationMatrix()
	assert(isinstance(rot,Matrix3))
	if self.cellSize!=Vector3.Zero:
		scene.periodic=True
		scene.cell.hSize=rot*Matrix3(self.cellSize[0],0,0, 0,self.cellSize[1],0, 0,0,self.cellSize[2])
		scene.cell.trsf=Matrix3.Identity

	from woo.dem import DemField
	if not self.hasClumps():
		if 'mat' not in kw.keys(): kw['mat']=utils.defaultMaterial()
		return scene.dem.par.append([utils.sphere(rot*c,r,**kw) for c,r in self])
	else:
		standalone,clumps=self.getClumps()
		# append standalone
		ids=scene.dem.par.append([utils.sphere(rot*self[i][0],self[i][1],**kw) for i in standalone])
		# append clumps
		clumpIds=[]
		for clump in clumps:
			clumpNode=scene.dem.par.appendClumped([utils.sphere(rot*(self[i][0]),self[i][1],**kw) for i in clump])
			# make all particles within one clump same color (as the first particle),
			# unless color was already user-specified
			clumpIds=[n.dem.parRef[0].id for n in clumpNode.dem.nodes] 
			if not 'color' in kw:
				c0=clumpNode.dem.nodes[0].dem.parRef[0].shape.color
				for n in clumpNode.dem.nodes[1:]: n.dem.parRef[0].shape.color=c0
		return ids+clumpIds

SpherePack.toSimulation=SpherePack_toSimulation

# in c++
SpherePack.filtered=SpherePack_filtered


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

	.. note::
		Padding is optionally supported by testing 6 points along the axes in the pad distance. This
		must be enabled in the ctor by saying doSlowPad=True. If it is not enabled and pad is not zero,
		warning is issued.
	"""
	def __init__(self,surf,noPad=False):
		# call base class ctor; necessary for virtual methods to work as expected.
		# see comments in _packPredicates.cpp for struct PredicateWrap.
		super(inGtsSurface_py,self).__init__()
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

# compiled without the local GTS module
if 'inGtsSurface' not in dir(): inGtsSurface=inGtsSurface_py

class inSpace(Predicate):
	"""Predicate returning True for any points, with infinite bounding box."""
	def __init__(self, _center=Vector3().Zero): self._center=_center
	def aabb(self):
		inf=float('inf'); return Vector3(-inf,-inf,-inf),Vector3(inf,inf,inf)
	def center(self): return self._center
	def dim(self):
		inf=float('inf'); return Vector3(inf,inf,inf)
	def __call__(self,pt,pad): return True

#####
## surface construction and manipulation
#####

def gtsSurface2Facets(surf,shareNodes=True,**kw):
	"""Construct facets from given GTS surface. ``**kw`` is passed to utils.facet."""
	import gts
	from woo.core import Node
	if not shareNodes:
		return [utils.facet([v.coords() for v in face.vertices()],**kw) for face in surf.faces()]
	else:
		nodesMap=dict([(v.id,Node(pos=v.coords())) for v in surf.vertices()])
		return [utils.facet([nodesMap[v.id] for v in face.vertices()],**kw) for face in surf.faces()]


def sweptPolylines2gtsSurface(pts,threshold=0,capStart=False,capEnd=False):
	"""Create swept suface (as GTS triangulation) given same-length sequences of points (as 3-tuples).

If threshold is given (>0), then

* degenerate faces (with edges shorter than threshold) will not be created
* gts.Surface().cleanup(threshold) will be called before returning, which merges vertices mutually closer than threshold. In case your pts are closed (last point concident with the first one) this will the surface strip of triangles. If you additionally have capStart==True and capEnd==True, the surface will be closed.

.. note:: capStart and capEnd make the most naive polygon triangulation (diagonals) and will perhaps fail for non-convex sections.

.. warning:: the algorithm connects points sequentially; if two polylines are mutually rotated or have inverse sense, the algorithm will not detect it and connect them regardless in their given order.
	"""
	import gts # will raise an exception in gts-less builds
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
				if (Vector3(e.v1.x,e.v1.y,e.v1.z)-Vector3(e.v2.x,e.v2.y,e.v2.z)).norm()<threshold: edges[i]=None
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
	import gts
	pts=[Vector3(v.x,v.y,v.z) for v in surf.vertices()]
	return cloudBestFitOBB(tuple(pts))

def revolutionSurfaceMeridians(sects,angles,origin=Vector3().Zero,orientation=Quaternion().Identity):
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
	if(max([mx[i]-mn[i] for i in 0,1,2])==float('inf')): raise ValueError("Aabb of the predicate must not be infinite (didn't you use union | instead of intersection & for unbounded predicate such as notInNotch?");
	xx,yy,zz=[arange(mn[i]+radius,mx[i]-radius,2*radius+gap) for i in 0,1,2]
	for xyz in itertools.product(xx,yy,zz):
		if predicate(xyz,radius): ret+=[utils.sphere(xyz,radius=radius,**kw)]
	return ret

def regularHexa(predicate,radius,gap,**kw):
	"""Return set of spheres in regular hexagonal grid, clipped inside solid given by predicate.
	Created spheres will have given radius and will be separated by gap space."""
	ret=[]
	a=2*radius+gap
	# thanks to Nasibeh Moradi for finding bug here:
	# http://www.mail-archive.com/woo-users@lists.launchpad.net/msg01424.html
	hy,hz=a*sqrt(3)/2.,a*sqrt(6)/3.
	mn,mx=predicate.aabb()
	dim=mx-mn
	if(max(dim)==float('inf')): raise ValueError("Aabb of the predicate must not be infinite (didn't you use union | instead of intersection & for unbounded predicate such as notInNotch?");
	ii,jj,kk=[range(0,int(dim[0]/a)+1),range(0,int(dim[1]/hy)+1),range(0,int(dim[2]/hz)+1)]
	for i,j,k in itertools.product(ii,jj,kk):
		x,y,z=mn[0]+radius+i*a,mn[1]+radius+j*hy,mn[2]+radius+k*hz
		if j%2==0: x+= a/2. if k%2==0 else -a/2.
		if k%2!=0: x+=a/2.; y+=hy/2.
		if predicate((x,y,z),radius): ret+=[utils.sphere((x,y,z),radius=radius,**kw)]
	return ret

def randomLoosePsd(predicate,psd,mass=True,discrete=False,maxAttempts=5000,clumps=[],returnSpherePack=False,**kw):
	'''Return loose packing based on given PSD.'''
	import woo.dem
	import woo.core
	import woo.log
	S=woo.core.Scene(fields=[woo.dem.DemField()])
	mn,mx=predicate.aabb()
	if not clumps: generator=woo.dem.PsdSphereGenerator(psdPts=psd,discrete=False,mass=True)
	else: generator=woo.dem.PsdClumpGenerator(psdPts=psd,discrete=False,mass=True,clumps=clumps)
	S.engines=[
		woo.dem.InsertionSortCollider([woo.dem.Bo1_Sphere_Aabb()]),
		woo.dem.BoxFactory(
			box=(mn,mx),
			maxMass=-1,
			maxNum=-1,
			massFlowRate=0,
			maxAttempts=maxAttempts,
			generator=generator,
			materials=[woo.dem.ElastMat(density=1)], # must have some density
			shooter=None,
			mask=1,
		)
	]
	S.one()
	if not returnSpherePack: raise ValueError('returnSpherePack must be True.')
	sp=SpherePack()
	sp.fromSimulation(S)
	return sp.filtered(predicate)
	#ret=[]
	#for p in S.dem.par:
	#	if predicate(p.pos,p.shape.radius): ret+=[utils.sphere(p.pos,radius=p.shape.radius,**kw)]
	#return ret


def filterSpherePack(predicate,spherePack,**kw):
	"""Using given SpherePack instance, return spheres the satisfy predicate.
	The packing will be recentered to match the predicate and warning is given if the predicate
	is larger than the packing."""
	return spherePack.filtered(predicate)

def _memoizePacking(memoizeDb,sp,radius,rRelFuzz,wantPeri,fullDim):
	if not memoizeDb: return
	import cPickle,sqlite3,time,os
	if os.path.exists(memoizeDb):
		conn=sqlite3.connect(memoizeDb)
	else:
		conn=sqlite3.connect(memoizeDb)
		c=conn.cursor()
		c.execute('create table packings (radius real, rRelFuzz real, dimx real, dimy real, dimz real, N integer, timestamp real, periodic integer, pack blob)')
	c=conn.cursor()
	packBlob=buffer(cPickle.dumps(sp.toList(),cPickle.HIGHEST_PROTOCOL))
	packDim=sp.cellSize if wantPeri else fullDim
	c.execute('insert into packings values (?,?,?,?,?,?,?,?,?)',(radius,rRelFuzz,packDim[0],packDim[1],packDim[2],len(sp),time.time(),wantPeri,packBlob,))
	c.close()
	conn.commit()
	print "Packing saved to the database",memoizeDb

def _getMemoizedPacking(memoizeDb,radius,rRelFuzz,x1,y1,z1,fullDim,wantPeri,fillPeriodic,spheresInCell,memoDbg=False):
	"""Return suitable SpherePack read from *memoizeDb* if found, None otherwise.
		
		:param fillPeriodic: whether to fill fullDim by repeating periodic packing
		:param wantPeri: only consider periodic packings
	"""
	import os,os.path,sqlite3,time,cPickle,sys
	if memoDbg:
		def memoDbgMsg(s): print s
	else:
		def memoDbgMsg(s): pass
	if not memoizeDb or not os.path.exists(memoizeDb):
		if memoizeDb: memoDbgMsg("Database %s does not exist."%memoizeDb)
		return None
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
		if not isPeri and wantPeri: memoDbgMsg("REJECT: is not periodic, which is requested."); continue
		if wantPeri and (X/x1>0.9 or X/x1<0.6):  memoDbgMsg("REJECT: initSize differs too much from scaled packing size."); continue
		if (rRelFuzz==0 and rDev!=0) or (rRelFuzz!=0 and rDev==0) or (rRelFuzz!=0 and abs((rDev-rRelFuzz)/rRelFuzz)>1e-2): memoDbgMsg("REJECT: radius fuzz differs too much (%g, %g desired)"%(rDev,rRelFuzz)); continue # radius fuzz differs too much
		if isPeri and wantPeri:
			if spheresInCell>NN and spheresInCell>0: memoDbgMsg("REJECT: Number of spheres in the packing too small"); continue
			if abs((y1/x1)/(Y/X)-1)>0.3 or abs((z1/x1)/(Z/X)-1)>0.3: memoDbgMsg("REJECT: proportions (y/x=%g, z/x=%g) differ too much from what is desired (%g, %g)."%(Y/X,Z/X,y1/x1,z1/x1)); continue
		else:
			if (X<fullDim[0] or Y<fullDim[1] or Z<fullDim[2]): memoDbgMsg("REJECT: not large enough"); continue # not large enough
		memoDbgMsg("ACCEPTED");
		print "Found suitable packing in %s (radius=%g±%g,N=%g,dim=%g×%g×%g,%s,scale=%g), created %s"%(memoizeDb,R,rDev,NN,X,Y,Z,"periodic" if isPeri else "non-periodic",scale,time.asctime(time.gmtime(timestamp)))
		c.execute('select pack from packings where timestamp=?',(timestamp,))
		sp=SpherePack(cPickle.loads(str(c.fetchone()[0])))
		sp.scale(scale);
		if isPeri and wantPeri:
			sp.cellSize=(X,Y,Z);
			if fillPeriodic: sp.cellFill(Vector3(fullDim[0],fullDim[1],fullDim[2]));
		#sp.cellSize=(0,0,0) # resetting cellSize avoids warning when rotating
		return sp
		#if orientation: sp.rotate(*orientation.toAxisAngle())
		#return filterSpherePack(predicate,sp,mat=mat)
	#print "No suitable packing in database found, running",'PERIODIC compression' if wantPeri else 'triaxial'
	#sys.stdout.flush()


def randomDensePack(predicate,radius,mat=-1,dim=None,cropLayers=0,rRelFuzz=0.,spheresInCell=0,memoizeDb=None,useOBB=True,memoDbg=False,color=None):
	"""Generator of random dense packing with given geometry properties, using TriaxialTest (aperiodic)
	or PeriIsoCompressor (periodic). The periodicity depens on whether	the spheresInCell parameter is given.

	*O.scene* is temporarily reassigned to have clean simulation for TriaxialTest without deleting the original simulation.
	This function therefore should never run in parallel with some code accessing your simulation.

	:param predicate: solid-defining predicate for which we generate packing
	:param spheresInCell: if given, the packing will be periodic, with given number of spheres in the periodic cell.
	:param radius: mean radius of spheres
	:param rRelFuzz: relative fuzz of the radius -- e.g. radius=10, rRelFuzz=.2, then spheres will have radii 10 ± (10*.2)).
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
	from woo import log, core, dem
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
		sp=_getMemoizedPacking(memoizeDb,radius,rRelFuzz,x1,y1,z1,fullDim,wantPeri,fillPeriodic=True,spheresInCell=spheresInCell,memoDbg=False)
		if sp:
			if orientation:
				sp.cellSize=(0,0,0) # resetting cellSize avoids warning when rotating
				sp.rotate(*orientation.toAxisAngle())
			return filterSpherePack(predicate,sp,mat=mat)
		else: print "No suitable packing in database found, running",'PERIODIC compression' if wantPeri else 'triaxial'
		sys.stdout.flush()
	S=core.Scene(fields=[dem.DemField()])
	if wantPeri:
		# x1,y1,z1 already computed above
		sp=SpherePack()
		S.periodic=True
		S.cell.setBox(x1,y1,z1)
		#print cloudPorosity,beta,gamma,N100,x1,y1,z1,S.cell.refSize
		#print x1,y1,z1,radius,rRelFuzz
		S.engines=[dem.ForceResetter(),dem.InsertionSortCollider([dem.Bo1_Sphere_Aabb()],verletDist=.05*radius),dem.ContactLoop([dem.Cg2_Sphere_Sphere_L6Geom()],[dem.Cp2_FrictMat_FrictPhys()],[dem.Law2_L6Geom_FrictPhys_IdealElPl()],applyForces=True),dem.Leapfrog(damping=.7,reset=False),dem.PeriIsoCompressor(charLen=2*radius,stresses=[-100e9,-1e8],maxUnbalanced=1e-2,doneHook='print "DONE"; S.stop();',globalUpdateInt=5,keepProportions=True,label='compressor')]
		num=sp.makeCloud(Vector3().Zero,S.cell.size0,radius,rRelFuzz,spheresInCell,True)
		mat=dem.FrictMat(young=30e9,tanPhi=.5,density=1e3,ktDivKn=.2)
		for s in sp: S.dem.par.append(utils.sphere(s[0],s[1],mat=mat))
		S.dem.collectNodes()
		S.dt=.5*utils.pWaveDt(S)
		S.run(); S.wait()
		sp=SpherePack(); sp.fromSimulation(S)
		#print 'Resulting cellSize',sp.cellSize,'proportions',sp.cellSize[1]/sp.cellSize[0],sp.cellSize[2]/sp.cellSize[0]
		# repetition to the required cell size will be done below, after memoizing the result
	else:
		raise RuntimError("Aperiodic compression not implemented.")
		assumedFinalDensity=0.6
		V=(4/3)*pi*radius**3; N=assumedFinalDensity*fullDim[0]*fullDim[1]*fullDim[2]/V;
		TriaxialTest(
			numberOfGrains=int(N),radiusMean=radius,radiusStdDev=rRelFuzz,
			# upperCorner is just size ratio, if radiusMean is specified
			upperCorner=fullDim,
			## no need to touch any the following
			noFiles=True,lowerCorner=[0,0,0],sigmaIsoCompaction=1e7,sigmaLateralConfinement=1e3,StabilityCriterion=.05,strainRate=.2,thickness=-1,maxWallVelocity=.1,wallOversizeFactor=1.5,autoUnload=True,autoCompressionActivation=False).load()
		log.setLevel('TriaxialCompressionEngine',log.WARN)
		S.run(); S.wait()
		sp=SpherePack(); sp.fromSimulation()
	_memoizePacking(memoizeDb,sp,radius,rRelFuzz,wantPeri,fullDim)
	if wantPeri: sp.cellFill(Vector3(fullDim[0],fullDim[1],fullDim[2]))
	if orientation:
		sp.cellSize=(0,0,0); # reset periodicity to avoid warning when rotating periodic packing
		sp.rotate(*orientation.toAxisAngle())
	return filterSpherePack(predicate,sp,mat=mat,color=color)

def randomPeriPack(radius,initSize,rRelFuzz=0.0,memoizeDb=None):
	"""Generate periodic dense packing.

	A cell of initSize is stuffed with as many spheres as possible, then we run periodic compression with PeriIsoCompressor, just like with randomDensePack.

	:param radius: mean sphere radius
	:param rRelFuzz: relative fuzz of sphere radius (equal distribution); see the same param for randomDensePack.
	:param initSize: initial size of the periodic cell.

	:return: SpherePack object, which also contains periodicity information.
	"""
	from math import pi
	from woo import core, dem
	sp=_getMemoizedPacking(memoizeDb,radius,rRelFuzz,initSize[0],initSize[1],initSize[2],fullDim=Vector3(0,0,0),wantPeri=True,fillPeriodic=False,spheresInCell=-1,memoDbg=True)
	if sp: return sp
	#oldScene=O.scene
	S=core.Scene(fields=[dem.DemField()])
	sp=SpherePack()
	S.periodic=True
	S.cell.setBox(initSize)
	sp.makeCloud(Vector3().Zero,S.cell.size0,radius,rRelFuzz,-1,True)
	from woo import log
	log.setLevel('PeriIsoCompressor',log.DEBUG)
	S.engines=[dem.ForceResetter(),dem.InsertionSortCollider([dem.Bo1_Sphere_Aabb()],verletDist=.05*radius),dem.ContactLoop([dem.Cg2_Sphere_Sphere_L6Geom()],[dem.Cp2_FrictMat_FrictPhys()],[dem.Law2_L6Geom_FrictPhys_IdealElPl()],applyForces=True),dem.PeriIsoCompressor(charLen=2*radius,stresses=[-100e9,-1e8],maxUnbalanced=1e-2,doneHook='print "done"; S.stop();',globalUpdateInt=20,keepProportions=True),dem.Leapfrog(damping=.8)]
	mat=dem.FrictMat(young=30e9,tanPhi=.1,ktDivKn=.3,density=1e3)
	for s in sp: S.dem.par.append(utils.sphere(s[0],s[1],mat=mat))
	S.dt=utils.pWaveDt(S)
	#O.timingEnabled=True
	S.run(); S.wait()
	ret=SpherePack()
	ret.fromSimulation(S)
	_memoizePacking(memoizeDb,ret,radius,rRelFuzz,wantPeri=True,fullDim=Vector3(0,0,0)) # fullDim unused
	return ret

def hexaNet( radius, cornerCoord=[0,0,0], xLength=1., yLength=0.5, mos=0.08, a=0.04, b=0.04, startAtCorner=True, isSymmetric=False, **kw ):
	"""Definition of the particles for a hexagonal wire net in the x-y-plane for the WireMatPM.

	:param radius: radius of the particle
	:param cornerCoord: coordinates of the lower left corner of the net
	:param xLenght: net length in x-direction
	:param yLenght: net length in y-direction
	:param mos: mesh opening size
	:param a: length of double-twist 
	:param b: height of single wire section
	:param startAtCorner: if true the generation starts with a double-twist at the lower left corner
	:param isSymmetric: defines if the net is symmetric with respect to the y-axis

	:return: set of spheres which defines the net (net) and exact dimensions of the net (lx,ly).
	
	note::
	This packing works for the WireMatPM only. The particles at the corner are always generated first.

	"""
	# check input dimension
	if(xLength<mos): raise ValueError("xLength must be greather than mos!");
	if(yLength<2*a+b): raise ValueError("yLength must be greather than 2*a+b!");
	xstart = cornerCoord[0]
	ystart = cornerCoord[1]
	z = cornerCoord[2]
	ab = a+b
	# number of double twisted sections in y-direction and real length ly
	ny = int( (yLength-a)/ab ) + 1
	ly = ny*a+(ny-1)*b
	jump=0
	# number of sections in x-direction and real length lx
	if isSymmetric:
		nx = int( xLength/mos ) + 1
		lx = (nx-1)*mos
		if not startAtCorner:
			nx+=-1
	else:
		nx = int( (xLength-0.5*mos)/mos ) + 1
		lx = (nx-1)*mos+0.5*mos
	net = []
	# generate corner particles
	if startAtCorner:
		if (ny%2==0): # if ny even no symmetry in y-direction
			net+=[utils.sphere((xstart,ystart+ly,z),radius=radius,**kw)] # upper left corner
			if isSymmetric:
				net+=[utils.sphere((xstart+lx,ystart+ly,z),radius=radius,**kw)] # upper right corner
			else:
				net+=[utils.sphere((xstart+lx,ystart,z),radius=radius,**kw)] # lower right corner
		else: # if ny odd symmetry in y-direction
			if not isSymmetric:
				net+=[utils.sphere((xstart+lx,ystart,z),radius=radius,**kw)] # lower right corner
				net+=[utils.sphere((xstart+lx,ystart+ly,z),radius=radius,**kw)] # upper right corner
		jump=1
	else: # do not start at corner
		if (ny%2==0): # if ny even no symmetry in y-direction
			net+=[utils.sphere((xstart,ystart,z),radius=radius,**kw)] # lower left corner
			if isSymmetric:
				net+=[utils.sphere((xstart+lx,ystart,z),radius=radius,**kw)] # lower right corner
			else:
				net+=[utils.sphere((xstart+lx,ystart+ly,z),radius=radius,**kw)] # upper right corner
		else: # if ny odd symmetry in y-direction
			net+=[utils.sphere((xstart,ystart,z),radius=radius,**kw)] # lower left corner
			net+=[utils.sphere((xstart,ystart+ly,z),radius=radius,**kw)] # upper left corner
			if isSymmetric:
				net+=[utils.sphere((xstart+lx,ystart,z),radius=radius,**kw)] # lower right corner
				net+=[utils.sphere((xstart+lx,ystart+ly,z),radius=radius,**kw)] # upper right corner
		xstart+=0.5*mos
	# generate other particles
	if isSymmetric:
		for i in range(ny):
			y = ystart + i*ab
			for j in range(nx):
				x = xstart + j*mos
				# add two particles of one vertical section (double-twist)
				net+=[utils.sphere((x,y,z),radius=radius,**kw)]
				net+=[utils.sphere((x,y+a,z),radius=radius,**kw)]
			# set values for next section
			xstart = xstart - 0.5*mos*pow(-1,i+jump)
			nx = int(nx + 1*pow(-1,i+jump))
	else:
		for i in range(ny):
			y = ystart + i*ab
			for j in range(nx):
				x = xstart + j*mos
				# add two particles of one vertical section (double-twist)
				net+=[utils.sphere((x,y,z),radius=radius,**kw)]
				net+=[utils.sphere((x,y+a,z),radius=radius,**kw)]
			# set values for next section
			xstart = xstart - 0.5*mos*pow(-1,i+jump)
	return [net,lx,ly]




def makePeriodicFeedPack(dim,psd,lenAxis=0,damping=.3,porosity=.5,goal=.15,maxNum=-1,dontBlock=False,returnSpherePack=False,memoizeDir=None,clumps=None,gen=None):
	if memoizeDir and not dontBlock:
		# increase number at the end for every change in the algorithm to make old feeds incompatible
		params=str(dim)+str(psd)+str(goal)+str(damping)+str(porosity)+str(lenAxis)+str(clumps)+('' if not gen else gen.dumps(format='expr'))+'5'
		import hashlib
		paramHash=hashlib.sha1(params).hexdigest()
		memoizeFile=memoizeDir+'/'+paramHash+'.perifeed'
		print 'Memoize file is ',memoizeFile
		if os.path.exists(memoizeDir+'/'+paramHash+'.perifeed'):
			print 'Returning memoized result'
			if not gen:
				sp=SpherePack()
				sp.load(memoizeFile)
				if returnSpherePack: return sp
				return zip(*sp)+[sp.cellSize[lenAxis],]
			else:
				import woo.dem
				sp=woo.dem.ShapePack(loadFrom=memoizeFile)
				return sp
	p3=porosity**(1/3.)
	if psd: rMax=psd[-1][0]
	elif hasattr(gen,'psdPts'): rMax=gen.psdPts[-1][0]
	else: raise NotImplementedError('Generators without PSD do not inform about the biggest particle radius.')
	minSize=rMax*5
	cellSize=Vector3(max(dim[0]/p3,minSize),max(dim[1]/p3,minSize),max(dim[2]/p3,minSize))
	print 'dimension',dim
	print 'initial cell size',cellSize
	print 'psd=',psd
	import woo.core, woo.dem, math
	S=woo.core.Scene(fields=[woo.dem.DemField()])
	S.periodic=True
	S.cell.setBox(cellSize)
	if gen: generator=gen
	elif not clumps: generator=woo.dem.PsdSphereGenerator(psdPts=psd,discrete=False,mass=True)
	else: generator=woo.dem.PsdClumpGenerator(psdPts=psd,discrete=False,mass=True,clumps=clumps)
	S.engines=[
		woo.dem.InsertionSortCollider([woo.dem.Bo1_Sphere_Aabb()]),
		woo.dem.BoxFactory(
			box=((0,0,0),cellSize),
			maxMass=-1,
			maxNum=maxNum,
			generator=generator,
			massFlowRate=0,
			maxAttempts=5000,
			materials=[woo.dem.FrictMat(density=1e3,young=1e7,ktDivKn=.2,tanPhi=math.tan(.5))],
			shooter=None,
			mask=1,
		)
	]
	S.one()
	print 'Created %d particles, compacting...'%(len(S.dem.par))
	S.dt=.9*utils.pWaveDt(S,noClumps=True)
	S.dtSafety=.9
	if clumps: warnings.warn('utils.pWaveDt called with noClumps=True (clumps ignored), the result (S.dt=%g) might be significantly off!'%S.dt)
	S.engines=[
		woo.dem.PeriIsoCompressor(charLen=2*rMax,stresses=[-1e8,-1e6],maxUnbalanced=goal,doneHook='print "done"; S.stop();',globalUpdateInt=1,keepProportions=True,label='peri'),
		# plots only useful for debugging - uncomment if needed
		# woo.core.PyRunner(100,'S.plot.addData(i=S.step,unb=S.lab.peri.currUnbalanced,sig=S.lab.peri.sigma)'),
		woo.core.PyRunner(100,'print S.lab.peri.stresses[S.lab.peri.state], S.lab.peri.sigma, S.lab.peri.currUnbalanced'),
	]+utils.defaultEngines(damping=damping,dynDtPeriod=100)
	S.plot.plots={'i':('unb'),' i':('sig_x','sig_y','sig_z')}
	if dontBlock: return S
	S.run(); S.wait()
	if gen: sp=woo.dem.ShapePack()
	else: sp=SpherePack()
	sp.fromSimulation(S)
	print 'Packing size is',sp.cellSize
	sp.canonicalize()
	if not gen: sp.makeOverlapFree()
	print 'Loose packing size is',sp.cellSize
	cc,rr=[],[]
	inf=float('inf')
	boxMin=Vector3(0,0,0);
	boxMax=Vector3(dim)
	boxMin[lenAxis]=-inf
	boxMax[lenAxis]=inf
	box=AlignedBox3(boxMin,boxMax)
	sp2=sp.filtered(inAlignedBox(box))
	print 'Box is ',box
	#for c,r in sp:
	#	if c-Vector3(r,r,r) not in box or c+Vector3(r,r,r) not in box: continue
	#	cc.append(c); rr.append(r)
	if memoizeDir or returnSpherePack or gen:
		#sp2=SpherePack()
		#sp2.fromList(cc,rr)
		#sp2.cellSize=sp.cellSize
		if memoizeDir:
			print 'Saving to',memoizeFile
			# print len(sp2)
			sp2.save(memoizeFile)
		if returnSpherePack or gen:
			return sp2
	cc,rr=sp2.toCcRr()
	return cc,rr,sp2.cellSize[lenAxis]




def makeBandFeedPack(dim,psd,mat,gravity,excessWd=None,damping=.3,porosity=.5,goal=.15,dontBlock=False,memoizeDir=None,botLine=None,leftLine=None,rightLine=None,clumps=[],returnSpherePack=False,useEnergy=True,gen=None):
	'''Create dense packing periodic in the +x direction, suitable for use with ConveyorFactory.
:param useEnergy: use :obj:`woo.utils.unbalancedEnergy` instead of :obj:`woo.utils.unbalancedForce` as stop criterion.
:param goal: target unbalanced force/energy; if unbalanced energy is used, this value is **multiplied by .2**.
:param psd: particle size distribution
:param mat: material for particles
:param gravity: gravity acceleration (as Vector3)
'''
	print 'woo.pack.makeBandFeedPack(dim=%s,psd=%s,mat=%s,gravity=%s,excessWd=%s,damping=%s,dontBlock=True,botLine=%s,leftLine=%s,rightLine=%s,clumps=%s,gen=%s)'%(repr(dim),repr(psd),mat.dumps(format='expr',width=-1,noMagic=True),repr(gravity),repr(excessWd),repr(damping),repr(botLine),repr(leftLine),repr(rightLine),repr(clumps),repr(gen))
	dim=list(dim) # make modifiable in case of excess width



	retWd=dim[1]
	nRepeatCells=0 # if 0, repetition is disabled
	# too wide band is created by repeating narrower one
	if excessWd:
		if dim[1]>excessWd[0]:
			print 'makeBandFeedPack: excess with %g>%g, using %g with packing repeated'%(dim[1],excessWd[0],excessWd[1])
			retWd=dim[1] # this var is used at the end
			dim[1]=excessWd[1]
			nRepeatCells=int(retWd/dim[1])+1
	cellSize=(dim[0],dim[1],(1+2*porosity)*dim[2])
	print 'cell size',cellSize,'target height',dim[2]
	factoryBottom=.3*cellSize[2] if not botLine else max([b[1] for b in botLine]) # point above which are particles generated
	factoryLeft=0 if not leftLine else max([l[0] for l in leftLine])
	#print 'factoryLeft =',factoryLeft,'leftLine =',leftLine,'cellSize =',cellSize
	factoryRight=cellSize[1] if not rightLine else min([r[0] for r in rightLine])
	if not leftLine: leftLine=[Vector2(0,cellSize[2])]
	if not rightLine:rightLine=[Vector2(cellSize[1],cellSize[2])]
	if not botLine:  botLine=[Vector2(0,0),Vector2(cellSize[1],0)]
	boundary2d=leftLine+botLine+rightLine
	# boundary clipped to the part filled by particles
	b2c=[Vector2(pt[0],min(pt[1],dim[2])) for pt in boundary2d]
	print 'Clipped boundary',b2c
	b2c+=[b2c[0],b2c[1]] # close the polygon
	area2d=.5*abs(sum([b2c[i][0]*(b2c[i+1][1]-b2c[i-1][1]) for i in range(1,len(b2c)-1)]))

	def printBulkParams(sp):
		volume=sp.solidVolume() # works with both ShapePack and SpherePack
		mass=volume*mat.density
		print 'Particle mass: %g kg (volume %g m3, mass density %g kg/m3).'%(mass,volume,mat.density)
		vol=area2d*sp.cellSize[0]
		print 'Bulk density: %g kg/m3 (area %g m2, length %g m).'%(mass/vol,area2d,sp.cellSize[0])
		print 'Porosity: %g %%'%(100*(1-(mass/vol)/mat.density))

	if memoizeDir and not dontBlock:
		params=str(dim)+str(cellSize)+str(psd)+str(goal)+str(damping)+mat.dumps(format='expr')+str(gravity)+str(porosity)+str(botLine)+str(leftLine)+str(rightLine)+str(clumps)+str(useEnergy)+(gen.dumps(format='expr') if gen else '')+'ver5'
		import hashlib
		paramHash=hashlib.sha1(params).hexdigest()
		memoizeFile=memoizeDir+'/'+paramHash+'.bandfeed'
		print 'Memoize file is ',memoizeFile
		if os.path.exists(memoizeDir+'/'+paramHash+'.bandfeed'):
			print 'Returning memoized result'
			if not gen:
				sp=SpherePack()
				sp.load(memoizeFile)
				printBulkParams(sp)
				if returnSpherePack: return sp
				return zip(*sp)
			else:
				import woo.dem
				sp=woo.dem.ShapePack(loadFrom=memoizeFile)
				printBulkParams(sp)
				return sp


	import woo, woo.core, woo.dem
	S=woo.core.Scene(fields=[woo.dem.DemField(gravity=gravity)])
	S.periodic=True
	S.cell.setBox(cellSize)
	# add limiting surface
	p=sweptPolylines2gtsSurface([utils.tesselatePolyline([Vector3(x,yz[0],yz[1]) for yz in boundary2d],maxDist=min(cellSize[0]/4.,cellSize[1]/4.,cellSize[2]/4.)) for x in numpy.linspace(0,cellSize[0],num=4)])
	S.dem.par.append(gtsSurface2Facets(p,mask=0b011))
	S.dem.loneMask=0b010


	massToDo=porosity*mat.density*dim[0]*dim[1]*dim[2]
	print 'Will generate %g mass'%massToDo

	## FIXME: decrease friction angle to help stabilization
	mat0,mat=mat,mat.deepcopy()
	mat.tanPhi=min(.2,mat0.tanPhi)

	if gen: generator=gen
	elif not clumps: generator=woo.dem.PsdSphereGenerator(psdPts=psd,discrete=False,mass=True)
	else: generator=woo.dem.PsdClumpGenerator(psdPts=psd,discrete=False,mass=True,clumps=clumps)
	
	# todo: move trackEnergy under useEnergy once we don't need comparisons
	S.trackEnergy=True
	if useEnergy:
		unbalancedFunc='woo.utils.unbalancedEnergy'
		# smaller goal for energy criterion
		goal*=.2
	else:
		unbalancedFunc='woo.utils.unbalancedForce'
	S.engines=utils.defaultEngines(damping=damping,dynDtPeriod=100)+[
		woo.dem.BoxFactory(
			box=((.01*cellSize[0],factoryLeft,factoryBottom),(cellSize[0],factoryRight,cellSize[2])),
			stepPeriod=200,
			maxMass=massToDo,
			massFlowRate=0,
			maxAttempts=20,
			generator=generator,
			materials=[mat],
			shooter=woo.dem.AlignedMinMaxShooter(dir=(0,0,-1),vRange=(0,0)),
			mask=1,
			label='factory',
			#periSpanMask=1, # x is periodic
		),
		#PyRunner(200,'plot.addData(uf=utils.unbalancedForce(),i=O.scene.step)'),
		# woo.core.PyRunner(300,'import woo\nprint "%g/%g mass, %d particles, unbalanced '+('energy' if useEnergy else 'force')+'%g/'+str(goal)+'"%(S.lab.factory.mass,S.lab.factory.maxMass,len(S.dem.par),'+unabalncedFunc+'(S))'),
		woo.core.PyRunner(300,'import woo\nprint "%g/%g mass, %d particles, unbalanced F: %g E: %g /'+str(goal)+'"%(S.lab.factory.mass,S.lab.factory.maxMass,len(S.dem.par),woo.utils.unbalancedForce(S),woo.utils.unbalancedEnergy(S))'),
		woo.core.PyRunner(300,'import woo\nif S.lab.factory.mass>=S.lab.factory.maxMass: S.engines[0].damping=1.5*%g'%damping),
		woo.core.PyRunner(200,'import woo\nif '+unbalancedFunc+'(S)<'+str(goal)+' and S.lab.factory.dead: S.stop()'),
	]
	# S.dt=.7*utils.spherePWaveDt(psd[0][0],mat.density,mat.young)
	S.dtSafety=.9
	print 'Factory box is',S.lab.factory.box
	S.dem.collectNodes()
	if dontBlock: return S
	else: S.run()
	S.wait()
	
	if gen: sp=woo.dem.ShapePack()
	else: sp=SpherePack()
	sp.fromSimulation(S)
	sp.canonicalize()
	# remove what is above the requested height
	sp=sp.filtered(woo.pack.inAxisRange(axis=2,range=(0,dim[2])),recenter=False)
	printBulkParams(sp)
	if nRepeatCells:
		print 'nRepeatCells',nRepeatCells
		sp.cellRepeat(Vector3i(1,nRepeatCells,1))
		sp.translate(Vector3(0,-dim[1]*.5*nRepeatCells,0))
		sp=sp.filtered(woo.pack.inAxisRange(axis=1,range=(-retWd/2.,retWd/2.)),recenter=False)
	else: sp.translate(Vector3(0,-.5*dim[1],0))
	# only periodic along the x-axis
	sp.cellSize[1]=sp.cellSize[2]=0
	if memoizeDir:
		print 'Saving to',memoizeFile
		sp.saveTxt(memoizeFile)
	if returnSpherePack or gen: return sp
	return zip(*sp)


##
## an attempt at a better randomDensePack
## currently ShapePack does not support moving, or the interface is not clear, so it just waits here

#	def randomDensePack2(predicate,generator,memoizeDir=None,debug=False):
#		box=predicate.aabb()
#		if memoizeDir:
#			import hashlib
#			hash=hashlib.sha1('1'+str(box)+generator.dumps(format='expr')).hexdigest()
#			memo=memoizeDir+'/'+hash+'.randomdense'
#			print 'Memoize file is',memo
#			if os.path.exists(memo):
#				print 'Returning memoized result'
#				import woo.pack
#				return woo.pack.ShapePack(loadFrom=memo).filtered(predicate)
#		S=woo.core.Scene(fields=[woo.dem.DemField()])
#		S.dtSafety=.9
#		S.periodic=True
#		S.cell.setBox(box.sizes())
#		S.engines=[	
#			woo.dem.InsertionSortCollider(list(woo.system.childClasses(woo.dem.BoundFunctor))),
#			woo.dem.BoxFactory(box=((0,0,0),box.sizes()),maxMass=-1,maxNum=-1,generator=generator,massFlowRate=0,maxAttempts=5000,materials=[woo.dem.FrictMat(density=1e3,young=1e7,ktDivKn=0,tanPhi=0)],shooter=None,mask=1)
#		]
#		S.one()
#		print 'Created %d particles, compacting...'%len(S.dem.par)
#		minRad=float('inf')
#		for p in S.dem.par:
#			r=p.shape.equivRadius()
#			if not isnan(r): minRad=min(minRad,r)
#		S.engines=[woo.dem.PeriIsoCompressor(charLen=2*minRad,stresses=[-1e8,-1e6],maxUnbalanced=goal,doneHook='print "done"; S.stop()',globalUpdateInt=1,keepProportions=True,label='peri'),woo.core.PyRunner(100,'print S>lab.peri.stresses[S.lab.peri.state], S.lab.peri.sigma, S.lab.peri.currUnbalanced')]+utils.defaultEngines(damping=.3)
#		# S.plot.plots={'i':('unb'),' i':('sig_x','sig_y','sig_z')}
#		if debug: return S
#		S.run(); S.wait()
#		sp=woo.pack.ShapePack()
#		sp.fromDem(S,S.dem)
#		print 'Compacted packing size is',sp.cellSize
#		sp.canonicalize()
#		if memoizeDir:
#			print 'saving to',memo
#			sp.save(memo)
#		return sp



