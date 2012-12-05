# encoding: utf-8
from woo import utils,pack
from woo.core import *
from woo.dem import *
from woo.pre import roro
import woo.config
import woo.plot
from minieigen import *
import math
import sys
import numpy
nan=float('nan')


import woo.pyderived
from woo.dem import PelletMat

class Chute(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'Python variant of the Chute preprocessor'
	_classTraits=None
	PAT=woo.pyderived.PyAttrTrait
	# constants
	shieldNone=-1
	shieldCylinder=0
	shieldStl=1
	stlHideIf='self.shieldType!=self.__class__.shieldStl'
	#
	_attrTraits=[
		PAT(float,'massFlowRate',2000*woo.unit['t/h'],unit='t/h',startGroup='General',doc='Mass flow rate of particles transported on the conveyor'),
		PAT(float,'convLayerHt',.2,unit='m',doc='Height of transported material'),
		PAT(float,'time',2,unit='s',doc='Time of the simulation'),

		# conveyor
		PAT([Vector2,],'convPts',[Vector2(.3,0),Vector2(.6,.1)],unit='m',startGroup='Conveyor',doc='Half of the the conveyor cross-section; the x-axis is horizontal, the y-axis vertical. The middle point is automatically at (0,0). The conveyor is always symmetric, reflected around the +y axis. Points must be given from the middle outwards.'),
		PAT([Vector2,],'wallPts',[Vector2(.35,.025),Vector2(.32,.15),Vector2(.4,.4)],unit='m',doc='Side wall cross-section; points are given using the same coordinates as the :ref:`bandPts`.'),
		PAT(bool,'half',True,"Only simulate one half of the problem, by making it symmetric around the middle plane."),
		PAT(float,'convWallLen',1.,unit='m',doc="Length of the part of the conveyor convered with wall."),
		PAT(float,'convBareLen',.5,unit='m',doc="Length of the conveyor without wall. In this part, the conveyor shape is interpolated so that it becomes flat at the exit cylinder's top (y=0 in cross-section coordinates)"),
		PAT(float,'exitCylDiam',.3,unit='m',doc="Diameter of the exit cylinder"),
		PAT(float,'convSlope',math.radians(15),unit='deg',doc="Slope of the conveyor; 0=horizontal, positive = ascending."),

		# shield
		PAT(int,'shieldType',0,choice=[(shieldNone,'none'),(shieldCylinder,'cylinder'),(shieldStl,'STL')],startGroup='Shield',doc='Type of chute shield'),
			# cylindrical shield
			PAT([float,],'cylShieldData',[-.25,-.5,1.,.02,0,math.radians(60)],hideIf='self.shieldType!=self.shieldCylinder',doc='Data for constructing shield. Meaning of numbers depends on :ref:`shieldType`. For shieldType==0, those are [horizontal distance, vertical distance, radius, subdivision size, low angle from the horizontal, high angle from the horizontal'),
			# sheidl from STL
			PAT(str,'stlShieldStl','conv1.stl',"STL file to load the shield from",hideIf=stlHideIf),
			PAT(float,'stlScale',.01,"Scale dimensions loaded from the STL with this factor (0.01: convert cm to m)",hideIf=stlHideIf),
			PAT(Vector3,'stlShift',Vector3(-5.13,14.,-27.1),"Move STL shield by this amount (after scaling)",unit='m',hideIf=stlHideIf),
			PAT(AlignedBox3,'stlFeedBox',AlignedBox3((-10,-.6,-.9),(10,.6,.2)),"Box defining input feed; facets enclosed in this volume will be removed and replaced by our feed.",unit='m',hideIf=stlHideIf),
			PAT(AlignedBox3,'stlOutBox',AlignedBox3((-.6,-.8,-2.8),(.4,-.5,-2.5)),"Box defining output feed; facets having at least one vertext in this volume will be marked as output conveyor with :ref:`stlOutVel` transport velocity.",unit='m/s',hideIf=stlHideIf),
			PAT(Vector3,'stlOutVel',Vector3(0,2.,0),"Transport velocity of the output band",hideIf=stlHideIf),
	
		PAT(float,'halfThick',0.,"Half thickness of created facets (if negative, relative to minimum particle radius"),


		# particles
		PAT([Vector2,],'psd',[Vector2(.03,0),Vector2(.05,.2),Vector2(.1,1.)],startGroup="Particles",unit=['mm','%'],doc="Particle size distribution of transported particles."),
		PAT(PelletMat,'material',PelletMat(density=3200,young=1e6,ktDivKn=.2,tanPhi=math.tan(.5),normPlastCoeff=50,kaDivKn=0.),"Material of particles"),
		PAT(PelletMat,'convMaterial',None,"Material of the conveyor (if not given, material of particles is used)"),

		# outputs
		PAT(str,'reportFmt',"/tmp/{tid}.xhtml",startGroup="Outputs",doc="Report output format (Scene.tags can be used)."),
		PAT(str,'feedCacheDir',".","Directory where to store pre-generated feed packings"),
		PAT(str,'saveFmt',"/tmp/{tid}-{stage}.bin.gz","Savefile format; keys are :ref:`Scene.tags` and additionally ``{stage}`` will be replaced by 'init', 'steady' and 'done'."),
		PAT(int,'backupSaveTime',1800,"How often to save backup of the simulation (0 or negative to disable)"),
		PAT(float,'vtkFreq',4,"How often should VtkExport run, relative to *factStepPeriod*. If negative, run never."),
		PAT(str,'vtkPrefix',"/tmp/{tid}-","Prefix for saving VtkExport data; formatted with ``format()`` providing :ref:`Scene.tags` as keys."),
		PAT([str,],'reportHooks',[],noGui=True,doc="Python expressions returning a 3-tuple with 1. raw HTML to be included in the report, 2. list of (figureName,matplotlibFigure) to be included in figures, 3. dictionary to be added to the 'custom' dict saved in the database."),

		# tunables
		PAT(int,'factStepPeriod',50,startGroup="Tunables",doc="Run factory (and deleters) every *factStepPeriod* steps."),
		PAT(float,'pWaveSafety',.7,"Safety factor for critical timestep"),
		PAT(float,'gravity',10.,unit='m/s²',doc="Gravity acceleration magnitude"),
		PAT(float,'rateSmooth',.2,"Smoothing factor for plotting rates in factory and deleters"),
		PAT(bool,'debugBand',False,"Return scene from makeBandFeedPack rather than the CHute scene, for debugging."),
	]
	def __init__(self,**kw):
		Preprocessor.__init__(self) ## important: http://boost.2283326.n4.nabble.com/Fwd-Passing-Python-derived-class-back-into-a-native-C-class-doesn-t-match-the-C-signature-td4034809.html
		self.wooPyInit(Chute,Preprocessor,**kw)
	def __call__(self):
		print 100*'@'
		import woo.pre.chute
		return woo.pre.chute.run(self)


def run(pre):
	import woo.pack
	print 'chute.run()'
	### input check
	for a in ['reportFmt','feedCacheDir','saveFmt','vtkPrefix']: setattr(pre,a,utils.fixWindowsPath(getattr(pre,a)))

	### create scene
	S=Scene(fields=[DemField()])
	S.pre=pre

	rMin=pre.psd[0][0]/2.
	rMax=pre.psd[-1][0]/2.
	S.dt=pre.pWaveSafety*utils.spherePWaveDt(rMin,pre.material.density,pre.material.young)

	if not pre.convMaterial: pre.convMaterial=pre.material.deepcopy()
	halfWallMat=pre.convMaterial.deepcopy()
	halfWallMat.tanPhi=0.0

	if pre.shieldType!=pre.shieldCylinder: pre.half=False

	wallMask=0b00110
	loneMask=0b00100 # particles with this mask don't interact with each other
	sphMask= 0b00011
	delMask= 0b00001 # particles which might be deleted by deleters
	S.dem.loneMask=loneMask

	relWd=(.5 if pre.half else 1.)
	convWd=2*pre.convPts[-1][0]
	if not pre.half:
		botLineCentered=[Vector2(-xy[0],xy[1]) for xy in reversed(pre.convPts)]+[Vector2(0,0)]+pre.convPts[:]
		botLineZero=[p+Vector2(.5*convWd,0) for p in botLineCentered]
		leftLineZero=[Vector2(-xy[0],xy[1])+Vector2(.5*convWd,0) for xy in reversed(pre.wallPts)] # unused when halved
		rightLineZero=[xy+Vector2(.5*convWd,0) for xy in pre.wallPts]
		ymin,ymax=min([p[0] for p in botLineCentered]),max([p[0] for p in botLineCentered])
	else:
		botLineZero=[Vector2(0,0)]+pre.convPts[:]
		botLineCentered=botLineZero
		rightLineZero=pre.wallPts[:]
		leftLineZero=None
		ymin,ymax=0,max([p[0] for p in botLineCentered])

	###
	### conveyor feed
	###
	print 'Preparing packing for conveyor feed, be patient'
	cellLen=10*pre.psd[-1][0]
	# excessWd=(30*rMax,15*rMax): unusable in our case, since we have walls
	ccrr=roro.makeBandFeedPack(dim=(cellLen,convWd*relWd,pre.convLayerHt),psd=pre.psd,mat=pre.material,gravity=(0,0,-pre.gravity),porosity=.5,damping=.3,memoizeDir=pre.feedCacheDir,botLine=botLineZero,leftLine=leftLineZero,rightLine=rightLineZero,dontBlock=pre.debugBand)
	if pre.debugBand: return ccrr # this is a Scene object
	else: cc,rr=ccrr

	# HACK: move to match conveyor position
	if pre.half: cc=[c+.25*convWd*Vector3.UnitY for c in cc] # center the feed when feeding both halves
	vol=sum([4/3.*math.pi*r**3 for r in rr])
	conveyorVel=(pre.massFlowRate*relWd)/(pre.material.density*vol/cellLen)
	print 'Feed velocity %g m/s to match feed mass %g kg/m (volume=%g m³, len=%gm, ρ=%gkg/m³) and massFlowRate %g kg/s (%g kg/s over real width)'%(conveyorVel,pre.material.density*vol/cellLen,vol,cellLen,pre.material.density,pre.massFlowRate*relWd,pre.massFlowRate)

	# if pre.halfThick is relative, compute its absolute value
	if pre.halfThick<0: pre.halfThick*=-rMin

	if 1:
		#  ^ +y in 2d, +z in 3d
		#  |
		#  |
		#
		#			wall
		#                  no wall   C = (0,0) in 
		#    ##########___.....-----+..
		#  A +----^^^^^            ( + ) D
		#      slope    B           ^-^ 
		#
		#  -----> +x in 2d, 3d

		C=Vector2(0,0)
		convRot2d=Vector2(math.cos(pre.convSlope),math.sin(pre.convSlope))
		convLen=pre.convBareLen+pre.convWallLen
		B=C-pre.convBareLen*convRot2d
		A=B-pre.convWallLen*convRot2d

		convRot3=Quaternion((0,1,0),-pre.convSlope)
		factoryNode=Node(pos=(A[0],0,A[1]),ori=convRot3)

		A3,B3,C3=Vector3(A[0],0,A[1]),Vector3(B[0],0,B[1]),Vector3(C[0],0,C[1])
		convGts=pack.sweptPolylines2gtsSurface(
			[[convRot3*(Vector3(0,p[0],p[1])+xShift*Vector3.UnitX) for p in botLineCentered] for xShift in (-pre.convBareLen-pre.convWallLen,-pre.convBareLen)]
			+[[Vector3(0,p[0],0) for p in botLineCentered]] # exit cylinder flattened conveyor
		)
		wallsGts=[]
		wallsGts.append(pack.sweptPolylines2gtsSurface([[convRot3*(Vector3(0,p[0],p[1])+Vector3(xShift,0,0)) for p in pre.wallPts] for xShift in (-convLen,-pre.convBareLen)]))
		if not pre.half:
			wallsGts.append(pack.sweptPolylines2gtsSurface([[convRot3*(Vector3(0,-p[0],p[1])+Vector3(xShift,0,0)) for p in reversed(pre.wallPts)] for xShift in (-convLen,-pre.convBareLen)]))
		
		S.dem.par.append(pack.gtsSurface2Facets(convGts,mat=pre.convMaterial,mask=wallMask,fakeVel=convRot3*Vector3(conveyorVel,0,0)))
		for wg in wallsGts: S.dem.par.append(pack.gtsSurface2Facets(wg,mat=pre.convMaterial,mask=wallMask))

		# exit cylinder
		exitCyl=utils.infCylinder(convRot3*Vector3(0,0,-pre.exitCylDiam),radius=pre.exitCylDiam,axis=1,glAB=(ymin,ymax),mat=pre.convMaterial,mask=wallMask)
		exitCyl.angVel=Vector3(0,conveyorVel/pre.exitCylDiam,0)
		S.dem.par.append(exitCyl)

		S.dem.collectNodes()
		xmin=min([min([n.pos[0] for n in p.shape.nodes]) for p in S.dem.par])
		zmin=-4*pre.exitCylDiam
		zmax=pre.exitCylDiam+3*pre.convLayerHt
		# use this to compute ballistic distance: http://physics.stackexchange.com/questions/27992/solving-for-initial-velocity-required-to-launch-a-projectile-to-a-given-destinat
		# take twice of what it gives
		xmax=2*(conveyorVel*math.cos(pre.convSlope)/pre.gravity)*(conveyorVel*math.sin(pre.convSlope)+math.sqrt((conveyorVel*math.sin(pre.convSlope))**2+2*pre.gravity*(abs(zmin)+pre.convLayerHt)))

	##
	## chute shield
	##
	if pre.shieldType==Chute.shieldNone: pass
	elif pre.shieldType==Chute.shieldCylinder:
		if len(pre.cylShieldData)!=6: raise ValueError('Cylindrical shield must provide exactly 6 cylShieldData values (not %d)'%len(pre.cylShieldData))
		shXoff,shZoff,shRad,shDivLen,shLow,shHigh=pre.cylShieldData
		shAng=shHigh-shLow
		thetas=numpy.linspace(shLow,shHigh,num=math.ceil(shAng*shRad/shDivLen))
		yy=numpy.linspace(ymin,ymax,num=math.ceil((ymax-ymin)/shDivLen))
		pts=[[Vector3(shXoff,y,shZoff)+shRad*Vector3(math.cos(theta),0,math.sin(theta)) for theta in thetas] for y in yy]
		shield=pack.gtsSurface2Facets(pack.sweptPolylines2gtsSurface(pts),mat=pre.convMaterial,mask=wallMask,wire=False,halfThick=pre.halfThick)
		for par in shield: par.matState=PelletMatState() # for dissipation tracking
		S.dem.par.append(shield)

		### middle wall
		S.dem.par.append(utils.wall(2*ymax,axis=1,sense=-1,visible=False,mat=pre.convMaterial,mask=wallMask))
		if pre.half: S.dem.par.append(utils.wall(0,axis=1,sense=1,visible=False,mat=halfWallMat,mask=wallMask))
		else: S.dem.par.append(utils.wall(2*ymin,axis=1,sense=-1,visible=False,mat=pre.convMaterial,mask=wallMask))
		## boundary walls
		S.dem.par.append([
			# utils.wall(zmin,axis=2,sense=1,visible=False,mat=pre.convMaterial,mask=wallMask),
			utils.wall(xmax,axis=0,sense=-1,visible=False,mat=pre.convMaterial,mask=wallMask)
		])

		zminmin=zmin-5*rMax
		deleters=[
			BoxDeleter(
				stepPeriod=pre.factStepPeriod,
				box=((0,2*ymin,zminmin),(xmax,2*ymax,-pre.exitCylDiam)),
				glColor=.1,
				save=True,
				mask=delMask,
				label='bucket',
				inside=True
			),
			BoxDeleter(
				stepPeriod=pre.factStepPeriod,
				box=((xmin,2*ymin,zminmin),(xmax,2*ymax,zmax)),
				glColor=.9,
				save=False,
				mask=delMask,
				label='lost',
				inside=False
			)
		]


	elif pre.shieldType==Chute.shieldStl:
		surf=stlShieldImport(pre.stlShieldStl)
		surf.scale(pre.stlScale,pre.stlScale,pre.stlScale)
		surf.translate(pre.stlShift[0],pre.stlShift[1],pre.stlShift[2])

		tri=pack.gtsSurface2Facets(surf,mat=pre.convMaterial,mask=wallMask,wire=True,halfThick=pre.halfThick)
		# remove feed band completely
		tri=[t for t in tri if not sum([1 for n in t.nodes if (n.pos in pre.stlFeedBox)])]
		for t in tri:
			#	out band
			if sum([1 for n in t.nodes if (n.pos in pre.stlOutBox)]):
				t.shape.fakeVel=pre.stlOutVel
				t.shape.wire=False
				t.shape.color=1
				t.matState=None
			# box itself
			else:
				t.matState=woo.dem.PelletMatState()
		bb=AlignedBox3()
		print 'Adding %d facets'%len(tri)
		S.dem.par.append(tri)
		for p in S.dem.par: 
			for n in p.nodes: bb.extend(n.pos)
		deleters=[BoxDeleter(stepPeriod=pre.factStepPeriod,box=bb,glColor=.1,save=True,mask=delMask,label='domain',inside=False)]
	else: raise ValueError("Unknow value for Chute.shieldType: %d"%(pre.shieldType))

	# for dissipation tracking
	S.trackEnergy=True

	factory=ConveyorFactory(
		stepPeriod=pre.factStepPeriod,
		material=pre.material,
		centers=cc,radii=rr,
		cellLen=cellLen,
		barrierColor=.3,
		barrierLayer=-10., # 10x max radius
		color=0,
		glColor=.5,
		#color=.4, # random colors
		node=factoryNode,
		mask=sphMask,
		vel=conveyorVel,
		label='factory',
		maxMass=-1, # not limited, until steady state is reached
		currRateSmooth=pre.rateSmooth,
	)

	###
	### engines
	###
	S.dem.gravity=(0,0,-pre.gravity)
	S.engines=utils.defaultEngines(damping=0.,verletDist=.05*rMin,
			cp2=Cp2_PelletMat_PelletPhys(),
			law=Law2_L6Geom_PelletPhys_Pellet(plastSplit=True)
		)+deleters+[factory,
		#PyRunner(factStep,'import woo.pre.roro; woo.pre.roro.savePlotData(S)'),
		#PyRunner(factStep,'import woo.pre.roro; woo.pre.roro.watchProgress(S)'),
		]+(
			[Tracer(stepPeriod=20,num=100,compress=2,compSkip=4,dead=True,scalar=Tracer.scalarRadius)] if 'opengl' in woo.config.features else []
		)+(
			[] if (not pre.vtkPrefix or pre.vtkFreq<=0) else [VtkExport(out=pre.vtkPrefix,stepPeriod=int(pre.vtkFreq*pre.factStepPeriod),what=VtkExport.all,subdiv=16)]
		)+(
			[] if pre.backupSaveTime<=0 else [PyRunner(realPeriod=pre.backupSaveTime,stepPeriod=-1,command='S.save(S.pre.saveFmt.format(stage="backup-%06d"%(S.step),S=S,**(dict(S.tags))))')]
		)

	#
	# TODO: this should be user-configurable; in particular, batches don't need that at all
	#
	S.dem.saveDeadNodes=True


	try:
		import woo.gl
		# set other display options and save them (static attributes)
		#woo.gl.Gl1_DemField.colorRanges[woo.gl.Gl1_DemField.colorVel].range=(0,.5)
		from woo.gl import Gl1_DemField
		Gl1_DemField.colorRanges[Gl1_DemField.colorMatState].cmap=(woo.master.cmaps.index('helix') if 'helix' in woo.master.cmaps else -1)
		S.any=[
			woo.gl.Renderer(iniUp=(0,0,1),iniViewDir=(0,1,0)),
			#woo.gl.Gl1_DemField(glyph=woo.gl.Gl1_DemField.glyphVel),
			Gl1_DemField(
				shape=Gl1_DemField.shapeNonSpheres,
				colorBy=Gl1_DemField.colorMatState,
				colorBy2=Gl1_DemField.colorVel,
			),
			woo.gl.Gl1_InfCylinder(wire=True),
			woo.gl.Gl1_Wall(div=1),
		]
	except ImportError: pass
	
	
	return S






def stlShieldImport(filename,cleanupRelSize=1e-4):
	from minieigen import AlignedBox3
	import gts
	tagStack=[]
	surf,bbox,loop=None,None,None
	numFacets=0
	for lineno,l in enumerate(open(filename).readlines()):
		l=l.strip().lower()
		if l.startswith('solid mesh'):
			surf=gts.Surface()
			tagStack.append('solid mesh')
		elif l.startswith('facet'): tagStack.append('facet')
		elif l.startswith('outer loop'):
			tagStack.append('outer loop')
			loop=[]
		elif l.startswith('endloop'):
			assert tagStack.pop()=='outer loop'
		elif l.startswith('endfacet'):
			assert tagStack.pop()=='facet'
			assert len(loop)==3
			numFacets+=1
			surf.add(gts.Face(gts.Edge(loop[0],loop[1]),gts.Edge(loop[1],loop[2]),gts.Edge(loop[2],loop[0])))
		elif l.startswith('endsolid mesh'):
			assert tagStack.pop()=='solid mesh'
			print 'Imported %d facets, bbox is'%numFacets,bbox
			sz=(bbox.max-bbox.min).norm() # FIXME: bbox.size().norm()
			surf.cleanup(sz*cleanupRelSize)
		elif l.startswith('vertex '):
			if tagStack!=['solid mesh','facet','outer loop']: raise ValueError('Structure of the STL file is unhandled at line %d: "vertex" can only appear inside ["solid mesh","facet","outer loop"] (not %s)'%(lineno,str(tagStack)))
			pt=Vector3([float(f) for f in l.split()[1:]])
			if bbox==None: bbox=AlignedBox3(pt,pt)
			else: bbox.extend(pt)
			loop.append(gts.Vertex(pt[0],pt[1],pt[2]))
	return surf

