import woo
from woo import utils
from woo.core import *
from woo.dem import *
from woo import plot
import woo.log
m=FrictMat(density=1e3,young=1e7,ktDivKn=.2,tanPhi=.5)
S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-10))])
S.dtSafety=0.1
S.dem.par.add([
	utils.wall(-.5,axis=2,sense=1,mat=m),
	utils.sphere((0,0,1),.5,fixed=False,wire=True,mat=m)
])
# S.dem.par[1].vel=(0,0,-1)
S.dem.collectNodes()
S.dtSafety=0.06
S.engines=utils.defaultEngines(damping=.0,cp2=Cp2_FrictMat_HertzPhys(gamma=0.,en=1.,label='cp2'),law=Law2_L6Geom_HertzPhys_DMT(noAttraction=True),dynDtPeriod=10)+[
	LawTester(ids=(0,1),abWeight=1.,label='tester',stages=[LawTesterStage(values=(0,0,0,0,0,0),whats='......',until='False',
		#,until='stage.rebound'
	)],done='tester.restart(); S.stop()'),
	PyRunner(1,'import woo; S.plot.addData(i=S.step,x=S.dem.par[1].pos,v=S.dem.par[1].vel,t=S.time,dt=S.dt,Etot=S.energy.total(),bounces=S.lab.tester.stages[0].bounces,vRel=S.lab.tester.v,**S.energy)'),
]
S.trackEnergy=True
S.plot.plots={'t':('x_z'),'i ':('S.energy.keys()'),'t ':('v_z')} #,None,('Etot','g--'))}# ,'i':('v_z','vRel_xx')}
S.plot.plot()
S.saveTmp()

