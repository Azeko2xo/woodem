#!/usr/bin/scons
# coding: UTF-8
# vim:syntax=python:

#
# This is the master build file for scons (http://www.scons.org). It is experimental, though it build very well for me. Prequisities for running are having scons installed (debian & family: package scons)
#
# Type "scons -h" for yade-specific options and "scons -H" for scons' options. Note that yade options will be remembered (saved in scons.config) so that you need to specify them only for the first time. Like this, for example:
#
#	scons -j2 pretty=1 debug=0 optimize=1 profile=1 exclude=extra,fem,lattice,realtime-rigidbody,mass-spring CPPPATH=/usr/local/include/wm3
#
# Next time, you can simply run "scons" or "scons -j4" (for 4-parallel builds) to rebuild targets that need it. IF YOU NEED TO READ CODE IN THIS FILE, SOMETHING IS BROKEN AND YOU SHOULD REALLY TELL ME (the goal is to replace qmake eventually).
#
# Scons will do preparatory steps (generating SConscript files from .pro files, creating local include directory and symlinking all headers from there, ...), compile files and install them.
#
# To clean the build, run `scons -c'. Please note that it will also _uninstall_ yade from $PREFIX!
#
# TODO:
#  1. [REMOVED] [DONE] retrieve target list and append install targets dynamically;
#  2. [DONE] configuration and option handling.
#  3. Have build.log with commands and all output...
#
# And as usually, clean up the code, get rid of workarounds and hacks etc.

import os,os.path,string,re,sys
import SCons
# SCons version numbers are needed a few times
sconsVersion=sum([int(SCons.__version__.split('.')[ord[0]])*ord[1] for ord in [(0,10000),(1,100),(2,1)]])

##########################################################################################
############# OPTIONS ####################################################################
##########################################################################################

optsFile='scons.config'

opts=Options(optsFile)
#
# The convention now is, that
#  1. CAPITALIZED options are
#   (a) notorious shell variables (they correspons most the time to SCons environment options of the same name - like CPPPATH)
#   (b) c preprocessor macros available to the program source (like PREFIX and POSTFIX)
#  2. lowercase options influence the building process, compiler options and the like.
#
opts.AddOptions(
	PathOption('PREFIX', 'Install path prefix', '/usr/local'),
	('POSTFIX','Local version appended to binary, library and config directory (beware: if PREFIX is the same, headers of the older version will still be overwritten','',None,lambda x:x),
	BoolOption('debug', 'Enable debugging information and disable optimizations',1),
	BoolOption('profile','Enable profiling information',0),
	BoolOption('optimize','Turn on heavy optimizations (generates SSE2 instructions)',0),
	ListOption('exclude','Components that will not be built','none',names=['extra','common','dem','fem','lattice','mass-spring','realtime-rigidbody']),
	('jobs','Number of jobs to run at the same time (same as -j, but saved)',4,None,int),
	('extraModules', 'Extra directories with their own SConscript files (must be in-tree) (whitespace separated)',None,None,Split),
	('CPPPATH', 'Additional paths for the C preprocessor (whitespace separated)',['/usr/include/wm3'],None,Split),
	('LIBPATH','Additional paths for the linker (whitespace separated)',None,None,Split),
	('QTDIR','Directories where to look for qt3',['/usr/share/qt3','/usr/lib/qt'],None,Split),
	('CXX','The c++ compiler','distcc g++-4.0'),
	BoolOption('pretty',"Don't show compiler command line (like the Linux kernel)",1)
)

### create THE environment
env=Environment(tools=['default'],options=opts)
# do not propagate PATH from outside, to ensure identical builds on different machines
#env.Append(ENV={'PATH':['/usr/local/bin','/bin','/usr/bin']})
# ccache needs $HOME to be set; colorgcc needs $TERM
propagatedEnvVars=['HOME','TERM','DISTCC_HOSTS']
for v in propagatedEnvVars:
	if os.environ.has_key(v): env.Append(ENV={v:os.environ[v]})

