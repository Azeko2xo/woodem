# encoding: utf-8
#
# utility functions for woo
#
# 2008-2009 © Václav Šmilauer <eudoxos@arcig.cz>

"""Heap of functions that don't (yet) fit anywhere else.
"""

import math,random,doctest,geom
import woo
import sys,os
from woo import *
from minieigen import *

from woo.dem import *
from woo.core import *

# c++ implementations for performance reasons
#from woo._utils import *
from woo._utils2 import *


# declare that _utils2 should be documented here
_docInlineModules=(woo._utils2,)

inf=float('inf')

def saveVars(mark='',loadNow=True,**kw):
	"""Save passed variables into the simulation so that it can be recovered when the simulation is loaded again.

	For example, variables *a*, *b* and *c* are defined. To save them, use::

		>>> from woo import utils
		>>> utils.saveVars('something',a=1,b=2,c=3)
		>>> from woo.params.something import *
		>>> a,b,c
		(1, 2, 3)

	those variables will be save in the .xml file, when the simulation itself is saved. To recover those variables once the .xml is loaded again, use

		>>> utils.loadVars('something')

	and they will be defined in the woo.params.\ *mark* module. The *loadNow* parameter calls :ref:`woo.utils.loadVars` after saving automatically.
	"""
	import cPickle
	woo.master.scene.tags['pickledPythonVariablesDictionary'+mark]=cPickle.dumps(kw)
	if loadNow: loadVars(mark)

def loadVars(mark=None):
	"""Load variables from :ref:`woo.utils.saveVars`, which are saved inside the simulation.
	If ``mark==None``, all save variables are loaded. Otherwise only those with
	the mark passed."""
	import cPickle, types, sys, warnings
	scene=woo.master.scene
	def loadOne(d,mark=None):
		"""Load given dictionary into a synthesized module woo.params.name (or woo.params if *name* is not given). Update woo.params.__all__ as well."""
		import woo.params
		if mark:
			if mark in woo.params.__dict__: warnings.warn('Overwriting woo.params.%s which already exists.'%mark)
			modName='woo.params.'+mark
			mod=types.ModuleType(modName)
			mod.__dict__.update(d)
			mod.__all__=list(d.keys()) # otherwise params starting with underscore would not be imported
			sys.modules[modName]=mod
			woo.params.__all__.append(mark)
			woo.params.__dict__[mark]=mod
		else:
			woo.params.__all__+=list(d.keys())
			woo.params.__dict__.update(d)
	if mark!=None:
		d=cPickle.loads(str(scene.tags['pickledPythonVariablesDictionary'+mark]))
		loadOne(d,mark)
	else: # load everything one by one
		for m in scene.tags.keys():
			if m.startswith('pickledPythonVariablesDictionary'):
				loadVars(m[len('pickledPythonVariableDictionary')+1:])

def spherePWaveDt(radius,density,young):
	r"""Compute P-wave critical timestep for a single (presumably representative) sphere, using formula for P-Wave propagation speed $\Delta t_{c}=\frac{r}{\sqrt{E/\rho}}$.
	If you want to compute minimum critical timestep for all spheres in the simulation, use :ref:`woo.utils.PWaveTimeStep` instead.

	>>> spherePWaveDt(1e-3,2400,30e9)
	2.8284271247461903e-07
	"""
	from math import sqrt
	return radius/sqrt(young/density)

#def randomColor():
#	"""Return random Vector3 with each component in interval 0…1 (uniform distribution)"""
#	return Vector3(random.random(),random.random(),random.random())

def defaultMaterial():
	"""Return default material, when creating bodies with :ref:`woo.utils.sphere` and friends, material is unspecified and there is no previous particle yet. By default, this function returns::

		FrictMat(density=1e3,young=1e7,ktDivKn=.2,tanPhi=tan(.5))
	"""
	import math
	return FrictMat(density=1e3,young=1e7,ktDivKn=.2,tanPhi=math.tan(.5))

def defaultEngines(damping=0.,gravity=None,verletDist=-.05,kinSplit=False,dontCollect=False,noSlip=False,noBreak=False,cp2=None,law=None,grid=False):
	"""Return default set of engines, suitable for basic simulations during testing."""
	if gravity: raise ValueError("gravity MUST NOT be specified anymore, set DemField.gravity=... instead.")
	if not grid: collider=InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb(),Bo1_Wall_Aabb(),Bo1_InfCylinder_Aabb()],label='collider',verletDist=verletDist)
	else: collider=GridCollider([Grid1_Sphere(),Grid1_Facet(),Grid1_Wall(),Grid1_InfCylinder()],label='collider',verletDist=verletDist)
	return [
		Leapfrog(damping=damping,reset=True,kinSplit=kinSplit,dontCollect=dontCollect),
		collider,
		ContactLoop(
			[Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom(),Cg2_Wall_Sphere_L6Geom(),Cg2_InfCylinder_Sphere_L6Geom()],
			[cp2 if cp2 else Cp2_FrictMat_FrictPhys()],
			[law if law else Law2_L6Geom_FrictPhys_IdealElPl(noSlip=noSlip,noBreak=noBreak)],applyForces=True,label='contactLoop'
		),
	]

