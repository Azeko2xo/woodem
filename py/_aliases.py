from yade import core
import warnings
core.Field.nod=core.Field.nodes
try:
	from yade import dem
	dem.DemField.par=dem.DemField.particles
	dem.DemField.con=dem.DemField.contacts
	#dem.Particle.con=dem.Particle.contacts
	dem.Particle.mat=dem.Particle.material
	# DemData defines those methods, which are used for transparent access to respective data field
	core.Node.dem=property(dem.DemData._getOnNode,dem.DemData._setOnNode)
	# support Node.dyn (legacy)
	def DemData_warnGet(self):
		warnings.warn("Node.dyn is deprecated, use Node.dem instead.",DeprecationWarning)
		return self.dem
	def DemData_warnSet(self,val):
		warnings.warn("Node.dyn is deprecated, use Node.dem instead.",DeprecationWarning)
		self.dem=val
	core.Node.dyn=property(DemData_warnGet,DemData_warnSet)
except ImportError: pass

try:
	from yade import sparc
	core.Node.sparc=property(sparc.SparcData._getOnNode,sparc.SparcData._setOnNode)
except ImportError: pass
