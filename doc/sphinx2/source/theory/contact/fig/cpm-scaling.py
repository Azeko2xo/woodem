import matplotlib
matplotlib.rc('text',usetex=True)
matplotlib.rc('text.latex',preamble=r'\usepackage{euler}')
from numpy import *
from matplotlib.mlab import movavg
sig,eps=array([]),array([])
dta=genfromtxt('tension-test.data')
eps,sig=dta[:,0],dta[:,2]
eps,sig=concatenate(([0],eps[2:-2])),concatenate(([0],movavg(sig,5)))

import pylab
pylab.grid()
pylab.plot(eps,sig,label=r'reference')
pylab.plot(eps,.5*sig,label=r'vertically scaled')
pylab.plot(.5*eps,.5*sig,label=r'radially scaled')
pylab.legend()
pylab.xlabel(r'$\varepsilon_N$')
pylab.ylabel(r'$\sigma_N$')
#pylab.show()
pylab.savefig('cpm-scaling.pdf')

