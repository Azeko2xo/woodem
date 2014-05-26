import woo, woo.core, woo.dem, woo.utils, woo.gl
from minieigen import *
from math import *

S=woo.master.scene=woo.core.Scene(dtSafety=.1,throttle=0.01,fields=[woo.dem.DemField(gravity=(0,0,-10))])
mat=woo.dem.FrictMat(young=1e6,ktDivKn=.2,density=3e3)
S.dem.par.append([woo.utils.capsule((0,0,.5),radius=.3,shaft=.8,ori=Quaternion((0,1,0),-pi/8.),wire=True,mat=mat),woo.utils.wall(0,axis=2,sense=1,mat=mat)])
S.dem.collectNodes()
S.engines=woo.utils.defaultEngines(dynDtPeriod=100,damping=.4)
S.saveTmp()

# view setup
woo.gl.Gl1_DemField.cPhys=True
woo.gl.Gl1_CPhys.relMaxRad=.1
woo.gl.Renderer.allowFast=False
woo.gl.Renderer.iniViewDir=(0,-1,0)
woo.gl.Gl1_Capsule.smooth=True

