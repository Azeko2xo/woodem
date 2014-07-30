'''Woo is internally unit-agnostic (using `SI units <http://en.wikipedia.org/wiki/Si_units>`_ is highly recommended). This module defines conversion multipliers for various units; values of multipliers are taken from c++ sources.

The :obj:`unit` map is exposed as ``woo.unit``, so that expressions like ``16*woo.unit['deg']`` can be used in python.
'''

unit={}
'Map units to their respective multipliers'

baseUnit={}
'Map units to their base units (with unit multiplier)'

import sys
PY3K=(sys.version_info[0]==3)

def _makeUnitsDicts():
	def addUnitMultiplier(name,mult):
		global unit
		if not PY3K: name=unicode(name,'utf-8')
		if name in unit:
			if unit[name]==mult: return
			raise ValueError('Inconsistent multipliers for unit "%s": %g, %g'%(name,unit[name],mult))
		else: unit[name]=mult
	def addUnitBase(name,base):
		global baseUnit
		if not PY3K: name,base=unicode(name,'utf-8'),unicode(base,'utf-8')
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

_makeUnitsDicts()
