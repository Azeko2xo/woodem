from woo.core import *
from woo.dem import *
import woo, woo.utils
S=woo.master.scene
S.fields=[DemField(gravity=(2,0,-10))]
S.engines=[
	Leapfrog(damping=0.,reset=True),
	GridCollider(domain=((-1,-1,-1),(2,3,4)),minCellSize=.2,gridBoundDispatcher=GridBoundDispatcher(functors=[Grid1_Sphere(),Grid1_Wall()]),label='collider'),
	ContactLoop(
		[Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom(),Cg2_Wall_Sphere_L6Geom(),Cg2_InfCylinder_Sphere_L6Geom()],
		[Cp2_FrictMat_FrictPhys()],
		[Law2_L6Geom_FrictPhys_IdealElPl(noSlip=True)],
		applyForces=True
	),
]


for c,r in [((0,0,.4),.2),((1,1,1),.3),((1.3,2,2),.5)]:
	S.dem.par.append(woo.utils.sphere(c,r))
S.dem.par.append(woo.utils.wall(0,axis=2))
S.dem.par.append(woo.utils.wall(1.9,axis=0))
S.dem.collectNodes()
S.dt=.7*woo.utils.pWaveDt()

S.saveTmp()
S.one()
#for p in S.dem.par:
#	print p,p.shape.bound.cells
