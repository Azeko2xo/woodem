# encoding: utf-8
# 2009 © Václav Šmilauer <eudoxos@arcig.cz>

"""
Core functionality (Scene in c++), such as accessing bodies, materials, interactions. Specific functionality tests should go to engines.py or elsewhere, not here.
"""
import unittest
import random
from minieigen import *
from math import *
from woo._customConverters import *
from woo import utils
from woo import *
from woo.core import *
from woo.dem import *
from woo.pre import *
try: from woo.sparc import *
except: pass
try: from woo.gl import *
except: pass
try: from woo.voro import *
except: pass
try: from woo.cld import *
except: pass
try: from woo.qt import *
except: pass

import woo

## TODO tests
class TestInteractions(unittest.TestCase): pass
class TestForce(unittest.TestCase): pass
class TestTags(unittest.TestCase): pass 



class TestScene(unittest.TestCase):
	def setUp(self):
		self.scene=woo.core.Scene()
	#def _testTags(self):
	#	'Core: Scene.tags are str (not unicode) objects'
	#	S=self.scene
	#	S.tags['str']='asdfasd'
	#	S.tags['uni']=u'→ Σ'
	#	self.assert_(type(S.tags['str']==unicode))
	#	def tagError(S): S.tags['error']=234
	#	self.assert_(type(S.tags['uni']==unicode))
	#	self.assertRaises(TypeError,lambda: tagError(S))

