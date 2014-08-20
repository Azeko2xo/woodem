import matplotlib
matplotlib.rc('text',usetex=True)
matplotlib.rc('text.latex',preamble=r'\usepackage{concrete}\usepackage{euler}')

import pylab
from yade import eudoxos


sigMinMax=-30e6,5e6
sig=pylab.arange(sigMinMax[0],sigMinMax[1],(sigMinMax[1]-sigMinMax[0])/1000)
sigg=pylab.concatenate((sig,sig[::-1]))
nFig=1
law=Law2_Dem3DofGeom_CpmPhys_Cpm(yieldLogSpeed=.4,yieldEllipseShift=-10e6)
plastConditions=['linear','parabolic','log','log+lin','elliptic','elliptic+lin']
for plastCondition in [0,3]: #[0,1,2,3,4,5]:
	#pylab.figure()
	for n,omega in enumerate(pylab.arange(0,1+(1./nFig),1./nFig)):
		pylab.axvline(linewidth=2)
		law.yieldSurfType=plastCondition
		pylab.plot(sigg,[(1 if i<len(sigg)/2 else -1)*law.yieldSigmaTMagnitude(sigmaN=s,omega=omega,undamagedCohesion=3.5e6,tanFrictionAngle=.8) for i,s in enumerate(sigg)],label='$\hbox{%s,}\,\omega$=%g'%(plastConditions[plastCondition],omega))
		pylab.xlabel('$\sigma_N$'); pylab.ylabel(r'$\pm|\sigma_T|$')
	#pylab.show()
pylab.legend()
pylab.grid()
#pylab.gcf().set_size_inches(10,10)
pylab.savefig('yield-surfaces.pdf')
pylab.show()
quit()
