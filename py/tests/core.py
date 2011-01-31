# encoding: utf-8
# 2009 © Václav Šmilauer <eudoxos@arcig.cz>

"""
Core functionality (Scene in c++), such as accessing bodies, materials, interactions. Specific functionality tests should go to engines.py or elsewhere, not here.
"""
import unittest
import random
from yade.wrapper import *
from miniEigen import *
from yade._customConverters import *
from yade import utils
from yade import *
from math import *

## TODO tests
class TestInteractions(unittest.TestCase): pass
class TestForce(unittest.TestCase): pass
class TestTags(unittest.TestCase): pass 

class TestLoop(unittest.TestCase):
	def setUp(self): O.reset()
	def testSubstepping(self):
		'Loop: substepping'
		O.engines=[ForceResetter(),PyRunner(initRun=True,iterPeriod=1,command='pass'),GravityEngine()]
		# value outside the loop
		self.assert_(O.subStep==-1)
		# O.subStep is meaningful when substepping
		O.subStepping=True
		O.step(); self.assert_(O.subStep==0)
		O.step(); self.assert_(O.subStep==1)
		# when substepping is turned off in the middle of the loop, the next step finishes the loop
		O.subStepping=False
		O.step(); self.assert_(O.subStep==-1)
		# subStep==0 inside the loop without substepping
		O.engines=[PyRunner(initRun=True,iterPeriod=1,command='if O.subStep!=0: raise RuntimeError("O.subStep!=0 inside the loop with O.subStepping==False!")')]
		O.step()
	def testEnginesModificationInsideLoop(self):
		'Loop: O.engines can be modified inside the loop transparently.'
		O.engines=[
			PyRunner(initRun=True,iterPeriod=1,command='from yade import *; O.engines=[ForceResetter(),GravityEngine(),NewtonIntegrator()]'), # change engines here
			ForceResetter() # useless engine
		]
		O.subStepping=True
		# run prologue and the first engine, which modifies O.engines
		O.step(); O.step(); self.assert_(O.subStep==1)
		self.assert_(len(O.engines)==3) # gives modified engine sequence transparently
		self.assert_(len(O._nextEngines)==3)
		self.assert_(len(O._currEngines)==2)
		O.step(); O.step(); # run the 2nd ForceResetter, and epilogue
		self.assert_(O.subStep==-1)
		# start the next step, nextEngines should replace engines automatically
		O.step()
		self.assert_(O.subStep==0)
		self.assert_(len(O._nextEngines)==0)
		self.assert_(len(O.engines)==3)
		self.assert_(len(O._currEngines)==3)
	def testDead(self):
		'Loop: dead engines are not run'
		O.engines=[PyRunner(dead=True,initRun=True,iterPeriod=1,command='pass')]
		O.step(); self.assert_(O.engines[0].nDone==0)
			


class TestIO(unittest.TestCase):
	def testSaveAllClasses(self):
		'I/O: All classes can be saved and loaded with boost::serialization'
		import yade.system
		failed=set()
		for c in yade.system.childClasses('Serializable'):
			O.reset()
			try:
				O.miscParams=[eval(c)()]
				O.saveTmp(quiet=True)
				O.loadTmp(quiet=True)
			except (RuntimeError,ValueError):
				failed.add(c)
		failed=list(failed); failed.sort()
		self.assert_(len(failed)==0,'Failed classes were: '+' '.join(failed))

class TestMaterialStateAssociativity(unittest.TestCase):
	def setUp(self): O.reset()
	def testThrowsAtBadCombination(self):
		"Material+State: throws when body has material and state that don't work together."
		b=Body()
		b.mat=CpmMat()
		b.state=State() #should be CpmState()
		O.bodies.append(b)
		self.assertRaises(RuntimeError,lambda: O.step()) # throws runtime_error
	def testThrowsAtNullState(self):
		"Material+State: throws when body has material but NULL state."
		b=Body()
		b.mat=Material()
		b.state=None # → shared_ptr<State>() by boost::python
		O.bodies.append(b)
		self.assertRaises(RuntimeError,lambda: O.step())
	def testMaterialReturnsState(self):
		"Material+State: CpmMat returns CpmState when asked for newAssocState"
		self.assert_(CpmMat().newAssocState().__class__==CpmState)

class TestBodies(unittest.TestCase):
	def setUp(self):
		O.reset()
		self.count=100
		O.bodies.append([utils.sphere([random.random(),random.random(),random.random()],random.random()) for i in range(0,self.count)])
		random.seed()
	def testIterate(self):
		"Bodies: Iteration"
		counted=0
		for b in O.bodies: counted+=1
		self.assert_(counted==self.count)
	def testLen(self):
		"Bodies: len(O.bodies)"
		self.assert_(len(O.bodies)==self.count)
	def testErase(self):
		"Bodies: erased bodies are None in python"
		O.bodies.erase(0)
		self.assert_(O.bodies[0]==None)
	def testNegativeIndex(self):
		"Bodies: Negative index counts backwards (like python sequences)."
		self.assert_(O.bodies[-1]==O.bodies[self.count-1])
	def testErasedIterate(self):
		"Bodies: Iterator silently skips erased ones"
		removed,counted=0,0
		for i in range(0,10):
			id=random.randint(0,self.count-1)
			if O.bodies[id]: O.bodies.erase(id);removed+=1
		for b in O.bodies: counted+=1
		self.assert_(counted==self.count-removed)
		
class TestMaterials(unittest.TestCase):
	def setUp(self):
		# common setup for all tests in this class
		O.reset()
		O.materials.append([
			FrictMat(young=1,label='materialZero'),
			ElastMat(young=100,label='materialOne')
		])
		O.bodies.append([
			utils.sphere([0,0,0],.5,material=0),
			utils.sphere([1,1,1],.5,material=0),
			utils.sphere([1,1,1],.5,material=1)
		])
	def testShared(self):
		"Material: shared_ptr's makes change in material immediate everywhere"
		O.bodies[0].mat.young=23423333
		self.assert_(O.bodies[0].mat.young==O.bodies[1].mat.young)
	def testSharedAfterReload(self):
		"Material: shared_ptr's are preserved when saving/loading"
		O.saveTmp(quiet=True); O.loadTmp(quiet=True)
		O.bodies[0].mat.young=9087438484
		self.assert_(O.bodies[0].mat.young==O.bodies[1].mat.young)
	def testLen(self):
		"Material: len(O.materials)"
		self.assert_(len(O.materials)==2)
	def testNegativeIndex(self):
		"Material: negative index counts backwards."
		self.assert_(O.materials[-1]==O.materials[1])
	def testIterate(self):
		"Material: iteration over O.materials"
		counted=0
		for m in O.materials: counted+=1
		self.assert_(counted==len(O.materials))
	def testAccess(self):
		"Material: find by index or label; KeyError raised for invalid label."
		self.assertRaises(KeyError,lambda: O.materials['nonexistent label'])
		self.assert_(O.materials['materialZero']==O.materials[0])

