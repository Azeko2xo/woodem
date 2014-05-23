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

class TestShapePack(unittest.TestCase):
	# def setUp(self):
	def testLoadSampleTxt(self):
		'ShapePack: load sample text file'
		data='''##PERIODIC:: 1. 1. 1.
0 Sphere 0 0 0 .1
1 Sphere .1 .1 .1 .1
1 Sphere .1 .1 .2 .1
2 Sphere .2 .2 .2 .2
2 Sphere .3 .3 .3 .3
2 Capsule .4 .4 .4 .3 .05 .05 .05
'''
		tmp=woo.master.tmpFilename()
		f=open(tmp,'w'); f.write(data); f.close()
		sp=ShapePack(loadFrom=tmp)
		self.assert_(len(sp.raws)==3)
		self.assert_(type(sp.raws[1])==SphereClumpGeom) # automatic conversion for sphere-only clumps
		self.assert_(sp.raws[2].rawShapes[2].className=='Capsule')
		# print sp.raws
	def testFromSim(self):
		'ShapePack: from/to simulation with particles'
		S=woo.core.Scene(fields=[DemField()])
		# add two clumped spheres first
		r1,r2,p0,p1=1,.5,Vector3.Zero,Vector3(0,0,3)
		# adds clump node to S.dem.nodes automatically
		S.dem.par.appendClumped([
			woo.utils.sphere((0,0,0),1),
			woo.utils.sphere((0,0,3),.5)
		])
		# add a capsule
		c=woo.utils.capsule(center=(5,5,5),shaft=.3,radius=.3)
		S.dem.par.append(c)
		S.dem.nodesAppend(c.shape.nodes[0])
		# from DEM
		sp=ShapePack()
		sp.fromDem(S,S.dem,mask=0)
		self.assert_(len(sp.raws)==2)
		self.assert_(type(sp.raws[0])==SphereClumpGeom)
		self.assert_(sp.raws[1].rawShapes[0].className=='Capsule')
		#print sp.raws
		# to DEM
		mat=woo.utils.defaultMaterial()
		S2=woo.core.Scene(fields=[DemField()])
		sp.toDem(S2,S2.dem,mat=mat)
		# for p in S2.dem.par: print p, p.shape, p.shape.nodes[0]
		# for n in S2.dem.nodes: print n
		self.assert_(len(S2.dem.par)==3) # two spheres and capsule
		self.assert_(S2.dem.par[0].shape.nodes[0].dem.clumped) # sphere node is in clump
		self.assert_(S2.dem.par[0].shape.nodes[0] not in S2.dem.nodes) # sphere node not in dem.nodes
		self.assert_(S2.dem.nodes[0].dem.clump) # two-sphere clump node
		self.assert_(not S2.dem.nodes[1].dem.clump) # capsule node
		# TODO: test that particle positions are as they should be
	def assertAlmostEqualRel(self,a,b,relerr,abserr=0):
		self.assertAlmostEqual(a,b,delta=max(max(abs(a),abs(b))*relerr,abserr))
	def testGridSamping(self):
		'ShapePack: grid samping gives good approximations of mass+inertia'
		# take a single shape, compare with clump of zero-sized sphere (to force grid samping) and that shape
		m=woo.utils.defaultMaterial()
		zeroSphere=woo.utils.sphere((0,0,0),.4) # sphere which is entirely inside the thing
		for p in [woo.utils.sphere((0,0,0),1,mat=m),woo.utils.ellipsoid((0,0,0),semiAxes=(.8,1,1.2),mat=m),woo.utils.capsule((0,0,0),radius=.8,shaft=.6,mat=m)]:
			# print p.shape
			sp=woo.dem.ShapePack()
			sp.add([p.shape,zeroSphere.shape])
			r=sp.raws[0]
			r.recompute(div=10)
			# this depends on how we define equivalent radius, which is not clear yet; so just skip it
			# self.assertAlmostEqualRel(r.equivRad,p.shape.equivRadius,1e-2)
			self.assertAlmostEqualRel(r.volume,p.mass/m.density,1e-2)
			# sorted since axes may be swapped
			ii1,ii2=sorted(r.inertia),sorted(p.inertia/m.density)
			for i1,i2 in zip(ii1,ii2): self.assertAlmostEqualRel(i1,i2,1e-2)
			for ax in (0,1,2): self.assertAlmostEqualRel(r.pos[ax],p.pos[ax],0,1e-2)

