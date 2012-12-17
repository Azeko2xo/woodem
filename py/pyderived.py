#encoding: utf-8
from woo.core import *
from minieigen import *

class PyAttrTrait:
	#		
	# fake cxxType
	#
	primitiveTypes={int:'int',str:'string',float:'Real',bool:'bool',
		Vector2:'Vector2r',Vector3:'Vector3r',Vector6:'Vector6r',Matrix3:'Matrix3r',Matrix6:'Matrix6r',
		Quaternion:'Quaternionr',Vector2i:'Vector2i',Vector3i:'Vector3i',Vector6i:'Vector6i',
		MatrixX:'MatrixXr',VectorX:'VectorXr'
	}
	def __init__(self,pyType,name,ini,doc,# *,
			unit=None,
			noGui=False,
			noDump=False,
			#multiUnit=False,
			rgbColor=False,
			#prefUnit=None,
			#altUnit=None,
			startGroup=None,
			hideIf=None,
			range=None,
			choice=None,
			bits=None,
			buttons=None,
			altUnits=None,
		):
		# validity checks
		if range:
			if (pyType,type(range)) not in [(int,Vector2i),(float,Vector2)]: raise TypeError("Range must be Vector2 for floats and Vector2i for ints")
		if isinstance(pyType,list):
			if len(pyType)!=1: raise TypeError('Type must be a list of length exactly one, or a plain type')
			if pyType[0] in self.primitiveTypes: self.cxxType='vector<%s>'%self.primitiveTypes[pyType[0]]
			elif isinstance(pyType[0],type): self.cxxType='vector<shared_ptr<%s>>'%(pyType[0].__name__)
			else: raise TypeError('List element must be a type, not a %s'%(str(pyType[0])))
			# force correct types in the sequence
			if pyType[0] in self.primitiveTypes: ini=[pyType[0](i) for i in ini]
			else:
				for i,v in enumerate(ini):
					if v==None: continue # this is OK
					if not isinstance(v,pyType[0]): raise TypeError("%d-th initial value item must be a %s, not a %s"%(i,pyType[0],type(v)))
		elif pyType in self.primitiveTypes:
			self.cxxType=self.primitiveTypes[pyType]
			ini=pyType(ini)
		elif isinstance(pyType,type):
			self.cxxType='shared_ptr<%s>'%pyType.__name__
		else: raise ValueError('Type must be a list of length one, or a primitive type; not a %s'%str(pyType))
		#
		#
		# mandatory args
		self.pyType=pyType
		self.name=name
		self.ini=ini 
		self.doc=doc
		# optional args
		self.noGui=noGui
		self.noDump=noDump
		self.rgbColor=rgbColor
		self.startGroup=startGroup
		self.hideIf=hideIf
		self.range=range
		self.choice=choice
		self.bits=bits
		self.buttons=buttons
		# those are unsupported in python
		self.noSave=self.readonly=self.triggerPostLoad=self.hidden=self.noResize=self.pyByRef=self.static=self.activeLabel=False
		# 
		# units
		#
		import woo._units
		baseUnit=woo._units.baseUnit
		if isinstance(unit,str) or isinstance(unit,unicode):
			unit=[unit]
			if altUnits: altUnits=[altUnits]
		if not unit:
			self.unit=None
			self.multiUnit=False
			self.prefUnit=None
			self.altUnits=None
		elif isinstance(unit,list) or isinstance(unit,tuple):
			self.multiUnit=(len(unit)>1)
			self.unit=[]
			self.altUnits=[]
			self.prefUnit=[]
			for u in unit:
				# print self.name,u
				if u not in baseUnit: raise ValueError('Unknown unit %s; admissible values are: '%u+', '.join(baseUnit.keys()))
				base=baseUnit[u]
				self.unit.append(base)
				self.prefUnit.append((u,1./woo.unit[u]))
				alts=[]
				for alt in baseUnit:
					if baseUnit[alt]==base and alt!=base: alts.append((alt,1./woo.unit[alt]))
				self.altUnits.append(alts)
			# user-specified alternative units
			if altUnits:
				if len(altUnits)!=len(self.unit): raise ValueError("altUnits must be of the same length as unit")
				for i,au in enumerate(altUnits):
					self.altUnits[i]+=au
		else: raise ValueError('Unknown unit type %s (must be list, tuple, str, unicode): %s'%(type(unit).__name__,str(unit)))
	def checkType(self,val):
		'Check whether *val* has type compatible with declared type (pyType). Raise exception if not.'
		def tName(T): return (T.__module__+'.' if T.__module__!='__builtin__' else '')+T.__name__
		# sequences
		if isinstance(self.pyType,list):
			assert len(self.pyType)==1
			if not hasattr(val,'__len__'): raise TypeError("Attribute {self.name} declared as sequence of {T} ({self.cxxT}), but its value {val!s} of type {valType} is not a sequence (__len__ not defined).".format(self=self,T=tName(self.pyType[0]),val=val,valType=tName(type(val))))
			T=self.pyType[0]
			if T in self.primitiveTypes: # check convertibility
				for i,v in enumerate(val):
					try:
						if type(v) in (str,unicode) and T in (float,int): raise TypeError("Don't allow conversions from strings to numbers, since that will fail if used without conversion")
						T(v)
					except: raise TypeError("Attribute {self.name} declared as sequence of {T}, but {i}'th item {v!s} of type {itemType} is not convertible to {T}.".format(self=self,i=i,v=v,itemType=tName(type(v)),T=tName(T)))
			else:
				for i,v in enumerate(val):
					if v==None: continue # python representation for NULL shared_ptr
					if not isinstance(v,T): raise TypeError("Attribute {self.name} declared as a sequence of {T}, but {i}'th item {v!s} of type {itemType} is not a {T}.".format(self=self,i=i,v=v,itemType=tName(type(v)),T=tName(T)))
		else:
			# do the same as for sequence items; ugly code duplication
			T=self.pyType
			if T in self.primitiveTypes:
				try:
					if type(val) in (str,unicode) and T in (float,int): raise TypeError("Don't allow conversions from strings to numbers, since that will fail if used without conversion")
					T(val)
				except: raise TypeError("Attribute {self.name} declared as {T}, but value {val!s} of type {valType} is not convertible to {T}".format(self=self,val=val,valType=tName(type(val)),T=tName(T)))
			else:
				# objects
				if val!=None and not isinstance(val,T): raise TypeError("Attribute {self.name} declared as {T}, but value {val!s} of type {valType} is not a {T}".format(self=self,val=val,valType=tName(type(val)),T=tName(T)))

	def __str__(self): return '<PyAttrTrait '+self.name+' @ '+str(id(self))+'>'
	def __repr__(self): return self.__str__()

