# encoding: utf-8
'''
Collection of classes providing python-only implementation of various implementation models, mostly for testing or plotting in documentation. Plus a class for selecting material model along with all its parameters for use in preprocessors.
'''

from math import pi,sqrt
import numpy
from minieigen import *

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
	def a_delta(self,delta):
		':math:`a(\\delta)=\\sqrt{R\delta}`'
		return sqrt(self.R*delta)
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
		'Return instance of the model with some "reasonable" parameters, which are overridden with ``**kw``. Useful for quick testing.'
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
	def deltaMin(self):
		r':math:`\delta_{\min}=-3R^\frac{1}{3}\xi^\frac{4}{3}`.'
		return -3*self.R**(1/3.)*self.xi**(4/3.)
	def aMin(self):
		r':math:`a_{\min}=(R\xi)^\frac{2}{3}`.'
		return (self.R*self.xi)**(2/3.)
	def aHi(self,delta):
		r':math:`a_{\max}=a_{\min}+\sqrt{R(\delta-\delta_{\min})}`.'
		return self.aMin()+sqrt(self.R*(delta-self.deltaMin()))

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
	def normalized_plot(what,alphaGammaName,N=1000,stride=50,aHi=[],legendAlpha=.5):
		'''
		Create normalized plot as it appears in :cite:`Maugis1992` including range of axes with normalized quantities. This function is mainly useful for documentation of Woo itself.

		:param what: one of ``a(delta)``, ``F(delta)``, ``a(F)``
		:param alphaGammaName: list of (alpha,gamma,name) tuples which are passed to the :obj:`SchwarzModel` constructor
		:param N: numer of points in linspace for each axis
		:param stride: stride for plotting inverse relationships (with points)
		:param aHi: with ``what==a(delta)``, show upper bracket for $a$ for i-th model, if *i* appears in the list
		'''
		assert what in ('a(delta)','F(delta)','a(F)')
		import pylab
		invKw=dict(linewidth=4,alpha=.3)
		kw=dict(linewidth=2)
		pylab.figure()
		for i,(alpha,gamma,name) in enumerate(alphaGammaName):
			m=SchwarzModel.makeDefault(alpha=alpha,gamma=gamma,name=name)
			if what=='a(delta)':
				ddHat=numpy.linspace(-1.3,2.,num=N)
				aaHat=numpy.linspace(0,2.1,num=N)
				pylab.plot(m.deltaHat_aHat(aaHat),aaHat,label='$\\alpha$=%g %s'%(m.alpha,m.name),**kw)
				ddH=m.deltaHat_aHat(aaHat)
				if i in aHi: pylab.plot(ddH,[m.aHat(m.aHi(m.deltaUnhat(dh))) for dh in ddH],label='$\\alpha$=%g $a_{\mathrm{hi}}$'%m.alpha,linewidth=1,alpha=.4)
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
			ddH=[m.deltaHat(HertzModel.delta_a(m,m.aUnhat(a))) for a in aaHat]
			pylab.plot(ddH,aaHat,label='Hertz',**kw)
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
		try:
			pylab.legend(loc='best',framealpha=legendAlpha)
		except:
			pylab.legend(loc='best')
		

