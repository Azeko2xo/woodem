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
		O.scene=S=Scene(fields=[DemField()])
		S=O.scene
		S.periodic=True;
		S.cell.setBox(2.5,2.5,3)
		self.cellDist=Vector3i(0,0,10) # how many cells away we go
		self.relDist=Vector3(0,.999999999999999999,0) # rel position of the 2nd ball within the cell
		self.initVel=Vector3(0,0,5)
		S.dem.par.append(utils.sphere((1,1,1),.5))
		self.initPos=Vector3([S.dem.par[0].pos[i]+self.relDist[i]+self.cellDist[i]*S.cell.hSize0.col(i).norm() for i in (0,1,2)])
		S.dem.par.append(utils.sphere(self.initPos,.5))
		S.dem.par[1].vel=self.initVel
		S.engines=[Leapfrog(reset=True)]
		S.cell.nextGradV=Matrix3(0,0,0, 0,0,0, 0,0,-1)
		S.cell.homoDeform=3
		S.dem.collectNodes() # avoid msg from Leapfrog
		S.dt=0 # do not change positions with dt=0 in NewtonIntegrator, but still update velocities from velGrad
	def testVelGrad(self):
		'PBC: velGrad changes hSize but not hSize0, accumulates in trsf'
		S=O.scene
		S.dt=1e-3
		hSize,trsf=O.scene.cell.hSize,Matrix3.Identity
		hSize0=hSize
		for i in range(0,10):
			S.one(); hSize+=O.scene.dt*S.cell.gradV*hSize; trsf+=O.scene.dt*S.cell.gradV*trsf
		for i in range(0,len(S.cell.hSize)):
			self.assertAlmostEqual(hSize[i],S.cell.hSize[i])
			self.assertAlmostEqual(trsf[i],S.cell.trsf[i])
			# ?? should work
			#self.assertAlmostEqual(hSize0[i],S.cell.hSize0[i])
	def testTrsfChange(self):
		'PBC: chaing trsf changes hSize0, but does not modify hSize'
		S=O.scene
		S.dt=1e-2
		S.one()
		S.cell.trsf=Matrix3.Identity
		for i in range(0,len(S.cell.hSize)):
			self.assertAlmostEqual(S.cell.hSize0[i],S.cell.hSize[i])
	def testDegenerate(self):
		"PBC: degenerate cell raises exception"
		S=O.scene
		self.assertRaises(RuntimeError,lambda: setattr(S.cell,'hSize',Matrix3(1,0,0, 0,0,0, 0,0,1)))
	def testSetBox(self):
		"PBC: setBox modifies hSize correctly"
		S=O.scene
		S.cell.setBox(2.55,11,45)
		self.assert_(S.cell.hSize==Matrix3(2.55,0,0, 0,11,0, 0,0,45));
	def testHomotheticResizeVel(self):
		"PBC: homothetic cell deformation adjusts particle velocity (homoDeform==3)"
		S=O.scene
		S.dt=1e-5
		s1=S.dem.par[1]
		print 'init',self.initPos,self.initVel
		print 'before',s1.pos,s1.vel
		S.one()
		print 'after',s1.pos,s1.vel
		self.assertAlmostEqual(s1.vel[2],self.initVel[2]+self.initPos[2]*S.cell.gradV[2,2])
	def testHomotheticResizePos(self):
		"PBC: homothetic cell deformation adjusts particle position (homoDeform==1)"
		S=O.scene
		S.cell.homoDeform=1
		S.one()
		s1=S.dem.par[1]
		self.assertAlmostEqual(s1.vel[2],self.initVel[2])
		self.assertAlmostEqual(s1.pos[2],self.initPos[2]+self.initPos[2]*S.cell.gradV[2,2]*S.dt)
	def testL6GeomIncidentVelocity(self):
		"PBC: L6Geom incident velocity (homoDeform==3)"
		S=O.scene
		S.one()
		S.engines=[ForceResetter(),ContactLoop([Cg2_Sphere_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl(noBreak=True)]),Leapfrog(reset=True)]
		i=utils.createContacts([0],[1])[0]
		S.dt=1e-10; S.one() # tiny timestep, to not move the normal too much
		self.assertAlmostEqual(self.initVel.norm(),i.geom.vel.norm())
	def testL3GeomIncidentVelocity_homoPos(self):
		"PBC: L6Geom incident velocity (homoDeform==1)"
		S=O.scene
		S.cell.homoDeform=1; S.one()
		S.engines=[ForceResetter(),ContactLoop([Cg2_Sphere_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl(noBreak=True)]),Leapfrog(reset=True)]
		i=utils.createContacts([0],[1])[0]
		S.dt=1e-10; S.one()
		self.assertAlmostEqual(self.initVel.norm(),i.geom.vel.norm())
		#self.assertAlmostEqual(self.relDist[1],1-i.geom.penetrationDepth)
	def testKineticEnergy(self):
		"PBC: utils.kineticEnergy considers only fluctuation velocity, not the velocity gradient (homoDeform==3)"
		S=O.scene
		S.one() # updates velocity with homotheticCellResize
		# ½(mv²+ωIω)
		# #0 is still, no need to add it; #1 has zero angular velocity
		# we must take self.initVel since S.dem.par[1].vel now contains the homothetic resize which utils.kineticEnergy is supposed to compensate back 
		Ek=.5*S.dem.par[1].mass*self.initVel.squaredNorm()
		self.assertAlmostEqual(Ek,S.dem.par[1].Ekt)
	def testKineticEnergy_homoPos(self):
		"PBC: utils.kineticEnergy considers only fluctuation velocity, not the velocity gradient (homoDeform==1)"
		S=O.scene
		S.cell.homoDeform=1; S.one()
		self.assertAlmostEqual(.5*S.dem.par[1].mass*self.initVel.squaredNorm(),S.dem.par[1].Ekt)


