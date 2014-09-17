'''Define various convenience attributes, such as ``Node.dem`` for accessing DEM data of a node (equivalent to ``node->getData<DemData>()`` in c++).'''

from woo import core
from woo.core import Master
import sys, warnings
core.Field.nod=core.Field.nodes

## proxy for attribute-like access to Scene.labels
## http://stackoverflow.com/questions/16061041/proxy-class-for-accessing-other-class-items-as-attributes-getitem-infinite
class LabelMapperProxy(object):
	'Proxy for attribute-like access to :obj:`woo.core.LabelMapper`.'
	def __init__(self,mapper,prefix=''): self.__dict__['_mapper'],self.__dict__['_prefix']=mapper,prefix
	def __getattr__(self,key):
		# some mapper method was requested
		if key=='__dir__': return lambda: self._mapper.__dir__(self._prefix)
		if key.startswith('_'):
			if self._prefix: raise AttributeError('Attributes/methods starting with _ must be obtained from root LabelMappr or proxy (this instance has prefix "'+self._prefix+'")')
			else: return getattr(self._mapper,key)
		# submodule requested, return proxy with new prefix
		if self._mapper._whereIs(self._prefix+key)==core.LabelMapper.inMod:
			return LabelMapperProxy(self._mapper,prefix=self._prefix+key+'.')
		# return object
		return self._mapper[self._prefix+key]
	def __setattr__(self,key,val): self._mapper[self._prefix+key]=val
	def __delattr__(self,key): del self._mapper[self._prefix+key]
	# def __dir__(self): return self._mapper.__dir__(prefix=self._prefix)
	# def __len__(self): return self._mapper.__len__(prefix=self._prefix)
	# def _newModule(self,mod): return self._mapper._newModule(self._prefix+mod)
def Scene_lab(scene):
	return LabelMapperProxy(scene.labels,prefix='')
core.Scene.lab=property(Scene_lab)





## deprecated classes
# if old class name is used, the new object is constructed and a warning is issued about old name being used
# keep chronologically ordered, oldest first; script/rename-class.py appends at the end
_deprecated={
	('woo.dem','FlexFacet'):('woo.fem','Membrane'),
	('woo.gl','Gl1_FlexFacet'):('woo.gl','Gl1_Membrane'),
	('woo.dem','In2_FlexFacet_ElastMat'):('woo.fem','In2_Membrane_ElastMat'),
	#
	('woo.dem','ParticleFactory'):('woo.dem','ParticleInlet'),
	('woo.dem','RandomFactory'):('woo.dem','RandomInlet'),
	('woo.dem','BoxFactory'):('woo.dem','BoxInlet'),
	('woo.dem','CylinderFactory'):('woo.dem','CylinderInlet'),
	('woo.dem','BoxFactory2d'):('woo.dem','BoxInlet2d'),
	('woo.dem','ConveyorFactory'):('woo.dem','ConveyorInlet'),
	('woo.dem','BoxDeleter'):('woo.dem','BoxOutlet'),
	#
	### END_RENAMED_CLASSES_LIST ### (do not delete this line; scripts/rename-class.py uses it
}

def injectDeprecated():
	''' Inject decprecated classes into woo modules as needed.
	'''
	proxyNamespace={}
	class warnWrap:
		def __init__(self,_old,_new):
			self.old,self.new=_old,_new
		def __call__(self,*args,**kw):
			warnings.warn("%s.%s was renamed to %s.%s; update your code!"%(self.old[0],self.old[1],self.new.__module__,self.new.__name__),DeprecationWarning,stacklevel=2);
			return self.new(*args,**kw)
	# deprecated names
	for deprec,curr in _deprecated.items():
		# try to import both modules
		try: mDep,mCurr=sys.modules[deprec[0]],sys.modules[curr[0]]
		except KeyError: continue
		# new name not found?!
		try: setattr(mDep,deprec[1],warnWrap(deprec,getattr(mCurr,curr[1])))
		except AttributeError: pass

injectDeprecated()

try:
	from woo import dem
	dem.DemField.par=dem.DemField.particles
	dem.DemField.con=dem.DemField.contacts
	dem.Particle.mat=dem.Particle.material
	core.Scene.dem=property(lambda s: dem.DemField.sceneGetField(s))
	core.Scene.hasDem=property(lambda s: dem.DemField.sceneHasField(s))
	# DemData defines those methods, which are used for transparent access to respective data field
	core.Node.dem=property(dem.DemData._getDataOnNode,dem.DemData._setDataOnNode)

	# those are deprecated
	def deprecWrapper(self,_oldName,_newName,_newFunc,*args,**kw):
		warnings.warn('The %s function is deprecated, use %s instead.'%(_oldName,_newName),DeprecationWarning,stacklevel=3)
		return _newFunc(self,*args,**kw)

	dem.ParticleContainer.append=lambda self, *args,**kw: deprecWrapper(self,'dem.ParticleContainer.append','dem.ParticleContainer.add',dem.ParticleContainer.add,*args,**kw)
	dem.ParticleContainer.appendClumped=lambda self, *args,**kw: deprecWrapper(self,'dem.ParticleContainer.appendClumped','dem.ParticleContainer.addClumped',dem.ParticleContainer.addClumped,*args,**kw)

	# nicer names
	import woo.utils
	dem.DemField.minimalEngines=staticmethod(woo.utils.defaultEngines)

	dem.Sphere.make=staticmethod(woo.utils.sphere)
	dem.Ellipsoid.make=staticmethod(woo.utils.ellipsoid)
	dem.Capsule.make=staticmethod(woo.utils.capsule)
	dem.Wall.make=staticmethod(woo.utils.wall)
	dem.InfCylinder.make=staticmethod(woo.utils.infCylinder)
	dem.Facet.make=staticmethod(woo.utils.facet)
	dem.Truss.make=staticmethod(woo.utils.truss)
except ImportError:
	core.Scene.hasDem=lambda o: False

try:
	from woo import fem
	fem.Membrane.make=staticmethod(woo.utils.membrane)
	fem.Tetra.make=staticmethod(woo.utils.tetra)
	fem.Tet4.make=staticmethod(woo.utils.tet4)
except ImportError:
	pass

try:
	from woo import sparc
	core.Scene.sparc=property(lambda s: sparc.SparcField.sceneGetField(s))
	core.Scene.hasSparc=property(lambda s: sparc.SparcField.sceneHasField(s))
	core.Node.sparc=property(sparc.SparcData._getDataOnNode,sparc.SparcData._setDataOnNode)
except ImportError:
	core.Scene.hasSparc=lambda o: False


try:
	import woo.cld
	core.Scene.clDem=property(lambda s: cld.CLDemField.sceneGetField(s))
	core.Scene.hasClDem=property(lambda s: woo.clDem.CLDemField.sceneHasField(s))
	core.Node.clDem=property(woo.cld.CLDemData._getDataOnNode,woo.cld.CLDemData._setDataOnNode)
except ImportError:
	core.Scene.hasClDem=lambda o: False


try:
	from woo import gl
	core.Node.gl=property(gl.GlData._getDataOnNode,gl.GlData._setDataOnNode)
except ImportError: pass

#try:
#	from woo import ancf
#	core.Node.ancf=property(ancf.AncfData._getDataOnNode,ancf.AncfData._setDataOnNode)
#except ImportError: pass