def _commonBodySetup(b,nodes,volumes,geomInertias,mat,masses=None,fixed=False):
	"""Assign common body parameters."""
	if isinstance(mat,Material): b.mat=mat
	elif callable(mat): b.mat=mat()
	else: raise TypeError("The 'mat' argument must be Material instance, or a callable returning Material.");
	if masses and volumes: raise ValueError("Only one of masses, volumes can be given")
	if volumes:
		if len(nodes)!=len(volumes) or len(volumes)!=len(geomInertias): raise ValueError("nodes, volumes (or masses) and geomInertias must have same lengths.")
		masses=[v*b.mat.density for v in volumes]
	if len(nodes)!=len(masses) or len(masses)!=len(geomInertias): raise ValueError("nodes, volumes (or masses) and geomInertias must have same lengths.")
	b.shape.nodes=nodes
	for i in range(0,len(nodes)):
		b.nodes[i].dem.mass=masses[i]
		b.nodes[i].dem.inertia=geomInertias[i]*b.mat.density
	for i,n in enumerate(b.nodes):
		n.dem.addParRef(b) # increase particle count
		if fixed: n.dem.blocked='xyzXYZ'
		else: n.dem.blocked=''.join(['XYZ'[ax] for ax in (0,1,2) if geomInertias[i][ax]==inf]) # block rotational DOFs where inertia is infinite


def _mkDemNode(**kw):
	'''Helper function to create new Node instance, with dem set to an new empty instance.
	dem can't be assigned directly in the ctor, since it is not a c++ attribute :-| '''
	n=Node(**kw); n.dem=DemData() 
	return n

def sphere(center,radius,mat=defaultMaterial,fixed=False,wire=False,color=None,highlight=False,mask=1):
	"""Create sphere with given parameters; mass and inertia computed automatically.

	:param Vector3 center: center
	:param float radius: radius
	:param float-or-None: particle's color as float; random color will be assigned if ``None`.
	:param mat:
		specify :ref:`Particle.material`; different types are accepted:
			* :ref:`Material` instance: this instance will be used
			* callable: will be called without arguments; returned Material value will be used (Material factory object, if you like)
	:param int mask: :ref:`dem.Particle.mask` for the body

	:return:
		Particle instance with desired characteristics.

	Instance of material can be given::

		>>> from woo import utils
		>>> s1=utils.sphere((0,0,0),1,wire=False,color=.7,mat=ElastMat(young=30e9,density=2e3))
		>>> s1.shape.wire
		False
		>>> s1.shape.color
		0.7
		>>> s1.mat.density
		2000.0

	Finally, material can be a callable object (taking no arguments), which returns a Material instance.
	Use this if you don't call this function directly (for instance, through woo.pack.randomDensePack), passing
	only 1 *material* parameter, but you don't want material to be shared.

	For instance, randomized material properties can be created like this:

		>>> import random
		>>> def matFactory(): return ElastMat(young=1e10*random.random(),density=1e3+1e3*random.random())
		...
		>>> s2=utils.sphere([0,2,0],1,mat=matFactory)
		>>> s3=utils.sphere([1,2,0],1,mat=matFactory)

	"""
	b=Particle()
	b.shape=Sphere(radius=radius,color=color if color else random.random())
	b.shape.wire=wire
	V=(4./3)*math.pi*radius**3
	geomInert=(2./5.)*V*radius**2
	_commonBodySetup(b,([center] if isinstance(center,Node) else [_mkDemNode(pos=center),]),volumes=[V],geomInertias=[geomInert*Vector3.Ones],mat=mat,fixed=fixed)
	b.aspherical=False
	b.mask=mask
	return b

