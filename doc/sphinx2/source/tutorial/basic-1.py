import woo
from woo.dem import *
from woo.core import *
# scene:
S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,-9.81))])
# particles:
S.dem.par.append([Wall.make(0,axis=2),Sphere.make((0,0,2),radius=.2)])
S.dem.collectNodes()
# engines:
S.engines=S.dem.minimalEngines(damping=.2)
# extras:
S.throttle=0.005
S.saveTmp()
