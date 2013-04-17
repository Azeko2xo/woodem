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
		self.S.lab.ghi=456
		self.assert_(self.S.labels['ghi']==456)
	def testList(self):
		'LabelMapper: sequences'
		o1,o2,o3=woo.core.Object(),woo.core.Object(),woo.core.Object()
		self.S.lab.objs=[None,o1,o2,o3]
		self.assert_(self.S.lab.objs[1]==o1)
		self.S.labels['objs2[2]']=o2
		self.assert_(self.S.lab.objs2[0]==None)
		self.assert_(self.S.lab.objs2[2]==o2)
		self.assert_(len(self.S.lab.objs2)==3)
	def testShared(self):
		'LabelMapper: shared objects'
		o1=woo.core.Object()
		self.S.engines=[woo.core.PyRunner()]
		self.S.lab.someEngine=self.S.engines[0]
		S2=self.S.deepcopy()
		self.assert_(S2.lab.someEngine==S2.engines[0])
	def testAutoLabel(self):
		'LabelMapper: labeled engines are added automatically'
		ee=woo.core.PyRunner(label='abc')
		self.S.engines=[ee]
		self.assert_(self.S.lab.abc==ee)
	def testAutoLabelOld(self):
		'LabelMapper: labeled engines are added automatically to woo.*; this will disappear in the future.'
		import woo
		ee=woo.core.PyRunner(label='abc')
		self.S.engines=[ee]
		self.assert_(hasattr(woo,'abc'))
		self.assert_(woo.abc==ee)

