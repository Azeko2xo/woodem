from woo import utils
from woo.core import *
from woo.dem import *
from woo import plot
from woo import *
import woo.log
woo.log.setLevel('LawTester',woo.log.INFO)
m=utils.defaultMaterial()
S=woo.master.scene=Scene(fields=[DemField()])
S.dem.par.append([
	utils.sphere((0,0,0),.5,fixed=False,wire=True,mat=m),
	utils.sphere((0,1.01,0),.5,fixed=False,wire=True,mat=m)
])
S.dem.collectNodes()
S.engines=utils.defaultEngines(damping=.01,gravity=(0,0,0))+[
	LawTester(ids=(0,1),abWeight=.3,smooth=1e-4,stages=[
			LawTesterStage(values=(-1,0,0,0,0,0),whats='v.....',until='bool(C)',done='print "Stage finished, at step",stage.step,", contact is",C'),
			LawTesterStage(values=(-.01,0,0,0,0,0),whats='v.....',until='C and C.geom.uN<-1e-2',done='print "Compressed to",C.geom.uN'),
			LawTesterStage(values=(.01,0,0,0,0,0),whats='v.....',until=('not C'),done='print "Contact broken",S.step;'),
			LawTesterStage(values=(-1e5,0,0,0,0,0),whats='fvv...',until=('C and tester.fErrRel[0]<1e-1'),done='print "Force-loaded contact stabilized";'),
			LawTesterStage(values=(-1e5,-.01,0,0,0,0),whats='fvvvvv',until=('"plast" in E and E["plast"]>0'),done='print "Plastic sliding reached";'),
			LawTesterStage(values=(0,-.01,0,0,0,0),whats=('vvvvvv'),until='stage.step>5000',done='print "5000 steps sliding done";'),
			LawTesterStage(values=(0,0,0,0,0,0),whats='vvvvvv',until='stage.step>100',done='E["plast"]=0.'),
			LawTesterStage(values=(0,0,-.01,0,0,0),whats='vvvvvv',until=('E["plast"]>1000'),done='print "sliding in the z-direction reached"'),
			LawTesterStage(values=(0,0,0,.1,0,0),whats='vvvvvv',until='stage.step>10000',done='print "Twist done"'),
			LawTesterStage(values=(0,0,0,0,.1,0),whats='vvvvvv',until='stage.step>10000',done='print "Bending done"'),
			LawTesterStage(values=(0,0,0,0,-.1,0),whats='vvvvvv',until='stage.step>10000',done='print "Bending back"'),
		],
		done='tester.dead=True; S.stop(); print "Everything done, making myself dead and pausing."',
		label='tester'
	),
	PyRunner(60,'dd={}; dd.update(**woo.tester.fuv()); dd.update(**S.energy); woo.plot.addData(i=S.step,dist=(S.dem.par[0].pos-S.dem.par[1].pos).norm(),t=S.time,**dd)'),
]
S.dt=1e-3
#S.pause()
S.trackEnergy=True
plot.plots={' i':(('fErrRel_xx','k'),None,'fErrAbs_xx'),'i ':('dist',None),' i ':(S.energy),'   i':('f_xx',None,'f_yy','f_zz'),'  i':('u_xx',None,'u_yy','u_zz'),'i  ':('u_yz',None,'u_zx','u_xy')}
plot.plot()
S.saveTmp()
S.run()
#O.reload()
