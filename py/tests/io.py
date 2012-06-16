'''
Test loading and saving yade objects in various formats
'''
import yade
import unittest
from yade import *
from yade.wrapper import *
from yade.core import *
from yade.dem import *
from yade.pre import *
from miniEigen import *
from yade import utils

class TestFormatsAndDetection(unittest.TestCase):
	def setUp(self):
		O.reset()
		O.scene.fields=[DemField()]
		O.scene.engines=utils.defaultEngines()
		O.dem.par.append(utils.sphere((0,0,0),radius=1))
	def tryDumpLoad(self,fmt='auto',ext='',load=True):
		for o in O.scene.engines+[O.dem.par[0]]:
			out=O.tmpFilename()+ext
			o.dump(out,format=fmt)
			if load:
				o2=Object.load(out,format='auto')
				self.assert_(type(o2)==type(o))
				o3=type(o).load(out,format='auto')
				self.assertRaises(TypeError,lambda: yade.core.Node.load(out))
				#if fmt=='expr': print open(out).read()
	def tryDumpLoadStr(self,fmt,load=True):
		for o in O.scene.engines+[O.dem.par[0]]:
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
		self.assertRaises(IOError,lambda: O.dem.par[0].dumps(format='bogus'))
	def testTmpStore(self):
		'IO: temporary store loadTmp, saveTmp'
		for o in O.scene.engines+[O.dem.par[0]]:
			o.saveTmp(quiet=True);
			o.__class__.loadTmp() # discard the result, but checks type


class TestSpecialDumpMethods(unittest.TestCase):
	def setUp(self):
		O.reset()
		O.scene.lastDump='foo'
		self.out=O.tmpFilename()
	def testSceneLastDump_direct(self):
		'IO: Scene.lastSave set (Object._boostSave overridden)'
		O.scene.save(self.out)
		self.assert_(O.scene.lastSave==self.out)