def wall(position,axis,sense=0,glAB=None,fixed=True,mass=0,color=None,mat=defaultMaterial,visible=True,mask=1):
	"""Return ready-made wall body.

	:param float-or-Vector3-or-Node position: center of the wall. If float, it is the position along given axis, the other 2 components being zero
	:param ∈{0,1,2} axis: orientation of the wall normal (0,1,2) for x,y,z (sc. planes yz, xz, xy)
	:param ∈{-1,0,1} sense: sense in which to interact (0: both, -1: negative, +1: positive; see :ref:`Wall`)

	See :ref:`woo.utils.sphere`'s documentation for meaning of other parameters."""
	p=Particle()
	p.shape=Wall(sense=sense,axis=axis,color=color if color else random.random())
	if glAB: p.shape.glAB=glAB
	if not fixed and mass<=0: raise ValueError("Non-fixed wall must have positive mass")
	if isinstance(position,(int,long,float)):
		pos2=Vector3(0,0,0); pos2[axis]=position
		node=_mkDemNode(pos=pos2)
	elif isinstance(position,Node):
		node=position
	else:
		node=_mkDemNode(pos=position)
	_commonBodySetup(p,[node],volumes=None,masses=[mass],geomInertias=[inf*Vector3.Ones],mat=mat,fixed=fixed)
	p.aspherical=False # wall never rotates anyway
	p.mask=mask
	p.shape.visible=visible
	return p

def facet(vertices,fakeVel=None,halfThick=0.,fixed=True,wire=True,color=None,highlight=False,mat=defaultMaterial,visible=True,mask=1,flex=False):
	"""Create facet with given parameters.

	:param [Vector3,Vector3,Vector3] vertices: coordinates of vertices in the global coordinate system.
	:param bool wire: if ``True``, facets are shown as skeleton; otherwise facets are filled

	See :ref:`woo.utils.sphere`'s documentation for meaning of other parameters."""
	p=Particle()
	nodes=[]
	if isinstance(vertices[0],Node):
		nodes=vertices
		for n in nodes:
			if not n.dem: n.dem=DemData()
	else:
		nodes=[_mkDemNode(pos=vertices[0]),_mkDemNode(pos=vertices[1]),_mkDemNode(pos=vertices[2])]
	p.shape=(FlexFacet if flex else Facet)(color=color if color else random.random(),halfThick=halfThick)
	if fakeVel: p.shape.fakeVel=fakeVel
	p.shape.wire=wire
	_commonBodySetup(p,nodes,volumes=None,masses=(0,0,0),geomInertias=[Vector3(0,0,0),Vector3(0,0,0),Vector3(0,0,0)],mat=mat,fixed=fixed)
	p.aspherical=False # mass and inertia are 0 anyway; fell free to change to ``True`` if needed
	p.mask=mask
	p.shape.visible=visible
	return p

def infCylinder(position,radius,axis,glAB=None,fixed=True,mass=0,color=None,wire=False,mat=defaultMaterial,mask=1):
	"""Return a read-made infinite cylinder particle."""
	p=Particle()
	p.shape=InfCylinder(radius=radius,axis=axis,color=color if color else random.random())
	if glAB: p.shape.glAB=glAB
	p.shape.wire=wire
	if(isinstance(position,Node)):
		node=position
	else: node=_mkDemNode(pos=position)
	_commonBodySetup(p,[node],volumes=None,masses=[mass],geomInertias=[inf*Vector3.Ones],mat=mat,fixed=fixed)
	p.aspherical=False # only rotates along one axis
	p.mask=mask
	return p

def aabbWalls(extrema=None,thickness=None,oversizeFactor=1.5,**kw):
	"""Return 6 boxes that will wrap existing packing as walls from all sides;
	extrema are extremal points of the Aabb of the packing (will be calculated if not specified)
	thickness is wall thickness (will be 1/10 of the X-dimension if not specified)
	Walls will be enlarged in their plane by oversizeFactor.
	returns list of 6 wall Bodies enclosing the packing, in the order minX,maxX,minY,maxY,minZ,maxZ.
	"""
	walls=[]
	if not extrema: extrema=aabbExtrema()
	if not thickness: thickness=(extrema[1][0]-extrema[0][0])/10.
	for axis in [0,1,2]:
		mi,ma=extrema
		center=[(mi[i]+ma[i])/2. for i in range(3)]
		extents=[.5*oversizeFactor*(ma[i]-mi[i]) for i in range(3)]
		extents[axis]=thickness/2.
		for j in [0,1]:
			center[axis]=extrema[j][axis]+(j-.5)*thickness
			walls.append(box(center=center,extents=extents,fixed=True,**kw))
			walls[-1].shape.wire=True
	return walls


def aabbDim(cutoff=0.,centers=False):
	"""Return dimensions of the axis-aligned bounding box, optionally with relative part *cutoff* cut away."""
	a=aabbExtrema(cutoff,centers)
	return (a[1][0]-a[0][0],a[1][1]-a[0][1],a[1][2]-a[0][2])

