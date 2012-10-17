# The purpose of this file is twofold: 
# 1. it tells python that woo (this directory) is package of python modules
#	see http://http://www.python.org/doc/2.1.3/tut/node8.html#SECTION008400000000000000000
#
# 2. import the runtime namespace (will be populated from within c++)
#

"""Common initialization core for woo.

This file is executed when anything is imported from woo for the first time.
It loads woo plugins and injects c++ class constructors to the __builtins__
(that might change in the future, though) namespace, making them available
everywhere.
"""

import wooOptions
from . import config
import warnings
import sys,os,os.path

if 'openmp' in config.features:
	if wooOptions.ompCores:
		cc=wooOptions.ompCores
		if wooOptions.ompThreads!=len(cc) and wooOptions.ompThreads>0:
			warnings.warn('ompThreads==%d ignored, using %d since ompCores are specified.'%(wooOptions.ompThreads,len(cc)))
			wooOptions.ompThreads=len(cc)
		os.environ['GOMP_CPU_AFFINITY']=' '.join([str(cc[0])]+[str(c) for c in cc])
		os.environ['OMP_NUM_THREADS']=str(len(cc))
	elif wooOptions.ompThreads>1:
		os.environ['OMP_NUM_THREADS']=str(wooOptions.ompThreads)
else:
	if wooOptions.ompCores or wooOptions.ompThreads: warnings.warn('--threads and --cores ignored, since compiled without OpenMP')
#
# QUIRKS
#
if wooOptions.quirks & wooOptions.quirkIntel:
	import os
	vgas=os.popen("LC_ALL=C lspci | grep VGA").readlines()
	if len(vgas)==1 and 'Intel' in vgas[0]:
		print 'Intel GPU detected, setting LIBGL_ALWAYS_SOFTWARE=1\n\t(use --no-soft-render to disable)'
		os.environ['LIBGL_ALWAYS_SOFTWARE']='1'

if 0:
	# this is no longer needed
	import ctypes,sys,dl
	# important: initialize c++ by importing libstdc++ directly
	# see http://www.abclinuxu.cz/poradna/programovani/show/286322
	# https://bugs.launchpad.net/bugs/490744
	libstdcxx='${libstdcxx}'
	ctypes.cdll.LoadLibrary(libstdcxx)

# enable warnings which are normally invisible, such as DeprecationWarning
import warnings
warnings.simplefilter('default')

try: # backwards-compat for miniEigen (new is minieigen)
	import miniEigen
	sys.modules['minieigen']=miniEigen
	warnings.warn("'miniEigen' was imported and aliased to the new name 'minieigen' (update your installation to use 'minieigen' only).")
except ImportError: pass

# import eigen before plugins because of its converters, so that default args to python methods can use Vector3r etc
import minieigen

# c++ initialization code
if not wooOptions.debug:
	try: from . import _cxxInternal
	except ImportError:
		try:
			from . import _cxxInternal_debug as _cxxInternal
			woo.modules['woo._cxxInternal']=_cxxInteral
			warnings.warn('Non-debug flavor of Woo was not found, using debug one!')
		except ImportError:
			warnings.warn('FATAL: Neither non-debug nor debug variant of Woo was found!')
			sys.exit(1)
else:
	try:
		from . import _cxxInternal_debug as _cxxInternal
		sys.modules['woo._cxxInternal']=_cxxInternal
	except ImportError:
		try:
			from . import _cxxInteral
			warnings.warn('Debug flavor of Woo was not found, using non-debug one!')
		except ImportError:
			warnings.warn('FATAL: Neither non-debug nor debug variant of Woo was found!')
			sys.exit(1)

cxxInternalFile=_cxxInternal.__file__

from . import core
master=core.Master.instance

#
# create compiled python modules
#
if 0:
	import imp
	for mod in master.compiledPyModules:
		print 'Loading compiled module',mod,'from',plugins[0]
		# this inserts the module to sys.modules automatically
		imp.load_dynamic(mod,cxxInternalFile)
# WORKAROUND: create temporary symlinks
# this is UGLY and will probably not work on Windows!!
else:
	allSubmodules=[]
	compiledModDir=master.tmpFilename() # this will be a directory
	import os,sys
	os.mkdir(compiledModDir)
	sys.path=[compiledModDir]+sys.path
	# move _customConverters to the start, so that imports reyling on respective converters don't fail
	# remove woo._cxxInternal since it is imported already
	cpm=master.compiledPyModules
	cc='woo._customConverters'
	assert cc in cpm
	## HACK: import _gts this way until it gets separated
	cpm=[cc]+[m for m in cpm if m!=cc and m!='woo._cxxInternal']+['_gts']
	# run imports now
	for mod in cpm:
		modpath=mod.split('.') 
		linkName=os.path.join(compiledModDir,modpath[-1])+'.so' # use just the last part to avoid hierarchy
		os.symlink(os.path.abspath(cxxInternalFile),linkName)
		if 'WOO_DEBUG' in os.environ: print 'Loading compiled module',mod,'from symlink',linkName
		sys.modules[mod]=__import__(modpath[-1])
		#__allSubmodules.append(modpath[1])
		if len(modpath)==1: pass # nothing to do, adding to sys.modules is enough
		elif len(modpath)==2: # subdmodule must be added to module
			globals()[modpath[1]]=sys.modules[mod]
			allSubmodules.append(modpath[1])
		elif len(modpath)==3: # must be added to module and submodule
			allSubmodules.append(modpath[1])
			setattr(__import__('.'.join(modpath[:2])),modpath[2],sys.modules[mod])
		else:
			raise RuntimeError('Module %s does not have 2 or 3 path items and will not be imported properly.'%mod)
	sys.path=sys.path[1:]
	## HACK: gts in-tree
	#import woo.gts
	#sys.modules['gts']=woo.gts

if wooOptions.clDev:
	if 'opencl' in config.features:
		if wooOptions.clDev:
			try:
				clDev=[int(a) for a in wooOptions.clDev.split(',')]
				if len(clDev)==1: clDev.append(-1) # default device
				if not len(clDev) in (1,2): raise ValueError()
			except (IndexError, ValueError, AssertionError):
				raise ValueError('Invalid --cl-dev specification %s, should an integer (platform), or a comma-separated couple (platform,device) of integers'%opts.clDev)
			master.clDev=clDev
	else: warnings.warn("--cl-dev ignored, since compiled without OpenCL.")

import _customConverters

from . import system
# create proxies for deprecated classes
deprecatedTypes=system.cxxCtorsDict()
# insert those in the module namespace
globals().update(deprecatedTypes)
# declare what should be imported for "from woo import *"
__all__=deprecatedTypes.keys()+['master']+allSubmodules

# avoids backtrace if crash at finalization (log4cxx)
system.setExitHandlers() 


# fake miniEigen being in woo itself
from minieigen import *

import _monkey # adds methods to c++ classes
import _units 
unit=_units.makeUnitsDict()

# out-of-class docstrings for some classes
try: import _extraDocs
except AttributeError:
	print 'WARN: Error importing py/_extraDocs.py'
	import traceback
	traceback.print_exc()
# attribute aliases
try: import _aliases
except AttributeError:
	print 'WARN: Error importing py/_aliases.py'
	import traceback
	traceback.print_exc()


## DON'T DO THIS:
# import a few "important" modules along with *
#import utils # some others?
# __all__+=[]+dir(minieigen)+dir(wrapper)

