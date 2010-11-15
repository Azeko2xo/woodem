from yade import pack,plot

sp=pack.SpherePack();
# bunch of balls, with an infinite plane just underneath
if 0: sp.makeCloud((0,0,0),(1,1,1),.05,.5);
# use clumps of 2 spheres instead, to have rotation without friction 
else: sp.makeClumpCloud((0,0,0),(1,1,1),[pack.SpherePack([((0,0,0),.05),((0,0,.08),.02)])],periodic=False)
sp.toSimulation()
O.bodies.append(utils.wall(position=0,axis=2))

O.engines=[
	ForceResetter(),
	InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Wall_Aabb()]),
	InteractionLoop([Ig2_Sphere_Sphere_ScGeom(),Ig2_Wall_Sphere_ScGeom()],[Ip2_FrictMat_FrictMat_FrictPhys(frictAngle=0)],[Law2_ScGeom_FrictPhys_CundallStrack()]),
	GravityEngine(gravity=(0,0,-9.81)),
	NewtonIntegrator(damping=0.),
	PyRunner(iterPeriod=10,command='addPlotData()'),
]
O.dt=utils.PWaveTimeStep()

def addPlotData():
	plot.addData(i=O.iter,total=O.energy.total(),**O.energy)

# turn on energy tracking
O.trackEnergy=True
O.saveTmp()
# run a bit to have all energy categories in O.energy.keys().

# The number of steps when all energy contributions are already non-zero
# is only empirical; you can always force replot by redefining plot.plots
# as below, closing plots and running plot.plot() again
#
# (unfortunately even if we were changing plot.plots periodically,
# plots would not pick up changes in plot.plots during live plotting)
O.run(400,True)  
plot.plots={'i':['total',]+O.energy.keys()}
plot.plot(subPlots=True)

O.run()
