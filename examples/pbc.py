import woo, woo.core, woo.utils, woo.dem, woo.gl
from minieigen import *
from math import *
S=woo.master.scene=woo.core.Scene(fields=[woo.dem.DemField()])
S.dem.par.add([
	#woo.utils.capsule((.5,.2,.2),radius=.1,shaft=.2,ori=Quaternion.Identity,color=0),
	#woo.utils.capsule((.2,.5,.2),radius=.1,shaft=.2,ori=Quaternion((0,0,1),pi/2.),color=.5),
	#woo.utils.capsule((.2,.2,.5),radius=.1,shaft=.2,ori=Quaternion((0,1,0),-pi/2.),color=1.),
	woo.utils.sphere((.5,.8,.8),radius=.1,color=0),
	woo.utils.sphere((.8,.5,.8),radius=.1,color=.5),
	woo.utils.sphere((.8,.8,.5),radius=.1,color=1),
])
S.dem.collectNodes()
woo.gl.Gl1_Sphere.wire=True
# for n in S.dem.nodes: n.dem.isAspherical=False
S.periodic=True
S.cell.nextGradV=(((0,1,1),(-1,0,1),(-1,-1,0)))
S.dt=1e-5
S.throttle=1e-4
S.engines=woo.utils.defaultEngines()
S.saveTmp()
print S.dem.par[0].angVel
S.one()
print S.dem.par[0].angVel
