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


def run(pre):
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

	### middle wall
	S.dem.par.append(utils.wall(2*ymax,axis=1,sense=-1,visible=False,mat=pre.convMaterial,mask=wallMask))
	if pre.half: S.dem.par.append(utils.wall(0,axis=1,sense=1,visible=False,mat=halfWallMat,mask=wallMask))
	else: S.dem.par.append(utils.wall(2*ymin,axis=1,sense=-1,visible=False,mat=pre.convMaterial,mask=wallMask))
	## boundary walls
	S.dem.par.append([
		utils.wall(zmin+4*rMax,axis=2,sense=1,visible=False,mat=pre.convMaterial,mask=wallMask),
		utils.wall(xmax,axis=0,sense=-1,visible=False,mat=pre.convMaterial,mask=wallMask)
	])


	bucket=BoxDeleter(
		stepPeriod=pre.factStepPeriod,
		box=((0,ymin,zmin),(xmax,2*ymax,-pre.exitCylDiam)),
		glColor=.1,
		save=True,
		mask=delMask,
		label='bucket',
		inside=True
	)
	lost=BoxDeleter(
		stepPeriod=pre.factStepPeriod,
		box=((xmin,ymin,zmin),(0,2*ymax,A[1])),
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
		# what falls inside
		#BoxDeleter(stepPeriod=factStep,inside=True,box=((cylXzd[-1][0]+cylXzd[-1][2]/2.,ymin,zmin),(xmax,ymax,zmax)),glColor=.9,save=True,mask=delMask,currRateSmooth=pre.rateSmooth,recoverRadius=recoverRadius,label='fallOver'),
		# generator
		bucket,
		lost,
		factory,
		#PyRunner(factStep,'import woo.pre.Roro_; woo.pre.Roro_.savePlotData(S)'),
		#PyRunner(factStep,'import woo.pre.Roro_; woo.pre.Roro_.watchProgress(S)'),
		]
		#]+(
		#	[Tracer(stepPeriod=20,num=100,compress=2,compSkip=4,dead=True,scalar=Tracer.scalarRadius)] if 'opengl' in woo.config.features else []
		#)+(
		#	[] if (not pre.vtkPrefix or pre.vtkFreq<=0) else [VtkExport(out=pre.vtkPrefix,stepPeriod=int(pre.vtkFreq*pre.factStepPeriod),what=VtkExport.all,subdiv=16)]
		#)+(
		#[] if pre.backupSaveTime<=0 else [PyRunner(realPeriod=pre.backupSaveTime,stepPeriod=-1,command='S.save(S.pre.saveFmt.format(stage="backup-%06d"%(S.step),S=S,**(dict(S.tags))))')]
		#)
	
	return S





