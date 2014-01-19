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
from woo import *
from woo.core import *
from woo.dem import *
from woo.pre import *
import woo
import woo.log


class TestFactoriesAndDeleters(unittest.TestCase):
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
	def testConveyorFactory(self):
		'Energy: conveyor factory traces kinetic energy'
		import os
		S=self.S
		S.engines=[
			ConveyorFactory(maxNum=1,material=self.mat,cellLen=.3,radii=[self.rad],centers=[(0,0,0)],vel=self.vel,node=Node(pos=(0,0,0))),
			BoxDeleter(inside=True,box=((-1,-1,-1),(1,1,1)))
		]
		S.dt=1. # produce one particle in the first step
		S.one()
		#print 100*'#',dict(S.energy)
		#print 'Number of particles:',len(S.dem.par)
		self.assert_(len(S.dem.par)==0)
		self.assert_(S.energy['kinDelete']==self.Ek)
		self.assert_(S.energy['kinFactory']==-self.Ek)
		self.assert_(S.energy.total()==0.)
	def testRandomFactory(self):
		'Energy: random box factory traces kinetic energy'
		S=self.S
		S.engines=[
			InsertionSortCollider(),
			BoxFactory(maxNum=1,materials=[self.mat],
				generator=MinMaxSphereGenerator(dRange=(self.rad,self.rad)),
				shooter=AlignedMinMaxShooter(dir=(1,0,0),vRange=(self.vel,self.vel)),
				box=((-.5,-.5,-.5),(.5,.5,.5)),
				massFlowRate=0,
			),
			BoxDeleter(inside=True,box=((-1,-1,-1),(1,1,1))),
		]
		S.dt=.05 # produce one particle in the first step
		S.one()
		self.assert_(S.energy['kinDelete']==self.Ek)
		self.assert_(S.energy['kinFactory']==-self.Ek)
		self.assert_(S.energy.total()==0.)

		