def aabbExtrema2d(pts):
	"""Return 2d bounding box for a sequence of 2-tuples."""
	inf=float('inf')
	min,max=[inf,inf],[-inf,-inf]
	for pt in pts:
		if pt[0]<min[0]: min[0]=pt[0]
		elif pt[0]>max[0]: max[0]=pt[0]
		if pt[1]<min[1]: min[1]=pt[1]
		elif pt[1]>max[1]: max[1]=pt[1]
	return tuple(min),tuple(max)

def perpendicularArea(axis):
	"""Return area perpendicular to given axis (0=x,1=y,2=z) generated by bodies
	for which the function consider returns True (defaults to returning True always)
	and which is of the type :ref:`Sphere`.
	"""
	ext=aabbExtrema()
	other=((axis+1)%3,(axis+2)%3)
	return (ext[1][other[0]]-ext[0][other[0]])*(ext[1][other[1]]-ext[0][other[1]])

def fractionalBox(fraction=1.,minMax=None):
	"""retrurn (min,max) that is the original minMax box (or aabb of the whole simulation if not specified)
	linearly scaled around its center to the fraction factor"""
	if not minMax: minMax=aabbExtrema()
	half=[.5*(minMax[1][i]-minMax[0][i]) for i in [0,1,2]]
	return (tuple([minMax[0][i]+(1-fraction)*half[i] for i in [0,1,2]]),tuple([minMax[1][i]-(1-fraction)*half[i] for i in [0,1,2]]))


def randomizeColors(onlyDynamic=False):
	"""Assign random colors to :ref:`Shape::color`.

	If onlyDynamic is true, only dynamic bodies will have the color changed.
	"""
	for b in O.bodies:
		color=(random.random(),random.random(),random.random())
		if b.dynamic or not onlyDynamic: b.shape.color=color

def avgNumInteractions(cutoff=0.,skipFree=False):
	r"""Return average numer of interactions per particle, also known as *coordination number* $Z$. This number is defined as

	.. math:: Z=2C/N

	where $C$ is number of contacts and $N$ is number of particles.

	With *skipFree*, particles not contributing to stable state of the packing are skipped, following equation (8) given in [Thornton2000]_:

	.. math:: Z_m=\frac{2C-N_1}{N-N_0-N_1}

	:param cutoff: cut some relative part of the sample's bounding box away.
	:param skipFree: see above.
	
"""
	if cutoff==0 and not skipFree: return 2*O.interactions.countReal()*1./len(O.bodies)
	else:
		nums,counts=bodyNumInteractionsHistogram(aabbExtrema(cutoff))
		## CC is 2*C
		CC=sum([nums[i]*counts[i] for i in range(len(nums))]); N=sum(counts)
		if not skipFree: return CC*1./N
		## find bins with 0 and 1 spheres
		N0=0 if (0 not in nums) else counts[nums.index(0)]
		N1=0 if (1 not in nums) else counts[nums.index(1)]
		NN=N-N0-N1
		return (CC-N1)*1./NN if NN>0 else float('nan')


def plotNumInteractionsHistogram(cutoff=0.):
	"Plot histogram with number of interactions per body, optionally cutting away *cutoff* relative axis-aligned box from specimen margin."
	nums,counts=bodyNumInteractionsHistogram(aabbExtrema(cutoff))
	import pylab
	pylab.bar(nums,counts)
	pylab.title('Number of interactions histogram, average %g (cutoff=%g)'%(avgNumInteractions(cutoff),cutoff))
	pylab.xlabel('Number of interactions')
	pylab.ylabel('Body count')
	pylab.show()

def plotDirections(aabb=(),mask=0,bins=20,numHist=True,noShow=False):
	"""Plot 3 histograms for distribution of interaction directions, in yz,xz and xy planes and
	(optional but default) histogram of number of interactions per body.

	:returns: If *noShow* is ``False``, displays the figure and returns nothing. If *noShow*, the figure object is returned without being displayed (works the same way as :ref:`woo.plot.plot`).
	"""
	import pylab,math
	from woo import utils
	for axis in [0,1,2]:
		d=utils.interactionAnglesHistogram(axis,mask=mask,bins=bins,aabb=aabb)
		fc=[0,0,0]; fc[axis]=1.
		subp=pylab.subplot(220+axis+1,polar=True);
		# 1.1 makes small gaps between values (but the column is a bit decentered)
		pylab.bar(d[0],d[1],width=math.pi/(1.1*bins),fc=fc,alpha=.7,label=['yz','xz','xy'][axis])
		#pylab.title(['yz','xz','xy'][axis]+' plane')
		pylab.text(.5,.25,['yz','xz','xy'][axis],horizontalalignment='center',verticalalignment='center',transform=subp.transAxes,fontsize='xx-large')
	if numHist:
		pylab.subplot(224,polar=False)
		nums,counts=utils.bodyNumInteractionsHistogram(aabb if len(aabb)>0 else utils.aabbExtrema())
		avg=sum([nums[i]*counts[i] for i in range(len(nums))])/(1.*sum(counts))
		pylab.bar(nums,counts,fc=[1,1,0],alpha=.7,align='center')
		pylab.xlabel('Interactions per body (avg. %g)'%avg)
		pylab.axvline(x=avg,linewidth=3,color='r')
		pylab.ylabel('Body count')
	if noShow: return pylab.gcf()
	else: pylab.show()


