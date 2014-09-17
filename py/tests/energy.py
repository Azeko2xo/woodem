# encoding: utf-8
# 2013 © Václav Šmilauer <eudoxos@arcig.cz>

'''
Test some parts of energy conservation in DEM simulations.
'''

import unittest
from minieigen import *
from math import *
from woo._customConverters import *
from woo import utils
from woo.core import *
from woo.dem import *
from woo.pre import *
import woo
import woo.log


class TestInletsAndOutlets(unittest.TestCase):
	def setUp(self):
		self.S=Scene(fields=[DemField(gravity=(0,0,0))])
		self.S.trackEnergy=True
		self.mat=woo.utils.defaultMaterial()
		self.vel=1.
		self.rad=.1
		self.m=self.mat.density*(4/3.)*pi*self.rad**3
		self.Ek=.5*self.m*self.vel**2
		if 0:
			woo.log.setLevel('BoxDeleter',woo.log.TRACE)
			woo.log.setLevel('ConveyorFactory',woo.log.TRACE)
			woo.log.setLevel('RandomFactory',woo.log.TRACE)
			woo.log.setLevel('DemField',woo.log.TRACE)
			woo.log.setLevel('ParticleContainer',woo.log.TRACE)
	def testConveyorInlet(self):
		'Energy: conveyor inlet traces kinetic energy'
		import os
		S=self.S
		S.engines=[
			ConveyorInlet(maxNum=1,material=self.mat,cellLen=.3,radii=[self.rad],centers=[(0,0,0)],vel=self.vel,node=Node(pos=(0,0,0))),
			BoxOutlet(inside=True,box=((-1,-1,-1),(1,1,1)))
		]
		S.dt=1. # produce one particle in the first step
		S.one()
		#print 100*'#',dict(S.energy)
		#print 'Number of particles:',len(S.dem.par)
		self.assertEqual(len(S.dem.par),0)
		self.assertEqual(S.energy['kinOutlet'],self.Ek)
		self.assertEqual(S.energy['kinInlet'],-self.Ek)
		self.assertEqual(S.energy.total(),0.)
	def testRandomInlet(self):
		'Energy: random box factory traces kinetic energy'
		S=self.S
		S.engines=[
			InsertionSortCollider(),
			BoxInlet(maxNum=1,materials=[self.mat],
				generator=MinMaxSphereGenerator(dRange=(2*self.rad,2*self.rad)),
				shooter=AlignedMinMaxShooter(dir=(1,0,0),vRange=(self.vel,self.vel)),
				box=((-.5,-.5,-.5),(.5,.5,.5)),
				massRate=0,
			),
			BoxOutlet(inside=True,box=((-1,-1,-1),(1,1,1))),
		]
		S.dt=.05 # produce one particle in the first step
		S.one()
		self.assertEqual(S.energy['kinOutlet'],self.Ek)
		self.assertEqual(S.energy['kinInlet'],-self.Ek)
		self.assertEqual(S.energy.total(),0.)

class TestLeapfrog(unittest.TestCase):
	def setUp(self):
		m=woo.utils.defaultMaterial()
		self.S=Scene(dtSafety=.9,trackEnergy=True,engines=DemField.minimalEngines(damping=.4),
			fields=[DemField(gravity=(0,0,-10),par=[Wall.make(0,axis=2,sense=1,mat=m),
			Sphere.make((0,0,.1),radius=.8,mat=m)])])
	def testGravitySkip(self):
		'Energy: DemData.gravitySkip'
		S=self.S
		for n in S.dem.nodes: n.dem.gravitySkip=True
		S.dem.par[-1].vel=(0,0,-1) # so that we hit ground and have dissipation
		S.run(400,True)
		self.assert_('grav' not in S.energy)
		self.assert_(S.energy['nonviscDamp']!=0.)
	def testDampingSkip(self):
		'Energy: DemData.dampingSkip'
		S=self.S
		for n in S.dem.nodes: n.dem.dampingSkip=True
		S.run(400,True)
		self.assert_(S.energy['grav']!=0.)
		self.assert_('nonviscDamp' not in S.energy)



