# encoding: utf-8
# 2013 © Václav Šmilauer <eu@doxos.eu>

import unittest
from minieigen import *
import woo._customConverters
import woo.dem


class TestGridStore(unittest.TestCase):
	def setUp(self):
		self.gs=woo.dem.GridStore(gridSize=(5,6,7),cellSize=4,locking=True,exNumMaps=4)
		self.ijk=Vector3i(2,3,4)
	def testEx(self):	
		'Grid: storage: dense and extra data'
		for i in range(0,10): self.gs.append(self.ijk,i)
		self.assert_(self.gs[self.ijk]==list(range(0,10)))
		dense,extra=self.gs._rawData(self.ijk)
		# print self.gs[self.ijk],dense,extra
		self.assert_(dense==[10,0,1,2,3])
		self.assert_(extra[:6]==[4,5,6,7,8,9])
		self.assert_(self.gs.countEx()=={tuple(self.ijk):6})
	def testAppend(self):
		'Grid: storage: appending data'
		for i in range(0,13):
			self.gs.append(self.ijk,i)
			self.assert_(i==self.gs[self.ijk][self.gs.size(self.ijk)-1])
	def testStorageOrder(self):
		'Grid: storage: storage order'
		self.assert_(self.gs.lin2ijk(1)==(0,0,1)) # last varies the fastest
		self.assert_(self.gs.ijk2lin((0,0,1))==1)
	def testPyAcces(self):
		'Grid: storage: python access'
		self.gs[self.ijk]=range(0,10)
		self.assert_(self.gs[self.ijk]==list(range(0,10)))
		self.assert_(self.gs.countEx()=={tuple(self.ijk):6})
		del self.gs[self.ijk]
		self.assert_(self.gs.countEx()=={})
		self.assert_(self.gs.size(self.ijk)==0)
		self.assert_(self.gs[self.ijk]==[])
	def testComplement(self):
		'Grid: storage: complements'
		# make insignificant parameters different
		g1=woo.dem.GridStore(gridSize=(3,3,3),cellSize=2,locking=False,exNumMaps=4)
		g2=woo.dem.GridStore(gridSize=(3,3,3),cellSize=3,locking=True,exNumMaps=2)
		c1,c2,c3,c4=(1,1,1),(2,2,2),(2,1,2),(1,2,1)
		g1[c1]=[0,1]; g2[c1]=[1,2] # mixed scenario
		g1[c2]=[1,2,3]; g2[c2]=[]  # b is empty (cheaper copy branch)
		g2[c3]=[]; g2[c3]=[1,2,3]  # a is empty (cheaper copy branch)
		g2[c4]=[]; g2[c4]=[]
		g12,g21=g1.computeRelativeComplements(g2)
		self.assert_(g12[c1]==[0])
		self.assert_(g21[c1]==[2])
		self.assert_(g12[c2]==[1,2,3])
		self.assert_(g21[c2]==[])
		self.assert_(g12[c3]==[])
		self.assert_(g21[c3]==[1,2,3])
		self.assert_(g21[c4]==[])
		self.assert_(g12[c4]==[])
		# incompatible dimensions
		self.assertRaises(RuntimeError,lambda: g1.computeRelativeComplements(woo.dem.GridStore(gridSize=(2,2,2))))


class TestGridColliderBasics(unittest.TestCase):
	def testParams(self):
		'GridCollider: used-definable parameters'
		gc=woo.dem.GridCollider()
		gc.domain=((0,0,0),(1,1,1))
		gc.minCellSize=.1
		self.assert_(gc.dim==Vector3i(10,10,10))
		self.assertAlmostEqual(gc.cellSize[0],.1)
		self.assertRaises(RuntimeError,lambda: setattr(gc,'minCellSize',0))
		gc.minCellSize=.1
		self.assertRaises(RuntimeError,lambda: setattr(gc,'domain',((0,0,0),(0,0,0))))
		self.assertRaises(RuntimeError,lambda: setattr(gc,'domain',((0,0,0),(-1,-1,-1))))
		
