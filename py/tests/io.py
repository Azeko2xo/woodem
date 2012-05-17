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
	def trySaveLoad(self,fmt='auto',ext='',load=True):
		for o in O.scene.engines+[O.dem.par[0]]:
			#print 'Saving object',o
			out=O.tmpFilename()+ext
			o.save(out,format=fmt)
			if load:
				o2=Serializable.load(out,format='auto')
				self.assert_(type(o2)==type(o))
				o3=type(o).load(out,format='auto')
				self.assertRaises(TypeError,lambda: yade.core.Node.load(out))
				#if fmt=='expr': print open(out).read()
	def testExpr(self):
		'IO: expression dump/load & format detection'
		self.trySaveLoad(fmt='expr')
	def testHtml(self):
		'IO: HTML dump'
		self.trySaveLoad(fmt='html',load=False)
	def testPickle(self):
		'IO: pickle dump/load & format detection'
		self.trySaveLoad(fmt='pickle')
	def testXml(self):
		'IO: XML save/load & format detection'
		self.trySaveLoad(ext='.xml')
	def testXmlBz2(self):
		'IO: XML save/load (bzip2 compressed) & format detection'
		self.trySaveLoad(ext='.xml.bz2')
	def testBin(self):
		'IO: binary save/load & format detection'
		self.trySaveLoad(ext='.bin')
	def testBinGz(self):
		'IO: binary save/load (gzip compressed) & format detection'
		self.trySaveLoad(ext='.bin.gz')

class TestSpecialSaveMethods(unittest.TestCase):
	def setUp(self):
		O.reset()
		O.scene.lastSave='foo'
		self.out=O.tmpFilename()
	def testSceneLastSave_direct(self):
		'IO: Scene.lastSave set'
		O.scene.save(self.out)
		self.assert_(O.scene.lastSave==self.out)
	def testSceneLastSave_omega(self):
		'IO: Scene.lastSave set'
		O.save(self.out)
		self.assert_(O.scene.lastSave==self.out)
	def testSceneLastSave_direct(self):
		'IO: Scene.lastSave not set with _boostSave'
		O.scene._boostSave(self.out)
		self.assert_(O.scene.lastSave=='foo')
