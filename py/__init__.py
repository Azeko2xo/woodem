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

WIN=sys.platform=='win32'

if WIN:
	class WooOsEnviron:
		'''Class setting env vars via both CRT and win32 API, so that values can be read back
		with getenv. This is needed for proper setup of OpenMP (which read OMP_NUM_THREADS).'''
		def __setitem__(self,name,val):
			import ctypes
			# in windows set, value in CRT in addition to the one manipulated via win32 api
			# http://msmvps.com/blogs/senthil/archive/2009/10/413/when-what-you-set-is-not-what-you-get-setenvironmentvariable-and-getenv.aspx
			## use python's runtime
			##ctypes.cdll[ctypes.util.find_msvcrt()]._putenv("%s=%s"%(name,value))
			# call MSVCRT (unversioned) as well
			ctypes.cdll.msvcrt._putenv("%s=%s"%(name,val))
			os.environ[name]=val
		def __getitem__(self,name): return os.environ[name]
	wooOsEnviron=WooOsEnviron()
	# this was set in wooMain (with -D, -vv etc), set again so that c++ sees it
	if 'WOO_DEBUG' is os.environ: wooOsEnviron['WOO_DEBUG']=os.environ['WOO_DEBUG']
else:
	wooOsEnviron=os.environ
	

# we cannot check for the 'openmp' feature yet, since woo.config is a compiled module
# we set the variable as normally, but will warn below, once the compiled module is imported
if wooOptions.ompCores:
	cc=wooOptions.ompCores
	if wooOptions.ompThreads!=len(cc) and wooOptions.ompThreads>0:
		warnings.warn('ompThreads==%d ignored, using %d since ompCores are specified.'%(wooOptions.ompThreads,len(cc)))
		wooOptions.ompThreads=len(cc)
	wooOsEnviron['GOMP_CPU_AFFINITY']=' '.join([str(cc[0])]+[str(c) for c in cc])
	wooOsEnviron['OMP_NUM_THREADS']=str(len(cc))
elif wooOptions.ompThreads:
	wooOsEnviron['OMP_NUM_THREADS']=str(wooOptions.ompThreads)
elif 'OMP_NUM_THREADS' not in os.environ:
	wooOsEnviron['OMP_NUM_THREADS']='1'
	
import distutils.sysconfig
soSuffix=distutils.sysconfig.get_config_vars()['SO']
#if WIN and 'TERM' in os.environ:
#	# unbuffered output on windows, in case we're in a real terminal
#	# http://stackoverflow.com/a/881751
#	import msvcrt
#	msvcrt.setmode(sys.stdout.fileno(),os.O_BINARY)
	

#
# QUIRKS
#
if not WIN and (wooOptions.quirks & wooOptions.quirkIntel):
	import os
	vgas=os.popen("LC_ALL=C lspci | grep VGA").readlines()
	if len(vgas)==1 and 'Intel' in vgas[0]:
		print 'Intel GPU detected, setting LIBGL_ALWAYS_SOFTWARE=1\n\t(use --no-soft-render to disable)'
		os.environ['LIBGL_ALWAYS_SOFTWARE']='1'

