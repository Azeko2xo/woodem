# encoding: utf-8
'''
Classes providing python-only implementation of various implementation models, mostly for testing or plotting in documentation.
'''

from math import pi,sqrt
import numpy

class CundallModel(object):
	'Linear model :cite:`Cundall1979`.'
	def __init__(self,r1,r2,E1,E2=None,ktDivKn=.2,name=''):
		'Initialize the model with constants. If E2 is not given, it is the same as E1'
		if not E2: E2=E1
		self.E1,self.E2=E1,E2
		self.r1,self.r2=r1,r2
		# fictious contact area for the linear model
		self.contA=pi*min(self.r1,self.r2)**2
		# normal stiffness
		self.kn=(1./(self.r1/(self.E1*self.contA)+self.r2/(self.E2*self.contA)))
		#
		self.name=name
	def F_u(uN):
		'Return :math:`F=k_n u_N` (uses the Woo convention).'
		return uN*self.kn
	@staticmethod
	def make_plot():
		import pylab
		pylab.figure()
		uu=numpy.linspace(0,1e-3,num=100)
		pylab.plot(uu,F_u(uu),label='Cundall model')
		pylab.grid(True)
		pylab.legend()


class HertzModel(CundallModel):
	'Purely elastic Hertz contact model; base class for :obj:`SchwarzModel`. For details, see :ref:`hertzian_contact_models`.'
	def __init__(self,r1,r2,E1,nu1,E2=None,nu2=None,name=''):
		'Initialize the model with constants. If nu2 is not given, it is the same as nu1. Passes most parameters to :obj:`CundallModel`.'
		super(HertzModel,self).__init__(r1=r1,r2=r2,E1=E1,E2=E2,name=name)
		if not nu2: nu2=nu1
		self.nu1,self.nu2=nu1,nu2
		# effective modulus
		self.K=(4/3.)*1./((1-self.nu1**2)/self.E1+(1-self.nu2**2)/self.E2)
		# effective radius
		self.R=1./(1./self.r1+1./self.r2)
	def delta_a(self,a):
		':math:`\\delta(a)`'
		return a**2/self.R
	def F_delta(self,delta):
		':math:`F(\\delta)`'
		return self.K*sqrt(self.R)*delta**(3/2.) if delta>=0 else float('nan')
	def a_F(self,F):
		':math:`a(F)`'
		return (F*self.R/self.K)**(1/3.) if F>=0 else float('nan')
	def F_a(self,a):
		':math:`F(a)`'
		return a**3*self.K/self.R
	# pylab.plot([mm[0].deltaHat(mm[0].aUnhat(a)**2/mm[0].R) for a in aaHat],aaHat,label='Hertz')