class TestObjectInstantiation(unittest.TestCase):
	def setUp(self):
		self.t=woo.core.WooTestClass()
		pass # no setup needed for tests here
	def testClassCtors(self):
		"Core: correct types are instantiated"
		# correct instances created with Foo() syntax
		import woo.system
		for r in woo.system.childClasses(woo.core.Object):
			obj=r();
			self.assert_(obj.__class__==r,'Failed for '+r.__name__)
	def testRootDerivedCtors_attrs_few(self):
		"Core: class ctor's attributes"
		# attributes passed when using the Foo(attr1=value1,attr2=value2) syntax
		gm=Shape(color=1.); self.assert_(gm.color==1.)
	def testDispatcherCtor(self):
		"Core: dispatcher ctors with functors"
		# dispatchers take list of their functors in the ctor
		# same functors are collapsed in one
		cld1=LawDispatcher([Law2_L6Geom_FrictPhys_IdealElPl(),Law2_L6Geom_FrictPhys_IdealElPl()]); self.assert_(len(cld1.functors)==1)
		# two different make two different
		cld2=LawDispatcher([Law2_L6Geom_FrictPhys_IdealElPl(),Law2_L6Geom_FrictPhys_LinEl6()]); self.assert_(len(cld2.functors)==2)
	def testContactLoopCtor(self):
		"Core: ContactLoop special ctor"
		# ContactLoop takes 3 lists
		id=ContactLoop([Cg2_Facet_Sphere_L6Geom(),Cg2_Sphere_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()],)
		self.assert_(len(id.geoDisp.functors)==2)
		self.assert_(id.geoDisp.__class__==CGeomDispatcher)
		self.assert_(id.phyDisp.functors[0].__class__==Cp2_FrictMat_FrictPhys)
		self.assert_(id.lawDisp.functors[0].__class__==Law2_L6Geom_FrictPhys_IdealElPl)
	def testParallelEngineCtor(self):
		"Core: ParallelEngine special ctor"
		pe=ParallelEngine([InsertionSortCollider(),[BoundDispatcher(),ForceResetter()]])
		self.assert_(pe.slaves[0].__class__==InsertionSortCollider)
		self.assert_(len(pe.slaves[1])==2)
		pe.slaves=[]
		self.assert_(len(pe.slaves)==0)
	##		
	## testing incorrect operations that should raise exceptions
	##
	def testWrongFunctorType(self):
		"Core: dispatcher and functor type mismatch is detected"
		# dispatchers accept only correct functors
		self.assertRaises(TypeError,lambda: LawDispatcher([Bo1_Sphere_Aabb()]))
	def testInvalidAttr(self):
		'Core: invalid attribute access raises AttributeError'
		# accessing invalid attributes raises AttributeError
		self.assertRaises(AttributeError,lambda: Sphere(attributeThatDoesntExist=42))
		self.assertRaises(AttributeError,lambda: Sphere().attributeThatDoesntExist)
	###
	### shared ownership semantics
	###
	def testShared(self):
		"Core: shared_ptr really shared"
		m=woo.utils.defaultMaterial()
		s1,s2=woo.utils.sphere((0,0,0),1,mat=m),woo.utils.sphere((0,0,0),2,mat=m)
		s1.mat.young=2342333
		self.assert_(s1.mat.young==s2.mat.young)
	def testSharedAfterReload(self):
		"Core: shared_ptr preserved when saving/loading"
		S=Scene(fields=[DemField()])
		m=woo.utils.defaultMaterial()
		S.dem.par.append([woo.utils.sphere((0,0,0),1,mat=m),woo.utils.sphere((0,0,0),2,mat=m)])
		S.saveTmp(quiet=True); S=Scene.loadTmp()
		S.dem.par[0].mat.young=9087438484
		self.assert_(S.dem.par[0].mat.young==S.dem.par[1].mat.young)
	##
	## attribute flags
	##
	def testHidden0(self):
		'Core: Attr::hidden [0]'
		# hidden attributes are not wrapped in python at all
		self.assert_(not hasattr(Contact(),'stepLastSeen'))
	def testNoSave(self):
		'Core: Attr::noSave [0]'
		# update bound of the particle
		S=Scene(fields=[DemField()])
		S.dem.par.append(utils.sphere((0,0,0),1))
		S.dem.collectNodes()
		S.engines=[InsertionSortCollider([Bo1_Sphere_Aabb()]),Leapfrog(reset=True)]
		S.one()
		S.saveTmp(quiet=True)
		mn0=Vector3(S.dem.par[0].shape.bound.min)
		S=S.loadTmp()
		mn1=Vector3(S.dem.par[0].shape.bound.min)
		# check that the minimum is not saved
		self.assert_(not isnan(mn0[0]))
		self.assert_(isnan(mn1[0]))
	def testReadonly0(self):
		'Core: Attr::readonly [0]'
		self.assertRaises(AttributeError,lambda: setattr(Particle(),'id',3))
	def testTriggerPostLoad0(self):
		'Core: Attr::triggerPostLoad [0]'
		# RadialEngine normalizes axisDir automatically
		# anything else could be tested
		te=RadialForce();
		te.axisDir=(0,2,0)
		self.assert_(te.axisDir==(0,1,0))
	def testReadonly(self):
		'Core: Attr::readonly'
		self.assert_(self.t.meaning42==42)
		self.assertRaises(AttributeError,lambda: setattr(self.t,'meaning42',43))
	def testTriggerPostLoad(self):
		'Core: postLoad & Attr::triggerPostLoad'
		WTC=woo.core.WooTestClass
		# stage right after construction
		self.assert_(self.t.postLoadStage==WTC.postLoad_ctor)
		baz0=self.t.baz
		self.t.foo_incBaz=1 # assign whatever, baz should be incremented
		self.assert_(self.t.baz==baz0+1)
		self.assert_(self.t.postLoadStage==WTC.postLoad_foo)
		self.t.foo_incBaz=1 # again
		self.assert_(self.t.baz==baz0+2)
		self.t.bar_zeroBaz=1 # assign to reset baz
		self.assert_(self.t.baz==0)
		self.assert_(self.t.postLoadStage==WTC.postLoad_bar)
		# assign to baz to test postLoad again
		self.t.baz=-1
		self.assert_(self.t.postLoadStage==WTC.postLoad_baz)
	def testHidden(self):
		'Core: Attr::hidden'
		# hidden attributes are not wrapped in python at all
		self.assert_(not hasattr(self.t,'hiddenAttribute'))




