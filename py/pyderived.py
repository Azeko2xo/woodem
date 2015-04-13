#encoding: utf-8
'Support subclassing c++ objects in python, with some limitations. Useful primarily for pure-python preprocessors.'

from woo.core import *
from minieigen import *


class PyAttrTrait(object):
	'''
	Class mimicking the `AttrTrait` template in c++, to be used when deriving from :obj:`PyWooObject`, like in this example (can be found in `woo.pyderived` module's source)::

		class _SamplePyDerivedPreprocessor(woo.core.Preprocessor,PyWooObject):
			'Sample preprocessor written in pure python'
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
				woo.core.Preprocessor.__init__(self)
				self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
			def __call__(self):
				pass
				# ...

	This class will be represented like this in the GUI:

	.. image:: fig/pyderived-gui.png


	:param pyType: python type object; can be

		* primitive type (like `bool`, `float`, `Vector3`, `str`, …)
		* sequence of primitive types, written as 1-list: `[bool,]`, `[float,]`, …
		* :obj:`woo Object <woo.core.Object>` or any derived type (:obj:`woo.core.Node`, :obj:`woo.dem.Sphere`, …)
		* sequence of woo objects, e.g. `[woo.core.Node,]`

		When a value is assigned to the attribute, provided type must be convertible to *pyType*, otherwise `TypeError` is raised. There are some additional restrictions:

		* `str` and `unicode` values will *not* be converted to `floats` and such − although python will accept `float('1.23')`

	:param name: name of the attribute, given as `str`
	:param ini: initial (default) value of the attribute
	:param str doc: documentation for this attribute, as it appears in generated docs and tooltips in the UI
	:param str unit: unit given as string, which is looked up in :obj:`woo.unit`; the multiplier is the ratio between *unit* and associated basic unit, which is found automatically.
		
		.. warning:: `unit` only determines multiplier for the GUI representation; it has no impact on the internal value used; in particular, the *ini* value is *unit-less*. If you want to give units to the initial value, say something like ``PyAttrTrait(float,'angle',60*woo.unit['deg'],units='deg','This is some angle')``.

	:param bool noGui: do not show this attribute in the GUI; use this for attributes which the GUI does not know how to represent (such as python objects, numpy arrays and such), to avoid warnings.
	:param bool noDump: skip this attribute when dumping/loading this object; that means that after loading (or after a :obj:`PyWooObjects.deepcopy`), the attribute will be default-initialized.
	:param bool rgbColor: this attribute is color in the RGB space (its type must be `Vector3`); the GUI will show color picker.
	:param str startGroup: start new attribute group, which is represented as collapsible blocks in the GUI.
	:param str hideIf: python expression which determines whether the GUI hide/show this attribute entry dynamically; use `self` to refer to the instance, as usual.
	:param range: give range (`Vector2` or `Vector2i`) to this numerical (`float` or `int`) attribute − a slider will be shown in the GUI.
	:param choice: this attribute chooses from predefined set of integer values; `choice` itself can be

		* list of unnamed values, e.g. `[0,1,2,3,4]` to choose from;
		* list of named values, e.g. `[(0,'choice0'),(1,'choice1'),(2,'choice2')]`, where the name will be displayed in the GUI, and the number will be assigned when the choice is made.

	:param bits: give names for bit values which this (integer) attribute which represents; they will be shown as array of checkboxes in the GUI.
	:param buttons: Tuple of *list* and *bool*; in the flat list of strings, where each consecutive triplet contains

		1. button label
		2. python expression to evaluate when the button is clicked
		3. label to be shown as button description in the GUI

		The bool at the end determined whether the button is created above (*True*) or below (*False*) the current attribute.
	
	:param filename: `str` attribute representing filename with file picker in the GUI; the file is possibly non-existent.
	:param existingFilename: `str` attribute for existing filename, with file picker in the GUI.
	:param dirname: `str` attribute for existing directory name, with directory picker in the GUI.
	:param triggerPostLoad: when this attribute is being assigned to, `postLoad(id(self.attr))` will be called.

	'''
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
			filename=False,
			existingFilename=False,
			dirname=False,
			psd=False,
			triggerPostLoad=False,
			guiReadonly=False,
			noGuiResize=False,
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
		self.filename=filename
		self.existingFilename=existingFilename
		self.dirname=dirname
		self.triggerPostLoad=triggerPostLoad
		self.noGuiResize=noGuiResize
		self.readonly=guiReadonly # this has different meaning in c++ and python, so call it differently
		# those are unsupported in python
		self.noSave=self.hidden=self.pyByRef=self.static=self.activeLabel=self.namedEnum=False
		#
		self.validator=None
		if self.choice and type(self.choice[0])==str:
			#print '%s: Choice of strings (%s)!!!!'%(self.name,str(self.choice))
			# choice from strings
			def validateStrChoice(self,val):
				if val not in self.choice: raise ValueError("%s: '%s' is not an admissible value (must be one of: %s)"%(self.name,str(val),', '.join(["'%s'"%str(c) for c in self.choice])))
			self.validator=validateStrChoice
		# PSD buttons
		if psd:
			if self.buttons: raise ValueError("%s: psd and buttons are mutually exclusive (psd created a button for displaying the PSD)"%self.name)
			self.buttons=(['Plot the PSD','import pylab; pylab.plot(*zip(*self.%s)); pylab.grid(True); pylab.show();'%(self.name),''],0)
		# 
		# units
		#
		import woo._units
		baseUnit=woo._units.baseUnit
		def _unicodeUnit(u):
			if isinstance(u,unicode): return u
			elif isinstance(u,str): return unicode(u,'utf-8')
			elif isinstance(u,tuple): return (_unicodeUnit(u[0]),u[1])
			raise ValueError(u"Unknown unit type %sfor %s"%(str(type(u)),u))
		if isinstance(unit,str) or isinstance(unit,unicode):
			unit=[_unicodeUnit(unit)]
			if altUnits: altUnits=[[_unicodeUnit(a) for a in altUnits]]
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
				if u not in baseUnit: raise ValueError(u'Unknown unit %s; admissible values are: '%u+', '.join(baseUnit.keys()))
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
	def validate(self,val):
		'Called when the attribute is set'
		if self.validator: self.validator(self,val)
	def coerceValue(self,val):
		'Check whether *val* has type compatible with declared type (pyType). Raise exception if not. Values converted to required types are returned (it is safe to ignore the return value). In addition, validate the (converted) value, if a validator is defined'
		def tName(T): return (T.__module__+'.' if T.__module__!='__builtin__' else '')+T.__name__
		# sequences
		if isinstance(self.pyType,list):
			assert len(self.pyType)==1
			ret=[]
			if not hasattr(val,'__len__'):
				raise TypeError("Attribute {self.name} declared as sequence of {T}{cxxType}, but its value {val!s} of type {valType} is not a sequence (__len__ not defined).".format(self=self,T=tName(self.pyType[0]),val=val,valType=tName(type(val)),cxxType=((' ('+self.cxxT+' in c++') if hasattr(self,'cxxT') else '')))
			T=self.pyType[0]
			if T in self.primitiveTypes: # check convertibility
				for i,v in enumerate(val):
					try:
						if type(v) in (str,unicode) and T in (float,int): raise TypeError("Don't allow conversions from strings to numbers, since that will fail if used without conversion")
						ret.append(T(v))
					except: raise TypeError("Attribute {self.name} declared as sequence of {T}, but {i}'th item {v!s} of type {itemType} is not convertible to {T}.".format(self=self,i=i,v=v,itemType=tName(type(v)),T=tName(T)))
			else:
				for i,v in enumerate(val):
					ret.append(v)
					if v==None: continue # python representation for NULL shared_ptr
					if not isinstance(v,T): raise TypeError("Attribute {self.name} declared as a sequence of {T}, but {i}'th item {v!s} of type {itemType} is not a {T}.".format(self=self,i=i,v=v,itemType=tName(type(v)),T=tName(T)))
		else:
			# do the same as for sequence items; ugly code duplication
			T=self.pyType
			if T in self.primitiveTypes:
				try:
					if type(val) in (str,unicode) and T in (float,int): raise TypeError("Don't allow conversions from strings to numbers, since that will fail if used without conversion")
					ret=T(val)
				except: raise TypeError("Attribute {self.name} declared as {T}, but value {val!s} of type {valType} is not convertible to {T}".format(self=self,val=val,valType=tName(type(val)),T=tName(T)))
			else:
				# objects
				ret=val
				if val!=None and not isinstance(val,T): raise TypeError("Attribute {self.name} declared as {T}, but value {val!s} of type {valType} is not a {T}".format(self=self,val=val,valType=tName(type(val)),T=tName(T)))
		self.validate(ret)
		return ret
	def __str__(self): return '<PyAttrTrait '+self.name+' @ '+str(id(self))+'>'
	def __repr__(self): return self.__str__()

