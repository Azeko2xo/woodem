'''
Test particle generator, that the resulting PSD curve matches the one on input.
'''
import unittest
from yade.core import *
from yade.dem import *
from miniEigen import *
import numpy

class PsdSphereGeneratorTest(unittest.TestCase):
	def setUp(self):
		self.gen=PsdSphereGenerator(psdPts=[(.05,0),(.1,20),(.2,40),(.4,60),(.5,90),(.6,100)])
		self.mat=FrictMat(density=1000)
	def testMassDiscrete(self):
		'PSD: discrete mass-based generator'
		self.gen.mass=True; self.gen.discrete=True
		self.checkOk()
	def testMassContinuous(self):
		'PSD: continuous mass-based generator'
		self.gen.mass=True; self.gen.discrete=False
		self.checkOk(relDeltaInt=.03)
	def testNumDiscrete(self):
		'PSD: discrete number-based generator'
		self.gen.mass=False; self.gen.discrete=True
		self.checkOk()
	def testNumContinuous(self):
		'PSD: continuous number-based generator'
		self.gen.mass=False; self.gen.discrete=False
		self.checkOk()
	def checkOk(self,relDeltaInt=.01,relDeltaD=.03):
		for i in range(10000): self.gen(self.mat)
		iPsd=self.gen.inputPsd(scale=True)
		iPsdNcum=self.gen.inputPsd(scale=True,cumulative=False,num=150)
		 # scale by mass rather than number depending on the generator setup
		oPsd=self.gen.psd(mass=self.gen.mass,num=150)
		oPsdNcum=self.gen.psd(mass=self.gen.mass,num=150,cumulative=False)
		iInt=numpy.trapz(*iPsd)
		oInt=numpy.trapz(*oPsd)
		if 0: # enable to show graphical output
			import pylab
			pylab.figure()

			pylab.subplot(211)
			pylab.plot(*iPsd,label='in (%g)'%iInt)
			pylab.plot(*oPsd,label='out (%g)'%oInt)
			desc=('mass' if self.gen.mass else 'num','discrete' if self.gen.discrete else 'continuous')
			pylab.suptitle('%s-based %s generator (rel. area err %g)'%(desc[0],desc[1],(oInt-iInt)/iInt))
			# pylab.xlabel('Particle diameter')
			pylab.ylabel('Cumulative '+('mass' if self.gen.mass else 'number of particles'))
			pylab.grid(True)
			pylab.legend(loc='upper left')

			pylab.subplot(212)
			pylab.plot(*iPsdNcum,label='in')
			pylab.plot(*oPsdNcum,label='out')
			desc=('mass' if self.gen.mass else 'num','discrete' if self.gen.discrete else 'continuous')
			pylab.suptitle('%s-based %s generator (rel. area err %g)'%(desc[0],desc[1],(oInt-iInt)/iInt))
			pylab.xlabel('Particle diameter')
			pylab.ylabel('Histogram: '+('mass' if self.gen.mass else 'number of particles'))
			pylab.grid(True)
			pylab.legend(loc='upper left')

			pylab.savefig('/tmp/psd-test-%s-%s.png'%desc)
		# tolerance of 1%
		self.assertAlmostEqual(iInt,oInt,delta=relDeltaInt*iInt) 
		# check that integration minima and maxima match
		dMin,dMax=self.gen.psdPts[0][0],self.gen.psdPts[-1][0]
		# 3% tolerance here
		self.assertAlmostEqual(dMin,oPsd[0][0],delta=relDeltaD*dMin)
		self.assertAlmostEqual(dMax,oPsd[0][-1],delta=relDeltaD*dMax)
