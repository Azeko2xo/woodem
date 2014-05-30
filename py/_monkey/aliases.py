'''Define various convenience attributes, such as ``Node.dem`` for accessing DEM data of a node (equivalent to ``node->getData<DemData>()`` in c++).'''

from woo import core
from woo.core import Master
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


try:
	from woo import dem
	dem.DemField.par=dem.DemField.particles
	dem.DemField.con=dem.DemField.contacts
	dem.Particle.mat=dem.Particle.material
	core.Scene.dem=property(lambda s: dem.DemField.sceneGetField(s))
	core.Scene.hasDem=property(lambda s: dem.DemField.sceneHasField(s))
	def _Master_dem(o): raise ValueError("Master.dem is no longer supported, use Scene.dem instead")
	def _Master_hasDem(o): raise ValueError("Master.hasDem is no longer supported, use Scene.hasDem instead")
	core.Master.dem=property(_Master_dem)
	core.Master.hasdem=property(_Master_hasDem)
	# DemData defines those methods, which are used for transparent access to respective data field
	core.Node.dem=property(dem.DemData._getDataOnNode,dem.DemData._setDataOnNode)
	# this can be removed later
	def _DemData_parCount_get(d): raise AttributeError("DemData.parCount is superceded by DemData.parRef!")
	def _DemData_parCount_set(d): raise AttributeError("DemData.parCount is superceded by DemData.parRef!")
	dem.DemData.parCount=property(_DemData_parCount_get,_DemData_parCount_set)

	# nicer names
	import woo.utils
	dem.DemField.minimalEngines=staticmethod(woo.utils.defaultEngines)

	dem.Sphere.make=staticmethod(woo.utils.sphere)
	dem.Ellipsoid.make=staticmethod(woo.utils.ellipsoid)
	dem.Capsule.make=staticmethod(woo.utils.capsule)
	dem.Wall.make=staticmethod(woo.utils.wall)
	dem.InfCylinder.make=staticmethod(woo.utils.infCylinder)
	dem.Facet.make=staticmethod(woo.utils.facet)
except ImportError:
	core.Scene.hasDem=lambda o: False

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
