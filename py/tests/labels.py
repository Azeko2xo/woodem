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
		self.S=woo.core.Scene()
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
	def testAutoLabelOld(self):
		'LabelMapper: labeled engines are added automatically to woo.*; this will disappear in the future.'
		import woo
		ee=woo.core.PyRunner(label='abc')
		self.S.engines=[ee]
		self.assert_(hasattr(woo,'abc'))
		self.assert_(woo.abc==ee)

