from woo.core import*
from woo.dem import *
import woo.gl
from woo import utils
S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-10))])
S.dem.par.append([
	utils.wall((0,0,0),axis=2,sense=1),
	utils.sphere((0,0,1),.2)
])
S.dem.par[1].vel=(0,1,0)
S.dt=.7*utils.pWaveDt()
S.engines=utils.defaultEngines()+[woo.dem.Tracer(num=64,compress=2,stepPeriod=10,compSkip=1)]
S.saveTmp()
import woo.qt
woo.qt.View()
S.run(10000)