if WIN:
	# http://stackoverflow.com/questions/1447575/symlinks-on-windows/4388195#4388195
	#
	# unfortunately symlinks are something dangerous under windows, it is a priviledge which must be granted
	# BUT the user must NOT be in the Administrators group?!
	# http://superuser.com/questions/124679/how-do-i-create-an-mklink-in-windows-7-home-premium-as-a-regular-user
	# 
	# for that reason, we use hardlinks (below), which are allowed to everybody
	# Since this would break if files were not on the same partition, we copy _cxxInternal*.pyd
	# to a tempdir first (see below). It will still fail on filesystems not supporting hardlinks
	# (FAT probably)
	def win_symlink(source,link_name):
		import ctypes, os.path
		csl=ctypes.windll.kernel32.CreateSymbolicLinkW
		csl.argtypes=(ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32)
		csl.restype=ctypes.c_ubyte
		flags=0
		if source is not None and os.path.isdir(source): flags=1
		if csl(link_name,source,flags)==0: raise ctypes.WinError()
	def win_hardlink(source,link_name):
		import ctypes
		csl=ctypes.windll.kernel32.CreateHardLinkW
		csl.argtypes=(ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_void_p)
		csl.restype=ctypes.c_ubyte
		if csl(link_name,source,None)==0: raise IOError('Hardlinking failed (files not on the same partition?)')
			

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
	if not WIN:
		_cxxInternal=__import__('woo.'+cxxInternalName,fromlist='woo')
	else:
		## on windows, copy _cxxInternal*.pyd to the tempdir first, so that we can hardlink to it later
		## symlinks are unusable, as they require elevated process (??)
		## it must be copied before it gets imported, so we create tempdir ourselves
		## and pass it via WOO_TEMP to woo::Master ctor, which will just use it
		import tempfile, pkgutil, imp, shutil
		tmpdir=wooOsEnviron['WOO_TEMP']=tempfile.mkdtemp(prefix='woo-tmp-')
		if not hasattr(sys,'frozen'):
			loader=pkgutil.get_loader('woo.'+cxxInternalName)
			if not loader: raise ImportError("Unable to get loader for module woo.%s"%cxxInternalName)
			pydFile=loader.filename
		else:
			# frozen install should have full path in sys.argv[0]
			pydFile=os.path.dirname(sys.argv[0])+'/woo.'+cxxInternalName+soSuffix
			if not os.path.exists(pydFile): raise ImportError("Unable to locate loadable module for woo._cxxInternal in frozen installation: the file %s does not exist"%pydFile)
		f=tmpdir+'/'+cxxInternalName+soSuffix
		shutil.copy2(pydFile,f)
		_cxxInternal=imp.load_dynamic('woo._cxxInternal',f)
		pidfile=tmpdir+'/'+'pid'
					
except ImportError:
	print 'Error importing woo.%s (--flavor=%s).'%(cxxInternalName,wooOptions.flavor if wooOptions.flavor else ' ')
	#traceback.print_exc()
	import glob
	sos=glob.glob(re.sub('__init__.py$','',__file__)+'/_cxxInternal_*'+soSuffix)
	flavs=[re.sub('(^.*/_cxxInternal_)(.*)(\\'+soSuffix+'$)',r'\2',so) for so in sos]
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
if 0: 
	# will only work when http://bugs.python.org/issue16421 is fixed (python 3.4??)
	import imp
	for mod in master.compiledPyModules:
		print 'Loading compiled module',mod,'from',cxxInternalFile
		# this inserts the module to sys.modules automatically
		imp.load_dynamic(mod,cxxInternalFile)
# WORKAROUND: create temporary symlinks
def hack_loadCompiledModulesViaLinks(compiledModDir,tryInAnotherTempdir=True):
	allSubmodules=[]
	import os,sys
	if not os.path.exists(compiledModDir): os.mkdir(compiledModDir)
	sys.path=[compiledModDir]+sys.path
	# move _customConverters to the start, so that imports reyling on respective converters don't fail
	# remove woo._cxxInternal since it is imported already
	cpm=master.compiledPyModules
	cc='woo._customConverters'
	#assert cc in cpm # FIXME: temporarily disabled
	## HACK: import _gts this way until it gets separated
	cpm=[cc]+[m for m in cpm if m!=cc and m!='woo._cxxInternal']
	# run imports now
	for iMod,mod in enumerate(cpm):
		modpath=mod.split('.') 
		linkName=os.path.join(compiledModDir,modpath[-1])+soSuffix # use just the last part to avoid hierarchy
		if WIN:
			try:
				win_hardlink(os.path.abspath(cxxInternalFile),linkName)
			except IOError:
				sys.stderr.write('Creating hardlink failed - on Windows, _cxxInternal.pyd is copied to tempdir before being imported, so that hardlinks should be on the same partition. What\'s happening here? If you are using FAT filesystem, you are out of luck. With NTFS, hardlinks should work. Please report this error so that it can be fixed or worked around.\n')
				raise
		else: os.symlink(os.path.abspath(cxxInternalFile),linkName)
		if 'WOO_DEBUG' in os.environ: print 'Loading compiled module',mod,'from symlink',linkName
		sys.stdout.flush()
		try: sys.modules[mod]=__import__(modpath[-1])
		except ImportError:
			# compiled without GTS
			if mod=='_gts' and False:
				if 'WOO_DEBUG' in os.environ: print 'Loading compiled module _gts: _gts module probably not compiled-in (ImportError)'
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
	sys.path=sys.path[1:] # remove temp dir from the path again
	return allSubmodules
	
