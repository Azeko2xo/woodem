from minieigen import *
from woo.dem import *
import woo.core, woo.models
from math import *
import numpy


class PourFeliciter(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'''Showcase for custom packing predicates, and importing surfaces from STL.'''
	_classTraits=None
	_PAT=woo.pyderived.PyAttrTrait # less typing
	_attrTraits=[
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		# preprocessor builds the simulation when called
		pass

class NewtonsCradle(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'''Showcase for custom packing predicates, and importing surfaces from STL.'''
	_classTraits=None
	_PAT=woo.pyderived.PyAttrTrait # less typing
	_attrTraits=[
		_PAT(int,'nSpheres',5,'Total number of spheres'),
		_PAT(int,'nFall',1,'The number of spheres which are out of the equilibrium position at the beginning.'),
		_PAT(float,'fallAngle',pi/4.,unit='deg',doc='Initial angle of falling spheres.'),
		_PAT(float,'rad',.005,unit='m',doc='Radius of spheres'),
		_PAT(Vector2,'cabHtWd',(.1,.1),unit='m',doc='Height and width of the suspension'),
		_PAT(float,'cabRad',.0005,unit='m',doc='Radius of the suspending cables'),
		_PAT(woo.models.ContactModelSelector,'model',woo.models.ContactModelSelector(name='Hertz',restitution=.99,numMat=(1,2),matDesc=['spheres','cables'],mats=[FrictMat(density=3e3,young=2e8),FrictMat(density=.001,young=2e8)]),doc='Select contact model. The first material is for spheres; the second, optional, material, is for the suspension cables.'),
		_PAT(Vector3,'gravity',(0,0,-9.81),'Gravity acceleration'),
		_PAT(int,'plotEvery',10,'How often to collect plot data'),
		_PAT(float,'dtSafety',.7,':obj:`woo.core.Scene.dtSafety`')
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		pre=self
		S=woo.core.Scene(fields=[DemField(gravity=pre.gravity)],dtSafety=self.dtSafety)
		S.pre=pre.deepcopy()

		# preprocessor builds the simulation when called
		xx=numpy.linspace(0,(pre.nSpheres-1)*2*pre.rad,num=pre.nSpheres)
		mat=pre.model.mats[0]
		cabMat=(pre.model.mats[1] if len(pre.model.mats)>1 else mat)
		ht=pre.cabHtWd[0]
		for i,x in enumerate(xx):
			color=min(.999,(x/xx[-1]))
			s=Sphere.make((x,0,0) if i>=pre.nFall else (x-ht*sin(pre.fallAngle),0,ht-ht*cos(pre.fallAngle)),radius=pre.rad,mat=mat,color=color)
			n=s.shape.nodes[0]
			S.dem.par.add(s)
			# sphere's node is integrated
			S.dem.nodesAppend(n)
			for p in [Vector3(x,-pre.cabHtWd[1]/2,pre.cabHtWd[0]),Vector3(x,pre.cabHtWd[1]/2,pre.cabHtWd[0])]:
				t=Truss.make([n,p],radius=pre.cabRad,wire=False,color=color,mat=cabMat,fixed=None)
				t.shape.nodes[1].blocked='xyzXYZ'
				S.dem.par.add(t)
		S.engines=DemField.minimalEngines(model=pre.model,dynDtPeriod=20)+[
			IntraForce([In2_Truss_ElastMat()]),
				woo.core.PyRunner(self.plotEvery,'S.plot.addData(i=S.step,t=S.time,total=S.energy.total(),relErr=(S.energy.relErr() if S.step>1000 else 0),**S.energy)'),
			]
		S.lab.dynDt.maxRelInc=1e-6
		S.trackEnergy=True
		S.plot.plots={'i':('total','**S.energy')}

		return S



