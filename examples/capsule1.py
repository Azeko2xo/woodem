import woo.core, woo.dem, woo.utils, woo.gl
from math import *
from minieigen import *
S=woo.master.scene=woo.core.Scene(fields=[woo.dem.DemField(gravity=(0,0,-10))])
m=woo.dem.FrictMat(density=5000,young=5e6,ktDivKn=.5)
S.dem.par.append([
	woo.utils.capsule((.33,.33,.7),shaft=.3,radius=.2,ori=Quaternion((0,1,0),pi/3.),mat=m),
	woo.utils.facet([(1,0,0),(0,1,0),(-1,0,0)],halfThick=0,mat=m),
	woo.utils.sphere((.23,.34,1.3),radius=.15,mat=m),
	woo.utils.wall(-.5,axis=2,mat=m),
	woo.utils.infCylinder((.33,.33,0),radius=.3,axis=0,mat=m),
])
S.dem.par[-1].angVel=(1,0,0)
S.dem.collectNodes()
S.engines=woo.utils.defaultEngines(damping=.4)
S.dtSafety=.1
#S.run(192,True);
S.throttle=5e-3
S.saveTmp()

from woo.gl import *
Gl1_Capsule(wire=True),Gl1_DemField(cPhys=True,cNode=Gl1_DemField.cNodeNode),Renderer(iniViewDir=(0,1,0))