opts.Save(optsFile,env)
opts.FormatOptionHelpText=lambda env,opt,help,default,actual: "%10s: %5s [%s] (%s)\n"%(opt,actual,default,help)
Help(opts.GenerateHelpText(env))


##########################################################################################
############# CONFIGURATION ##############################################################
##########################################################################################

env.Append(CPPPATH='',LIBPATH='',LIBS='')

def CheckQt(context, qtdirs):
	# make sure they exist and save them for restoring if a test fails
	origs={'LIBS':context.env['LIBS'],'LIBPATH':context.env['LIBPATH'],'CPPPATH':context.env['CPPPATH']}
	for qtdir in qtdirs:
		context.Message( 'Checking for qt-mt in '+qtdir+'... ' )
		context.env['QTDIR']=qtdir
		context.env.Append(LIBS='qt-mt',LIBPATH=qtdir+'/lib',CPPPATH=qtdir+'/include' )
		ret=context.TryLink('#include<qapp.h>\nint main(int argc, char **argv){QApplication qapp(argc, argv);return 0;}\n','.cpp')
		context.Result(ret)
		if not ret:
			for k in origs.keys(): context.env[k]=origs[k]
		else:
			return ret
	return False

def CheckPython(context):
	origs={'LIBS':context.env['LIBS'],'LIBPATH':context.env['LIBPATH'],'CPPPATH':context.env['CPPPATH'],'LINKFLAGS':context.env['LINKFLAGS']}
	context.Message('Checking for Python development files... ')
	try:
		#FIXME: once caught, exception disappears along with the actual message of what happened...
		import distutils.sysconfig as ds
		context.env.Append(CPPPATH=ds.get_python_inc(),LIBS=ds.get_config_var('LIBS').split())
		context.env.Append(LINKFLAGS=ds.get_config_var('LINKFORSHARED').split()+ds.get_config_var('BLDLIBRARY').split())
		ret=context.TryLink('#include<Python.h>\nint main(int argc, char **argv){Py_Initialize(); Py_Finalize();}\n','.cpp')
		if not ret: raise RuntimeError
	except (ImportError,RuntimeError,ds.DistutilsPlatformError):
		for k in origs.keys(): context.env[k]=origs[k]
		context.Result(False)
		return False
	context.Result(True)
	return True

def CheckScientificPython(context):
	context.Message('Checkgin for scientific python module (debian: package python-scientific)... ')
	try:
		import Scientific
		context.Result(True); return True
	except ImportError:
		context.Result(False); return False

def CheckYadeVersion(context):
	context.Message('Getting Yade version... ')
	svnRevision=None
	for l in os.popen("svn info").readlines():
		m=re.match(r'Revision: ([0-9]+)',l)
		if m:
			svnRevision=m.group(1)
			break
	if svnRevision:
		env['VERSION']='svn'+svnRevision
		context.Result(env['VERSION'])
	else:
		env['VERSION']=''
		context.Result('Svn revision not found, leaving empty')

def CheckCXX(context):
	context.Message('Checking whether c++ compiler "%s" works...'%env['CXX'])
	ret=context.TryLink('#include<iostream>\nint main(int argc, char**argv){std::cerr<<std::endl;return 0;}\n','.cpp')
	context.Result(ret)
	return ret


