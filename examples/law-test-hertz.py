import woo
from woo import utils
from woo.core import *
from woo.dem import *
from woo import plot
from woo import *
import woo.log
woo.log.setLevel('LawTester',woo.log.INFO)
woo.log.setLevel('Law2_L6Geom_FrictPhys_Pellet',woo.log.TRACE)
m=FrictMat(density=1e3,young=1e7,ktDivKn=.2,tanPhi=.5)
S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,0))])
S.dtSafety=0.01
S.dem.par.append([
	utils.sphere((0,0,0),.05,fixed=False,wire=True,mat=m),
	utils.sphere((0,.10001,0),.05,fixed=False,wire=True,mat=m)
])
S.dem.collectNodes()
# S.engines=utils.defaultEngines(damping=.0,cp2=Cp2_FrictMat_FrictPhys(),law=Law2_L6Geom_FrictPhys_IdealElPl())+[
S.engines=utils.defaultEngines(damping=.0,cp2=Cp2_FrictMat_HertzPhys(gamma=10,en=1.,alpha=.3,label='cp2'),law=Law2_L6Geom_HertzPhys_DMT(model='COS'),dynDtPeriod=10)+[
	LawTester(ids=(0,1),abWeight=.5,smooth=1e-4,stages=[
			# LawTesterStage(values=(-1e-0,0,0,0,0,0),whats='ivv...',until='stage.rebound',done='print "Rebound with v0=1e-0m/s"'),LawTesterStage(values=(1e-2,0,0,0,0,0),whats='vvv...',until='not C',done='print "Contact broken."'),
			LawTesterStage(values=(-1e-1,0,0,0,0,0),whats='ivv...',until='stage.rebound',done='print "Rebound with v0=1e-1m/s"'),LawTesterStage(values=(1e-2,0,0,0,0,0),whats='vvv...',until='not C',done='print "Contact broken."'),
			LawTesterStage(values=(-1e-2,0,0,0,0,0),whats='ivv...',until='stage.rebound',done='print "Rebound with v0=1e-2m/s"'),LawTesterStage(values=(1e-2,0,0,0,0,0),whats='vvv...',until='not C',done='print "Contact broken."'),
			#LawTesterStage(values=(-1e-3,0,0,0,0,0),whats='ivv...',until='stage.rebound',done='print "Rebound with v0=1e-3m/s"'),LawTesterStage(values=(1e-3,0,0,0,0,0),whats='vvv...',until='not C',done='print "Contact broken."'),
			#LawTesterStage(values=(-1e-4,0,0,0,0,0),whats='ivv...',until='stage.rebound',done='print "Rebound with v0=1e-4m/s"'),LawTesterStage(values=(1e-4,0,0,0,0,0),whats='vvv...',until='not C',done='print "Contact broken."'),
		],
		done='tester.dead=True; S.stop(); print "Everything done, making myself dead and pausing."',
		label='tester'
	),
	PyRunner(10,'import woo; dd={}; dd.update(**S.lab.tester.fuv()); dd.update(**S.energy); S.plot.addData(i=S.step,dist=(S.dem.par[0].pos-S.dem.par[1].pos).norm(),v1=S.dem.par[0].vel.norm(),v2=S.dem.par[1].vel.norm(),t=S.time,bounces=S.lab.tester.stages[S.lab.tester.stage].bounces,dt=S.dt,**dd)'),
]
#S.pause()
S.trackEnergy=True
#plot.plots={' i':(('fErrRel_xx','k'),None,'fErrAbs_xx'),'i ':('dist',None),' i ':(S.energy),'   i':('f_xx',None,'f_yy','f_zz'),'  i':('u_xx',None,'u_yy','u_zz'),'i  ':('u_yz',None,'u_zx','u_xy')}
S.plot.plots={'i':('u_xx',None,('f_xx','g--')),'u_xx':('f_xx',),'i ':('S.energy.keys()'),' i':('k_xx',None,('dt','g-'))}
S.plot.plot()
import woo.gl
S.any=[woo.gl.Gl1_DemField(glyph=woo.gl.Gl1_DemField.glyphForce)]
S.saveTmp()
#S.run()
S.one()
#O.reload()
