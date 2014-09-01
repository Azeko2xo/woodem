# encoding: utf-8
# 2013 © Václav Šmilauer <eu@doxos.eu>

import unittest
from minieigen import *
import woo._customConverters
import woo.core
import woo.dem
import woo.utils
from woo.dem import *

class TestHertz(unittest.TestCase):
	'Test :obj:`Law2_L6Geom_HertzPhys_DMT`.'
	def setUp(self):
		S=self.S=woo.core.Scene(fields=[woo.dem.DemField()])
		m=FrictMat(density=1e3,young=1e8,ktDivKn=.2,tanPhi=.5)
		S.dem.par.add([
			woo.utils.sphere((0,0,0),.05,fixed=False,wire=True,mat=m),
			woo.utils.sphere((0,.10001,0),.05,fixed=False,wire=True,mat=m)
		])
		S.dem.collectNodes()
		# dtSafety=0.4 work with 1% relative tolerance,
		# dtSafety=0.8 makes collision time not fit within 1%.
		S.dtSafety=0.4
		S.engines=woo.utils.defaultEngines(damping=.0,cp2=Cp2_FrictMat_HertzPhys(gamma=0,en=0,label='cp2',poisson=.2),law=Law2_L6Geom_HertzPhys_DMT(),dynDtPeriod=10)+[
			# -123 is replaced by v0 before actually used
			LawTester(ids=(0,1),abWeight=.5,stages=[LawTesterStage(values=(-123,0,0,0,0,0),whats='ivv...',until='stage.rebound',done='S.stop(); tester.dead=True')],label='tester')
		]
	def testElasticCollisionTime(self):
		'Hertz: elastic collision time for different impact velocities (simulation/analytic)'
		S=self.S
		S.lab.cp2.gamma=S.lab.en=0 # no adhesion and no viscosity
		E1,E2=S.dem.par[0].material.young,S.dem.par[1].material.young
		nu1,nu2=S.lab.cp2.poisson,S.lab.cp2.poisson
		d=S.dem.par[0].shape.radius+S.dem.par[1].shape.radius
		rho=S.dem.par[0].material.density
		Eeff=(E1*E2)/(E2*(1-nu1**2)+E1*(1-nu2**2))
		for v0 in (1,1e-1,1e-2,1e-3,1e-4,1e-5):
			S.lab.tester.stages[0].values[0]=-v0
			S.lab.tester.dead=False
			S.lab.tester.restart()
			S.run(); S.wait()
			def tauHertz(rho,Eeff,d,v0): return 2.214*(rho/Eeff)**(2/5.)*(d/v0**(1/5.))
			#print 'Collision time with v0=%g: %g sim, %g analytical'%(v0,S.lab.tester.stages[0].cTime,tauHertz(rho,Eeff,d,v0))
			delta=0.01*S.lab.tester.stages[0].cTime # 1% relative tolerance
			self.assertAlmostEqual(S.lab.tester.stages[0].cTime,tauHertz(rho,Eeff,d,v0),delta=delta)
		#self.assert_(1==1)



