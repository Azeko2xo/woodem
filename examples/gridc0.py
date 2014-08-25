from woo.core import *
from woo.dem import *
from woo import timing
import woo, woo.utils
import sys
S=woo.master.scene
S.fields=[DemField(gravity=(5,0,-10))]
S.lab.spheMask=0b001
S.lab.wallMask=0b011
S.lab.loneMask=0b010
S.dem.loneMask=S.lab.loneMask
woo.master.timingEnabled=True

rr=.02
#rr=.1
cell=rr
verlet=.1*rr
verletSteps=0
mat=woo.utils.defaultMaterial()

gridCollider=GridCollider(domain=((-1,-1,-1),(2,3,3)),minCellSize=cell,boundDispatcher=GridBoundDispatcher(functors=[Grid1_Sphere(),Grid1_Wall(movable=False)]),label='collider',verletDist=verlet,verletSteps=verletSteps,gridDense=5,exIniSize=4,complSetMinSize=3,useDiff=('diff' in sys.argv))
sortCollider=InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Wall_Aabb()],verletDist=verlet)



S.engines=[
	Leapfrog(damping=0.3,reset=True),
	(sortCollider if 'sort' in sys.argv else gridCollider),
	ContactLoop(
		[Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom(),Cg2_Wall_Sphere_L6Geom(),Cg2_InfCylinder_Sphere_L6Geom()],
		[Cp2_FrictMat_FrictPhys()],
		[Law2_L6Geom_FrictPhys_IdealElPl(noSlip=True)],
		applyForces=True,
		label='contactLoop',
	),
	PyRunner(1000,'import woo.timing; print("step %d, %d contacts, %g%% real"%(S.step,len(S.dem.con),S.dem.con.realRatio()*100)); woo.timing.stats(); woo.timing.reset(); S.lab.collider.nFullRuns=0;'),
]

if 0:
	for c,r in [((0,0,.4),.2),((1,1,1),.3),((1.3,2,2),.5),((.5,.5,.5),rr)]:
		S.dem.par.add(woo.utils.sphere(c,r,mask=S.lab.spheMask))
	S.lab.collider.renderCells=True
else:
	import woo.pack
	sp=woo.pack.SpherePack()
	print 'Making cloud...'
	sp.makeCloud((.1,.1,.0),(1.1,1.1,1.0),rMean=rr,rRelFuzz=.3)
	print 'Generated cloud with %d spheres'%(len(sp))
	sp.toSimulation_fast(S,mat=mat,mask=S.lab.spheMask)
	print 'Spheres added to Scene'
S.dem.par.add(woo.utils.wall(0,axis=2,sense=1,mat=mat,mask=S.lab.wallMask))
S.dem.par.add(woo.utils.wall(1.9,axis=0,sense=-1,mat=mat,mask=S.lab.wallMask))
S.dem.collectNodes()
S.dt=.5*woo.utils.pWaveDt()

S.saveTmp()
#S.run(3001,wait=True)
