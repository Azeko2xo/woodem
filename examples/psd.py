from yade.core import *
from yade.dem import *
from yade import log
#log.setLevel("PsdSphereGenerator",log.TRACE)
doMass=True
pp=PsdSphereGenerator(psdPts=[(.3,.2),(.4,.9),(.5,1.)],mass=doMass,discrete=True)
m=FrictMat(density=1000)
for i in range(0,100000):
	pp(m)
	#print '.',
import pylab
pylab.ion()
pylab.plot(*pp.inputPsd(scale=True),label="input")
pylab.plot(*pp.psd(mass=doMass),label="output")
pylab.grid()
pylab.show()
