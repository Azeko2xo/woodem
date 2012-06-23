# encoding: utf-8
# 2009 © Václav Šmilauer <eudoxos@arcig.cz>
"""All defined functionality tests for woo."""
import unittest,inspect

# add any new test suites to the list here, so that they are picked up by testAll
allTests=['core','pbc','clump','psd','io']

# all woo modules (ugly...)
import woo.export,woo.linterpolation,woo.log,woo.pack,woo.plot,woo.post2d,woo.timing,woo.utils,woo.ymport,woo.geom
allModules=(woo.export,woo.linterpolation,woo.log,woo.pack,woo.plot,woo.post2d,woo.timing,woo.utils,woo.ymport,woo.geom)
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

def testAll():
	"""Run all tests defined in all woo.tests.* modules and return
	TestResult object for further examination."""
	suite=unittest.defaultTestLoader.loadTestsFromNames(allTestsFQ)
	import doctest
	for mod in allModules:
		suite.addTest(doctest.DocTestSuite(mod))
	return unittest.TextTestRunner(verbosity=2).run(suite)

	