class PyWooObject:
	'Define some c++-compatibility functions for python classes'
	def wooPyInit(self,derivedClass,cxxBaseClass,**kw):
		'''Inject methods into derivedClass, so that it behaves like woo.core.Object,
		for the purposes of the GUI and expression dumps'''
		cxxBaseClass.__init__(self) # repeat, just to make sure
		self.cxxBaseClass=cxxBaseClass
		self.derivedClass=derivedClass
		for a in derivedClass._attrTraits: setattr(self,a.name,a.ini)
		for k in kw:
			if not hasattr(self,k): raise ValueError('No such attribute: %s'%k)
			setattr(self,k,kw[k])
		derivedClass.__str__=lambda o:'<%s @ %d (py)>'%(derivedClass.__name__,id(o))
		derivedClass.__repr__=derivedClass.__str__
		derivedClass._cxxAddr=property(lambda sefl: id(self))
		# pickle support
		def __getstate__(self):
			#print '__getstate__ in python'
			ret={}
			for a in derivedClass._attrTraits: ret[a.name]=getattr(self,a.name)
			ret.update(cxxBaseClass.__getstate__(self)) # get boost::python stuff as well
			return ret
		def __setstate__(self,st):
			#print '__setstate__ in python'
			self.__dict__.update(st)
		def deepcopy(self):
			'''The c++ dedepcopy uses boost::serialization, we need to use pickle. As long as deepcopy
			is called from python, this function gets precende over the c++ one.'''
			import pickle
			return pickle.loads(pickle.dumps(self))
		def checkAttrTypes(self):
			for a in derivedClass._attrTraits: a.checkType(getattr(self,a.name))
		derivedClass.__getstate__=__getstate__
		derivedClass.__setstate__=__setstate__
		derivedClass.deepcopy=deepcopy
		derivedClass.checkAttrTypes=checkAttrTypes
		self.checkAttrTypes() # call to make sure we don't carry garbage from the very start
		
	

# inheritance order must not change, due to us using __bases__[0] frequently
# when traversing the class hierarchy
class SamplePyDerivedPreprocessor(Preprocessor,PyWooObject):
	'Sample preprocessor written in pure python'
	_classTraits=None
	_attrTraits=[
		PyAttrTrait(bool,'switch',True,"Some bool switch, starting group",startGroup='General'),
		PyAttrTrait(int,'a',2,"Integer argument in python"),
		PyAttrTrait(str,'b','whatever',"String argument in python"),
		PyAttrTrait([Node,],'c',[],"List of nodes in python"),
		PyAttrTrait(float,'d',.5,"Parameter with range",range=Vector2(0,.7),startGroup='Advanced'),
		PyAttrTrait(int,'choice',2,"Param with choice",choice=[(0,'choice0'),(1,'choice1'),(2,'choice2')]),
		PyAttrTrait(int,'flags',1,"Param with bits",bits=['bit0','bit1','bit2','bit3','bit4'],buttons=(['Clear flags','self.flags=0','Set flags to zero'],True)),
		PyAttrTrait(int,'choice2',2,'Param with unnamed choice',choice=[0,1,2,3,4,5,6,-1]),
		PyAttrTrait(Vector3,'color',Vector3(.2,.2,.2),"Color parameter",rgbColor=True),
		PyAttrTrait(float,'length',1.5e-3,"Length",unit='mm'),
	]
	def __init__(self,**kw):
		# construct all instance attributes
		Preprocessor.__init__(self)
		self.wooPyInit(SamplePyDerivedPreprocessor,Preprocessor,**kw)
	def __call__(self):
		import woo.core, woo.dem, woo.utils
		S=woo.core.Scene(fields=[woo.dem.DemField()])
		S.dem.par.append([
			woo.utils.wall(0,axis=2,sense=1),
			woo.utils.sphere((0,0,1),radius=.2)
		])
		S.dem.gravity=(0,0,-10)
		S.dt=1e-4*woo.utils.pWaveDt(S)
		S.engines=woo.utils.defaultEngines(damping=.01)
		S.dem.collectNodes()
		return S

if __name__=='__main__':
	import woo.pre
	woo.pre.SamplePyDerivedPreprocessor=SamplePyDerivedPreprocessor

