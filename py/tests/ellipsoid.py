# encoding: utf-8
# 2013 © Václav Šmilauer <eu@doxos.eu>

import unittest
from minieigen import *
import woo._customConverters
import woo.core
import woo.dem
import woo.utils
import math
from woo.dem import *

class TestEllipsoid(unittest.TestCase):
	'Test :obj:`Cg2_Ellipsoid_Ellipsoid_L6Geom`.'
	def setUp(self):
		S=self.S=woo.core.Scene(fields=[woo.dem.DemField()])
		self.mat=FrictMat(density=1e3,young=1e8,ktDivKn=.2,tanPhi=.5)
		S.dtSafety=0.7
		S.engines=[Leapfrog(reset=True,damping=.4),InsertionSortCollider([Bo1_Ellipsoid_Aabb()]),ContactLoop([Cg2_Ellipsoid_Ellipsoid_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl(noBreak=True)])]
		#,DynDt(stepPeriod=1000)]
		#woo.utils.defaultEngines(damping=.0,cp2=Cp2_FrictMat_HertzPhys(gamma=0,en=0,label='cp2',poisson=.2),law=Law2_L6Geom_HertzPhys_DMT(),dynDtPeriod=10)+[
			# -123 is replaced by v0 before actually used
		#	LawTester(ids=(0,1),abWeight=.5,stages=[LawTesterStage(values=(-123,0,0,0,0,0),whats='ivv...',until='stage.rebound',done='S.stop(); tester.dead=True')],label='tester')
		#]
	def testNormalDisplacementEllipsoid(self):
		'Ellipsoid: normal displacement on contact with ellipsoid.'
		self.S.saveTmp()
		# try the same thing with different sizes of ellipsoids
		# divide by 3. so that number are nice (initial distance is .1+.2=.3)
		for scale in [.5/3.,1./3.,10./3.,100./3.,1000/3.]:
			S=woo.core.Scene.loadTmp()
			# two ellipsoids exteranlly touching perpendicularly; the whole setup it rotated by gOri
			gOri=Quaternion((.1,1,.3),math.pi/3.) # some rather random orientation
			gOri.normalize() # important; axis is normalized automatically in minieigen newly, but not yet always
			S.dem.par.add([
				woo.utils.ellipsoid((0,0,0),semiAxes=scale*Vector3(.1,.2,.1),ori=gOri,mat=self.mat,fixed=True),
				woo.utils.ellipsoid(gOri*(scale*Vector3(.3,0,0)),semiAxes=scale*Vector3(.2,.1,.1),ori=gOri,mat=self.mat,fixed=True)
			])
			S.dem.collectNodes()
			e0,e1=S.dem.par[0],S.dem.par[1]
			e1.vel=gOri*Vector3(-.1,0,0)
			S.dt=.1
			if 0: # debugging stuff
				l0=scale*(.1+.2)
				print
				print 'distance',(S.dem.par[0].pos-S.dem.par[1].pos).norm()
				print 'distance',l0
			S.one()
			C=S.dem.con[0]
			if 0: # debugging stuff
				print C, C.geom.uN
				l=(S.dem.par[0].pos-S.dem.par[1].pos).norm()
				print 'distance after',l
				print S.dem.par[0].pos, S.dem.par[1].pos, 'dist',l0,l
				print 'displacement',l-l0
				print 'displacement',C.geom.uN
			# e1 should move by -.1×.1 = .01 towards the first one, which should be the overlap distance
			self.assertAlmostEqual(C.geom.uN,-.01,delta=1e-5*0.01)
	def testNormalDisplacementWall(self):
		'Ellipsoid: normal displacement on contact with wall'
		pass
		#self.S.saveTmp()
		#for scale in [.1,1.,10.]:
		#	gOri=Quaternion((1,2,3),math.pi/4.) # some random orientation
	def testMassInertia(self):
		'Ellipsoid: mass and inertia computation'
		a,b,c=1,2,3
		rho=1
		e=Ellipsoid.make(center=(0,0,0),semiAxes=(a,b,c),mat=FrictMat(density=rho))
		m=rho*(4/3.)*math.pi*a*b*c
		I=(1/5.)*m*Vector3(b**2+c**2,a**2+c**2,a**2+b**2)
		self.assertAlmostEqual(e.mass,m)
		self.assertAlmostEqual(e.inertia,I)