def makeVideo(frameSpec,out,renameNotOverwrite=True,fps=24,kbps=6000,bps=None):
	"""Create a video from external image files using `mencoder <http://www.mplayerhq.hu>`__. Two-pass encoding using the default mencoder codec (mpeg4) is performed, running multi-threaded with number of threads equal to number of OpenMP threads allocated for Yade.

	:param frameSpec: wildcard | sequence of filenames. If list or tuple, filenames to be encoded in given order; otherwise wildcard understood by mencoder's mf:// URI option (shell wildcards such as ``/tmp/snap-*.png`` or and printf-style pattern like ``/tmp/snap-%05d.png``)
	:param str out: file to save video into
	:param bool renameNotOverwrite: if True, existing same-named video file will have -*number* appended; will be overwritten otherwise.
	:param int fps: Frames per second (``-mf fps=…``)
	:param int kbps: Bitrate (``-lavcopts vbitrate=…``) in kb/s
	"""
	import os,os.path,subprocess,warnings
	if bps!=None:
		warnings.warn('plot.makeVideo: bps is deprecated, use kbps instead (the significance is the same, but the name is more precise)',stacklevel=2,category=DeprecationWarning)
		kbps=bps
	if renameNotOverwrite and os.path.exists(out):
		i=0
		while(os.path.exists(out+"~%d"%i)): i+=1
		os.rename(out,out+"~%d"%i); print "Output file `%s' already existed, old file renamed to `%s'"%(out,out+"~%d"%i)
	if isinstance(frameSpec,list) or isinstance(frameSpec,tuple): frameSpec=','.join(frameSpec)

	devNull='nul' if (sys.platform=='win32') else '/dev/null'
	# mencoder normally creates divx2pass.log in the current dir, which might fail
	# use temp file instead, explicitly
	passLogFile=woo.master.tmpFilename()+'.log'
	for passNo in (1,2):
		cmd=['mencoder','mf://%s'%frameSpec,'-mf','fps=%d'%int(fps),'-ovc','lavc','-lavcopts','vbitrate=%d:vpass=%d:threads=%d:%s'%(int(kbps),passNo,woo.master.numThreads,'turbo' if passNo==1 else ''),'-passlogfile',passLogFile,'-o',(devNull if passNo==1 else out)]
		print 'Pass %d:'%passNo,' '.join(cmd)
		ret=subprocess.call(cmd)
		if ret!=0: raise RuntimeError("Error when running mencoder.")

def _procStatus(name):
	import os
	for l in open('/proc/%d/status'%os.getpid()):
		if l.split(':')[0]==name: return l
	raise "No such line in /proc/[pid]/status: "+name
def vmData():
	"Return memory usage data from Linux's /proc/[pid]/status, line VmData."
	l=_procStatus('VmData'); ll=l.split(); assert(ll[2]=='kB')
	return int(ll[1])

def trackPerfomance(updateTime=5):
	"""
	Track perfomance of a simulation. (Experimental)
	Will create new thread to produce some plots.
	Useful for track perfomance of long run simulations (in bath mode for example).
	"""

	def __track_perfomance(updateTime):
		pid=os.getpid()
		threadsCpu={}
		lastTime,lastIter=-1,-1
		while 1:
			time.sleep(updateTime)
			if not O.running: 
				lastTime,lastIter=-1,-1
				continue
			if lastTime==-1: 
				lastTime=time.time();lastIter=O.iter
				plot.plots.update({'Iteration':('Perfomance',None,'Bodies','Interactions')})
				continue
			curTime=time.time();curIter=O.iter
			perf=(curIter-lastIter)/(curTime-lastTime)
			out=subprocess.Popen(['top','-bH','-n1', ''.join(['-p',str(pid)])],stdout=subprocess.PIPE).communicate()[0].splitlines()
			for s in out[7:-1]:
				w=s.split()
				threadsCpu[w[0]]=float(w[8])
			plot.addData(Iteration=curIter,Iter=curIter,Perfomance=perf,Bodies=len(O.bodies),Interactions=len(O.interactions),**threadsCpu)
			plot.plots.update({'Iter':threadsCpu.keys()})
			lastTime=time.time();lastIter=O.iter

	thread.start_new_thread(__track_perfomance,(updateTime))