class PyWooObject:
	'''
	Define some c++-compatibility functions for python classes. Derived class is created as::
		
		class SomeClass(woo.core.Object,woo.pyderived.PyWooObject): # order of base classes important!
			_attrTraits=[
				# see below
			]
			def __init__(self,**kw):
				woo.core.Object.__init__(self)
				self.wooPyInit(self.__class__,woo.core.Object,**kw)		

	This new class automatically obtains several features:

	* dumping/loading via :obj:`woo.core.Object.dump` etc works.
	* the GUI (:obj:`woo.qt.ObjectEditor`) will know how to present this class.
	* documentation for this class will be generated
	* all attributes are type-checked when assigned
	* support for postLoad hooks (see below)

	The `_attrTraits` ist a list of :obj:`PyAttrTrait`; each attribute
	is defined via its traits, which declare its type, default value, documentation and so on -- this
	is documented with :obj:`PyAttrTrait`.

	This example shows trait definitions, and also the `triggerPostLoad` flag::

		class SomeClass(woo.core.Object,woo.pyderived.PyWooObject):
			_PAT=woo.pyderived.PyAttrTrait # alias for class name, to save typing
			_attrTraits=[
				_PAT(float,'aF',1.,'float attr'),
				_PAT([float,],'aFF',[0.,1.,2.],'list of floats attr'),
				_PAT(Vector2,'aV2',(0.,1.),'vector2 attr'),
				_PAT([Vector2,],'aVV2',[(0.,0.),(1.,1.)],'list of vector2 attr'),
				_PAT(woo.core.Node,'aNode',woo.core.Node(pos=(1,1,1)),'node attr'),
				_PAT([woo.core.Node,],'aNNode',[woo.core.Node(pos=(1,1,1)),woo.core.Node(pos=(2,2,2))],'List of nodes'),
				_PAT(float,'aF_trigger',1.,triggerPostLoad=True,doc='Float triggering postLoad'),
			]
			def postLoad(self,I):
				if I==None: pass                  # called when constructed/loaded
				elif I==id(self.aF_trigger): pass # called when aF_trigger is modified
			def __init__(self,**kw):
				pass
				# ...

	The `postLoad` function is called with
	
	* `None` when the instance has just been created (or loaded); it *shoud* be idempotent, i.e. calling `postLoad(None)` the second time should have no effect::

		SomeClass()         # default-constructed; will call postLoad(None)
		SomeClass(aF=3.)    # default-construct, assign, call postLoad(None)

	* `id(self.attr)` when `self.attr` is modified; this can be used to check for some particular conditions or modify other variables::

		instance=SomeClass()  # calls instance.postLoad(None)
		instance.aF_trigger=3 # calls instance.postLoad(id(instance.aF_trigger))
	
	  .. note:: Pay attention to not call `postLoad` in infinite regression. 
	
	'''
	def wooPyInit(self,derivedClass,cxxBaseClass,**kw):
		'''Inject methods into derivedClass, so that it behaves like woo.core.Object,
		for the purposes of the GUI and expression dumps'''
		cxxBaseClass.__init__(self) # repeat, just to make sure
		self.cxxBaseClass=cxxBaseClass
		self.derivedClass=derivedClass
		self._instanceTraits={}
		self._attrValues={}
		self._attrTraitsDict=dict([(trait.name,trait) for trait in derivedClass._attrTraits])
		for trait in derivedClass._attrTraits:
			# basic getter/setter
			getter=(lambda self,trait=trait: self._attrValues[trait.name])
			setter=(lambda self,val,trait=trait: self._attrValues.__setitem__(trait.name,trait.coerceValue(val)))
			if trait.triggerPostLoad:
				if not hasattr(derivedClass,'postLoad'): raise RuntimeError('%s.%s declared with triggerPostLoad, but %s.postLoad is not defined.'%(derivedClass.__name__,trait.name,derivedClass.__name__))
				def triggerSetter(self,val,trait=trait):
					self._attrValues[trait.name]=trait.coerceValue(val)
					self.postLoad(id(self._attrValues[trait.name]))
				setter=triggerSetter
			# chain validation and actual setting
			def validatingSetter(self,val,trait=trait,setter=setter):
				trait.validate(val)
				setter(self,val)
			setattr(derivedClass,trait.name,property(getter,validatingSetter,None,trait.doc))
			self._attrValues[trait.name]=trait.ini
		#print derivedClass,self._attrValues
		if kw:
			for k in kw:
				if not hasattr(self,k): raise AttributeError('No such attribute: %s'%k)
				if k in self._attrValues: self._attrValues[k]=self._attrTraitsDict[k].coerceValue(kw[k])
				else: setattr(self,k,kw[k])
		derivedClass.__str__=lambda o:'<%s @ %d (py)>'%(derivedClass.__name__,id(o))
		derivedClass.__repr__=derivedClass.__str__
		derivedClass._cxxAddr=property(lambda sefl: id(self))
		# pickle support
		def __getstate__(self):
			#print '__getstate__ in python'
			ret=self._attrValues.copy()
			ret.update(cxxBaseClass.__getstate__(self)) # get boost::python stuff as well
			return ret
		def __setstate__(self,st):
			#print '__setstate__ in python'
			# set managed attributes indirectly, to avoid side-effects
			for k in st.keys():
				if k in self._attrValues:
					self._attrValues[k]=self._attrTraitsDict[k].coerceValue(st.pop(k))
			# set remaining attributes using setattr
			for k,v in st.items(): setattr(self,k,v)
			# call postLoad as if after loading
			if hasattr(derivedClass,'postLoad'): self.postLoad(None)
		def deepcopy(self):
			'''The c++ dedepcopy uses boost::serialization, we need to use pickle. As long as deepcopy
			is called from python, this function gets precende over the c++ one.'''
			import pickle
			return pickle.loads(pickle.dumps(self))
		derivedClass.__getstate__=__getstate__
		derivedClass.__setstate__=__setstate__
		derivedClass.deepcopy=deepcopy
		if hasattr(derivedClass,'postLoad'):
			self.postLoad(None)
		


if __name__=='wooMain':
	# do not define this class when running woo normally,
	# so that it does not show up in the preprocessor dialogue
	
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
			PyAttrTrait(str,'outDir','/tmp',dirname=True,doc='output directory'),
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

	import woo.pre
	woo.pre.SamplePyDerivedPreprocessor=SamplePyDerivedPreprocessor

	t=PyAttrTrait(str,'sChoice','aa',choice=['aa','bb','cc'],doc='string choice with validation')
	t.validate('abc')