import woo.pyderived
class ContactModelSelector(woo.core.Object,woo.pyderived.PyWooObject):
	'User interface for humanely selecting contact model and all its features, plus functions returning respective :obj:`woo.dem.CPhysFunctor` and :obj:`woo.dem.LawFunctor` objects.'
	_classTrait=None
	_PAT=woo.pyderived.PyAttrTrait
	_attrTraits=[
		_PAT(str,'name','linear',triggerPostLoad=True,choice=['linear','pellet','Hertz','DMT','Schwarz','concrete','ice'],doc='Material model to use.'),
		_PAT(Vector2i,'numMat',Vector2i(1,1),noGui=True,guiReadonly=True,triggerPostLoad=True,doc='Minimum and maximum number of material definitions.'),
		_PAT([str,],'matDesc',[],noGui=True,triggerPostLoad=True,doc='List of strings describing individual materials. Keep the description very short (one word) as it will show up in the UI combo box for materials.'),
		_PAT([woo.dem.Material],'mats',[],doc='Material definitions'),
		_PAT(float,'distFactor',1.,doc='Distance factor for sphere-sphere contacts (copied to :obj:`woo.dem.Bo1_Sphere_Aabb.distFactor` and :obj:`woo.dem.Cg2_Sphere_Sphere_L6Geom.distFactor`)'),
		# hertzian models
		_PAT(float,'poisson',.2,hideIf='self.name not in ("Hertz","DMT","Schwarz")',doc='Poisson ratio (:obj:`woo.dem.Cp2_FrictMat_HertzPhys.poisson`)'),
		_PAT(float,'surfEnergy',.01,unit=u'J/m²',hideIf='self.name not in ("DMT","Schwarz")',doc='Surface energy for adhesive models (:obj:`woo.dem.Cp2_FrictMat_HertzPhys.gamma`)'),
		_PAT(float,'restitution',1.,hideIf='self.name not in ("Hertz","DMT","Schwarz")',doc='Restitution coefficient for models with viscosity (:obj:`woo.dem.Cp2_FrictMat_HertzPhys.en`).'),
		_PAT(float,'alpha',.5,hideIf='self.name not in ("Schwarz",)',doc='Parameter interpolating between DMT and JKR extremes in the Schwarz model. :math:`alpha` was introduced in :cite:`Carpick1999`.'),
		# linear model
		_PAT(float,'damping',.2,hideIf='self.name not in ("linear",)',doc='Numerical (non-viscous) damping (:obj:`woo.dem.Leapfrog.damping`)'),
		_PAT(bool,'linRoll',False,hideIf='self.name!="linear"',doc='*Linear model*: enable rolling, with parameters set in :obj:`linRollParams`.'),
		_PAT(Vector3,'linRollParams',Vector3(1.,1.,1.),hideIf='self.name!="linear" or not self.linRoll',doc='Rolling parameters for the linear model, in the order of :obj:`relRollStiff <woo.dem.Law2_L6Geom_FrictPhys_IdealElPl.relRollStiff>`, :obj:`relTwistStiff <woo.dem.Law2_L6Geom_FrictPhys_IdealElPl.relTwistStiff>`, :obj:`rollTanPhi <woo.dem.Law2_L6Geom_FrictPhys_IdealElPl.rollTanPhi>`.'),
		# pellet model
		_PAT(bool,'plastSplit',False,hideIf='self.name not in ("pellet",)',doc='Split plastic dissipation into the normal and tangent component (obj:`woo.dem.Law2_L6Geom_PelletPhys_Pellet.plastSplit`).'),
		_PAT(Vector3,'pelletThin',(0,0,0),hideIf='self.name!="pellet"',doc='*Pellet model:* parameters for plastic thinning (decreasing pellet radius during normal plastic loading); their order is :obj:`thinRate <woo.dem.Law2_L6Geom_PelletPhys_Pellet.thinRate>`, :obj:`thinRelRMin <woo.dem.Law2_L6Geom_PelletPhys_Pellet.thinRelRMin>`, :obj:`thinExp <woo.dem.Law2_L6Geom_PelletPhys_Pellet.thinExp>`. Set the first value to zero to deactivate.'),
		_PAT(Vector3,'pelletConf',(0,0,0),hideIf='self.name!="pellet"',doc='*Pellet model:* parameters for history-independent adhesion ("confinement"); the values are :obj:`confSigma <woo.dem.Law2_L6Geom_PelletPhys_Pellet.confSigma>`, :obj:`confRefRad <woo.dem.Law2_L6Geom_PelletPhys_Pellet.confRefRad>` and :obj:`confExp <woo.dem.Law2_L6Geom_PelletPhys_Pellet.confExp>`.'),
	]
	def __init__(self,**kw):
		woo.core.Object.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Object,**kw)
	def postLoad(self,what):
		'Do various consistency adjustments, such as materials matching the selected model type'
		# check size constraints
		if len(self.mats)>self.numMat[1]: self.mats=self.mats[:self.numMat[1]]
		elif len(self.mats)<self.numMat[0]: self.mats+=[self.getMat() for i in range(self.numMat[0]-len(self.mats))]
		# check types with the current material model
		for i,m0 in enumerate(self.mats):
			# if class does not match, replace things and preserve what we can preserve
			if m0.__class__!=self.getMatClass():
				m=self.getMat()
				for trait in m._getAllTraits():
					if hasattr(m0,trait.name): setattr(m,trait.name,getattr(m0,trait.name))
				self.mats[i]=m
		# modify traits so that the sequence only accepts required material type
		### FIXME: per-instance traits!!!
		### FIXME: document this
		if 'mats' not in self._instanceTraits:
			import copy
			matsTrait=[t for t in self._attrTraits if t.name=='mats'][0]
			self._instanceTraits['mats']=copy.deepcopy(matsTrait)
		matsTrait=self._instanceTraits['mats']
		matsTrait.pyType=[self.getMatClass(),] # indicate array of instances of this type
		matsTrait.range=self.numMat
		matsTrait.choice=self.matDesc

	def getFunctors(self):
		'''Return tuple of ``([CPhysFunctor,...],[LawFunctor,...])`` corresponding to the selected model and parameters.'''
		import woo.dem, math
		if self.name=='linear':
			law=woo.dem.Law2_L6Geom_FrictPhys_IdealElPl()
			if self.linRoll: law.relRollStiff,law.relTwistStiff,law.rollTanPhi=self.linRollParams 
			return [woo.dem.Cp2_FrictMat_FrictPhys()],[law]
		elif self.name=='pellet':
			law=woo.dem.Law2_L6Geom_PelletPhys_Pellet(plastSplit=self.plastSplit,confSigma=self.pelletConf[0],confRefRad=self.pelletConf[1],confExp=self.pelletConf[2],thinRate=self.pelletThin[0],thinRelRMin=self.pelletThin[1],thinExp=self.pelletThin[2])
			if not math.isnan(self.pelletYf1Params[0]): law.yieldFunc,law.yf1_beta,law.yf1_w=1,self.pelletYf1Params[0],self.pelletYf1Params[1]
			return [woo.dem.Cp2_PelletMat_PelletPhys()],[law]
		elif self.name=='Hertz':
			return [woo.dem.Cp2_FrictMat_HertzPhys(poisson=self.poisson,alpha=0.,gamma=0.,en=self.restitution)],[woo.dem.Law2_L6Geom_HertzPhys_DMT()]
		elif self.name=='DMT':
			return [woo.dem.Cp2_FrictMat_HertzPhys(poisson=self.poisson,alpha=0.,gamma=self.surfEnergy,en=self.restitution)],[woo.dem.Law2_L6Geom_HertzPhys_DMT()]
		elif self.name=='Schwarz':
			return [woo.dem.Cp2_FrictMat_HertzPhys(poisson=self.poisson,alpha=self.alpha,gamma=self.surfEnergy,en=self.restitution)],[woo.dem.Law2_L6Geom_HertzPhys_DMT()]
		elif self.name=='concrete':
			return [woo.dem.Cp2_ConcreteMat_ConcretePhys()],[woo.dem.Law2_L6Geom_ConcretePhys()]
		elif self.name=='ice':
			return [woo.dem.Cp2_IceMat_IcePhys()],[woo.dem.Law2_L6Geom_IcePhys()]
		else: raise ValueError('Unknown model: '+self.name)
	def getMat(self):
		'''Return default-initialized material for use with this model.'''
		return self.getMatClass()()
	def getMatClass(self):
		'''Return class object of material for use with this model.'''
		import woo.dem
		d={'linear':woo.dem.FrictMat,'pellet':woo.dem.PelletMat,'Hertz':woo.dem.FrictMat,'DMT':woo.dem.FrictMat,'Schwarz':woo.dem.FrictMat,'concrete':woo.dem.ConcreteMat,'ice':woo.dem.IceMat}
		if self.name not in d: raise ValueError('Unknown model: '+self.name)
		return d[self.name]
	def getNonviscDamping(self):
		'''Return the value for :obj:`woo.dem.Leapfrog.damping`; returns zero for models with internal damping, and :obj:`damping` for the "linear" model.'''
		if self.name=='linear': return self.damping
		else: return 0.


if __name__=='__main__':

	alphaGammaName=[(1.,.1,'JKR'),(.5,.1,''),(.01,.1,u'→DMT')]
	SchwarzModel.normalized_plot('a(delta)',alphaGammaName)
	SchwarzModel.normalized_plot('F(delta)',alphaGammaName)
	SchwarzModel.normalized_plot('a(F)',alphaGammaName)
	import pylab
	pylab.show()
