from yade import core
core.Field.nod=core.Field.nodes

try:
	from yade import dem
	dem.DemField.par=dem.DemField.particles
	dem.DemField.con=dem.DemField.contacts
	dem.Particle.mat=dem.Particle.material
	# DemData defines those methods, which are used for transparent access to respective data field
	core.Node.dem=property(dem.DemData._getDataOnNode,dem.DemData._setDataOnNode)
	# support Node.dyn (legacy)
	def DemData_warnGet(self):
		import warnings
		warnings.warn("Node.dyn is deprecated, use Node.dem instead.",DeprecationWarning)
		return self.dem
	def DemData_warnSet(self,val):
		import warnings
		warnings.warn("Node.dyn is deprecated, use Node.dem instead.",DeprecationWarning)
		self.dem=val
	core.Node.dyn=property(DemData_warnGet,DemData_warnSet)
except ImportError: pass

try:
	from yade import sparc
	core.Node.sparc=property(sparc.SparcData._getDataOnNode,sparc.SparcData._setDataOnNode)
except ImportError: pass

try:
	from yade import gl
	core.Node.gl=property(gl.GlData._getDataOnNode,gl.GlData._setDataOnNode)
except ImportError: pass

#try:
#	from yade import ancf
#	core.Node.ancf=property(ancf.AncfData._getDataOnNode,ancf.AncfData._setDataOnNode)
#except ImportError: pass