if not env.GetOption('clean'):
	#essential libraries first
	ok=True
	conf=Configure(env,custom_tests={'CheckQt':CheckQt,'CheckCXX':CheckCXX,'CheckPython':CheckPython,'CheckScientificPython':CheckScientificPython,'CheckYadeVersion':CheckYadeVersion})
	conf.CheckYadeVersion()
	# set some variables
	env['POSTFIX']='-'+env['VERSION']+env['POSTFIX']
	buildDir=env.subst('build$POSTFIX')
	# these MUST be first so that builddir's headers are read before any locally installed ones
	env.Append(CPPPATH=[os.path.join('#',buildDir,'include')])

	ok&=conf.CheckCXX()
	if not ok:
			print "\nYour compiler is broken, no point in continuing. See `config.log' for what went wrong and use the CXX parameter to change your compiler."
			Exit(1)
	ok&=conf.CheckLibWithHeader('pthread','pthread.h','c','pthread_exit(NULL);')
	ok&=conf.CheckLibWithHeader('glut','GL/glut.h','c','glutGetModifiers();')
	ok&=conf.CheckLibWithHeader('boost_date_time','boost/date_time/posix_time/posix_time.hpp','c++','boost::posix_time::time_duration::time_duration();')
	ok&=conf.CheckLibWithHeader('boost_thread','boost/thread/thread.hpp','c++','boost::thread::thread();')
	ok&=conf.CheckLibWithHeader('boost_filesystem','boost/filesystem/path.hpp','c++','boost::filesystem::path();')
	ok&=conf.CheckLibWithHeader('Wm3Foundation','Wm3Math.h','c++','Wm3::Math<double>::PI;')
	ok&=conf.CheckQt(env['QTDIR'])
	env.Tool('qt'); env.Replace(QT_LIB='qt-mt')
	ok&=conf.CheckLibWithHeader('QGLViewer','QGLViewer/qglviewer.h','c++','QGLViewer(1);')
	if not ok:
		print "\nOne of the essential libraries above was not found, unable to continue.\n\nCheck config.log for possible causes, note that there are options that you may need to customize:\n\n"+opts.GenerateHelpText(env)
		Exit(1)
	env.Append(LIBS=['glut','boost_date_time','boost_filesystem','boost_thread','pthread','Wm3Foundation'])

	#optional libraries
	if conf.CheckLibWithHeader('log4cxx','log4cxx/logger.h','c++','log4cxx::Logger::getLogger("foo");'):
		env.Append(LIBS='log4cxx',CPPDEFINES=['LOG4CXX'])
	if conf.CheckPython() and conf.CheckScientificPython(): env.Append(CPPDEFINES=['EMBED_PYTHON'])

	env=conf.Finish()


##########################################################################################
############# BUILDING ###################################################################
##########################################################################################

if 1:
	env.SourceSignatures('MD5')
	env.SetOption('max_drift',5) # cache md5sums of files older than 5 seconds
	SetOption('implicit_cache',1) # cache #include files etc.
	env.SourceCode(".",None) # skip dotted directories
	SetOption('num_jobs',env['jobs'])

### DIRECTORIES
libDirs=['yade-libs','yade-packages/yade-package-common','yade-packages/yade-package-dem','yade-packages/yade-package-fem','yade-packages/yade-package-lattice','yade-packages/yade-package-mass-spring','yade-packages/yade-package-realtime-rigidbody','yade-extra','yade-guis']
# exclude stuff that should be excluded
libDirs=[x for x in libDirs if not re.match('^.*-('+'|'.join(env['exclude'])+')$',x)]
# all things will be built here...

instDirs=[os.path.join('$PREFIX','bin')]+[os.path.join('$PREFIX','lib','yade$POSTFIX',string.split(x,os.path.sep)[-1]) for x in libDirs]
instIncludeDirs=['yade-core']+[os.path.join('$PREFIX','include','yade',string.split(x,os.path.sep)[-1]) for x in libDirs]
### PREPROCESSOR
env.Append(CPPDEFINES=[('POSTFIX',r'$POSTFIX'),('PREFIX',r'$PREFIX')])

### COMPILER
if env['debug']: env.Append(CXXFLAGS='-ggdb3',CPPDEFINES=['DEBUG','YADE_DEBUG'])
else: env.Append(CXXFLAGS='-O2')
if env['optimize']:
	env.Append(CXXFLAGS=Split('-O3 -ffast-math'))
	# -floop-optimize2 is a gcc-4.x flag, doesn't exist on previous version
	# CRASH -ffloat-store
	# maybe not CRASH?: -fno-math-errno
	# CRASH?: -fmodulo-sched  
	#   it is probably --ffast-fath that crashes !!!
	# gcc-4.1 only: -funsafe-loop-optimizations -Wunsafe-loop-optimizations
	# sure CRASH: -ftree-vectorize
	# CRASH (one of them): -fivopts -fgcse-sm -fgcse-las (one of them - not sure which one exactly)
	# ?: -ftree-loop-linear -ftree-loop-ivcanon
	archFlags=Split('-march=pentium4 -mfpmath=sse,387') #-malign-double')
	env.Append(CXXFLAGS=archFlags,LINKFLAGS=archFlags,SHLINKFLAGS=archFlags)
