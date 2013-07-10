from woo.core import *
from woo.dem import *
from minieigen import *

import woo.log
#woo.log.setLevel('PsdClumpGenerator',woo.log.TRACE)
#woo.log.setLevel('PsdSphereGenerator',woo.log.TRACE)
woo.log.setLevel('RandomFactory',woo.log.DEBUG)

S=woo.master.scene=Scene(fields=[DemField()])
S.engines=[
	InsertionSortCollider([Bo1_Sphere_Aabb()]),
	BoxFactory(
		box=((0,0,0),(1,1,1)),
		maxMass=-1,maxNum=-1,massFlowRate=0,maxAttempts=50000,attemptPar=50,atMaxAttempts=BoxFactory.maxAttWarn,
		generator=PsdClumpGenerator(
			psdPts=[(.1,0),(.2,1.)],discrete=False,mass=True,
			clumps=[
				SphereClumpGeom(centers=[(0,0,-.15),(0,0,-.05),(0,0,.05)],radii=(.05,.10,.05),scaleProb=[(0,1.)]),
				SphereClumpGeom(centers=[(.05,0,0) ,(0,.05,0) ,(0,0,.05)],radii=(.05,.05,.05),scaleProb=[(0,.5)]),
			],
		),
		materials=[woo.dem.ElastMat(density=1)],
	)
]
S.one()

from woo import pack
sp=pack.SpherePack()
sp.fromSimulation(S)

#S2=woo.core.Scene()
#sp.toSimulation(S2)
#woo.master.scene=S2