if sys.platform=='win32':
	def fixWindowsPath(p):
		'''On Windows, replace some well-known UNIX directories by their native equivalents,
		so that defaults works cross-platform. Currently, the following substitutions are made:
		1. leading `/tmp` is transformed to `%TEMP%` (or c:/temp is undefined)
		'''
		if p=='/tmp' or p.startswith('/tmp/'):
			temp=os.environ.get('TEMP','c:/temp')
			return temp+p[4:]
		return p
else:
	def fixWindowsPath(p): return p
	
	


def xMirror(half):
	"""Mirror a sequence of 2d points around the x axis (changing sign on the y coord).
The sequence should start up and then it will wrap from y downwards (or vice versa).
If the last point's x coord is zero, it will not be duplicated."""
	return list(half)+[(x,-y) for x,y in reversed(half[:-1] if half[-1][0]==0 else half)]

def tesselatePolyline(l,maxDist):
	'''Return polyline tesselated so that distance between consecutive points is no more than maxDist. *l* is list of points (Vector2 or Vector3).'''
	ret=[]
	for i in range(len(l)-1):
		x0,x1=l[i],l[i+1]
		dX=x1-x0
		dist=dX.norm()
		if dist>maxDist:
			nDiv=int(dist/maxDist+(1 if int(dist/maxDist)*maxDist<dist else 0))
			for j in range(int(nDiv)): ret.append(x0+dX*(j*1./nDiv))
		else: ret.append(x0)
	ret.append(l[-1])
	return ret


def htmlReportHead(S,headline,dialect='xhtml',repBase=''):
	'''
		Return (X)HTML fragment for simulation report: (X)HTML header, title, Woo logo, configuration and preprocessor parameters. 

		In order to obtain a well-formed XHTML document, don't forget to add '</body></html>'.
	'''
	import time, platform, pkg_resources, shutil
	headers=dict(xhtml='''<?xml version="1.0" encoding="UTF-8"?>
	<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"> 
	<html xmlns="http://www.w3.org/1999/xhtml" xmlns:svg="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">\n''',html5='<!DOCTYPE html><html lang="en">',html4='<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd"><html>')
	logoSrc=pkg_resources.resource_filename('woo','data/woo-icon.128.png')
	logoExtern='%s_wooLogo.png'%(repBase)
	logoBase64='<img src="data:image/png;base64,'+open(logoSrc).read().encode('base64')+'" alt="Woo logo"/>'
	logoImgExtern='<img src="%s_wooLogo.png" alt="Woo logo"/>'%(repBase)
	svgLogos=dict(
		# embedded svg
		xhtml=svgFileFragment(pkg_resources.resource_filename('woo','data/woodem-6.woo.svg')),
		# embedded base64-encoded pngs
		html5=logoBase64,
		html4=logoImgExtern,
	)
	if dialect not in headers: raise ValueError("Unsupported HTML dialect '%s'; must be one of: "+', '.join(headers.keys()))
	if dialect=='html4': shutil.copyfile(logoSrc,logoExtern)
	# <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
	html=(u'''<head><title>{headline}</title><meta charset="utf-8"/></head><body>
		<h1>{headline}</h1>
		<h2>General</h2>
		<table style="text-align: center">
			<tr><td rowspan="3">{svgLogo}</td><td><a href="http://www.woodem.eu/">Woo[dem]</a></td></tr>
			<tr><td>© <a href="mailto:vaclav.smilauer@woodem.eu">Václav Šmilauer</a></td></tr>
			<tr><td>Licensed under <a href="http://opensource.org/licenses/gpl-2.0.php">GNU GPL v2</a></td></tr>
		</table>
		<table>
			<tr><td>title</td><td align="right">{title}</td></tr>
			<tr><td>id</td><td align="right">{id}</td></tr>
			<tr><td>started</td><td align="right">{started}</td></tr>
			<tr><td>duration</td><td align="right">{duration:g} s ({step} steps)</td></tr>
			<tr><td>average speed</td><td align="right">{stepsPerSec:g} steps/sec</td></tr>
			<tr><td>operator</td><td align="right">{user}</td></tr>
			<tr><td>number of cores used</td><td align="right">{nCores}</td></tr>
			<tr><td>engine</td><td align="right">{engine}</td></tr>
			<tr><td>compiled with</td><td align="right">{compiledWith}</td></tr>
			<tr><td>platform</td><td align="right">{platform}</td></tr>
		</table>
		'''.format(headline=headline,title=(S.tags['title'] if S.tags['title'] else '<i>[none]</i>'),id=S.tags['id'],user=S.tags['user'].decode('utf-8'),started=time.ctime(time.time()-woo.master.realtime),step=S.step,duration=woo.master.realtime,nCores=woo.master.numThreads,stepsPerSec=S.step/woo.master.realtime,engine='wooDem '+woo.config.version+'/'+woo.config.revision+(' (debug)' if woo.config.debug else ''),compiledWith=','.join(woo.config.features),platform=platform.platform().replace('-',' '),svgLogo=svgLogos[dialect])
		+'<h2>Input data</h2>'+S.pre.dumps(format='html',fragment=True,showDoc=False)
	)
	return headers[dialect]+html