if env['profile']: env.Append(CXXFLAGS=['-pg'],LINKFLAGS=['-pg'],SHLINKFLAGS=['-pg'])
env.Append(CXXFLAGS=['-pipe','-Wall'])
#env.Append(CXXFLAGS=['-save-temps'])

### LINKER
env.Append(LIBS=[]) # ensure existence of the flag
env.Append(SHLINKFLAGS=['-Wl,-soname=${TARGET.file}','-rdynamic'])
# if this is not present, vtables & typeinfos for classes in yade binary itself are not exported; breaks plugin loading
env.Append(LINKFLAGS=['-rdynamic']) 
# makes dynamic library loading easied (no LD_LIBRARY_PATH) and perhaps faster
env.Append(RPATH=[os.path.join('$PREFIX','lib','yade$POSTFIX',string.split(x,os.path.sep)[-1]) for x in libDirs])
# find already compiled but not yet installed libraries for linking
#env.Append(LIBPATH=[os.path.join('#',x) for x in libDirs]	# -floop-optimize2 is a gcc-4.x flag, doesn't exist on previous version
	# CRASH -ffloat-store
	# maybe not CRASH?: -fno-math-errno
	# CRASH?: -fmodulo-sched  
	#   it is probably --ffast-fath that crashes !!!
	# gcc-4.1 only: -funsafe-loop-optimizations -Wunsafe-loop-optimizations
	# sure CRASH: -ftree-vectorize
	# CRASH (one of them): -fivopts -fgcse-sm -fgcse-las (one of them - not sure which one exactly)
	# ?: -ftree-loop-linear -ftree-loop-ivcanon)
env.Append(LIBPATH=env['RPATH'])



### this workaround is only needed for scons<=0.96.92, will disappear soon
###  (env.Install method chokes on no-existing directories)
if sconsVersion<=9692:
	## should reside in prepareIncludes or sth similar
	def createDirs(dirList):
		for d in dirList:
			dd=env.subst(d) #d.replace('$PREFIX',env['PREFIX'])
			if not os.path.exists(dd):
				print dd
				os.makedirs(dd)
			elif not os.path.isdir(dd): raise OSError("Installation directory `%s' is a file?!"%dd)
	createDirs(instDirs)
	createDirs(instIncludeDirs)
	
