import matplotlib
matplotlib.rc('text',usetex=True)
matplotlib.rc('text.latex',preamble=r'\usepackage{euler}')
from numpy import *
from matplotlib.mlab import movavg
sig,eps=array([]),array([])
dta=genfromtxt('tension-test.data')
eps,sig=dta[:,0],dta[:,2]
eps,sig=concatenate(([0],eps[2:-2])),concatenate(([0],movavg(sig,5)))
epsFt,ft=eps[sig==max(sig)][0],max(sig)
ft02=.2*ft
sig2=array([(s if eps[i]>epsFt else (ft/epsFt)*eps[i]) for i,s in enumerate(sig) if eps[i]<epsFt or s>ft02]+[0])
#sig2=concatenate((sig2,[sig2[-1]]))
eps2=concatenate((eps[0:len(sig2)-1],[eps[len(sig2)-2]]))
epsFt02=max(eps2)
print epsFt,epsFt02
print shape(eps2),shape(sig2)
import pylab
pylab.grid()
pylab.plot(eps,sig,label=r'$\sigma_N(\eps_N)$')
pylab.fill(eps2,sig2,color='b',alpha=.3)
pylab.annotate(r'$f_t$',xytext=(epsFt,ft),xy=(0,ft),arrowprops=dict(arrowstyle='-',linestyle='dashed',relpos=(0,0),shrinkA=0,shrinkB=0))
pylab.annotate(r'$0.2\,f_t$',xytext=(epsFt02,ft02),xy=(0,ft02),arrowprops=dict(arrowstyle='-',linestyle='dashed',relpos=(0,0),shrinkA=0,shrinkB=0))
pylab.text(epsFt,.5*ft,'$\Large G_f$',fontsize=20)
pylab.xlabel(r'$\varepsilon_N$')
pylab.ylabel(r'$\sigma_N(\varepsilon_N)$')
#pylab.show()
pylab.savefig('fracture-energy.pdf')