class TestLoop(unittest.TestCase):
	def setUp(self):
		woo.master.reset()
		woo.master.scene.fields=[DemField()]
	def testSubstepping(self):
		'Loop: substepping'
		S=woo.master.scene
		S.engines=[PyRunner(1,'pass'),PyRunner(1,'pass'),PyRunner(1,'pass')]
		# value outside the loop
		self.assert_(S.subStep==-1)
		# O.subStep is meaningful when substepping
		S.subStepping=True
		S.one(); self.assert_(S.subStep==0)
		S.one(); self.assert_(S.subStep==1)
		# when substepping is turned off in the middle of the loop, the next step finishes the loop
		S.subStepping=False
		S.one(); self.assert_(S.subStep==-1)
		# subStep==0 inside the loop without substepping
		S.engines=[PyRunner(1,'if scene.subStep!=0: raise RuntimeError("scene.subStep!=0 inside the loop with Scene.subStepping==False!")')]
		S.one()
	def testEnginesModificationInsideLoop(self):
		'Loop: Scene.engines can be modified inside the loop transparently.'
		S=woo.master.scene
		S.engines=[
			PyRunner(stepPeriod=1,command='from woo import*; from woo.dem import *; scene.engines=[ForceResetter(),ForceResetter(),Leapfrog(reset=False)]'), # change engines here
			ForceResetter() # useless engine
		]
		S.subStepping=True
		# run prologue and the first engine, which modifies O.engines
		S.one(); S.one(); self.assert_(S.subStep==1)
		self.assert_(len(S.engines)==3) # gives modified engine sequence transparently
		self.assert_(len(S._nextEngines)==3)
		self.assert_(len(S._currEngines)==2)
		S.one(); S.one(); # run the 2nd ForceResetter, and epilogue
		self.assert_(S.subStep==-1)
		# start the next step, nextEngines should replace engines automatically
		S.one()
		self.assert_(S.subStep==0)
		self.assert_(len(S._nextEngines)==0)
		self.assert_(len(S.engines)==3)
		self.assert_(len(S._currEngines)==3)
	def testDead(self):
		'Loop: dead engines are not run'
		S=woo.master.scene
		S.engines=[PyRunner(1,'pass',dead=True)]
		S.one(); self.assert_(S.engines[0].nDone==0)
	def testLabels(self):
		'Loop: engine/functor labels (plain and array)'
		S=woo.master.scene
		self.assertRaises(NameError,lambda: setattr(S,'engines',[PyRunner(label='this is not a valid identifier name')]))
		self.assertRaises(NameError,lambda: setattr(S,'engines',[PyRunner(label='foo'),PyRunner(label='foo[1]')]))
		cloop=ContactLoop([Cg2_Facet_Sphere_L6Geom(label='cg2fs'),Cg2_Sphere_Sphere_L6Geom(label='cg2ss')],[Cp2_FrictMat_FrictPhys(label='cp2ff')],[Law2_L6Geom_FrictPhys_IdealElPl(label='law2elpl')],)
		S.engines=[PyRunner(label='foo'),PyRunner(label='bar[2]'),PyRunner(label='bar [0]'),cloop]
		self.assert_(type(woo.bar)==list)
		self.assert_(woo.foo==S.engines[0])
		self.assert_(woo.bar[0]==S.engines[2])
		self.assert_(woo.bar[1]==None)
		self.assert_(woo.bar[2]==S.engines[1])
		self.assert_(type(woo.cg2fs)==Cg2_Facet_Sphere_L6Geom)
	def testPausedContext(self):
		'Loop: "with Scene.paused()" context manager'
		import time
		S=woo.master.scene
		S.engines=[]
		S.run()
		with S.paused():
			i=S.step
			time.sleep(.1)
			self.assert_(i==S.step) # check there was no advance during those .1 secs
			self.assert_(S.running) # running should return true nevertheless
		time.sleep(.1)
		self.assert_(i<S.step) # check we run during those .1 secs again
		S.stop()
	def testNoneEngine(self):
		'Loop: None engine raises exception.'
		S=woo.master.scene
		self.assertRaises(RuntimeError,lambda: setattr(S,'engines',[ContactLoop(),None,ContactLoop()]))

		
class TestIO(unittest.TestCase):
	def testSaveAllClasses(self):
		'I/O: All classes can be saved and loaded with boost::serialization'
		import woo.system
		failed=set()
		for c in woo.system.childClasses(woo.core.Object):
			woo.master.reset()
			S=woo.master.scene
			try:
				S.iscParams=[c()]
				S.saveTmp(quiet=True)
				S=Scene.loadTmp()
			except (RuntimeError,ValueError):
				failed.add(c.__name__)
		failed=list(failed); failed.sort()
		self.assert_(len(failed)==0,'Failed classes were: '+' '.join(failed))


