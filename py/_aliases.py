from yade import Omega
from yade import core
core.Field.nod=core.Field.nodes

try:
	from yade import dem
	dem.DemField.par=dem.DemField.particles
	dem.DemField.con=dem.DemField.contacts
	dem.Particle.mat=dem.Particle.material
	Omega.dem=property(lambda o: dem.DemField.sceneGetField())
	Omega.hasDem=property(lambda o: dem.DemField.sceneHasField)
	# DemData defines those methods, which are used for transparent access to respective data field
	core.Node.dem=property(dem.DemData._getDataOnNode,dem.DemData._setDataOnNode)
except ImportError: pass

try:
	from yade import sparc
	Omega.sparc=property(lambda o: sparc.SparcField.sceneGetField())
	Omega.hasSparc=property(lambda o: sparc.SparcField.sceneHasField)
	core.Node.sparc=property(sparc.SparcData._getDataOnNode,sparc.SparcData._setDataOnNode)
except ImportError: pass

try:
	import yade.clDem
	Omega.clDem=property(lambda o: yade.clDem.CLDemField.sceneGetField())
	#Omega.hasClDem=property(lambda o: yade.clDem.CLDemField.sceneHasField)
	#core.Node.clDem=property(yade.clDem.SparcData._getDataOnNode,yade.clDem.SparcData._setDataOnNode)
except ImportError: pass


try:
	from yade import gl
	core.Node.gl=property(gl.GlData._getDataOnNode,gl.GlData._setDataOnNode)
except ImportError: pass

#try:
#	from yade import ancf
#	core.Node.ancf=property(ancf.AncfData._getDataOnNode,ancf.AncfData._setDataOnNode)
#except ImportError: pass