def htmlReport(S,repFmt,headline,afterHead='',figures=[],dialect=None,figFmt=None,svgEmbed=False,show=False):
	'''
	Generate (X)HTML report for simulation.

	:param S: :obj:`Scene` object; must contain :obj:`Scene.pre`
	:param repFmt: format for the output file; will be expanded using :obj:`Scene.tags`; it should end with `.html` or `.xhtml`.
	:param headline: title of the report, used as HTML title and the first `<h1>` heading.
	:param afterHead: contents (XHTML fragment) to be added verbatim after the header (title, woo config table, preprocessor parameters)
	:param figs: figures included in the report; they are either embedded (SVG, optionally ) or saved to files in the same directory as the report, using its name as prefix, and referenced via relative links from the XHTML. Figures are given as list of 2- or 3-tuples, where each item contains:
	   
		 1. name of the figure (will generate a `<h2>` heading)
		 2. figure object (`Matplotlib <http://matplotlib.org>` figure object)
		 3. optionally, format to save the figure to (`svg`, `png`, ...)

	:param figFmt: format of figures, if the format is not specified by the figure itself; if None, choose format appropriate for given dialect (png for html4, svg for html5 and xhtml)
	:param dialect: one of "xhtml", "html5", "html4"; if not given (*None*), selected from file suffix (``.html`` for ``html4``, ``.xhtml`` for ``xhtml``). ``html4`` will save figures as PNG (even if something else is specified) so that the resulting file is easily importable into LibreOffice (which does not import HTML with SVG correctly now).
	:param svgEmbed: don't save SVG as separate files, embed them in the report instead. 
	:param show: open the report in browser via the webbrowser module, unless running in batch.
	:return: (filename of the report, list of external figures)

	'''
	import codecs, re, os.path, warnings
	import woo,woo.batch
	dialects=set(['html5','html4','xhtml'])
	if not dialect:
		if repFmt.endswith('.xhtml'): dialect='xhtml'
		elif repFmt.endswith('.html'): dialect='html4'
		else: raise ValueError("Unable to guess dialect (not given) from *repFmt* '%s' (not ending in .xhtml or .html)")
	if dialect not in dialects: raise ValueError("Unknown dialect '%s': must be one of "+', '.join(dialects)+'.')
	if figFmt==None: figFmt={'html5':'png','html4':'png','xhtml':'svg'}[dialect]
	if dialect in ('html5',) and svgEmbed and figFmt=='svg':
		svgEmbed=False
		warnings.warn("Dialect '%s' does not support embedded SVG -- will not be embedded."%dialect)
	if dialect in ('html4',) and not figFmt=='png':
		warnings.warn("Dialect '%s' will save images as 'png', not '%s'"%(dialect,figFmt))
		figFmt='png'
	repName=unicode(repFmt).format(S=S,**(dict(S.tags)))
	rep=codecs.open(repName,'w','utf-8','replace')
	print 'Writing report to file://'+os.path.abspath(repName)
	repBase=re.sub('\.x?html$','',repName)
	s=htmlReportHead(S,headline,dialect=dialect,repBase=repBase)
	s+=afterHead


	if figures: s+='<h2>Figures</h2>\n'
	extFiles=[]
	from matplotlib.backends.backend_agg import FigureCanvasAgg
	for ith,figspec in enumerate(figures):
		if len(figspec)==2: figspec=(figspec[0],figspec[1],figFmt)
		figName,figObj,figSuffix=figspec
		if not figName: figName='Figure %i'%(ith+1)
		s+='<h3>'+figName+'</h3>'
		canvas=FigureCanvasAgg(figObj)
		if figSuffix=='svg' and svgEmbed:
			figFile=woo.master.tmpFilename()+'.'+figSuffix
			figObj.savefig(figFile)
			s+=svgFileFragment(figFile)
		else: 
			figFile=repBase+'.'+re.sub('[^a-zA-Z0-9_-]','_',figName)+'.'+figSuffix
			figObj.savefig(figFile)
			s+='<img src="%s" alt="%s"/>'%(os.path.basename(figFile),figName)
			extFiles.append(os.path.abspath(figFile))
	s+='</body></html>'
	rep.write(s)
	rep.close() # flushed write buffers

	# attempt conversion to ODT
	if 1:
		def convertToOdt(html,odt):
			try:
				import subprocess
				# may raise OSError if the executable is not found
				ret=subprocess.call(['abiword','--to=ODT','--to-name='+odt,html])
				if ret!=0: raise RuntimeError('Conversion failed')
				print 'Report converted to ODT via Abiword: file://'+os.path.abspath(odt)
			except:
				print 'Report conversion to ODT failed (Abiword not installed?), ignoring.'
		# if this is run in the background, main process may finish before that thread, leading potentially to some error messages?? run in foreground
		if 0: convertToOdt(repName,repBase+'.odt') 
		else:
			import threading
			threading.Thread(target=convertToOdt,args=(repName,repBase+'.odt')).start()
				
	if show and not woo.batch.inBatch():
		import webbrowser
		webbrowser.open('file://'+os.path.abspath(repName))
	return repName,extFiles