allSubmodules=hack_loadCompiledModulesViaLinks(master.tmpFilename()) # this will be a directory

from . import config
if 'gts' in config.features:
	if 'gts' in sys.modules: raise RuntimeError("Woo was compiled with woo.gts; do not import external gts module, they clash with each other.")
	from . import gts
	# so that it does not get imported twice
	sys.modules['gts']=gts


##
## temps cleanup at exit
##
import atexit, shutil, threading, glob
def exitCleanup(path):
	#print 'Purging',path
	shutil.rmtree(path)
# this would fail under windows anyway
if not WIN:
	atexit.register(exitCleanup,master.tmpFileDir)
## clean old temps in bg thread
def cleanOldTemps(prefix,keep):
	try: import psutil
	except ImportError:
		sys.stderr.write('Not cleaning old temps, since the psutil module is missing.\n')
		return
	for d in glob.glob(prefix+'/woo-tmp-*'):
		if d==keep: continue
		pidfile=d+'/pid'
		if not os.path.exists(pidfile): continue
		try:
			pid=int(open(pidfile).readlines()[0][:-1])
			if not psutil.pid_exists(pid):
				sys.stderr.write('Purging old %s (pid=%d)\n'%(d,pid))
				shutil.rmtree(d)
		except: pass
threading.Thread(target=cleanOldTemps,args=(os.path.dirname(master.tmpFileDir),master.tmpFileDir)).start()


##
## Warn if OpenMP threads are not what is probably expected by the user
##
if wooOptions.ompThreads>1 or wooOptions.ompCores:
	if 'openmp' not in config.features:
		warnings.warn('--threads and --cores ignored, since compiled without OpenMP.')
	elif master.numThreads!=wooOptions.ompThreads:
		warnings.warn('--threads/--cores did not set number of OpenMP threads correctly (requested %d, current %d). Was OpenMP initialized in this process already?'%(wooOptions.ompThreads,master.numThreads))
elif master.numThreads>1:
	if 'OMP_NUM_THREADS' in os.environ:
		if master.numThreads!=int(os.environ['OMP_NUM_THREADS']): warnings.warn('OMP_NUM_THREADS==%s, but woo.master.numThreads==%d'%(os.environ['OMP_NUM_THREADS'],master.numThreads))
	else:
		warnings.warn('OpenMP is using %d cores without --threads/--cores being used - the default should be 1'%master.numThreads)

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
unit=_units.unit
# hint fo pyinstaller to freeze this module
from . import pyderived

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

# recursive import of everything under wooExtra
try:
	import wooExtra
	import pkgutil, zipimport
	for importer, modname, ispkg in pkgutil.iter_modules(wooExtra.__path__):
		try:
			m=__import__('wooExtra.'+modname,fromlist='wooExtra')
			print 'wooExtra.%s: loaded'%modname
			if hasattr(sys,'frozen') and not hasattr(m,'__loader__') and len(m.__path__)==1:
				zip=m.__path__[0].split('/wooExtra/')[0].split('\\wooExtra\\')[0]
				if not (zip.endswith('.zip') or zip.endswith('.egg')):
					print 'wooExtra.%s: not a .zip or .egg, no __loader__ set (%s)'%(modname,zip)
				else:
					print 'wooExtra.%s: setting __loader__ and __file__'%modname
					m.__loader__=zipimport.zipimporter(zip)
					m.__file__=os.path.join(m.__path__[0],os.path.basename(m.__file__))
		except ImportError:
			print 'ERROR importing wooExtra.%s:'%modname
			raise
except ImportError:
	# no wooExtra packages are installed
	pass
