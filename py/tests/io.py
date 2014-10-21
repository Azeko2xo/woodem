'''
Test loading and saving woo objects in various formats
'''
import woo
import unittest
from woo.core import *
from woo.dem import *
from woo.pre import *
from minieigen import *
from woo import utils

class TestFormatsAndDetection(unittest.TestCase):
	def setUp(self):
		woo.master.scene=S=Scene(fields=[DemField()])
		S.engines=utils.defaultEngines()
		S.dem.par.add(utils.sphere((0,0,0),radius=1))
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
	def testRefuseDerivedPyObject(self):
		'IO: python-derived objects refuse to save via boost::serialization.'
		import woo.pre.horse
		fh=woo.pre.horse.FallingHorse()
		out=woo.master.tmpFilename()+'.bin.gz'
		self.assertRaises(IOError,lambda: fh.dump(out)) # this should deted boost::serialization anyway
		self.assertRaises(IOError,lambda: fh.dump(out,format='boost::serialization'))
	def testStrFile(self):
		'IO: file can be given as str'
		out=woo.master.tmpFilename()+'.expr'
		woo.master.scene.dem.par[0].dump(out,format='expr')
	def testUnicodeFile(self):
		'IO: filename can be given as unicode'
		out=unicode(woo.master.tmpFilename()+'.expr')
		woo.master.scene.dem.par[0].dump(out,format='expr')
	def testExpr(self):
		'IO: expression dump/load & format detection (file+string)'
		self.tryDumpLoad(fmt='expr')
		self.tryDumpLoadStr(fmt='expr')
	def testJson(self):
		'IO: JSON dump/load & format detection (file+string)'
		self.tryDumpLoad(fmt='json')
		self.tryDumpLoadStr(fmt='json')
	def testHtml(self):
		'IO: HTML dump (file+string)'
		self.tryDumpLoad(fmt='html',load=False)
		self.tryDumpLoadStr(fmt='html',load=False)
	def testPickle(self):
		'IO: pickle dump/load & format detection (file+string)'
		self.tryDumpLoad(fmt='pickle')
		self.tryDumpLoadStr(fmt='pickle',load=True)
	@unittest.skipIf('noxml' in woo.config.features,"Compiled with the 'noxml' feature")
	def testXml(self):
		'IO: XML save/load & format detection'
		self.tryDumpLoad(ext='.xml')
	@unittest.skipIf('noxml' in woo.config.features,"Compiled with the 'noxml' feature")
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
	def testExprSpecialComments(self):
		'IO: special comments #: inside expr dumps'
		expr='''
#: import os
#: g=[]		
#: for i in range(3):
#:   g.append((i+1)*os.getpid())
woo.dem.DemField(
	gravity=g
)
'''
		field=woo.dem.DemField.loads(expr,format='expr')
		import os
		self.assert_(field.gravity[0]==os.getpid())
		self.assert_(field.gravity[1]==2*os.getpid())
		self.assert_(field.gravity[2]==3*os.getpid())



class TestSpecialDumpMethods(unittest.TestCase):
	def setUp(self):
		woo.master.reset()
		self.out=woo.master.tmpFilename()
	def testSceneLastDump_direct(self):
		'IO: Scene.lastSave set (Object._boostSave overridden)'
		woo.master.scene.save(self.out)
		self.assert_(woo.master.scene.lastSave==self.out)

class TestArraySerialization(unittest.TestCase):
	def testMatrixX(self):
		'IO: serialization of arrays'
		t0=woo.core.WooTestClass()
		t0.matX=MatrixX([[0,1,2],[3,4,5]])
		out=woo.master.tmpFilename()
		t0.save(out)
		t1=woo.core.Object.load(out)
		self.assert_(t1.matX.rows()==2)
		self.assert_(t1.matX.cols()==3)
		self.assert_(t1.matX.sum()==15)
	def testBoostMultiArray(self):
		'IO: serialization of boost::multi_array'
		t0=woo.core.WooTestClass()
		t0.arr3d_set((2,2,2),[0,1,2,3,4,5,6,7])
		out=woo.master.tmpFilename()
		t0.save(out)
		t1=woo.core.Object.load(out)
		self.assert_(t1.arr3d==[[[0.0, 1.0], [2.0, 3.0]], [[4.0, 5.0], [6.0, 7.0]]])

		
