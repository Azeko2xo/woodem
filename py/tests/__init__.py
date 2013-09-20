# encoding: utf-8
# 2009 © Václav Šmilauer <eudoxos@arcig.cz>
"""All defined functionality tests for woo."""
import unittest, types

# import all test suites so that they can be picked up by testAll
# explicit imports here so that suites are packed by pyInstaller
from . import core
from . import pbc
from . import clump
from . import psd
from . import io
from . import energy
from . import grid
from . import labels
from . import hertz
# this is ugly, but automatic
allTests=[m for m in dir() if type(eval(m))==types.ModuleType and eval(m).__name__.startswith('woo.tests')]
# should the above break, do it manually (but keep the imports above):
## allTests=['core','pbc','clump','psd','io']



# all woo modules (ugly...)
import woo.export,woo.linterpolation,woo.log,woo.pack,woo.plot,woo.post2d,woo.timing,woo.utils,woo.ymport,woo.geom,woo.batch
allModules=(woo.export,woo.linterpolation,woo.log,woo.pack,woo.plot,woo.post2d,woo.timing,woo.utils,woo.ymport,woo.geom,woo.batch)
try:
	import woo.qt
	allModules+=(woo.qt,)
except ImportError: pass

# fully qualified module names
allTestsFQ=['woo.tests.'+test for test in allTests]

def testModule(module):
	"""Run all tests defined in the module specified, return TestResult object 
	(http://docs.python.org/library/unittest.html#unittest.TextTestResult)
	for further processing.

	@param module: fully-qualified module name, e.g. woo.tests.core
	"""
	suite=unittest.defaultTestLoader().loadTestsFromName(module)
	return unittest.TextTestRunner(verbosity=2).run(suite)

def testAll(sysExit=False):
	"""Run all tests defined in all woo.tests.* modules. Return
	TestResult object for further examination. If *sysExit* is true, call sys.exit
	with status 0 (all tests passed), 1 (some tests failed),
	2 (an exception was raised).
	"""
	suite=unittest.defaultTestLoader.loadTestsFromNames(allTestsFQ)
	import doctest, sys
	for mod in allModules:
		suite.addTest(doctest.DocTestSuite(mod))
	try:
		result=unittest.TextTestRunner(verbosity=2).run(suite)
		if not sysExit: return result
		if result.wasSuccessful():
			print '*** ALL TESTS PASSED ***'
			sys.exit(0)
		else:
			print 20*'*'+' SOME TESTS FAILED '+20*'*'
			sys.exit(1)
	except SystemExit: raise # re-raise
	except:
		print 20*'*'+' UNEXPECTED EXCEPTION WHILE RUNNING TESTS '+20*'*'
		print 20*'*'+' '+str(sys.exc_info()[0])
		print 20*'*'+" Please report bug to bugs@woodem.eu providing the following traceback:"
		import traceback; traceback.print_exc()
		print 20*'*'+' Thank you '+20*'*'
		if sysExit: sys.exit(2)
		raise


	

