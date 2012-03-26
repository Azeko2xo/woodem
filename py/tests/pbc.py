# encoding: utf-8
# 2010 © Václav Šmilauer <eudoxos@arcig.cz>

'''
Various computations affected by the periodic boundary conditions.
'''

import unittest
import random,math
from yade.wrapper import *
from miniEigen import *
from yade._customConverters import *
from yade import utils
from yade import *
from yade.core import *
from yade.dem import *

class TestPBC(unittest.TestCase):
	# prefix test names with PBC: 
	def setUp(self):
		O.reset(); O.scene.periodic=True;
		O.scene.fields=[DemField()]
		O.scene.cell.setBox(2.5,2.5,3)
		self.cellDist=Vector3i(0,0,10) # how many cells away we go
		self.relDist=Vector3(0,.999999999999999999,0) # rel position of the 2nd ball within the cell
		self.initVel=Vector3(0,0,5)
		O.dem.par.append(utils.sphere((1,1,1),.5))
		self.initPos=Vector3([O.dem.par[0].pos[i]+self.relDist[i]+self.cellDist[i]*O.scene.cell.hSize0.col(i).norm() for i in (0,1,2)])
		O.dem.par.append(utils.sphere(self.initPos,.5))
		O.dem.par[1].vel=self.initVel
		O.scene.engines=[Leapfrog()]
		O.scene.cell.nextGradV=Matrix3(0,0,0, 0,0,0, 0,0,-1)
		O.scene.cell.homoDeform=3
		O.dem.collectNodes() # avoid msg from Leapfrog
		O.scene.dt=0 # do not change positions with dt=0 in NewtonIntegrator, but still update velocities from velGrad
	def testVelGrad(self):
		'PBC: velGrad changes hSize but not hSize0, accumulates in trsf'
		O.scene.dt=1e-3
		hSize,trsf=O.scene.cell.hSize,Matrix3.Identity
		hSize0=hSize
		for i in range(0,10):
			O.step(); hSize+=O.scene.dt*O.scene.cell.gradV*hSize; trsf+=O.scene.dt*O.scene.cell.gradV*trsf
		for i in range(0,len(O.scene.cell.hSize)):
			self.assertAlmostEqual(hSize[i],O.scene.cell.hSize[i])
			self.assertAlmostEqual(trsf[i],O.scene.cell.trsf[i])
			# ?? should work
			#self.assertAlmostEqual(hSize0[i],O.scene.cell.hSize0[i])
	def testTrsfChange(self):
		'PBC: chaing trsf changes hSize0, but does not modify hSize'
		O.scene.dt=1e-2
		O.step()
		O.scene.cell.trsf=Matrix3.Identity
		for i in range(0,len(O.scene.cell.hSize)):
			self.assertAlmostEqual(O.scene.cell.hSize0[i],O.scene.cell.hSize[i])
	def testDegenerate(self):
		"PBC: degenerate cell raises exception"
		self.assertRaises(RuntimeError,lambda: setattr(O.scene.cell,'hSize',Matrix3(1,0,0, 0,0,0, 0,0,1)))
	def testSetBox(self):
		"PBC: setBox modifies hSize correctly"
		O.scene.cell.setBox(2.55,11,45)
		self.assert_(O.scene.cell.hSize==Matrix3(2.55,0,0, 0,11,0, 0,0,45));
	def testHomotheticResizeVel(self):
		"PBC: homothetic cell deformation adjusts particle velocity (homoDeform==3)"
		O.scene.dt=1e-5
		O.step()
		s1=O.dem.par[1]
		self.assertAlmostEqual(s1.vel[2],self.initVel[2]+self.initPos[2]*O.scene.cell.gradV[2,2])
	def testHomotheticResizePos(self):
		"PBC: homothetic cell deformation adjusts particle position (homoDeform==1)"
		O.scene.cell.homoDeform=1
		O.step()
		s1=O.dem.par[1]
		self.assertAlmostEqual(s1.vel[2],self.initVel[2])
		self.assertAlmostEqual(s1.pos[2],self.initPos[2]+self.initPos[2]*O.scene.cell.gradV[2,2]*O.scene.dt)
	#def testScGeomIncidentVelocity(self):
	#	"PBC: ScGeom computes incident velocity correctly (homoDeform==3)"
	#	O.step()
	#	O.engines=[ContactLoop([Ig2_Sphere_Sphere_ScGeom()],[Ip2_FrictMat_FrictMat_FrictPhys()],[])]
	#	i=utils.createInteraction(0,1)
	#	self.assertEqual(self.initVel,i.geom.incidentVel(i,avoidGranularRatcheting=True))
	#	self.assertEqual(self.initVel,i.geom.incidentVel(i,avoidGranularRatcheting=False))
	#	self.assertAlmostEqual(self.relDist[1],1-i.geom.penetrationDepth)
	#def testScGeomIncidentVelocity_homoPos(self):
	#	"PBC: ScGeom computes incident velocity correctly (homoDeform==1)"
	#	O.scene.cell.homoDeform=1
	#	O.step()
	#	O.engines=[ContactLoop([Ig2_Sphere_Sphere_ScGeom()],[Ip2_FrictMat_FrictMat_FrictPhys()],[])]
	#	i=utils.createInteraction(0,1)
	#	self.assertEqual(self.initVel,i.geom.incidentVel(i,avoidGranularRatcheting=True))
	#	self.assertEqual(self.initVel,i.geom.incidentVel(i,avoidGranularRatcheting=False))
	#	self.assertAlmostEqual(self.relDist[1],1-i.geom.penetrationDepth)
	def testL6GeomIncidentVelocity(self):
		"PBC: L3Geom computes incident velocity correctly (homoDeform==3)"
		O.step()
		O.scene.engines=[ForceResetter(),ContactLoop([Cg2_Sphere_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl(noBreak=True)]),Leapfrog()]
		i=utils.createContacts([0],[1])[0]
		O.scene.dt=1e-10; O.step() # tiny timestep, to not move the normal too much
		self.assertAlmostEqual(self.initVel.norm(),i.geom.vel.norm())
	def testL3GeomIncidentVelocity_homoPos(self):
		"PBC: L3Geom computes incident velocity correctly (homoDeform==1)"
		O.scene.cell.homoDeform=1; O.step()
		O.scene.engines=[ForceResetter(),ContactLoop([Cg2_Sphere_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl(noBreak=True)]),Leapfrog()]
		i=utils.createContacts([0],[1])[0]
		O.scene.dt=1e-10; O.step()
		self.assertAlmostEqual(self.initVel.norm(),i.geom.vel.norm())
		#self.assertAlmostEqual(self.relDist[1],1-i.geom.penetrationDepth)
	def testKineticEnergy(self):
		"PBC: utils.kineticEnergy considers only fluctuation velocity, not the velocity gradient (homoDeform==3)"
		O.step() # updates velocity with homotheticCellResize
		# ½(mv²+ωIω)
		# #0 is still, no need to add it; #1 has zero angular velocity
		# we must take self.initVel since O.dem.par[1].vel now contains the homothetic resize which utils.kineticEnergy is supposed to compensate back 
		Ek=.5*O.dem.par[1].mass*self.initVel.squaredNorm()
		self.assertAlmostEqual(Ek,O.dem.par[1].Ekt)
	def testKineticEnergy_homoPos(self):
		"PBC: utils.kineticEnergy considers only fluctuation velocity, not the velocity gradient (homoDeform==1)"
		O.scene.cell.homoDeform=1; O.step()
		self.assertAlmostEqual(.5*O.dem.par[1].mass*self.initVel.squaredNorm(),O.dem.par[1].Ekt)


