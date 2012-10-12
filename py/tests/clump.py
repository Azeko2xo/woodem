
'''
Various computations affected by the periodic boundary conditions.
'''

import unittest
import random
from minieigen import *
from woo._customConverters import *
from woo import utils
from woo import *
from woo.dem import *
from woo.core import *
from math import *
import woo

class TestSimpleClump(unittest.TestCase):
	"Test things on a simple clump composed of 2 spheres."
	def setUp(self):
		woo.master.reset()
		woo.master.scene=S=Scene(fields=[DemField()])
		r1,r2,p0,p1=1,.5,Vector3.Zero,Vector3(0,0,3)
		S.dem.par.appendClumped([
			utils.sphere(p0,r1),
			utils.sphere(p1,r2)
		])
		S.dem.nodes=[S.dem.par[0].shape.nodes[0],S.dem.par[1].shape.nodes[0],S.dem.clumps[0]]
		#O.dem.clumps.clear()
	def testConsistency(self):
		"Clump: ids and flags consistency"
		S=woo.master.scene
		b1,b2,bC=[S.dem.nodes[i] for i in (0,1,2)]
		#self.assertEqual(b1.clumpId,bC.id)
		#self.assertEqual(b2.clumpId,bC.id)
		#self.assertEqual(bC.clumpId,bC.id)
		self.assert_(b1 in bC.dem.nodes)
		self.assert_(b2 in bC.dem.nodes)
		self.assert_(bC.dem.clump)
		self.assert_(b1.dem.clumped)
		self.assert_(b2.dem.clumped)
	def testStaticProperties(self):
		"Clump: mass, centroid, intertia"
		S=woo.master.scene
		b1,b2,bC=[S.dem.nodes[i] for i in (0,1,2)]
		# mass
		self.assertEqual(bC.dem.mass,b1.dem.mass+b2.dem.mass)
		# centroid
		SS=b1.dem.mass*b1.pos+b2.dem.mass*b2.pos
		c=SS/bC.dem.mass
		self.assertEqual(bC.pos,c);
		# inertia
		i1,i2=(8./15)*pi*S.dem.par[0].mat.density*S.dem.par[0].shape.radius**5, (8./15)*pi*S.dem.par[1].mat.density*S.dem.par[1].shape.radius**5 # inertia of spheres
		iMax=i1+i2+b1.dem.mass*(b1.pos-c).norm()**2+b2.dem.mass*(b2.pos-c).norm()**2 # minimum principal inertia
		iMin=i1+i2 # perpendicular to the 
		# the order of bC.state.inertia is arbitrary (though must match the orientation)
		iC=list(bC.dem.inertia); iC.sort()
		self.assertAlmostEqual(iC[0],iMin)
		self.assertAlmostEqual(iC[1],iMax)
		self.assertAlmostEqual(iC[2],iMax)
		# check orientation...?
		#self.assertAlmostEqual
	def testVelocity(self):
		"Clump: velocities of member assigned by Leapfrog"
		S=woo.master.scene
		b1,b2,bC=[S.dem.nodes[i] for i in (0,1,2)]
		S.dt=0
		#print bC.dem.vel,bC.dem.angVel
		bC.dem.vel=(1.,.2,.4)
		bC.dem.angVel=(0,.4,.1)
		S.engines=[Leapfrog(reset=True)]; S.one() # update velocities
		# linear velocities
		self.assertEqual(b1.dem.vel,bC.dem.vel+bC.dem.angVel.cross(b1.pos-bC.pos))
		self.assertEqual(b2.dem.vel,bC.dem.vel+bC.dem.angVel.cross(b2.pos-bC.pos))
		# angular velocities
		self.assertEqual(b1.dem.angVel,bC.dem.angVel);
		self.assertEqual(b2.dem.angVel,bC.dem.angVel);

