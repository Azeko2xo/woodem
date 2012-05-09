'''
Test particle generator, that the resulting PSD curve matches the one on input.
'''
import unittest
from yade.wrapper import *
from yade.core import *
from yade.dem import *
from miniEigen import *
import numpy

class PsdSphereGeneratorTest(unittest.TestCase):
	def setUp(self):
		self.gen=PsdSphereGenerator(psdPts=[(.1,20),(.2,40),(.4,60),(.5,90),(.6,100)])
		self.mat=FrictMat(density=1000)
	def testMassDiscrete(self):
		'PSD: discrete mass-based generator'
		self.gen.mass=True; self.gen.discrete=True
		self.checkOk()
	def testMassContinuous(self):
		'PSD: continuous mass-based generator'
		self.gen.mass=True; self.gen.discrete=False
		self.checkOk()
	def testNumDiscrete(self):
		'PSD: discrete number-based generator'
		self.gen.mass=False; self.gen.discrete=True
		self.checkOk()
	def testNumContinuous(self):
		'PSD: continuous number-based generator'
		self.gen.mass=False; self.gen.discrete=False
		self.checkOk()
	def checkOk(self):
		for i in range(10000): self.gen(self.mat)
		iPsd=self.gen.inputPsd(scale=True)
		 # scale by mass rather than number depending on the generator setup
		oPsd=self.gen.psd(mass=self.gen.mass,num=150)
		iInt=numpy.trapz(*iPsd)
		oInt=numpy.trapz(*oPsd)
		if 0: # enable to show graphical output
			import pylab
			pylab.figure()
			pylab.plot(*iPsd,label='in'%iInt)
			pylab.plot(*oPsd,label='out'%oInt)
			desc=('mass' if self.gen.mass else 'num','discrete' if self.gen.discrete else 'continuous')
			pylab.suptitle('%s-based %s generator (rel. area err %g)'%(desc[0],desc[1],(oInt-iInt)/iInt))
			pylab.grid(True)
			pylab.legend(loc='upper left')
			pylab.savefig('/tmp/psd-test-%s-%s.png'%desc)
		# tolerance of 1%
		self.assertAlmostEqual(iInt,oInt,delta=.01*iInt) 
		# check that integration minima and maxima match
		dMin,dMax=self.gen.psdPts[0][0],self.gen.psdPts[-1][0]
		# 1% tolerance hee as well
		self.assertAlmostEqual(dMin,oPsd[0][0],delta=.01*dMin)
		self.assertAlmostEqual(dMax,oPsd[0][-1],delta=.01*dMax)
