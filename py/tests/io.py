'''
Test loading and saving yade objects in various formats
'''
import yade
import unittest
from yade import *
from yade.core import *
from yade.dem import *
from yade.pre import *
from miniEigen import *
from yade import utils

class TestFormatsAndDetection(unittest.TestCase):
	def setUp(self):
		yade.master.scene=S=Scene(fields=[DemField()])
		S.engines=utils.defaultEngines()
		S.dem.par.append(utils.sphere((0,0,0),radius=1))
	def tryDumpLoad(self,fmt='auto',ext='',load=True):
		S=yade.master.scene
		for o in S.engines+[S.dem.par[0]]:
			out=yade.master.tmpFilename()+ext
			o.dump(out,format=fmt)
			if load:
				o2=Object.load(out,format='auto')
				self.assert_(type(o2)==type(o))
				o3=type(o).load(out,format='auto')
				self.assertRaises(TypeError,lambda: yade.core.Node.load(out))
				#if fmt=='expr': print open(out).read()
	def tryDumpLoadStr(self,fmt,load=True):
		S=yade.master.scene
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
		self.assertRaises(IOError,lambda: yade.master.scene.dem.par[0].dumps(format='bogus'))
	def testTmpStore(self):
		'IO: temporary store loadTmp, saveTmp'
		S=yade.master.scene
		for o in S.engines+[S.dem.par[0]]:
			o.saveTmp(quiet=True);
			o.__class__.loadTmp() # discard the result, but checks type


class TestSpecialDumpMethods(unittest.TestCase):
	def setUp(self):
		yade.master.reset()
		yade.master.scene.lastSave='foo'
		self.out=yade.master.tmpFilename()
	def testSceneLastDump_direct(self):
		'IO: Scene.lastSave set (Object._boostSave overridden)'
		yade.master.scene.save(self.out)
		self.assert_(yade.master.scene.lastSave==self.out)
