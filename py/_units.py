unit={}
baseUnit={}

def makeUnitsDicts():
	def addUnitMultiplier(name,mult):
		global unit
		if name in unit:
			if unit[name]==mult: return
			raise ValueError('Inconsistent multipliers for unit "%s": %g, %g'%(name,unit[name],mult))
		else: unit[name]=mult
	def addUnitBase(name,base):
		global baseUnit
		if name in baseUnit:
			if baseUnit[name]==base: return
			raise ValueError('Inconsistent base units for unit "%s": %s, %s'%(name,baseUnit[name],base))
		else: baseUnit[name]=base
	import woo.core
	for c in woo.core.Object._derivedCxxClasses:
		for t in c._attrTraits:
			# multipliers
			for u in t.unit: addUnitMultiplier(u,1.)
			for aa in t.altUnits:
				for a in aa:
					addUnitMultiplier(a[0],1./a[1])
			# base units
			for i,u in enumerate(t.unit):
				addUnitBase(u,u) #base unit maps to itself
				for a in t.altUnits[i]: addUnitBase(a[0],u)

makeUnitsDicts()
