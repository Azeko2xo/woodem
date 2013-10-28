# encoding: utf-8
# 2013 © Václav Šmilauer <eu@doxos.eu>

import unittest
from minieigen import *
import woo._customConverters
import woo.dem, woo.core, woo.pre, woo.batch
import sys, math, numpy

class TestBatchResults(unittest.TestCase):
	def setUp(self):
		self.misc=dict(a='a',pi=math.pi,sphere=woo.dem.Sphere(radius=2.3))
		self.scene=woo.core.Scene(fields=[woo.dem.DemField()])
		self.scene.pre=woo.pre.horse.FallingHorse()
		self.series=dict(random=numpy.random.rand(100),ones=numpy.ones(10))
		self.series['group1/item1']=numpy.ones(10)
		self.series['group1/item2']=2*numpy.ones(10)
		self.series['group2/item1']=3*numpy.ones(10)
	def _checkDbContents(self,db):
		r=woo.batch.dbReadResults(db)[0]
		self.assert_(isinstance(r['pre'],woo.pre.horse.FallingHorse))
		self.assert_(r['misc']['a']=='a')
		self.assert_(r['misc']['pi']==math.pi)
		self.assert_(r['misc']['sphere'].radius==2.3)
		self.assert_(r['series']['ones'][2]==1)
	def _writeDb(self,db):
		woo.batch.writeResults(self.scene,defaultDb=db,syncXls=False,quiet=True,series=self.series,**self.misc)
	def _writeXls(self,db,xls):
		woo.batch.dbToSpread(db,xls)
	def _writeCsv(self,db,out):
		woo.batch.dbToSpread(db,out,series=False)
	def testSqlite(self):
		'Batch: results in SQLite (store/read/XLS/CSV)'
		db=woo.master.tmpFilename()+'.sqlite'
		self._writeDb(db)
		self._checkDbContents(db)
		self._writeXls(db,db+'.xls')
		self._writeCsv(db,db+'.csv')
	@unittest.skipIf(sys.platform=='win32','HDF5 not supported under Windows yet.')	
	def testHdf5(self):
		'Batch: results in HDF5 (store/read/XLS/CSV)'
		db=woo.master.tmpFilename()+'.hdf5'
		self._writeDb(db)
		self._checkDbContents(db)
		self._writeXls(db,db+'.xls')
		self._writeCsv(db,db+'.csv')

