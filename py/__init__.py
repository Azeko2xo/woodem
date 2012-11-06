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
import warnings,traceback
import sys,os,os.path,re,string

# we cannot check for the 'openmp' feature yet, since woo.config is a compiled module
# we set the variable as normally, but will warn below, once the compiled module is imported
if wooOptions.ompCores:
	cc=wooOptions.ompCores
	if wooOptions.ompThreads!=len(cc) and wooOptions.ompThreads>0:
		warnings.warn('ompThreads==%d ignored, using %d since ompCores are specified.'%(wooOptions.ompThreads,len(cc)))
		wooOptions.ompThreads=len(cc)
	os.environ['GOMP_CPU_AFFINITY']=' '.join([str(cc[0])]+[str(c) for c in cc])
	os.environ['OMP_NUM_THREADS']=str(len(cc))
elif wooOptions.ompThreads>1:
	os.environ['OMP_NUM_THREADS']=str(wooOptions.ompThreads)

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
	
if 1:
	# http://stackoverflow.com/questions/1447575/symlinks-on-windows/4388195#4388195
	def win_symlink(source,link_name):
		import ctypes, os.path
		csl=ctypes.windll.kernel32.CreateSymbolicLinkW
		csl.argtypes=(ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32)
		csl.restype=ctypes.c_ubyte
		flags=0
		if source is not None and os.path.isdir(source): flags=1
		if csl(link_name,source,flags)==0: raise ctypes.WinError()
	if sys.platform=='win32': os.symlink=win_symlink
			

# enable warnings which are normally invisible, such as DeprecationWarning
warnings.simplefilter('default')

try: # backwards-compat for miniEigen (new is minieigen)
	import miniEigen
	sys.modules['minieigen']=miniEigen
	warnings.warn("'miniEigen' was imported and aliased to the new name 'minieigen' (update your installation to use 'minieigen' only).")
except ImportError: pass

# import eigen before plugins because of its converters, so that default args to python methods can use Vector3r etc
import minieigen

# c++ initialization code
cxxInternalName='_cxxInternal'
if wooOptions.flavor: cxxInternalName+='_'+re.sub('[^a-zA-Z0-9_]','_',wooOptions.flavor)
if wooOptions.debug: cxxInternalName+='_debug'
try:
	_cxxInternal=__import__('woo.'+cxxInternalName,fromlist='woo')
except ImportError:
	print 'Error importing woo.%s (--flavor=%s).'%(cxxInternalName,wooOptions.flavor if wooOptions.flavor else ' ')
	#traceback.print_exc()
	import glob
	sos=glob.glob(re.sub('__init__.py$','',__file__)+'/_cxxInternal_*.so')
	flavs=[re.sub(r'(^.*/_cxxInternal_)(.*)(\.so$)',r'\2',so) for so in sos]
	if sos:
		maxFlav=max([len(flav) for flav in flavs])
		print 'Available flavors are:'
		for so,flav in zip(sos,flavs):
			print '\t',string.ljust(flav,maxFlav),so
	raise
sys.modules['woo._cxxInternal']=_cxxInternal

cxxInternalFile=_cxxInternal.__file__

from . import core
master=core.Master.instance

#
# create compiled python modules
#
if sys.platform=='win32':
	import imp
	for mod in master.compiledPyModules:
		print 'Loading compiled module',mod,'from',cxxInternalFile
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
	#assert cc in cpm # FIXME: temporarily disabled
	## HACK: import _gts this way until it gets separated
	cpm=[cc]+[m for m in cpm if m!=cc and m!='woo._cxxInternal']+['_gts']
	# run imports now
	for mod in cpm:
		modpath=mod.split('.') 
		linkName=os.path.join(compiledModDir,modpath[-1])+'.so' # use just the last part to avoid hierarchy
		os.symlink(os.path.abspath(cxxInternalFile),linkName)
		if 'WOO_DEBUG' in os.environ: print 'Loading compiled module',mod,'from symlink',linkName
		try: sys.modules[mod]=__import__(modpath[-1])
		except ImportError:
			# compiled without GTS
			if mod=='_gts':
				if 'WOO_DEBUG' in os.environ: print 'Loading compiled module _gts: _gts module not compiled-in (ImportError)'
				pass 
			else: raise # otherwise it is serious
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


if wooOptions.ompThreads>1 or wooOptions.ompCores:
	if 'openmp' not in config.features:
		warnings.warn('--threads and --cores ignored, since compiled without OpenMP.')
	elif master.numThreads!=wooOptions.ompThreads:
		warnings.watn('--threads/--cores did not set number of OpenMP threads correctly (requested %d, current %d). Was OpenMP initialized in this process already?'%(wooOptions.ompThreads,master.numThreads))

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
	traceback.print_exc()
# attribute aliases
try: import _aliases
except AttributeError:
	print 'WARN: Error importing py/_aliases.py'
	traceback.print_exc()


## DON'T DO THIS:
# import a few "important" modules along with *
#import utils # some others?
# __all__+=[]+dir(minieigen)+dir(wrapper)

