def makeUnitsDict():
	unitDict={}
	def addUnit(name,mult):
		if name in unitDict:
			if unitDict[name]==mult: return
			raise ValueError('Inconsistent values for unit "%s": %g, %g'%(name,unitDict[name],mult))
		else:
			unitDict[name]=mult
	import woo.core
	for c in woo.core.Object._derivedCxxClasses:
		for t in c._attrTraits:
			for u in t.unit: addUnit(u,1.)
			for aa in t.altUnits:
				for a in aa:
					addUnit(a[0],1./a[1])
	return unitDict