class SchwarzModel(HertzModel):
	'Schwarz contact model, following :cite:`Schwarz2003`. Details are given in :ref:`adhesive_contact_models`.'
	@staticmethod
	def makeDefault(**kw):
		'Return instance of the model with some "reasonable" parameters, which are overridden with **kw. Useful for quick testing.'
		params=dict(r1=1e-2,r2=1e-2,E1=1e6,E2=1e6,nu1=.2,nu2=.2,gamma=.1,alpha=.5)
		params.update(kw)
		return SchwarzModel(**params)
	def __init__(self,r1,r2,E1,nu1,alpha,gamma,E2=None,nu2=None,name=''):
		'Initialize the model; passes elastic parameters to :obj:`HertzModel`.'
		super(SchwarzModel,self).__init__(r1=r1,r2=r2,E1=E1,nu1=nu1,E2=E2,nu2=nu2,name=name)
		# adhesive parameters
		self.alpha,self.gamma=alpha,gamma
		# critical force
		self.Fc=-(6*pi*self.R*self.gamma)/(self.alpha**2+3)
		# auxiliary term
		self.xi=sqrt((2*pi*self.gamma)*(1-3/(self.alpha**2+3))/(3*self.K))
		# name of the model, for using in plots etc
		self.name=name
	def F_a(self,a):
		':math:`F(a)`'
		return (sqrt(a**3*self.K/self.R)-self.alpha*sqrt(-self.Fc))**2+self.Fc
	def a_F(self,F):
		':math:`a(F)`'
		# return both stable and unstable solution
		Fc=-(6*pi*self.R*self.gamma)/(self.alpha**2+3)
		if F-self.Fc<0: return float('nan'),float('nan')
		i1=self.alpha*sqrt(-self.Fc)+sqrt(F-self.Fc)
		i2=self.alpha*sqrt(-self.Fc)-sqrt(F-self.Fc)
		return (self.R/self.K)**(1/3.)*(i1)**(2/3.),((self.R/self.K)**(1/3.)*i2**(2/3.) if i2>0 else float('nan'))
	def delta_a(self,a):
		':math:`\\delta(a)`'
		return a**2/self.R-4*self.xi*sqrt(a)
	def a_delta(self,delta,loading=True):
		':math:`a(\\delta)`'
		import scipy.optimize
		def fa(a,delta): return self.delta_a(a)-delta
		def fa1(a,delta): return 2*a/self.R-2*self.xi/sqrt(a)
		def fa2(a): return 2/self.R+self.xi*a**(-3/2.)
		aMin=(self.R*self.xi)**(2/3.)
		dMin=-3*self.xi**(4/3.)*self.R**(1/3.)
		if delta<dMin: return float('nan')
		if not loading and delta>0: return float('nan')
		if loading:
			return scipy.optimize.newton(fa,x0=2*aMin,fprime=fa1,args=(delta,),maxiter=1000)
		else:
			return scipy.optimize.bisect(fa,a=0,b=aMin,args=(delta,))

	def fHat_aHat(self,aHat): return self.fHat(numpy.array([self.F_a(self.aUnhat(ah)) for ah in aHat]))
	def aHat_fHat(self,fHat): return self.aHat(numpy.array([self.a_F(self.fUnhat(fh)) for fh in fHat]))
	def deltaHat_aHat(self,aHat): return self.deltaHat(numpy.array([self.delta_a(self.aUnhat(ah)) for ah in aHat]))
	def aHat_deltaHat(self,deltaHat,loading=True): return self.aHat(numpy.array([self.a_delta(self.deltaUnhat(dh),loading=loading) for dh in deltaHat]))
	# def deltaHat_fHat(self,fHat): return numpy.array([self.deltaHat(self.delta_a(self.a_F(self.fUnhat(fh))[0])) for fh in fHat])

	def aHat(self,a):
		':math:`\\hat a(a)`'
		return a*((pi*self.gamma*self.R**2)/self.K)**(-1/3.)
	def aUnhat(self,aHat):
		':math:`a(\\hat a)`'
		return aHat*((pi*self.gamma*self.R**2)/self.K)**(1/3.)
	def fHat(self,F):
		':math:`\\hat F(F)`'
		return F/(pi*self.gamma*self.R)
	def fUnhat(self,fHat):
		':math:`F(\\hat F)`'
		return fHat*(pi*self.gamma*self.R)
	def deltaHat(self,delta):
		':math:`\\hat\\delta(\\delta)`'
		return delta*(pi**2*self.gamma**2*self.R/self.K**2)**(-1/3.)
	def deltaUnhat(self,deltaHat):
		':math:`\\delta(\\hat \\delta)`'
		return deltaHat*(pi**2*self.gamma**2*self.R/self.K**2)**(1/3.)

	@staticmethod
	def normalized_plot(what,alphaGammaName,N=1000,stride=50):
		'''
		Create normalized plot as it appears in :cite:`Maugis1992` including range of axes with normalized quantities. This function is mainly useful for documentation of Woo itself.

		:param what: one of ``a(delta)``, ``F(delta)``, ``a(F)``
		:param alphaGammaName: list of (alpha,gamma,name) tuples which are passed to the :obj:`SchwarzModel` constructor
		:param N: numer of points in linspace for each axis
		:param stride: stride for plotting inverse relationships (with points)
		'''
		assert what in ('a(delta)','F(delta)','a(F)')
		import pylab
		invKw=dict(linewidth=4,alpha=.3)
		kw=dict(linewidth=2)
		pylab.figure()
		for (alpha,gamma,name) in alphaGammaName:
			m=SchwarzModel.makeDefault(alpha=alpha,gamma=gamma,name=name)
			if what=='a(delta)':
				ddHat=numpy.linspace(-1.3,2.,num=N)
				aaHat=numpy.linspace(0,2.1,num=N)
				pylab.plot(m.deltaHat_aHat(aaHat),aaHat,label='$\\alpha$=%g %s'%(m.alpha,m.name),**kw)
				pylab.plot(ddHat[::stride],m.aHat_deltaHat(ddHat[::stride],loading=True),'o',**invKw)
				pylab.plot(ddHat[::stride],m.aHat_deltaHat(ddHat[::stride],loading=False),'o',**invKw)
			elif what=='F(delta)':
				ffHat=numpy.linspace(-2.1,1.5,num=N)
				ddHat=numpy.linspace(-1.3,2.,num=N)
				pylab.plot(ddHat,m.fHat_aHat(m.aHat_deltaHat(ddHat)),label='$\\alpha$=%g %s'%(m.alpha,m.name),**kw)
				pylab.plot(ddHat,m.fHat_aHat(m.aHat_deltaHat(ddHat,loading=False)),**kw)
				pylab.plot([m.deltaHat(m.delta_a(m.a_F(m.fUnhat(fh))[0])) for fh in ffHat[::stride]],ffHat[::stride],'o',**invKw)
				pylab.plot([m.deltaHat(m.delta_a(m.a_F(m.fUnhat(fh))[1])) for fh in ffHat[::stride]],ffHat[::stride],'o',**invKw)
			elif what=='a(F)':
				aaHat=numpy.linspace(0,2.5,num=N)
				ffHat=numpy.linspace(-3,4,num=N)
				pylab.plot(ffHat,[a[0] for a in m.aHat_fHat(ffHat)],label='$\\alpha$=%g %s'%(m.alpha,m.name),**kw)
				pylab.plot(ffHat,[a[1] for a in m.aHat_fHat(ffHat)],**kw)
				pylab.plot(m.fHat_aHat(aaHat[::stride]),aaHat[::stride],'o',label=None,**invKw)
		if what=='a(delta)': 
			pylab.plot([m.deltaHat(HertzModel.delta_a(m,m.aUnhat(a))) for a in aaHat],aaHat,label='Hertz',**kw)
			pylab.xlabel('$\hat\delta$')
			pylab.ylabel('$\hat a$')
			pylab.ylim(ymax=2.1)
			pylab.xlim(xmax=2.)
		elif what=='F(delta)':
			pylab.plot(ddHat,[m.fHat(HertzModel.F_delta(m,m.deltaUnhat(d))) for d in ddHat],label='Hertz',**kw)
			pylab.xlabel('$\hat\delta$')
			pylab.ylabel('$\hat P$')
			pylab.xlim(-1.3,2)
			pylab.ylim(-2.1,2)
			pylab.axhline(color='k')
			pylab.axvline(color='k')
		elif what=='a(F)':
			pylab.plot(ffHat,[m.aHat(HertzModel.a_F(m,m.fUnhat(f))) for f in ffHat],label='Hertz',**kw)
			pylab.xlabel('$\hat P$')
			pylab.ylabel('$\hat a$')
			pylab.xlim(xmax=4)
		pylab.grid(True)
		pylab.legend(loc='best')
		

if __name__=='__main__':

	alphaGammaName=[(1.,.1,'JKR'),(.5,.1,''),(.01,.1,u'→DMT')]
	SchwarzModel.normalized_plot('a(delta)',alphaGammaName)
	SchwarzModel.normalized_plot('F(delta)',alphaGammaName)
	SchwarzModel.normalized_plot('a(F)',alphaGammaName)
	import pylab
	pylab.show()
