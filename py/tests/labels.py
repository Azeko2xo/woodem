# encoding: utf-8
# 2013 © Václav Šmilauer <eu@doxos.eu>

import unittest
from minieigen import *
import woo._customConverters
import woo.core
import woo.dem

class TestSceneLabels(unittest.TestCase):
	'Test :obj:`LabelMapper` and related functionality.'
	def setUp(self):
		self.S=woo.core.Scene(fields=[woo.dem.DemField()])
	def testAccess(self):
		'LabelMapper: access'
		self.S.labels['abc']=123
		self.assert_(self.S.lab.abc==123)
		self.assert_(self.S.labels._whereIs('abc')==woo.core.LabelMapper.inPy)
		self.S.lab.ghi=456
		self.assert_(self.S.labels['ghi']==456)
	def testDel(self):
		'LabelMapper: delete'
		self.S.labels['foo']=123
		self.assert_(self.S.lab.foo==123)
		del self.S.lab.foo
		self.assert_(self.S.labels._whereIs('foo')==woo.core.LabelMapper.nowhere)
	def testSeq(self):
		'LabelMapper: sequences'
		o1,o2,o3=woo.core.Object(),woo.core.Object(),woo.core.Object()
		# from list
		self.S.lab.objs=[None,o1,o2,o3]
		self.assert_(self.S.lab.objs[1]==o1)
		self.assert_(self.S.labels._whereIs('objs')==woo.core.LabelMapper.inWooSeq)
		# from tuple
		self.S.lab.objs2=(None,o1,o2,o3)
		self.assert_(self.S.lab.objs2[1]==o1)
		self.assert_(self.S.labels._whereIs('objs2')==woo.core.LabelMapper.inWooSeq)
		# from indexed label
		self.S.labels['objs3[2]']=o2
		self.assert_(self.S.lab.objs3[0]==None)
		self.assert_(self.S.lab.objs3[2]==o2)
		self.assert_(len(self.S.lab.objs3)==3)
	def testMixedSeq(self):
		'LabelMapper: mixed/empty sequences rejected'
		# mixed sequences
		o1,o2=woo.core.Object(),woo.core.Object()
		self.assertRaises(ValueError,lambda: setattr(self.S.lab,'ll',[o1,o2,12]))
		self.assertRaises(ValueError,lambda: setattr(self.S.lab,'ll',(12,23,o1)))
		# undetermined sequences
		self.assertRaises(ValueError,lambda: setattr(self.S.lab,'ll',[]))
		self.assertRaises(ValueError,lambda: setattr(self.S.lab,'ll',()))
		self.assertRaises(ValueError,lambda: setattr(self.S.lab,'ll',[None,None]))
		# this is legitimate
		try: self.S.lab.mm=[o1,None]
		except: self.fail("[woo.Object,None] rejected by LabelMapper as mixed.")
		self.assert_(self.S.labels._whereIs('mm')==woo.core.LabelMapper.inWooSeq)
		# this as well
		try: self.S.lab.nn=[231,None]
		except: self.fail("[python-object,None] rejected by LabelMapper as mixed.")
		self.assert_(self.S.labels._whereIs('nn')==woo.core.LabelMapper.inPy)
	def testShared(self):
		'LabelMapper: shared objects'
		o1=woo.core.Object()
		self.S.engines=[woo.core.PyRunner()]
		self.S.lab.someEngine=self.S.engines[0]
		self.assert_(self.S.labels._whereIs('someEngine')==woo.core.LabelMapper.inWoo)
		S2=self.S.deepcopy()
		self.assert_(S2.lab.someEngine==S2.engines[0])
	def testAutoLabel(self):
		'LabelMapper: labeled engines are added automatically'
		ee=woo.core.PyRunner(label='abc')
		self.S.engines=[ee]
		self.assert_(self.S.lab.abc==ee)
		self.assert_(self.S.labels._whereIs('abc')==woo.core.LabelMapper.inWoo)
	def testPseudoModules(self):
		'LabelMapper: pseudo-modules'
		S=self.S
		# using name which does not exist yet
		self.assertRaises(NameError,lambda: S.lab.abc)
		# using name which does not exist yet as pseudo-module
		self.assertRaises(NameError,lambda: setattr(S.lab,'abc.defg',1))
		S.lab._newModule('abc')
		self.assert_(S.lab._whereIs('abc')==woo.core.LabelMapper.inMod)
		S.lab.abc.a1=1
		# fail using method on proxyed pseudo-module
		self.assertRaises(AttributeError, lambda: S.lab.abc._newModule('a1')) 
		#self.assertRaises(ValueError, lambda: S.lab._newModule('abc.a1'))
		# fail when recreating existing module
		self.assertRaises(ValueError, lambda: S.lab._newModule('abc'))
		# nested
		S.lab._newModule('foo.bar')
		print 'where is foo.bar?:',S.lab._whereIs('foo.bar')
		self.assert_(S.lab._whereIs('foo')==woo.core.LabelMapper.inMod)
		#self.assert_(S.lab._whereIs('foo.bar')==woo.core.LabelMapper.inMod)
		S.lab.foo.bar.bb=1
		print 'KEYS:',S.labels.keys()
		self.assert_(S.lab.foo.bar.bb==1)
	def testWritable(self):
		self.S.lab.if_overwriting_this_causes_warning_it_is_a_bug=3
		self.S.lab._setWritable('if_overwriting_this_causes_warning_it_is_a_bug')
		# should not emit warning
		self.S.lab.if_overwriting_this_causes_warning_it_is_a_bug=4

		

	#def testAutoLabelOld(self):
	#	'LabelMapper: labeled engines are added automatically to woo.*; this will disappear in the future.'
	#	import woo
	#	ee=woo.core.PyRunner(label='abc')
	#	self.S.engines=[ee]
	#	self.assert_(hasattr(woo,'abc'))
	#	self.assert_(woo.abc==ee)

	def testEngineLabels(self):
		'LabelMapper: engine/functor labels (mix of older tests)'
		S=self.S
		self.assertRaises(NameError,lambda: setattr(S,'engines',[woo.core.PyRunner(label='this is not a valid identifier name')]))
		#self.assertRaises(NameError,lambda: setattr(S,'engines',[PyRunner(label='foo'),PyRunner(label='foo[1]')]))
		cloop=woo.dem.ContactLoop([woo.dem.Cg2_Facet_Sphere_L6Geom(label='cg2fs'),woo.dem.Cg2_Sphere_Sphere_L6Geom(label='cg2ss')],[woo.dem.Cp2_FrictMat_FrictPhys(label='cp2ff')],[woo.dem.Law2_L6Geom_FrictPhys_IdealElPl(label='law2elpl')],)
		S.engines=[woo.core.PyRunner(label='foo'),woo.core.PyRunner(label='bar[2]'),woo.core.PyRunner(label='bar [0]'),cloop]
		# print S.lab.bar,type(S.lab.bar)
		self.assert_(hasattr(type(S.lab.bar),'__len__'))
		self.assert_(S.lab.foo==S.engines[0])
		self.assert_(S.lab.bar[0]==S.engines[2])
		self.assert_(S.lab.bar[1]==None)
		self.assert_(S.lab.bar[2]==S.engines[1])
		self.assert_(type(S.lab.cg2fs)==woo.dem.Cg2_Facet_Sphere_L6Geom)

