import woo
from woo.dem import *
from woo.core import *
from woo import *
from woo import utils

S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-10))])
m=utils.defaultMaterial()
S.dem.par.add([
	utils.facet([(0,0,0),(1,0,0),(0,1,0)],halfThick=0.3,mat=m),
	utils.sphere((.2,.2,1),.3,mat=m)
])
S.engines=utils.defaultEngines(damping=.4)
S.dt=.1*utils.pWaveDt(S)
S.dem.collectNodes()
S.saveTmp()
