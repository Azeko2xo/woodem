from woo.core import *
from woo.dem import *
from woo import log
#log.setLevel("PsdSphereGenerator",log.TRACE)
doMass=True
pp=PsdSphereGenerator(psdPts=[(.3,.5),(.5,1.)],mass=doMass,discrete=False)
m=FrictMat(density=1000)
for i in range(0,100000): pp(m)
kw=dict(cumulative=False)
import pylab
pylab.ion()
pylab.plot(*pp.inputPsd(**kw),label="in",linewidth=3)
pylab.plot(*pp.psd(**kw),label="out",linewidth=3)
pylab.grid(True)
pylab.legend(loc='best')
#pylab.show()

genNum=PsdSphereGenerator(psdPts=[(.3,0),(.3,.2),(.4,.9),(.5,1.)],mass=False)
for i in range(0,10000): genNum(m)
pylab.figure(max(pylab.get_fignums())+1); \
pylab.plot(*genNum.inputPsd(),label= 'in'); \              # plot count-based PSD
pylab.plot(*genNum.psd(),label='out'); \                   # mass=True is the default
pylab.plot(*genNum.psd(mass=False),label='out (count)'); \ # mass=False, count-based
pylab.legend(loc='best');
pylab.show()
