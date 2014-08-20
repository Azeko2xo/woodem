import matplotlib
matplotlib.rc('text',usetex=True)
matplotlib.rc('text.latex',preamble=r'\usepackage{concrete}\usepackage{euler}')
from pylab import *
from numpy import *
ktkn=linspace(0,1,20)
def nu(ktkn): return (1-ktkn)/(4+ktkn)
plot(ktkn,nu(ktkn),'r',label=r'theoretical $\nu=\frac{1-k_T/k_N}{4+k_T/k_N}$')
plot([.15,.2,.25],[.215,.2,.185],'bo',label='numerical')
#arrow(0,.2,.2,.2,ls='dashed',alpha=.5)
arrow(0,.2,.2,0,ls='dashed',edgecolor='black',facecolor='black',alpha=.8,head_length=0)
arrow(.2,.2,0,-.2,ls='dashed',edgecolor='black',facecolor='black',alpha=.8,length_includes_head=True,head_width=.015,head_length=.015)
text(.22,.02,r'$k_T=0.2k_N$',fontsize=16)
text(.24,.01,r'$\nu=0.2$',fontsize=16)
grid()
xlabel('$K_T/K_N$')
ylabel(r'$\nu$')
legend()
savefig('nu-calibration.pdf')