def prepareIncludes(prefix=None):
	"""symlink all in-tree headers into the ./include directory so that we can build before headers are installed.
	
	If prefix is defined, instruct scons to Install every header file to an appropriate place instead."""
	#print "prepareIncludes(prefix='%s')"%prefix
	global env
	import os,string,re
	from os.path import join,split,isabs,isdir,exists,islink,isfile
	if not prefix: yadeRoot=env.subst('build$POSTFIX') # MUST be relative, otherwise relative symlinks for the local includes will break badly
	else: yadeRoot=prefix
	yadeInc=join(yadeRoot,'include','yade')
	#### POSTFIX_INCLUDES
	# make the headure install directory symlink to postfixed version
	# e.g. ./include/yade -> ./include/yade-debug
	# 	./include/yade will be deleted at this point
	# 	./include/yade-debug will be created if it doesn't exist
	# 	symlink yade -> yade->debug will be created
	# This makes all headers installed to ./include/yade go actually to ./include/yade-debug
	#
	# I hope this doesn't break scons' algorithm for dependency detection
	if 0 and len(env['POSTFIX'])>0:
		import shutil
		yadeIncPostfix=yadeInc+env.subst("$POSTFIX")
		
		if not exists(yadeInc): pass
		elif islink(yadeInc) or isfile(yadeInc): os.remove(yadeInc) # simply remove the link/file
		else: shutil.rmtree(yadeInc)
		assert(not isfile(yadeInc))

		if not exists(yadeIncPostfix): os.makedirs(yadeIncPostfix)
		os.symlink(env.subst("yade$POSTFIX"),yadeInc)

	for root, dirs, files in os.walk('.'):
		for d in ('.svn','yade-flat','include'):
			try: dirs.remove(d)
			except ValueError: pass
		for f in files:
			if f.split('.')[-1] in ('hpp','inl','ipp','tpp','h'):
				m=re.match('^.*?'+os.path.sep+'(yade-((extra|core)|((gui|lib|package)-.*?)))'+os.path.sep+'.*$',root)
				subInc=join(yadeInc,m.group(1))
				if not prefix: # local include directory
					if not isdir(subInc): os.makedirs(subInc)
					upLevels=len(subInc.split(os.path.sep))
					linkTarget=os.path.sep.join(['..' for x in range(0,upLevels)]+[root]+[f])
					try: os.symlink(linkTarget,join(subInc,f));
					except OSError: pass # if the file already exists, symlink fails
				else: env.Install(subInc,join(root,f))

# 1. re-generate SConscript files if some of them do not exist. This must be done in-place for
#		scons postpones all actions after having processed SConscruct and SConscript files.
# 2. symlink all headers to ./include/... so that we can build without installing headers to $PREFIX first (pre-build)
# 3. instruct scons to install (not symlink) all headers to the right place as well
# 4. set the "install" target as default (if scons is called without any arguments)
# Should be cleaned up.

if not env.GetOption('clean'):
	## re-run erskine if needed
	from os.path import exists
	if len([1 for x in [os.path.join(x,'SConscript') for x in libDirs+['yade-core']] if not exists(x)]):
		print "Generating SConscript files (warnings can be ignored safely)..."
		ret=os.system('cd yade-scripts && sh erskine3-apply.sh')
		if ret!=0:
			print "Error running yade-scripts/erskine3-apply.sh, try doing it by hand."
			Exit(1)

	prepareLocalIncludesProxy=prepareIncludes()
	#prepareIncludesProxy=prepareIncludes(prefix=env['PREFIX'])
	prebuildAlias=env.Alias('prebuild',[prepareLocalIncludesProxy])
	installAlias=env.Alias('install',[instDirs]) #,os.path.join('$PREFIX','include','yade')]) #prepareIncludesProxy,
	Depends(installAlias,prebuildAlias)
	Default(installAlias)

# read all SConscript files
env.Export('env');

if env.has_key('extraModules'): extraModules=env['extraModules']
else: extraModules=[]

for libDir in extraModules+libDirs+['yade-core']:
	#print libDir
	env.SConscript(dirs=libDir,build_dir=os.path.join(buildDir,libDir),duplicate=0)
env.Append(LIBPATH=[os.path.join('#',x) for x in extraModules])


#env.NoClean('$PREFIX/bin/yade$POSTFIX','$PREFIX/lib/yade$POSTFIX','$PREFIX/include/yade')
##########################################################################################
############# INSTALLATION ###############################################################
##########################################################################################


if env['pretty']:
	## http://www.scons.org/wiki/HidingCommandLinesInOutput
	env.Replace(CXXCOMSTR='C ${SOURCES}') # → ${TARGET.file}')
	env.Replace(SHCXXCOMSTR='C ${SOURCES}')  #→ ${TARGET.file}')
	env.Replace(SHLINKCOMSTR='L ${SOURCES}') # → ${TARGET.file}')
	env.Replace(LINKCOMSTR='L ${SOURCES}') # → ${TARGET.file}')
	env.Replace(INSTALLSTR='⇒ $TARGET')
	env.Replace(QT_UICCOMSTR='U ${SOURCES}')
	env.Replace(QT_MOCCOMSTR='M ${SOURCES}')
