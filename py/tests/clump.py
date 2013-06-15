
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
		for n in (S.dem.par[0].shape.nodes[0],S.dem.par[1].shape.nodes[0]): S.dem.nodesAppend(n);
		self.bC,self.b1,self.b2=S.dem.nodes
		#print 100*'#'
		#print self.b1,self.b1.dem.master,id(self.b1.dem.master),self.b1.dem.master._cxxAddr
		#print self.b2,self.b2.dem.master,id(self.b2.dem.master),self.b1.dem.master._cxxAddr
		#print self.bC,id(self.bC),self.bC._cxxAddr
	def testConsistency(self):
		"Clump: ids and flags consistency"
		S=woo.master.scene
		b1,b2,bC=self.b1,self.b2,self.bC
		#self.assertEqual(b1.clumpId,bC.id)
		#self.assertEqual(b2.clumpId,bC.id)
		#self.assertEqual(bC.clumpId,bC.id)
		self.assert_(b1 in bC.dem.nodes)
		self.assert_(b2 in bC.dem.nodes)
		self.assert_(bC.dem.clump)
		self.assert_(b1.dem.clumped)
		self.assert_(b2.dem.clumped)
		self.assert_(b1.dem.master==bC)
		self.assert_(b2.dem.master==bC)
	def testStaticProperties(self):
		"Clump: mass, centroid, intertia"
		S=woo.master.scene
		b1,b2,bC=self.b1,self.b2,self.bC
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
		b1,b2,bC=self.b1,self.b2,self.bC
		S.dt=0
		#print bC.dem.vel,bC.dem.angVel
		bC.dem.vel=(1.,.2,.4)
		bC.dem.angVel=(0,.4,.1)
		self.assert_(self.b1.dem.master==self.bC)
		S.engines=[Leapfrog(reset=True)]; S.one() # update velocities
		# linear velocities
		self.assertEqual(b1.dem.vel,bC.dem.vel+bC.dem.angVel.cross(b1.pos-bC.pos))
		self.assertEqual(b2.dem.vel,bC.dem.vel+bC.dem.angVel.cross(b2.pos-bC.pos))
		# angular velocities
		self.assertEqual(b1.dem.angVel,bC.dem.angVel);
		self.assertEqual(b2.dem.angVel,bC.dem.angVel);
	def teestNoCollide(self):
		"Clump: particles inside one clump don't collide with each other"
		S.engines=[InsertionSortCollider([Bo1_Sphere_Aabb()])]
		S.one()
		self.assert_(len(S.dem.con)==0)

def sphereClumpPrincipalAxes(cc,rr):
	'Return vol,pos,ori,inertia of sphere clump defined by centers and radii of spheres'
	ii=range(len(rr))
	Ii=[Matrix3(Vector3.Ones*((8/15.)*pi*rr[i]**5)) for i in ii]
	Vi=[(4/3.)*pi*rr[i]**3 for i in ii]
	V=sum(Vi)
	Sg=sum([Vi[i]*cc[i] for i in ii],Vector3.Zero)
	Ig=sum([Ii[i]+Vi[i]*(cc[i].dot(cc[i])*Matrix3.Identity-cc[i].outer(cc[i])) for i in ii],Matrix3.Zero)
	pos=Sg/V
	import numpy, numpy.linalg
	Ic_orientG=Ig-V*(pos.dot(pos)*Matrix3.Identity-pos.outer(pos))
	eigval,eigvec=numpy.linalg.eigh(numpy.matrix(Ic_orientG))
	ori=Quaternion(Matrix3(*eigvec.tolist()))
	inertia=Vector3(eigval)
	return V,pos,ori,inertia


class TestSphereClumpGeom(unittest.TestCase):
	"Test geometry of clumps composed of spheres only"
	def setUp(self):
		# c has no self-intersections
		self.c=SphereClumpGeom(centers=[(0,0,0),(0,0,3)],radii=(1,.5),div=-1)
		# c2 has several identical (overlapping) spheres
		self.c2=SphereClumpGeom(centers=[(0,0,0),(0,0,0)],radii=(1,1),div=-1)
	def testSteiner(self):
		"SphereClumpGeom: properties via Steiner's theorem"
		vol,pos,ori,inertia=sphereClumpPrincipalAxes(self.c.centers,self.c.radii)
		self.c.recompute(div=0)
		self.assertAlmostEqual(min(inertia),min(self.c.inertia))
		self.assertAlmostEqual(max(inertia),max(self.c.inertia))
		for i in (0,1,2):	self.assertAlmostEqual(pos[i],self.c.pos[i])
		self.assertAlmostEqual(vol,self.c.volume)
	def testSampling(self):
		"SphereClumpGeom: properties via grid sampling"
		self.c2.recompute(div=10)
		exactV=(4/3.)*pi*self.c.radii[0]**3
		exactI=(8/15.)*pi*self.c.radii[0]**5
		self.assertAlmostEqual(self.c2.volume,exactV,delta=exactV*1e-2)
		self.assertAlmostEqual(self.c2.inertia[0],exactI,delta=exactI*2e-2)
		#print 'volume',sum([(4/3.)*pi*self.c.radii[i]**3 for i in range(len(self.c.radii))]),self.c.volume
		#self.assertAlmostEqual()