class TestParticles(unittest.TestCase):
	def setUp(self):
		woo.master.reset()
		woo.master.scene.fields=[DemField()]
		S=woo.master.scene
		self.count=100
		S.dem.par.append([utils.sphere([random.random(),random.random(),random.random()],random.random()) for i in range(0,self.count)])
		random.seed()
	def testIterate(self):
		"Particles: Iteration"
		counted=0
		S=woo.master.scene
		for b in S.dem.par: counted+=1
		self.assert_(counted==self.count)
	def testLen(self):
		"Particles: len(S.dem.par)"
		S=woo.master.scene
		self.assert_(len(S.dem.par)==self.count)
	def testRemove(self):
		"Particles: acessing removed particles raises IndexError"
		S=woo.master.scene
		S.dem.par.remove(0)
		self.assertRaises(IndexError,lambda: S.dem.par[0])
	def testRemoveShrink(self):
		"Particles: removing particles shrinks storage size"
		S=woo.master.scene
		for i in range(self.count-1,-1,-1):
			S.dem.par.remove(i)
			self.assert_(len(S.dem.par)==i)
	def testNegativeIndex(self):
		"Particles: Negative index counts backwards (like python sequences)."
		S=woo.master.scene
		self.assert_(S.dem.par[-1]==S.dem.par[self.count-1])
	def testRemovedIterate(self):
		"Particles: Iterator silently skips erased ones"
		S=woo.master.scene
		removed,counted=0,0
		for i in range(0,10):
			id=random.randint(0,self.count-1)
			if S.dem.par.exists(id): S.dem.par.remove(id); removed+=1
		for b in S.dem.par: counted+=1
		self.assert_(counted==self.count-removed)


class TestArrayAccu(unittest.TestCase):
	def setUp(self):
		self.t=woo.core.WooTestClass()
		self.N=woo.master.numThreads
	def testResize(self):
		'OpenMP array accu: resizing'
		self.assert_(len(self.t.aaccuRaw)==0) # initial zero size
		for sz in (4,8,16,32,33):
			self.t.aaccuRaw=[i for i in range(0,sz)]
			r=self.t.aaccuRaw
			for i in range(0,sz):
				self.assert_(r[i][0]==i)  # first thread is assigned the value
				for n in range(1,self.N): self.assert_(r[i][n]==0) # other threads are reset
	def testPreserveResize(self):
		'OpenMP array accu: preserve old data on resize'
		self.t.aaccuRaw=(0,1)
		self.t.aaccuWriteThreads(2,[2]) # write whatever to index 2: resizes, but should preserve 0,1
		self.assert_(self.t.aaccuRaw[0][0]==0)
		self.assert_(self.t.aaccuRaw[1][0]==1)
	def testThreadWrite(self):
		'OpenMP array accu: concurrent writes'
		self.t.aaccuWriteThreads(0,[i for i in range(0,self.N)])
		for i in range(0,self.N):
			self.assert_(self.t.aaccuRaw[0][i]==i) # each thread has written its own index


