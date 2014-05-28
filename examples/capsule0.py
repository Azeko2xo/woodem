import woo.core, woo.utils, woo
from woo.dem import *
from woo.gl import *
from minieigen import *
from math import *
import numpy
from random import random
S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,-10))])
S.dem.loneMask=0b0010

mat=FrictMat(young=1e6,density=3200)

generators=[
	PsdCapsuleGenerator(psdPts=[(.15,0),(.25,.8),(.3,1.)],discrete=False,mass=True,shaftRadiusRatio=(1.,2.)),
	# PsdEllipsoidGenerator(psdPts=[(.15,0),(.3,1.)],discrete=False,mass=True,axisRatio2=(.2,.6),axisRatio3=(.2,.6)),
	PsdClumpGenerator(psdPts=[(.15,0),(.25,1.)],discrete=False,mass=True,
		clumps=[
			SphereClumpGeom(centers=[(0,0,-.06),(0,0,0),(0,0,.06)],radii=(.05,.08,.05),scaleProb=[(0,1.)]),
			SphereClumpGeom(centers=[(-.03,0,0),(0,.03,0)],radii=(.05,.05),scaleProb=[(0,.5)]),
		],
	),
	PsdSphereGenerator(psdPts=[(.15,0),(.25,.8),(.3,1.)],discrete=False,mass=True),
]


S.dem.par.append([
	woo.utils.facet([(.6,0,.3),(0,.6,.3),(-.6,0,.3)],halfThick=.1,wire=False),
	woo.utils.wall(0,axis=2,sense=1),
	woo.utils.infCylinder((.33,.33,0),radius=.3,axis=1),
])
S.dem.par[-1].angVel=(0,.5,0)
S.dem.collectNodes()
S.dtSafety=.8
S.engines=woo.utils.defaultEngines(damping=.4,dynDtPeriod=10)+[
	BoxFactory(
		box=((-1,-1,1),(1,1,3)),
		stepPeriod=100,
		maxMass=3e3,maxNum=-1,massFlowRate=0,maxAttempts=100,attemptPar=50,atMaxAttempts=BoxFactory.maxAttWarn,
		generator=generators[0],
		materials=[mat]
	),
#	BoxFactory(
#		box=((-1,-1,3),(1,1,5)),
#		stepPeriod=200,
#		maxMass=3e3,maxNum=-1,massFlowRate=0,maxAttempts=100,attemptPar=50,atMaxAttempts=BoxFactory.maxAttWarn,
#		generator=generators[1],
#		materials=[mat]
#	),
#	BoxFactory(
#		box=((-1,-1,5),(1,1,7)),
#		stepPeriod=100,
#		maxMass=3e3,maxNum=-1,massFlowRate=0,maxAttempts=100,attemptPar=50,atMaxAttempts=BoxFactory.maxAttWarn,
#		generator=generators[2],
#		materials=[mat]
#	)
]

# S.any=[Gl1_Ellipsoid(wire=True),Gl1_DemField(cPhys=True,cNode=Gl1_DemField.cNodeNode),Renderer(iniViewDir=(0,1,0))]
# S.throttle=.03
S.saveTmp()
Gl1_DemField.colorBy=Gl1_DemField.colorRadius
Gl1_Sphere.quality=3
Renderer.engines=False
Renderer.iniViewDir=Vector3(-0.38001633552330316,-0.5541855363199173,-0.7405848878212721)
Renderer.iniUp=Vector3(-0.4745913986125537,-0.582504699162141,0.6598873235765362)
# Renderer.allowFast=False
# woo.master.timingEnabled=True
# Gl1_DemField(nodes=False,bound=False,cNode=0)
