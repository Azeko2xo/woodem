'''
Test loading and saving woo objects in various formats
'''
import woo
import unittest
from woo import *
from woo.core import *
from woo.dem import *
from woo.pre import *
from miniEigen import *
from woo import utils

class TestFormatsAndDetection(unittest.TestCase):
	def setUp(self):
		woo.master.scene=S=Scene(fields=[DemField()])
		S.engines=utils.defaultEngines()
		S.dem.par.append(utils.sphere((0,0,0),radius=1))
	def tryDumpLoad(self,fmt='auto',ext='',load=True):
		S=woo.master.scene
		for o in S.engines+[S.dem.par[0]]:
			out=woo.master.tmpFilename()+ext
			o.dump(out,format=fmt)
			if load:
				o2=Object.load(out,format='auto')
				self.assert_(type(o2)==type(o))
				o3=type(o).load(out,format='auto')
				self.assertRaises(TypeError,lambda: woo.core.Node.load(out))
				#if fmt=='expr': print open(out).read()
	def tryDumpLoadStr(self,fmt,load=True):
		S=woo.master.scene
		for o in S.engines+[S.dem.par[0]]:
			dump=o.dumps(format=fmt)
			if load: Object.loads(dump,format='auto')
	def testExpr(self):
		'IO: expression dump/load & format detection (file+string)'
		self.tryDumpLoad(fmt='expr')
		self.tryDumpLoadStr(fmt='expr')
	def testHtml(self):
		'IO: HTML dump (file+string)'
		self.tryDumpLoad(fmt='html',load=False)
		self.tryDumpLoadStr(fmt='html',load=False)
	def testPickle(self):
		'IO: pickle dump/load & format detection (file+string)'
		self.tryDumpLoad(fmt='pickle')
		self.tryDumpLoadStr(fmt='pickle',load=True)
	def testXml(self):
		'IO: XML save/load & format detection'
		self.tryDumpLoad(ext='.xml')
	def testXmlBz2(self):
		'IO: XML save/load (bzip2 compressed) & format detection'
		self.tryDumpLoad(ext='.xml.bz2')
	def testBin(self):
		'IO: binary save/load & format detection'
		self.tryDumpLoad(ext='.bin')
	def testBinGz(self):
		'IO: binary save/load (gzip compressed) & format detection'
		self.tryDumpLoad(ext='.bin.gz')
	def testInvalidFormat(self):
		'IO: invalid formats rejected'
		self.assertRaises(IOError,lambda: woo.master.scene.dem.par[0].dumps(format='bogus'))
	def testTmpStore(self):
		'IO: temporary store (loadTmp, saveTmp)'
		S=woo.master.scene
		for o in S.engines+[S.dem.par[0]]:
			o.saveTmp(quiet=True);
			o.__class__.loadTmp() # discard the result, but checks type
	def testDeepcopy(self):
		'IO: temporary store (Object.deepcopy)'
		S=woo.master.scene
		for o in S.engines+[S.dem.par[0]]:
			o2=o.deepcopy()
			self.assert_(type(o)==type(o2))
			self.assert_(id(o)!=id(o2))


class TestSpecialDumpMethods(unittest.TestCase):
	def setUp(self):
		woo.master.reset()
		woo.master.scene.lastSave='foo'
		self.out=woo.master.tmpFilename()
	def testSceneLastDump_direct(self):
		'IO: Scene.lastSave set (Object._boostSave overridden)'
		woo.master.scene.save(self.out)
		self.assert_(woo.master.scene.lastSave==self.out)