class TestPyDerived(unittest.TestCase):
	import woo.pyderived, woo.core
	class _TestPyClass(woo.core.Object,woo.pyderived.PyWooObject):
		'Sample pure-python class integrated into woo (mainly used with preprocessors)'
		PAT=woo.pyderived.PyAttrTrait
		_attrTraits=[
			PAT(float,'aF',1.,'float attr'),
			PAT([float,],'aFF',[0.,1.,2.],'list of floats attr'),
			PAT(Vector2,'aV2',(0.,1.),'vector2 attr'),
			PAT([Vector2,],'aVV2',[(0.,0.),(1.,1.)],'list of vector2 attr'),
			PAT(woo.core.Node,'aNode',woo.core.Node(pos=(1,1,1)),'node attr'),
			PAT(woo.core.Node,'aNodeNone',None,'node attr, uninitialized'),
			PAT([woo.core.Node,],'aNNode',[woo.core.Node(pos=(1,1,1)),woo.core.Node(pos=(2,2,2))],'List of nodes'),
			PAT(float,'aF_trigger',1.,triggerPostLoad=True,doc='Float triggering postLoad, copying its value to aF'),
			PAT(int,'postLoadCounter',0,doc='counter for postLoad (readonly). Incremented by 1 after construction, incremented by 10 when assigning to aF_trigger.')
		]
		def postLoad(self,I):
			# print '_TestPyClass.postLoad(%s)'%I
			if I==None:
				self.postLoadCounter+=1
			elif I==id(self.aF_trigger):
				print 'postLoad / aF_trigger'
				self.postLoadCounter+=10
				self.aF=self.aF_trigger
			else: raise RuntimeError(self.__class__.__name__+'.postLoad called with unknown attribute id %s'%I)
		def __init__(self,**kw):
			woo.core.Object.__init__(self)
			self.wooPyInit(self.__class__,woo.core.Object,**kw)
	def setUp(self):
		self.t=self._TestPyClass()
	def testTrigger(self):
		'PyDerived: postLoad triggers'
		print 'postLoadCounter after ctor:',self.t.postLoadCounter
		self.assert_(self.t.postLoadCounter==1)
		self.t.aF_trigger=514.
		self.assert_(self.t.aF_trigger==514.)
		self.assert_(self.t.aF==514.)
		self.assert_(self.t.postLoadCounter==11)
	def testPickle(self):
		'PyDerived: deepcopy'
		self.assert_(self.t.aF==1.)
		self.assert_(self.t.aNode.pos==Vector3(1,1,1))
		self.t.aF=2.
		self.t.aNode.pos=Vector3(0,0,0)
		self.assert_(self.t.aF==2.)
		self.assert_(self.t.aNode.pos==Vector3(0,0,0))
		# pickle needs the class to be found in the module itself
		#    PicklingError: Can't pickle <class 'woo.tests.core._TestPyClass'>: it's not found as woo.tests.core._TestPyClass
		globals()['_TestPyClass']=self._TestPyClass
		t2=self.t.deepcopy()
		self.assert_(t2.aF==2.)
		self.assert_(t2.aNode.pos==Vector3(0,0,0))
	def testTypeCoerceFloats(self):
		'PyDerived: type coercion (primitive types)'
		# numbers and number sequences			
		self.assertRaises(TypeError,lambda:setattr(self.t,'aF','asd'))
		self.assertRaises(TypeError,lambda:setattr(self.t,'aF','123')) # disallow conversion from strings
		self.assertRaises(TypeError,lambda:setattr(self.t,'aFF',(1,2,'ab')))
		self.assertRaises(TypeError,lambda:setattr(self.t,'aFF','ab'))
		self.assertRaises(TypeError,lambda:setattr(self.t,'aFF',[(1,2,3),(4,5,6)]))
		try: self.t.aFF=[]
		except: self.fail("Empty list not accepter for list of floats")
		try: self.t.aFF=Vector3(1,2,3)
		except: self.fail("Vector3 not accepted for list of floats")
		try: self.t.aV2=(0,1.)
		except: self.fail("2-tuple not accepted for Vector2")
	def testTypeCoerceObject(self):
		'PyDerived: type coercion (woo.core.Object)'
		# c++ objects
		try: self.t.aNode=None
		except: self.fail("None not accepted as woo.core.Node")
		self.assertRaises(TypeError,lambda:setattr(self.t,'aNode',woo.core.Scene()))
		# list of c++ objects
		self.assertRaises(TypeError,lambda:setattr(self.t,'aNNode',(woo.core.Node(),woo.core.Scene())))
		try: self.t.aNNode=[None,woo.core.Node()]
		except: self.fail("[None,Node] not accepted for list of Nodes")
	def testTypeCoerceCtor(self):
		'PyDerived: type coercion (ctor)'
		self.assertRaises(TypeError,lambda:self._TestPyClass(aF='abc'))
	def testTraits(self):
		'PyDerived: PyAttrTraits'
		self.assert_(self.t._attrTraits[0].ini==1.)
		self.assert_(self.t._attrTraits[0].pyType==float)
	def testIniDefault(self):
		'PyDerived: default initialization'
		self.assert_(self.t.aF==1.)
		self.assert_(self.t.aFF==[0.,1.,2.])
		self.assert_(self.t.aNodeNone==None)
	def testIniUser(self):
		'PyDerived: user initialization'
		t2=self._TestPyClass(aF=2.)
		self.assert_(t2.aF==2.)
		self.assertRaises(AttributeError,lambda: self._TestPyClass(nonsense=123))
