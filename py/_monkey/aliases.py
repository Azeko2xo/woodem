'''Define various convenience attributes, such as ``Node.dem`` for accessing DEM data of a node (equivalent to ``node->getData<DemData>()`` in c++).'''

from woo import core
from woo.core import Master
core.Field.nod=core.Field.nodes

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
except ImportError:
	core.Scene.hasDem=lambda o: False

try:
	from woo import sparc
	Scene.sparc=property(lambda s: sparc.SparcField.sceneGetField(s))
	Scene.hasSparc=property(lambda s: sparc.SparcField.sceneHasField(s))
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
