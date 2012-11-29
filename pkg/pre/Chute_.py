# encoding: utf-8
from woo import utils,pack
from woo.core import *
from woo.dem import *
from woo.pre import Roro_
import woo.config
import woo.plot
from minieigen import *
import math
import sys
import numpy
nan=float('nan')


import woo.pyderived
from woo.dem import PelletMat

class ChutePy(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'Python variant of the Chute preprocessor'
	_classTraits=None
	PAT=woo.pyderived.PyAttrTrait
	_attrTraits=[
		PAT(float,'massFlowRate',1000*woo.unit['t/h'],unit='t/h',startGroup='General',doc='Mass flow rate of particles transported on the conveyor'),
		PAT(float,'convLayerHt',.2,unit='m',doc='Height of transported material'),
		PAT(float,'time',2,unit='s',doc='Time of the simulation'),

		# conveyor
		PAT([Vector2,],'convPts',[Vector2(.2,0),Vector2(.4,.1)],unit='m',startGroup='Conveyor',doc='Half of the the conveyor cross-section; the x-axis is horizontal, the y-axis vertical. The middle point is automatically at (0,0). The conveyor is always symmetric, reflected around the +y axis. Points must be given from the middle outwards.'),
		PAT([Vector2,],'wallPts',[Vector2(.25,.025),Vector2(.22,.15),Vector2(.3,.3)],unit='m',doc='Side wall cross-section; points are given using the same coordinates as the :ref:`bandPts`.'),
		PAT(bool,'half',True,"Only simulate one half of the problem, by making it symmetric around the middle plane."),
		PAT(float,'convWallLen',.5,unit='m',doc="Length of the part of the conveyor convered with wall."),
		PAT(float,'convBareLen',.5,unit='m',doc="Length of the conveyor without wall. In this part, the conveyor shape is interpolated so that it becomes flat at the exit cylinder's top (y=0 in cross-section coordinates)"),
		PAT(float,'exitCylDiam',.2,unit='m',doc="Diameter of the exit cylinder"),
		PAT(float,'convSlope',math.radians(15),unit='deg',doc="Slope of the conveyor; 0=horizontal, positive = ascending."),

		# shield
		PAT(int,'shieldType',0,choice=[(-1,'none'),(0,'cylinder')],startGroup='Shield',doc='Type of chute shield'),
		PAT([float,],'shieldData',[-.45,-.5,1.,.01,0,math.radians(60)],'Data for constructing shield. Meaning of numbers depends on :ref:`shieldType`. For shieldType==0, those are [horizontal distance, vertical distance, radius, subdivision size, low angle from the horizontal, high angle from the horizontal'),

		# particles
		PAT([Vector2,],'psd',[Vector2(.02,0),Vector2(.03,.2),Vector2(.04,1.)],startGroup="Particles",unit=['mm','%'],doc="Particle size distribution of transported particles."),
		PAT(PelletMat,'material',PelletMat(density=3200,young=1e5,ktDivKn=.2,tanPhi=math.tan(.5),normPlastCoeff=50,kaDivKn=0.),"Material of particles"),
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
		PAT(int,'factStepPeriod',200,startGroup="Tunables",doc="Run factory (and deleters) every *factStepPeriod* steps."),
		PAT(float,'pWaveSafety',.7,"Safety factor for critical timestep"),
		PAT(float,'gravity',10.,unit='m/s²',doc="Gravity acceleration magnitude"),
		PAT(float,'rateSmooth',.2,"Smoothing factor for plotting rates in factory and deleters"),
	]
	def __init__(self,**kw):
		Preprocessor.__init__(self) ## important: http://boost.2283326.n4.nabble.com/Fwd-Passing-Python-derived-class-back-into-a-native-C-class-doesn-t-match-the-C-signature-td4034809.html
		self.wooPyInit(ChutePy,Preprocessor,**kw)
	def __call__(self):
		print 100*'@'
		import woo.pre.Chute_
		return woo.pre.Chute_.run(self)



def run(pre):
	import woo.pack
	print 'Chute_.run()'
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
	cc,rr=Roro_.makeBandFeedPack(dim=(cellLen,convWd*relWd,pre.convLayerHt),psd=pre.psd,mat=pre.material,gravity=(0,0,-pre.gravity),porosity=.7,damping=.3,memoizeDir=pre.feedCacheDir,botLine=botLineZero,leftLine=leftLineZero,rightLine=rightLineZero,dontBlock=False)
	# HACK: move to match conveyor position
	if pre.half: cc=[c+.25*convWd*Vector3.UnitY for c in cc] # center the feed when feeding both halves
	vol=sum([4/3.*math.pi*r**3 for r in rr])
	conveyorVel=(pre.massFlowRate*relWd)/(pre.material.density*vol/cellLen)
	print 'Feed velocity %g m/s to match feed mass %g kg/m (volume=%g m³, len=%gm, ρ=%gkg/m³) and massFlowRate %g kg/s (%g kg/s over real width)'%(conveyorVel,pre.material.density*vol/cellLen,vol,cellLen,pre.material.density,pre.massFlowRate*relWd,pre.massFlowRate)

	facetHalfThick=4*rMin

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
	if pre.shieldType==-1: pass
	elif pre.shieldType==0:
		if len(pre.shieldData)!=6: raise ValueError('Cylindrical shield must provide exactly 6 shieldData values (not %d)'%len(pre.shieldData))
		shXoff,shZoff,shRad,shDivLen,shLow,shHigh=pre.shieldData
		shAng=shHigh-shLow
		thetas=numpy.linspace(shLow,shHigh,num=math.ceil(shAng*shRad/shDivLen))
		yy=numpy.linspace(ymin,ymax,num=math.ceil((ymax-ymin)/shDivLen))
		pts=[[Vector3(shXoff,y,shZoff)+shRad*Vector3(math.cos(theta),0,math.sin(theta)) for theta in thetas] for y in yy]
		shield=pack.gtsSurface2Facets(pack.sweptPolylines2gtsSurface(pts),mat=pre.convMaterial,mask=wallMask,wire=False)
		for par in shield: par.matState=PelletMatState() # for dissipation tracking
		S.dem.par.append(shield)

		S.trackEnergy=True



	### middle wall
	S.dem.par.append(utils.wall(2*ymax,axis=1,sense=-1,visible=False,mat=pre.convMaterial,mask=wallMask))
	if pre.half: S.dem.par.append(utils.wall(0,axis=1,sense=1,visible=False,mat=halfWallMat,mask=wallMask))
	else: S.dem.par.append(utils.wall(2*ymin,axis=1,sense=-1,visible=False,mat=pre.convMaterial,mask=wallMask))
	## boundary walls
	S.dem.par.append([
		utils.wall(zmin,axis=2,sense=1,visible=False,mat=pre.convMaterial,mask=wallMask),
		utils.wall(xmax,axis=0,sense=-1,visible=False,mat=pre.convMaterial,mask=wallMask)
	])

	zminmin=zmin-5*rMax
	bucket=BoxDeleter(
		stepPeriod=pre.factStepPeriod,
		box=((0,ymin,zminmin),(xmax,2*ymax,-pre.exitCylDiam)),
		glColor=.1,
		save=True,
		mask=delMask,
		label='bucket',
		inside=True
	)
	lost=BoxDeleter(
		stepPeriod=pre.factStepPeriod,
		box=((xmin,ymin,zminmin),(0,2*ymax,A[1])),
		glColor=.9,
		save=False,
		mask=delMask,
		label='lost',
		inside=True
	)
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
		)+[
		bucket,
		lost,
		factory,
		#PyRunner(factStep,'import woo.pre.Roro_; woo.pre.Roro_.savePlotData(S)'),
		#PyRunner(factStep,'import woo.pre.Roro_; woo.pre.Roro_.watchProgress(S)'),
		]+(
			[Tracer(stepPeriod=20,num=100,compress=2,compSkip=4,dead=True,scalar=Tracer.scalarRadius)] if 'opengl' in woo.config.features else []
		)+(
			[] if (not pre.vtkPrefix or pre.vtkFreq<=0) else [VtkExport(out=pre.vtkPrefix,stepPeriod=int(pre.vtkFreq*pre.factStepPeriod),what=VtkExport.all,subdiv=16)]
		)+(
			[] if pre.backupSaveTime<=0 else [PyRunner(realPeriod=pre.backupSaveTime,stepPeriod=-1,command='S.save(S.pre.saveFmt.format(stage="backup-%06d"%(S.step),S=S,**(dict(S.tags))))')]
		)


	try:
		import woo.gl
		# set other display options and save them (static attributes)
		#woo.gl.Gl1_DemField.colorRanges[woo.gl.Gl1_DemField.colorVel].range=(0,.5)
		S.any=[
			woo.gl.Renderer(iniUp=(0,0,1),iniViewDir=(0,1,0)),
			#woo.gl.Gl1_DemField(glyph=woo.gl.Gl1_DemField.glyphVel),
			woo.gl.Gl1_DemField(colorBy=woo.gl.Gl1_DemField.colorMatState),
			woo.gl.Gl1_InfCylinder(wire=True),
			woo.gl.Gl1_Wall(div=1),
		]
	except ImportError: pass
	
	
	return S