def svgFileFragment(filename):
	'Return fragment beginning with ``<svg`` till the end of given *filename*. Those data may be used for embedding SVG in other formats, such as XHTML.'
	import codecs,re
	data=codecs.open(filename,encoding='utf-8').read()
	# returns the first match
	for m in re.finditer(r'<svg( |$)',data,flags=re.MULTILINE): return data[m.start():]
	raise RuntimeError('"<svg" was not found in %s'%filename)
	


def ensureWriteableDir(d):
	'Make sure given directory is writeable; raise exception (IOError) if not.'
	import errno, os
	try:
		os.makedirs(d)
		print 'Created directory:',d
	except OSError as e:
		if e.errno!=errno.EEXISTS:
			print 'ERROR: failed to create directory '+d
			raise
	if not os.access(d,os.W_OK): raise IOError('Directory %s not writeable.'%d)


def runAllPreprocessors():
	'Run all preprocessors with default parameters, and run the very first step of their simulation.'
	for P in woo.system.childClasses(woo.core.Preprocessor):
		print 50*'#'+'\n'+10*' '+P.__module__+'.'+P.__name__+'\n'+50*'#'
		S=woo.master.scene=P()()
		S.one
	print 20*'*'+'   ALL OK   '+20*'*'

def unbalancedEnergy(S):
	if not S.trackEnergy: raise RuntimError('Scene.trackEnergy==False')
	if not 'elast' in S.energy or 'kinetic' not in S.energy: return float('nan')
	#raise RuntimeError('Scene.energy: does not contain *elast* or *kinetic* keys')
	# this line seems to be necessary under Windows, zero raises ZeroDivisionError...?!
	if S.energy['elast']==0: return float('nan')
	return S.energy['kinetic']/S.energy['elast']


#############################
##### deprecated functions


def _deprecatedUtilsFunction(old,new):
	"Wrapper for deprecated functions, example below."
	import warnings
	warnings.warn('Function utils.%s is deprecated, use %s instead.'%(old,new),stacklevel=2,category=DeprecationWarning)

# example of _deprecatedUtilsFunction usage:
#
# def import_mesh_geometry(*args,**kw):
#    "|ydeprecated|"
#    _deprecatedUtilsFunction('import_mesh_geometry','woo.import.gmsh')
#    import woo.ymport
#    return woo.ymport.stl(*args,**kw)

import woo.batch

def runningInBatch():
	_deprecatedUtilsFunction('runningInBatch','woo.batch.inBatch')
	return woo.batch.inBatch()
def waitIfBatch():
	_deprecatedUtilsFunction('waitIfBatch','woo.batch.wait')
	return woo.batch.wait()
def readParamsFromTable(*args,**kw):
	_deprecatedUtilsFunction('readParamsFromTable','woo.batch.readParamsFromTable')
	return woo.batch.readParamsFromTable(*args,**kw)
def runPreprocessorInBatch(*args,**kw):
	_deprecatedUtilsFunction('runPreprocessorInBatch','woo.batch.runPreprocessor')
	return woo.batch.runPreprocessor(*args,**kw)

