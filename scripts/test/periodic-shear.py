O.periodic=True
O.cell.refSize=(.55,.55,.55)
O.bodies.append(utils.facet([[.4,.0001,.3],[.2,.0001,.3],[.3,.2,.2]]))
O.bodies.append(utils.sphere([.3,.1,.4],.05,dynamic=True))
O.bodies.append(utils.sphere([.200001,.2000001,.4],.05,dynamic=False))
O.bodies.append(utils.sphere([.3,0,0],.1,dynamic=False))
O.engines=[
	ForceResetter(),
	InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()],label='isc'),
	InteractionLoop(
		[Ig2_Sphere_Sphere_Dem3DofGeom(),Ig2_Facet_Sphere_Dem3DofGeom()],
		[Ip2_FrictMat_FrictMat_FrictPhys()],
		[Law2_Dem3DofGeom_FrictPhys_CundallStrack()]
	),
	GravityEngine(gravity=(0,0,-10)),
	NewtonIntegrator(),
	#PyRunner(command='doCellFlip()',realPeriod=5)
]

def doCellFlip():
	flip=1 if O.cell.trsf[1,2]<0 else -1;
	utils.flipCell(Matrix3(0,0,0, 0,0,flip, 0,0,0))

#g=0.
#while False:
#	O.cellShear=Vector3(.2*sin(g),.2*cos(pi*g),.2*sin(2*g)+.2*cos(3*g))
#	time.sleep(0.001)
#	g+=1e-3
O.cell.trsf=Matrix3(1,0,0, 0,1,.5, 0,0,1)
O.dt=2e-2*utils.PWaveTimeStep()
O.step()
O.saveTmp()
rrr=yade.qt.Renderer()
rrr.intrAllWire,rrr.bound=True,True
#isc.watch1,isc.watch2=0,-1

#from woo import log
#import woo.qt,time
#v=yade.qt.View()
#v.axes=True
#v.grid=(True,True,True)

from woo import log
#log.setLevel('Shop',log.TRACE)
#log.setLevel('InsertionSortCollider',log.TRACE)

