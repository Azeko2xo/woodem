# encoding: utf-8
from woo import utils,pack
from woo.core import *
from woo.dem import *
from woo.pre import *
import woo.plot
import math
import os.path
from minieigen import *
import sys
import cStringIO as StringIO
import numpy
import pprint
nan=float('nan')

try:
	from PyQt4.QtGui import *
	from PyQt4.QtCore import *
except ImportError: pass

# http://www.mail-archive.com/matplotlib-users@lists.sourceforge.net/msg05474.html
from matplotlib.ticker import FuncFormatter
percent=FuncFormatter(lambda x,pos=0: '%g%%'%(100*x))
milimeter=FuncFormatter(lambda x,pos=0: '%3g'%(1000*x))
# transparency of figure legends
legendAlpha=.6

import woo.pyderived
import woo.core
class BinSeg(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'Preprocessor for bin segregation simulation'
	_classTraits=None
	PAT=woo.pyderived.PyAttrTrait
	_attrTraits=[
		# geometry
		PAT(Vector3,'size',Vector3(1,1,.2),unit='mm',startGroup="Geometry",doc="Size of the bin: width, height and depth (thickness). Width is real horizontal dimension of the bin. Height is height of the left boundary (which is different from the right one, if :ref:`inclination` is non-zero)."),
		PAT(float,'inclination',0,unit="deg",doc="Inclination of the bottom of the bin. Positive angle makes the right hole higher than the left one."),
		PAT(Vector2,'hole1',Vector2(.1,.08),unit='mm',doc="Distance from the left wall and width of the first hole. Lengths are measured in the bottom (possibly inclined) plane."),
		PAT(Vector2,'hole2',Vector2(.25,.08),unit="mm",doc="Distance from the right wall and width of the second hole. Lengths are measured in the bottom (possibly inclined) plane."),
		PAT(float,'holeLedge',.03,unit='mm',doc="Distance between front wall and hole, and backwall and hole"),
		PAT(Vector2,'feedPos',Vector2(.1,.15),unit="mm",doc="Distance of the feed from the right wall and its width (vertical lengths)"),
		PAT(float,'halfThick',0.,unit="mm",doc="Half-thickenss of boundary plates; all dimensions are given as inner sizes (including hole widths), there is no influence of halfThick on geometry"),
		PAT(bool,'halfDp',True,"Only simulate half of the bin, by putting frictionless wall in the middle"),

		# feed
		PAT([Vector2,],'psd',[Vector2(0.015,.0),Vector2(.025,.2),Vector2(.03,1.)],startGroup="Feed",unit=["mm",'%'],doc="Particle size distribution of generated particles: first value is diameter, second value is cummulative fraction"),
		PAT(woo.dem.FrictMat,'material',woo.dem.FrictMat(density=3200,young=1e5,ktDivKn=.2,tanPhi=math.tan(.5)),"Material of particles"),
		PAT(float,'feedRate',1.,unit='kg/s',doc="Feed rate"),
		PAT(woo.dem.FrictMat,'wallMaterial',None,"Material of walls (if not given, material for particles is used)"),
		PAT(str,'loadSpheresFrom','',"Load initial packing from file (format x,y,z,radius)"),
		PAT(Vector2,'startStopMass',Vector2(nan,nan),"Mass for starting and stopping PSD measuring. Once the first mass is reached, PSD counters are engaged; once the second value is reached, report is generated and simulation saved and stopped. Negative values are relative to initial total particles mass. 0 or NaN deactivates; both limits must be active or both inactive."),

		# Outputs
		PAT(str,'reportFmt',"/tmp/{tid}.xhtml",startGroup="Outputs",doc="Report output format (Scene.tags can be used)."),
		PAT(str,'feedCacheDir',".","Directory where to store pre-generated feed packings"),
		PAT(str,'saveFmt',"/tmp/{tid}-{stage}.bin.gz","Savefile format; keys are :ref:`Scene.tags` and additionally ``{stage}`` will be replaced by 'init', 'backup-[step]'."),
		PAT(int,'backupSaveTime',1800,"How often to save backup of the simulation (0 or negative to disable)"),
		#PAT(float,'vtkFreq',4,"How often should VtkExport run, relative to *factStepPeriod*. If negative, run never."),
		#PAT(str,'vtkPrefix',"/tmp/{tid}-","Prefix for saving VtkExport data; formatted with ``format()`` providing :ref:`Scene.tags` as keys."),

		# Tunables
		PAT(float,'gravity',10.,unit='m/s²',startGroup="Tunables",doc="Gravity acceleration magnitude"),
		PAT(float,'damping',.4,"Non-viscous damping coefficient"),
		PAT(float,'pWaveSafety',.7,"Safety factor for critical timestep"),
		PAT(float,'rateSmooth',.2,"Smoothing factor for plotting rates in factory and deleters"),
		PAT(float,'feedAdjustCoeff',.3,"Coefficient for adjusting feed rate"),
		PAT(int,'factStep',100,"Interval at which particle factory and deleters are run"),
		PAT([Vector2,],'deadTriangles',[],"Triangles given as sequence of points in the xz plane, where spheres will be removed from *loadSpheres* from. 2*rMax from the edge, they will be created as fixed-position"),
		PAT(float,'deadFixedRel',1.,"Thickness of fixed boundary between live and dead regions, relative to maximum radius of the PSD"),

		# internal, for simulation state
		PAT([int,],'hole1ids',[],noGui=True,doc="Ids of hole 1 particles (for later removal)"),
		PAT([int,],'hole2ids',[],noGui=True,doc="Ids of hole 2 particles (for later removal)"),
		PAT(int,'holeMask',0,noGui=True,noDump=True,doc="Mask of hole particles (used for ParticleContainer.reappear)"),
	]
	def __init__(self,**kw):
		Preprocessor.__init__(self)
		self.wooPyInit(BinSeg,Preprocessor,**kw)
	def __call__(self):
		import woo.pre.binseg
		print self.hole1ids
		return woo.pre.binseg.run(self)


def makePeriodicFeedPack(dim,psd,lenAxis=0,damping=.3,porosity=.5,goal=.15,dontBlock=False,memoizeDir=None):
	if memoizeDir:
		params=str(dim)+str(psd)+str(goal)+str(damping)+str(porosity)+str(lenAxis)
		import hashlib
		paramHash=hashlib.sha1(params).hexdigest()
		memoizeFile=memoizeDir+'/'+paramHash+'.perifeed'
		print 'Memoize file is ',memoizeFile
		if os.path.exists(memoizeDir+'/'+paramHash+'.perifeed'):
			print 'Returning memoized result'
			sp=pack.SpherePack()
			sp.load(memoizeFile)
			return zip(*sp)+[sp.cellSize[lenAxis],]
	p3=porosity**(1/3.)
	rMax=psd[-1][0]
	minSize=rMax*5
	cellSize=Vector3(max(dim[0]*p3,minSize),max(dim[1]*p3,minSize),max(dim[2]*p3,minSize))
	print 'dimension',dim
	print 'initial cell size',cellSize
	print 'psd=',psd
	S=Scene(fields=[DemField()])
	S.periodic=True
	S.cell.setBox(cellSize)
	S.engines=[
		InsertionSortCollider([Bo1_Sphere_Aabb()]),
		BoxFactory(
			box=((0,0,0),cellSize),
			maxMass=-1,
			massFlowRate=0,
			maxAttempts=5000,
			generator=PsdSphereGenerator(psdPts=psd,discrete=False,mass=True),
			materials=[FrictMat(density=1e3,young=1e7,ktDivKn=.2,tanPhi=math.tan(.5))],
			shooter=None,
			mask=1,
		)
	]
	S.one()
	print 'Created %d particles'%(len(S.dem.par))
	S.dt=.9*utils.pWaveDt(S)
	S.engines=[
		woo.dem.PeriIsoCompressor(charLen=2*psd[-1][0],stresses=[-1e8,-1e6],maxUnbalanced=goal,doneHook='print "done"; S.stop();',globalUpdateInt=1,keepProportions=True)
	]+utils.defaultEngines(damping=damping)
	if dontBlock: return S
	S.run(); S.wait()
	sp=woo.pack.SpherePack()
	sp.fromSimulation(S)
	print 'Packing size is',sp.cellSize
	sp.makeOverlapFree()
	print 'Loose packing size is',sp.cellSize
	sp.canonicalize()
	cc,rr=[],[]
	inf=float('inf')
	boxMin=Vector3(0,0,0);
	boxMax=dim
	boxMin[lenAxis]=-inf
	boxMax[lenAxis]=inf
	box=AlignedBox3(boxMin,boxMax)
	for c,r in sp:
		if c not in box: continue
		cc.append(c); rr.append(r)
	if memoizeDir:
		sp2=pack.SpherePack()
		sp2.fromList(cc,rr)
		sp2.cellSize=sp.cellSize
		print 'Saving to',memoizeFile
		sp2.save(memoizeFile)
	return cc,rr,sp.cellSize[lenAxis]


def yRectPlate(xz0,xz1,yy,shift=True,halfThick=0.,**kw):
	xx,zz=(xz0[0],xz1[0]),(xz0[1],xz1[1])
	if halfThick!=0.:
		if shift:
			A,B=Vector2(xx[0],zz[0]),Vector2(xx[1],zz[1])
			dx,dz=Vector2(+(B[1]-A[1]),-(B[0]-A[0])).normalized()*halfThick
			xx,zz=[x+dx for x in xx],[z+dz for z in zz]
		kw['halfThick']=abs(halfThick) # copy arg
	A,B,C,D=(xx[0],yy[0],zz[0]),(xx[1],yy[0],zz[1]),(xx[0],yy[1],zz[0]),(xx[1],yy[1],zz[1])
	return [utils.facet(vertices,**kw) for vertices in (A,B,C),(C,B,D)]

def run(pre):
	de=DemField(gravity=(0,0,-pre.gravity))
	S=Scene(fields=[de])
	if not pre.wallMaterial: pre.wallMaterial=pre.material

	for a in ['reportFmt','feedCacheDir','saveFmt']: setattr(pre,a,utils.fixWindowsPath(getattr(pre,a)))


	# particle masks
	wallMask=0b00110
	loneMask=0b00100 # particles with this mask don't interact with each other
	sphMask= 0b00011
	delMask= 0b00001 # particles which might be deleted by deleters
	S.dem.loneMask=loneMask

	pre.holeMask=wallMask



	#
	#
	#   ^ +z
	#   |
	#
	#
	#   A                               feedPos[1]   <-feedPos[0]->
	#   +                                    +======+              + 
	#   |                                    J      I              | H
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |<--hole1[0]---> <-hole1[1]->       K        h2[1]   h2[0] |
	#   +---------------+============+------+------+=======+-------+    ~~~~~~~> +x
	#   B=(0,0)         C            D      |      E       F       G
   #                                       |
   #                                       |
   #                                       + L
	#                                        
	#
	# to account for inclination, +x is botVec (bottomVec), |botVec|==1.
	#
	wd,ht,dp=pre.size
	B=Vector2(0,0)
	botVec=Vector2(math.cos(pre.inclination),math.sin(pre.inclination)) # unit vector
	botUpVec=Vector2(-math.sin(pre.inclination),math.cos(pre.inclination))
	cosIncl=math.cos(pre.inclination) # scaling factor for horizontal lengths
	A=B+(0,ht)
	C=B+botVec*(pre.hole1[0]-pre.halfThick)
	D=C+botVec*(pre.hole1[1]+2*pre.halfThick)
	G=B+botVec*(wd/cosIncl) # oblique length correspondingly longer
	F=G-botVec*(pre.hole2[0]-pre.halfThick)
	E=F-botVec*(pre.hole2[1]+2*pre.halfThick)
	H=A+(wd,0)
	I=H-(pre.feedPos[0],0)
	J=I-(pre.feedPos[1],0)
	K=.5*(D+E)-pre.halfThick*botUpVec
	L=Vector2(K[0],min(B[1],G[1]))-(0,2*pre.hole1[1])

	feedHt=pre.feedPos[1]

	xmin,xmax=0,wd
	zmin,zmax=L[1],ht+feedHt
	ymin,ymax=(0 if pre.halfDp else -dp/2.),dp/2.

	wallKw=dict(mat=pre.wallMaterial,mask=wallMask,halfThick=pre.halfThick)
	yy=(ymin,ymax)
	for a,b in ((A,B),(B,C),(D,E),(F,G),(G,H),(K,L)):
		de.par.append(yRectPlate(a,b,yy,**wallKw))
	# holes with ledges
	for i,(a,b) in enumerate([(C,D),(E,F)]):
		if pre.holeLedge>0 and not pre.halfDp: de.par.append(yRectPlate(a,b,(ymin,ymin+pre.holeLedge),**wallKw))
		# TODO: save ids of those, so that they can be opened during simulation
		ids=de.par.append(yRectPlate(a,b,(ymin+(0 if pre.halfDp else pre.holeLedge),ymax-pre.holeLedge),**wallKw))
		if pre.holeLedge>0: de.par.append(yRectPlate(a,b,(ymax-pre.holeLedge,ymax),**wallKw))
		if i==0: pre.hole1ids=ids
		else: pre.hole2ids=ids
	
	## side walls
	noFrictMat=pre.wallMaterial.deepcopy()
	noFrictMat.tanPhi=0
	de.par.append([
		utils.wall((0,ymax,0),sense=-1,axis=1,mat=pre.wallMaterial,mask=wallMask,glAB=((zmin,xmin),(zmax,xmax))),
		utils.wall((0,ymin,0),sense=1,axis=1,mat=(noFrictMat if pre.halfDp else pre.wallMaterial),mask=wallMask,glAB=((zmin,xmin),(zmax,xmax))),
	])
	## domain walls
	de.par.append([
		utils.wall((0,0,zmin),sense=1,axis=2,mat=pre.wallMaterial,mask=wallMask,glAB=((xmin,ymin),(xmax,ymax))),
		utils.wall((xmin,0,0),sense=1,axis=0,mat=pre.wallMaterial,mask=wallMask,glAB=((ymin,zmin),(ymax,zmax))),
		utils.wall((xmax,0,0),sense=-1,axis=0,mat=pre.wallMaterial,mask=wallMask,glAB=((ymin,zmin),(ymax,zmax))),
	])

	rMax=pre.psd[-1][0]

	## initial spheres
	if pre.loadSpheresFrom:
		import woo.pack
		sp=woo.pack.SpherePack()
		print 'Loading spheres from',pre.loadSpheresFrom
		sp.load(pre.loadSpheresFrom)
		# http://stackoverflow.com/a/312464/761090
		def chunks(l, n):
			'Yield successive n-sized chunks from l.'
			for i in xrange(0,len(l),n): yield l[i:i+n]
		if pre.deadTriangles:
			spAlive=woo.pack.SpherePack()
			spFixed=woo.pack.SpherePack()
			for c,r in sp:
				what=0
				for t in chunks(pre.deadTriangles,3):
					sd=utils.outerTri2Dist(Vector2(c[0],c[2]),t[0],t[1],t[2])
					if sd>pre.deadFixedRel*rMax: what=max(what,0) # live
					elif sd>0: what=max(what,1) # edge
					else: what=max(what,2) # dead
				if what==0: spAlive.add(c,r) # live
				elif what==1: spFixed.add(c,r) # edge
				else: continue # dead
			spFixed.toSimulation(S,mat=pre.material,mask=sphMask,fixed=True,color=-.5)
		else: spAlive=sp
		spAlive.toSimulation(S,mat=pre.material,mask=sphMask)
		## open holes if initial spheres are loaded
		print 'Opening both holes and enabling rate auto-adjusting as spheres were loaded externally'
		S.dem.par.disappear(pre.hole1ids+pre.hole2ids,mask=S.dem.loneMask)

	cc,rr,cellLen=makePeriodicFeedPack([20*rMax,(ymax-ymin-2*rMax),pre.feedPos[1]],lenAxis=0,psd=pre.psd,memoizeDir=pre.feedCacheDir)
	factoryNode=Node(pos=(.5*(J[0]+I[0]),ymin+rMax,J[1]),ori=Quaternion((0,1,0),math.pi/2.)) # local x-axis is downwards

	# compute relative startStopMass
	initMass=sum([p.mass for p in S.dem.par if type(p.shape)==Sphere])
	pre.startStopMass=[(m if m>0 else (initMass*abs(m) if m<0 else 0)) for m in pre.startStopMass] 
	#
	# _counterState can be:
	#    1. 'noStartStop' when no maxMass is given at all
	#    2. 'waitStartMass' between 0 and startMass
	#    3. 'waitStopMass' between startMass and stopMass
	#
	S.tags['_counterState']='noStartStop' # no state at all
	initMaxMass=-1 # no limit by default
	if pre.startStopMass[0]==0:
		if pre.startStopMass[1]>0:
			S.tags['_counterState']='waitStopMass'
			initMaxMass=pre.startStopMass[1]
	else:
		S.tags['_counterState']='waitStartMass'
		initMaxMass=pre.startStopMass[0]

	print 'Using startStopMass ',pre.startStopMass
	print '    _counterState is',S.tags['_counterState']

	factory=ConveyorFactory(
		stepPeriod=pre.factStep,
		material=pre.material,
		centers=cc,radii=rr,
		cellLen=cellLen,
		barrierColor=0,
		color=float('nan'),
		glColor=.5,
		#color=.4, # random colors
		node=factoryNode,
		mask=sphMask,
		prescribedRate=pre.feedRate,
		label='feed',
		maxMass=initMaxMass,
		currRateSmooth=pre.rateSmooth,
		zeroRateAtStop=False
	)

	S.engines=utils.defaultEngines(damping=pre.damping,dontCollect=True,verletDist=pre.psd[0][0]*.1)+[
		BoxDeleter(
			stepPeriod=pre.factStep,
			inside=True,
			box=box,
			glColor=.3+.5*i,
			save=True,
			mask=delMask,
			currRateSmooth=pre.rateSmooth,
			label='bucket[%d]'%i
		)
		for i,box in enumerate([((xmin,ymin,zmin),(L[0],ymax,min(B[1],K[1]))),((L[0],ymin,zmin),(xmax,ymax,min(K[1],G[1])))])
	]+[
		factory,
		PyRunner(pre.factStep,'import woo.pre.binseg; woo.pre.binseg.adjustFeedRate(S)',label='feedAdjuster',dead=(not pre.loadSpheresFrom)),
		PyRunner(pre.factStep,'import woo.pre.binseg; woo.pre.binseg.savePlotData(S)'),
		PyRunner(pre.factStep,'import woo.pre.binseg; woo.pre.binseg.watchProgress(S)',initRun=False),
	#]+( # VTK
	#	[] if (not pre.vtkPrefix or pre.vtkFreq<=0) else [VtkExport(out=pre.vtkPrefix,stepPeriod=int(pre.vtkFreq*pre.factStepPeriod),what=VtkExport.all,subdiv=16)]
	]+( # backups
		[] if pre.backupSaveTime<=0 else [PyRunner(realPeriod=pre.backupSaveTime,initRun=False,stepPeriod=-1,command='S.save(S.pre.saveFmt.format(stage="backup-%06d"%(S.step),S=S,**(dict(S.tags))))')]
	)


	S.dem.collectNodes()


	S.dt=pre.pWaveSafety*utils.spherePWaveDt(pre.psd[0][0]/2.,pre.material.density,pre.material.young)

	S.pre=pre.deepcopy()

	S.plot.plots={'t':('feedRate','hole1rate','hole2rate','holeRate',None,'feedMass')}
	if S.trackEnergy: S.plot.plots.update({'  t':(S.energy,None,('ERelErr','g--'))})

	S.uiBuild='import woo.pre.binseg; woo.pre.binseg.uiBuild(S,area)'

	try:
		import woo.gl
		crg=woo.gl.Gl1_DemField.colorRanges[woo.gl.Gl1_DemField.colorDisplacement]
		crg.autoAdjust=False
		crg.mnmx=(0,1*rMax)
		grg=woo.gl.Gl1_DemField.glyphRanges[woo.gl.Gl1_DemField.glyphVel]
		grg.autoAdjust=False
		grg.mnmx=(0,1e-2*rMax/S.dt)
		S.any=[
			woo.gl.Renderer(iniUp=(0,0,1),iniViewDir=(0,1,0)),
			woo.gl.Gl1_DemField(colorBy=woo.gl.Gl1_DemField.colorDisplacement,glyphRelSz=.05),
			woo.gl.Gl1_Wall(div=1),
			woo.gl.Gl1_Sphere(quality=.4)
		]

	except ImportError: pass

	out=pre.saveFmt.format(stage='init',S=S,**(dict(S.tags)))
	S.save(out)
	print 'Saved initial simulation also to ',out

	return S

	
def savePlotData(S):
	import woo
	import woo.plot
	S.plot.addData(i=S.step,t=S.time,feedMass=woo.feed.mass,feedRate=woo.feed.currRate,hole1rate=woo.bucket[0].currRate,hole2rate=woo.bucket[1].currRate,numPar=len(S.dem.par),holeRate=sum([woo.bucket[i].currRate for i in (0,1)]))

def adjustFeedRate(S):
	'Change feed massFlowRate so that it comes close to holes deletion rate'
	import woo
	fr=woo.feed.currRate
	br=sum([b.currRate for b in woo.bucket])
	newRate=fr+S.pre.feedAdjustCoeff*(br-fr)
	if not math.isnan(newRate):
		woo.feed.prescribedRate=newRate
		print 'New feed rate %g (old %g, hole rate %g)'%(woo.feed.prescribedRate,fr,br)
	else:
		print 'No feed rate computed (nan), keeping previous %g'%(woo.feed.prescribedRate)



def holeOpen(i,S):
	assert i in (1,2)
	return S.dem.par[(S.pre.hole1ids if i==1 else S.pre.hole2ids)[0]].mask==S.dem.loneMask

def toggleHole(i):
	import woo
	if i not in (1,2): raise ValueError("Hole number must be 0 or 1.")
	S=woo.master.scene
	if type(S.pre)!=woo.pre.BinSeg: raise RuntimeError("Current Scene.pre is not a BinSeg.")
	ids=(S.pre.hole1ids if i==1 else S.pre.hole2ids)
	holeOpen=S.dem.par[ids[0]].mask==S.dem.loneMask
	with S.paused():
		if holeOpen:
			#print 'Closing hole ',i
			S.dem.par.reappear(ids,mask=S.pre.holeMask,removeOverlapping=True)
		else:
			#print 'Opening hole ',i
			S.dem.par.disappear(ids,mask=S.dem.loneMask)
		#for i in ids:
		#	print '#%d:\n\t%s\n\tmask:%d\n\tcon:%s'%(i,'visible' if S.dem.par[i].shape.visible else 'invisible',S.dem.par[i].mask,str(S.dem.par[i].con))

def saveSpheres(S):
	import woo
	if type(S.pre)!=woo.pre.BinSeg: raise RuntimeError("Current Scene.pre is not a BinSeg.")
	if len(woo.qt.views())>0: woo.qt.views()[0].close()
	f=str(QFileDialog.getSaveFileName(None,'Save spheres to','.'))
	if not f: return
	# find maximum contact z-coordinate (top of the heap)
	maxZ=max([c.geom.node.pos[2] for c in S.dem.con])
	import woo.pack
	sp=woo.pack.SpherePack()
	for p in S.dem.par:
		if type(p.shape)!=woo.dem.Sphere or p.pos[2]>maxZ: continue
		sp.add(p.pos,p.shape.radius)
	sp.save(f)
	print 'Spheres (with z<%g) saved to %s'%(maxZ,f)

def feedHolesPsdTable(S,massScale=1.):
	psdSplits=[df[0] for df in S.pre.psd]
	feedHolesMasses=[]
	for ddmm in (woo.feed.diamMass(),woo.bucket[0].diamMass(),woo.bucket[1].diamMass()):
		mm=[]
		for i in range(len(psdSplits)-1):
			dmdm=zip(*ddmm)
			mm.append(sum([dm[1]*massScale for dm in zip(*ddmm) if (dm[0]>=psdSplits[i] and dm[0]<psdSplits[i+1])]))
		feedHolesMasses.append(mm)
		#print buck.mass,sum(mm)
	colNames=['feed','hole 1','hole 2']
	from genshi.builder import tag as t
	tab=t.table(
		t.tr(t.th('diameter',t.br,'[mm]'),*tuple([t.th(colName,t.br,'[mass %]',colspan=2) for colName in colNames])),
		cellpadding='2px',frame='box',rules='all'
	)
	dScale=1e3 # m to mm
	for i in range(len(psdSplits)-1):
		tr=t.tr(t.td('%g-%g'%(dScale*psdSplits[i],dScale*psdSplits[i+1])))
		for bm in feedHolesMasses:
			bMass=sum(bm) # if it is zero, don't error on integer division by zero
			if bMass>0:
				tr.append(t.td('%.4g'%(100*bm[i]/bMass),align='right'))
				tr.append(t.td('%.4g'%(100*sum(bm[:i+1])/bMass),align='right'))
			else:
				tr.append(t.td('-',align='center',colspan=2))
		tab.append(tr)
	colTotals=sum([sum(bm) for bm in feedHolesMasses])
	tab.append(t.tr(t.th('mass [kg]',rowspan=2),*tuple([t.th('%.4g'%sum(bm),colspan=2) for bm in feedHolesMasses])))
	tab.append(t.tr(*tuple([t.td('%.3g %%'%(sum(bm)*100./sum(feedHolesMasses[0])),colspan=2,align='right') for bm in feedHolesMasses])))
	return unicode(tab.generate().render('xhtml'))

def velocityFieldPlots(S,nameBase):
	import woo
	from woo import post2d
	flattener=post2d.AxisFlatten(useRef=False,axis=1,swapAxes=True)
	maxVel=.05
	exVel=lambda p: p.vel if p.vel.norm()<=maxVel else p.vel/(p.vel.norm()/maxVel)
	exVelNorm=lambda p: exVel(p).norm()
	def plotBin(S,**kw):
		from matplotlib.lines import Line2D
		import woo.utils
		P=S.pre
		for A,B in (
			(Vector2(0,P.size[1]),Vector2(0,0)),
			(Vector2(0,0),Vector2(P.hole1[0],0)),
			(Vector2(P.hole1[0]+P.hole1[1],0),Vector2(P.size[0]-P.hole2[0]-P.hole2[1],0)),
			(Vector2(P.size[0]-P.hole2[0],0),Vector2(P.size[0],0)),
			(Vector2(P.size[0],0),Vector2(P.size[0],P.size[1])),
		):
			pylab.gca().add_line(Line2D((A[0],B[0]),(A[1],B[1]),solid_capstyle='butt',**kw))
	import pylab
	fVRaw=pylab.figure()
	post2d.plot(post2d.data(S,exVel,flattener),alpha=.3,minlength=.1,cmap='jet')
	plotBin(S,linewidth=8,color='k')
	fV2=pylab.figure()
	post2d.plot(post2d.data(S,exVel,flattener,stDev=.5*S.pre.psd[0][0],div=(80,80)),minlength=.4,cmap='jet')
	plotBin(S,linewidth=8,color='k')
	fV1=pylab.figure()
	post2d.plot(post2d.data(S,exVelNorm,flattener,stDev=.5*S.pre.psd[0][0],div=(80,80)),cmap='jet')
	plotBin(S,linewidth=8,color='k')
	outs=[]
	for name,fig in [('particle-velocity',fVRaw),('smooth-velocity',fV2),('smooth-velocity-norm',fV1)]:
		out=nameBase+'.%s.png'%name
		fig.savefig(out)
		outs.append(out)
	return outs
	


def feedHolesPsdFigure(massBased=True):
	import pylab
	fig=pylab.figure()
	for i,buck in enumerate(woo.bucket):
		if buck.num<=1: continue # bucket empty, or just one particle (range would be automatically -.4 to +.4 around that single particle)
		dm=buck.diamMass()
		h,bins=numpy.histogram(dm[0],weights=(dm[1] if massBased else len(dm[0])*[1.]),bins=50)
		h/=1.*h.sum()
		pylab.plot(bins,[0]+list(h.cumsum()),label='hole %d'%(i+1),linewidth=2)
	if woo.feed.num>1:
		pylab.plot(*woo.feed.psd(normalize=True,mass=massBased),label='feed',linewidth=2)

	pylab.ylim(ymin=-.05,ymax=1.05)
	pylab.grid(True)
	pylab.gca().xaxis.set_major_formatter(milimeter)
	pylab.xlabel('diameter [mm]')
	pylab.gca().yaxis.set_major_formatter(percent)
	pylab.ylabel(('mass' if massBased else 'COUNT')+' fraction')
	leg=pylab.legend(loc='best')
	leg.get_frame().set_alpha(legendAlpha)
	return fig


def watchProgress(S):
	import woo
	# not yet finished
	if S.tags['_counterState']=='noStartStop': return
	# this is currently enforced
	assert woo.feed.maxMass>0
	# stage not yet finished
	if woo.feed.mass<woo.feed.maxMass: return
	if S.tags['_counterState']=='waitStartMass':
		# reset buckets and feed
		for b in woo.bucket: b.clear()
		woo.feed.clear()
		woo.feed.dead=False
		# set new mass limit and wait for it
		woo.feed.maxMass=S.pre.startStopMass[1]
		S.tags['_counterState']='waitStopMass'
		print 'pre.startStopMass[0]=%g reached, counters engaged; mass to do is %g'%(S.pre.startStopMass[0],S.pre.startStopMass[1])
	elif S.tags['_counterState']=='waitStopMass':
		# exit here
		out=S.pre.saveFmt.format(stage='done',S=S,**(dict(S.tags)))
		if S.lastSave!=out:
			S.save(out)
			print 'Saved to',out
		repName,extraResults=writeReport()
		woo.batch.writeResults(defaultDb='BinSeg.sqlite',simulationName='BinSeg',material=S.pre.material,report=repName,saveFile=os.path.abspath(S.lastSave),**extraResults)
		S.stop()
	else:
		raise RuntimeError('Unknown value of S.tags["_counterState"]='+S.tags['_counterState'])


def writeReport():	
	import woo
	import numpy,pylab
	S=woo.master.scene
	massScale=2 if S.pre.halfDp else 1.

	figs=[]

	# per-bucket PSD
	def scaledFlowPsd(x,y): return numpy.array(x),massScale*numpy.array(y)

	if 1:
		figs.append(('Per-hole PSD',feedHolesPsdFigure()))
	if 1:
		fig=pylab.figure()
		if woo.feed.num: pylab.plot(*woo.feed.psd(cumulative=False,normalize=False,num=20),label='feed')
		if woo.bucket[0].num: pylab.plot(*woo.bucket[0].psd(cumulative=False,normalize=False,num=20),label='hole 1')
		if woo.bucket[1].num: pylab.plot(*woo.bucket[1].psd(cumulative=False,normalize=False,num=20),label='hole 2')
		pylab.grid(True)
		pylab.gca().xaxis.set_major_formatter(milimeter)
		pylab.xlabel('diameter [mm]')
		#pylab.gca().yaxis.set_major_formatter(percent)
		pylab.ylabel('histogram')
		leg=pylab.legend(loc='best')
		leg.get_frame().set_alpha(legendAlpha)
		figs.append(('PSD',fig))

	try:
		import pylab
		fig=pylab.figure()
		import woo.plot
		d=S.plot.data
		pylab.plot(d['t'],massScale*numpy.array(d['feedRate']),label='feed')
		pylab.plot(d['t'],massScale*numpy.array(d['hole1rate']),label='hole1')
		pylab.plot(d['t'],massScale*numpy.array(d['hole2rate']),label='hole2')
		pylab.plot(d['t'],massScale*numpy.array(d['holeRate']),label='holes')
		pylab.ylim(ymin=0)
		leg=pylab.legend(loc='lower left')
		leg.get_frame().set_alpha(legendAlpha)
		pylab.grid(True)
		pylab.xlabel('time [s]')
		pylab.ylabel('mass rate [kg/s]')
		figs.append(('Regime',fig))
	except (KeyError,IndexError) as e:
		print 'No woo.plot plots done due to lack of data:',str(e)

	extraResults={}
	import re
	plotBase=unicode(re.sub('(\.xhtml)$','',S.pre.reportFmt)).format(S=S,**(dict(S.tags)))
	extraResults['velocityFieldPlots']=velocityFieldPlots(S,plotBase)
	for name,obj in ('Hole1',woo.bucket[0]),('Hole2',woo.bucket[1]),('Feed',woo.feed):
		extraResults['dAverage'+name]=numpy.average(obj.diamMass()[0])
		extraResults['dMedian'+name]=numpy.median(obj.diamMass()[0])
		extraResults['mass'+name]=obj.mass
	pprint.pprint(extraResults)


	import codecs
	repName=unicode(S.pre.reportFmt).format(S=S,**(dict(S.tags)))
	rep=codecs.open(repName,'w','utf-8','replace')
	import os.path
	import woo.pre.roro
	print 'Writing report to file://'+os.path.abspath(repName)
	s=woo.pre.roro.xhtmlReportHead(S,'Report for BinSeg simulation')
	s+='<h2>Mass</h2>'+feedHolesPsdTable(S,massScale=massScale)

	svgs=[]
	for name,fig in figs:
		svgs.append((name,woo.master.tmpFilename()+'.svg'))
		fig.savefig(svgs[-1][-1])
	s+='\n'.join(['<h2>'+svg[0]+'</h2>'+woo.pre.roro.svgFileFragment(svg[1]) for svg in svgs])

	s+='</body></html>'

	s=s.replace(u'\xe2\x88\x92','-') # long minus
	s=s.replace(u'\xc3\x97','x') # × multiplicator
	try:
		rep.write(s)
	except UnicodeDecodeError as e:
		print e.start,e.end
		print s[max(0,e.start-20):min(e.end+20,len(s))]
		raise e
	# close before the webbrowser reads the file
	rep.close()
	
	import woo.batch
	if not woo.batch.inBatch():
		import webbrowser
		webbrowser.open('file://'+os.path.abspath(repName),new=2) # new=2: new tab if possible

	return os.path.abspath(repName),extraResults


def uiRefresh(grid,S,area):
	grid.feedButt.setChecked(not woo.feed.dead)
	grid.feedButt.setText('Start feed' if woo.feed.dead else 'Stop feed')
	areOpen=[None,holeOpen(1,S),holeOpen(2,S)]
	for butt,i in (grid.h1butt,1),(grid.h2butt,2):
		butt.setChecked(areOpen[i])
		butt.setText('Close %d'%i if areOpen[i] else 'Open %d'%i)
	if areOpen[1]==areOpen[2]:
		op=areOpen[1]
		grid.h12butt.setChecked(op)
		grid.h12butt.setText('Close 1+2' if op else 'Open 1+2')
		grid.h12butt.setEnabled(True)
	else:
		grid.h12butt.setText('one closed, one open')
		grid.h12butt.setEnabled(False)

def holeClicked(S,i,grid):
	import woo
	assert i in (1,2); assert type(S.pre)==woo.pre.BinSeg
	ids=(S.pre.hole1ids if i==1 else S.pre.hole2ids)
	op=holeOpen(i,S)
	with S.paused():
		if op:
			#print 'Closing hole ',i
			S.dem.par.reappear(ids,mask=S.pre.holeMask,removeOverlapping=True)
		else:
			#print 'Opening hole ',i
			S.dem.par.disappear(ids,mask=S.dem.loneMask)
	#if not holeOpen(0,S) and not holeOpen(1,S) and not woo.feedAdjuster.dead:
	#	autoAdjustChanged(S,grid,0)
def resetCounters(S):
	import woo
	with S.paused():
		for b in woo.bucket: b.clear()
		woo.feed.clear()

def hole12Clicked(S,grid):
	holeClicked(S,1,grid)
	holeClicked(S,2,grid)
def feedToggled(S,checked,grid):
	import woo
	woo.feed.dead=not checked
	# HACK: since the engine was dead for a long time,
	# it would attempt to catch up now and generate all the mass
	# at once, which would exhaust its attempts for sure
	# therefore, make it think it has just run now
	# by setting stepLast
	if not woo.feed.dead:
		woo.feed.stepLast=S.step
	grid.adjCheckbox.setEnabled(not woo.feed.dead)
def reportClicked(S,grid):
	with S.paused():
		rep=writeReport()[0]
	import webbrowser
	webbrowser.open(rep)
def autoAdjustChanged(S,grid,state):
	import woo
	if state==0:
		# stop auto-adjust
		woo.feedAdjuster.dead=True
	else:
		woo.feedAdjuster.dead=False

def uiBuild(S,area):
	grid=QGridLayout(area); grid.setSpacing(0); grid.setMargin(0)
	f=QPushButton('Stop feed')
	adj=QCheckBox('Auto-adjust rate')
	s=QPushButton('Save spheres')
	h1=QPushButton('Open 1')
	h2=QPushButton('Open 2')
	h12=QPushButton('Open 1+2')
	r=QPushButton('Report')
	reset=QPushButton('Reset counters')
	psd=QPushButton('PSD (mass)')
	psdCount=QPushButton('PSD (count)')
	for b in (f,h1,h2,h12): b.setCheckable(True)
	f.toggled.connect(lambda checked: feedToggled(S,checked,grid))
	h1.clicked.connect(lambda: holeClicked(S,1,grid))
	h2.clicked.connect(lambda: holeClicked(S,2,grid))
	h12.clicked.connect(lambda: hole12Clicked(S,grid))
	s.clicked.connect(lambda: saveSpheres(S))
	reset.clicked.connect(lambda: resetCounters(S))
	psd.clicked.connect(lambda: feedHolesPsdFigure(True).show())
	psdCount.clicked.connect(lambda: feedHolesPsdFigure(False).show())
	r.clicked.connect(lambda: reportClicked(S,grid))
	adj.setChecked(not woo.feedAdjuster.dead)
	adj.stateChanged.connect(lambda state: autoAdjustChanged(S,grid,state))
	grid.feedButt=f
	grid.adjCheckbox=adj
	grid.h1butt,grid.h2butt,grid.h12butt=h1,h2,h12
	grid.addWidget(adj,0,1)
	grid.addWidget(f,0,2)
	grid.addWidget(s,1,1)
	grid.addWidget(h1,2,0)
	grid.addWidget(h2,2,2)
	grid.addWidget(h12,3,0,1,3)
	grid.addWidget(reset,4,0)
	grid.addWidget(psd,4,1)
	grid.addWidget(psdCount,4,2)
	grid.addWidget(r,5,1)
	grid.refreshTimer=QTimer(grid)
	grid.refreshTimer.timeout.connect(lambda: uiRefresh(grid,S,area))
	grid.refreshTimer.start(500)
	#grid.addWidget(QLabel('Hi there!'))
	

	






