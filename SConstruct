#!/usr/bin/scons
# coding: UTF-8
# vim:syntax=python:

#
# This is the master build file for scons (http://www.scons.org). It is experimental, though it build very well for me. Prequisities for running are having scons installed (debian & family: package scons)
#
# Type "scons -h" for woo-specific options and "scons -H" for scons' options. Note that Woo options will be remembered (saved in scons.config) so that you need to specify them only for the first time. Like this, for example:
#
#	scons -j2 brief=1 debug=0 optimize=1 flavor=1 exclude=extra,lattice,snow
#
# Next time, you can simply run "scons" or "scons -j4" (for 4-parallel builds) to rebuild targets that need it. IF YOU NEED TO READ CODE IN THIS FILE, SOMETHING IS BROKEN AND YOU SHOULD REALLY TELL ME.
#
# Scons will do preparatory steps (generating SConscript files from .pro files, creating local include directory and symlinking all headers from there, ...), compile files and install them.
#
# To clean the build, run `scons -c'. Please note that it will also _uninstall_ Woo from $PREFIX!
#
# TODO:
#  1. [REMOVED] [DONE] retrieve target list and append install targets dynamically;
#  2. [DONE] configuration and option handling.
#  3. Have build.log with commands and all output...
#
# And as usually, clean up the code, get rid of workarounds and hacks etc.
import os,os.path,string,re,sys,shutil
import SCons
# SCons version numbers are needed a few times
# rewritten for python2.4
ver=[str(x) for x in SCons.__version__.split('.')]
for i in range(0,len(ver)):
	def any2num(x):
		if x in ('0123456789'): return x
		else: return '0'
	ver[i]="".join([any2num(x) for x in ver[i]])
	if len(ver[i])>2: ver[i]=ver[i][0:2]+"."+ver[i][2:]
sconsVersion=10000*float(ver[0])
if len(ver)>1: sconsVersion+=100*float(ver[1])
if len(ver)>2: sconsVersion+=float(ver[2])
# print sconsVersion
#

##########################################################################################
############# OPTIONS ####################################################################
##########################################################################################

### hardy compatibility (may be removed at some later point probably)
### used only when building debian package, since at that point auto download of newer scons is disabled
if 'Variables' not in dir():
	Variables=Options
	BoolVariable,ListVariable,EnumVariable=BoolOption,ListOption,EnumOption

env=Environment(ENV=os.environ,tools=['default','textfile'])
flavorFile='scons.current-flavor'
env['sourceRoot']=os.getcwd()

flavorOpts=Variables(flavorFile)
flavorOpts.Add(('flavor','Config flavor to use (predefined: default or ""); append ! to use it but not save for next build (in scons.current-flavor)','default'))
flavorOpts.Update(env)
# multiple flavors - run them all at the same time
# take care not to save current flavor for those parallel builds
# if env['flavor']=='': env['flavor']='default'
# save the flavor only if the last char is not !
saveFlavor=True
if env['flavor'] and env['flavor'][-1]=='!': env['flavor'],saveFlavor=env['flavor'][:-1],False
if saveFlavor: flavorOpts.Save(flavorFile,env)

#if env['flavor']=='': env['flavor']='default'
optsFile='scons.flavor-'+(env['flavor'] if env['flavor'] else 'default')
if env['flavor']=='default': env['flavor']=''
flavor=env['flavor']
env['cxxFlavor']=('_'+re.sub('[^a-zA-Z0-9_]','_',env['flavor']) if env['flavor'] else '')
print '@@@ Using flavor',(flavor if flavor else '[none]'),'('+optsFile+') @@@'
if not os.path.exists(optsFile):
	print '@@@ Will create new flavor file',optsFile
	opts=Variables()
	opts.Save(optsFile,env)
else:
	opts=Variables(optsFile)
## compatibility hack again
if 'AddVariables' not in dir(opts): opts.AddVariables=opts.AddOptions

# save flavor file so that we know compilation flags
env['buildFlavor']=dict([(l.split('=',1)[0].strip(),eval(l.split('=',1)[1])) for l in file(flavorFile)])

env['buildCmdLine']=sys.argv

def colonSplit(x): return x.split(':')

#
# The convention now is, that
#  1. CAPITALIZED options are
#   (a) notorious shell variables (they correspons most the time to SCons environment options of the same name - like CPPPATH)
#   (b) c preprocessor macros available to the program source (like PREFIX and SUFFIX)
#  2. lowercase options influence the building process, compiler options and the like.
#

## detect virtual environment
## http://stackoverflow.com/a/1883251/761090
import sys,site
VENV=hasattr(sys,'real_prefix')
if not VENV and not hasattr(site,'getsitepackages'):
	# avoid this warning for rebuilds using -R, which should actually work just fine
	if saveFlavor:
		print 'WARN: it seems that you are running SCons inside a virtual environment, without having set it up properly (e.g. "source /my/virtual/env/bin/activate"). (sys.real_prefix is not defined, but site.getsitepackages is not defined either.) I will pretend we are inside a virtual environment, but things may break.'
	VENV=True

if VENV:
	defaultEXECDIR=sys.prefix+'/bin'
	pp=[p for p in sys.path if p.endswith('/site-packages')]
	defaultLIBDIR=pp[-1] if pp else None
	if not defaultLIBDIR: print 'WARN: no good default value of LIBDIR was found, you will have to specify one my hand'
else:
	defaultEXECDIR='/usr/local/bin'
	defaultLIBDIR=site.getsitepackages()[0]


opts.AddVariables(
	('LIBDIR','Install directory for python modules (the default is obtained via "import site; site.getsitepackages()[0]"; in virtual environments, where getsitepackages it not defined, it MUST be specified; in that case, also specify EXECDIR and use the virtual python interpreter to run SCons)',defaultLIBDIR),
	('EXECDIR','Install directory for executables; defaults to /usr/local/bin in normal environemtns and to $VIRTUAL_ENV/bin in virtual environments',defaultEXECDIR),
	BoolVariable('debug', 'Enable debugging information',0),
	BoolVariable('gprof','Enable profiling information for gprof',0),
	('optimize','Turn on optimizations; negative value sets optimization based on debugging: not optimize with debugging and vice versa. -3 (the default) selects -O3 for non-debug and no optimization flags for debug builds',-3,None,int),
	EnumVariable('PGO','Whether to "gen"erate or "use" Profile-Guided Optimization','',['','gen','use'],{'no':'','0':'','false':''},1),
	ListVariable('features','Optional features that are turned on','log4cxx,opengl,opencl,gts,openmp,vtk,qt4',names=['opengl','log4cxx','cgal','openmp','opencl','gts','vtk','gl2ps','qt4','cldem','sparc','noxml','voro','oldabi','nocapsule','never_use_this_one']),
	('jobs','Number of jobs to run at the same time (same as -j, but saved)',2,None,int),
	#('extraModules', 'Extra directories with their own SConscript files (must be in-tree) (whitespace separated)',None,None,Split),
	('cxxstd','Name of the c++ standard (or dialect) to compile with. With gcc, use gnu++11 (gcc >=4.7) or gnu++0x (with gcc 4.5, 4.6)','gnu++0x'),
	('buildPrefix','Where to create build-[version][variant] directory for intermediary files','..'),
	('hotPlugins','Files (without the .cpp extension) that will be compiled separately even in the monolithic build (use for those that you modify frequently); comma-separated.',''),
	('chunkSize','Maximum files to compile in one translation unit when building plugins. (unlimited if <= 0, per-file linkage is used if 1)',7,None,int),
	('version','Woo version (if not specified, guess will be attempted)',None),
	('realVersion','Revision (usually bzr revision); guessed automatically unless specified',None),
	('CPPPATH', 'Additional paths for the C preprocessor (colon-separated)','/usr/include/vtk-5.8:/usr/include/eigen3:/usr/include/vtk'), # hardy has vtk-5.0
	('LIBPATH','Additional paths for the linker (colon-separated)',None),
	('libstdcxx','Specify libstdc++ location by hand (opened dynamically at startup), usually not needed',None),
	('QT4DIR','Directory where Qt4 is installed','/usr/share/qt4'),
	('PATH','Path (not imported automatically from the shell) (colon-separated)',None),
	('CXX','The c++ compiler','g++'),
	('CXXFLAGS','Additional compiler flags for compilation (like -march=core2).',None,None,Split),
	('LIBS','Additional libs to link to (like python2.6 for cygwin)',None,None,Split),
	('march','Architecture to use with -march=... when optimizing','native',None,None),
	('execCheck','Name of the main script that should be installed; if the current one differs, an error is raised (do not use directly, only intended for --rebuild',None),
	('defThreads','No longer used, specify -j each time Woo is run (defaults to 1 now)',-1),
	BoolVariable('lto','Use link-time optimization',0),
	#('SHLINK','Linker for shared objects','g++'),
	#('SHCCFLAGS','Additional compiler flags for linking (for plugins).',None,None,Split),
	('EXTRA_SHLINKFLAGS','Additional compiler flags for linking (for plugins).',None,None,Split),
	BoolVariable('QUAD_PRECISION','typedef Real as long double (=quad)',0),
	BoolVariable('brief',"Don't show commands being run, only what files are being compiled/linked/installed",True),
)
opts.Update(env)

if str(env['features'])=='all':
	print 'ERROR: using "features=all" is illegal, since it breaks feature detection at runtime (SCons limitation). Write out all features separated by commas instead. Sorry.'
	Exit(1)

if saveFlavor: opts.Save(optsFile,env)
# set optimization based on debug, if required 
if env['optimize']<0: env['optimize']=(None if env['debug'] else -env['optimize']) 

# handle colon-separated lists:
for k in ('CPPPATH','LIBPATH','QTDIR','PATH'):
	if env.has_key(k):
		env[k]=colonSplit(env[k])

# do not propagate PATH from outside, to ensure identical builds on different machines
#env.Append(ENV={'PATH':['/usr/local/bin','/bin','/usr/bin']})
# ccache needs $HOME to be set, also CCACHE_PREFIX if used; colorgcc needs $TERM; distcc wants DISTCC_HOSTS
# fakeroot needs FAKEROOTKEY and LD_PRELOAD
propagatedEnvVars=['HOME','TERM','DISTCC_HOSTS','LD_PRELOAD','FAKEROOTKEY','LD_LIBRARY_PATH','CCACHE_PREFIX']
for v in propagatedEnvVars:
	if os.environ.has_key(v): env.Append(ENV={v:os.environ[v]})
if env.has_key('PATH'): env.Append(ENV={'PATH':env['PATH']})

# get number of jobs from DEB_BUILD_OPTIONS if defined
# see http://www.de.debian.org/doc/debian-policy/ch-source.html#s-debianrules-options
if os.environ.has_key('DEB_BUILD_OPTIONS'):
	for opt in os.environ['DEB_BUILD_OPTIONS'].split():
		if opt.startswith('parallel='):
			j=opt.split('=')[1].split(',')[0] # there was a case of 5, in hardy...
			print "Setting number of jobs (using DEB_BUILD_OPTIONS) to `%s'"%j
			env['jobs']=int(j)



if sconsVersion>9700: opts.FormatOptionHelpText=lambda env,opt,help,default,actual,alias: "%10s: %5s [%s] (%s)\n"%(opt,actual,default,help)
else: opts.FormatOptionHelpText=lambda env,opt,help,default,actual: "%10s: %5s [%s] (%s)\n"%(opt,actual,default,help)
Help(opts.GenerateHelpText(env))

###########################################
################# BUILD DIRECTORY #########
###########################################
def getRealWooVersion():
	"Attempts to get woo version from RELEASE file if it exists or from bzr/svn, or from VERSION"
	import os.path,re,os
	if os.path.exists('RELEASE'):
		return file('RELEASE').readline().strip()
	if os.path.exists('.git'):
		for l in os.popen("LC_AL=C git rev-list HEAD --count 2>/dev/null").readlines():
			return 'r'+l[:-1]
	if os.path.exists('.bzr'):
		for l in os.popen("LC_ALL=C bzr revno 2>/dev/null").readlines():
			return 'r'+l[:-1]
	if os.path.exists('VERSION'):
		return file('VERSION').readline().strip()
	return None

## ALL generated stuff should go here - therefore we must determine it very early!!
if not env.has_key('realVersion') or not env['realVersion']: env['realVersion']=getRealWooVersion() or 'unknown' # unknown if nothing returned
if not env.has_key('version'): env['version']=env['realVersion']

suffix=('-'+env['flavor'] if (env['flavor'] and env['flavor']!='default') else '')
print "Woo version is `%s' (%s), installed files will be suffixed with `%s'."%(env['version'],env['realVersion'],suffix)

buildDir=os.path.abspath(env.subst('$buildPrefix/build'+suffix+('' if not env['debug'] else '/dbg')))
print "All intermediary files will be in `%s'."%env.subst(buildDir)
env['buildDir']=buildDir
# these MUST be first so that builddir's headers are read before any locally installed ones
buildInc='$buildDir/include'
env.Append(CPPPATH=[buildInc])

env.SConsignFile(buildDir+'/scons-signatures')

##########################################################################################
############# SHORTCUT TARGETS ###########################################################
##########################################################################################

if len(sys.argv)>1 and ('clean' in sys.argv) or ('tags' in sys.argv) or ('doc' in sys.argv):
	if 'clean' in sys.argv:
		if os.path.exists(buildDir):
			print "Cleaning: %s."%buildDir; shutil.rmtree(buildDir)
		else: print "Nothing to clean: %s."%buildDir
		sys.argv.remove('clean')
	if 'tags' in sys.argv:
		cmd="ctags -R --extra=+q --fields=+n --exclude='.*' --exclude=attic --exclude=doc --exclude=scons-local --exclude=include --exclude=lib/triangulation --exclude='*0' --exclude=debian --exclude='*.so' --exclude='*.s' --exclude='*.ii' --langmap=c++:+.inl,c++:+.tpp,c++:+.ipp"
		print cmd; os.system(cmd)
		sys.argv.remove('tags')
	if 'doc' in sys.argv:
		raise RuntimeError("'doc' argument not supported by scons now. See doc/README")
	#	cmd="cd doc; doxygen Doxyfile"
	#	print cmd; os.system(cmd)
	#	sys.argv.remove('doc')
	# still something on the command line? Ignore, but warn about that
	if len(sys.argv)>1: print "!!! WARNING: Shortcuts (clean,tags,doc) cannot be mixed with regular targets or options; ignoring:\n"+''.join(["!!!\t"+a+"\n" for a in sys.argv[1:]])
	# bail out
	Exit(0)


##########################################################################################
############# CONFIGURATION ##############################################################
##########################################################################################

# ensure non-None
env.Append(CPPPATH='',LIBPATH='',LIBS='',CXXFLAGS=('-std='+env['cxxstd'] if env['cxxstd'] else ''),SHCCFLAGS='',SHLINKFLAGS='')

def CheckCXX(context):
	# see http://llvm.org/bugs/show_bug.cgi?id=13530#c3, workaround number 4.
	if 'clang' in context.env['CXX']:
		context.env.Append(CPPDEFINES={'__float128':'void'})
	context.Message('Checking whether c++ compiler "%s %s" works...'%(env['CXX'],' '.join(env['CXXFLAGS'])))
	ret=context.TryLink('#include<iostream>\nint main(int argc, char**argv){std::cerr<<std::endl;return 0;}\n','.cpp')
	context.Result(ret)
	# we REQUIRE gold to build Woo under Linux (not ld.bfd, the old GNU linker)
	# binutils now require us to select gold explicitly (see https://launchpad.net/ubuntu/saucy/+source/binutils/+changelog)
	# this option adds this to gcc and is hopefully backwards-compatible as to not break other builds
	prev=env['LINKFLAGS']
	env.Append(LINKFLAGS='-fuse-ld=gold')
	ret2=context.TryLink('#include<iostream>\nint main(int argc, char**argv){std::cerr<<std::endl;return 0;}\n','.cpp')
	if not ret2:
		print '(-fuse-ld=gold not supported by the compiler, make sure that gold is used yourself)'
		env.Replace(LINKFLAGS=prev)
	return ret

def CheckPython(context):
	"Checks for functional python/c API. Sets variables if OK and returns true; otherwise returns false."
	origs={'LIBS':context.env['LIBS'],'LIBPATH':context.env['LIBPATH'],'CPPPATH':context.env['CPPPATH'],'LINKFLAGS':context.env['LINKFLAGS']}
	context.Message('Checking for Python development files... ')
	try:
		#FIXME: once caught, exception disappears along with the actual message of what happened...
		import distutils.sysconfig as ds
		context.env.Append(CPPPATH=ds.get_python_inc(),LIBS=ds.get_config_var('LIBS').split() if ds.get_config_var('LIBS') else None)
		# FIXME: there is an inconsistency between cygwin and linux here?!
		if sys.platform=='cygwin':
			context.env.Append(LINKFLAGS=ds.get_config_var('LINKFORSHARED').split(),LIBS=ds.get_config_var('BLDLIBRARY').split())
		else:
			context.env.Append(LINKFLAGS=ds.get_config_var('LINKFORSHARED').split()+ds.get_config_var('BLDLIBRARY').split())
		ret=context.TryLink('#include<Python.h>\nint main(int argc, char **argv){Py_Initialize(); Py_Finalize();}\n','.cpp')
		if not ret: raise RuntimeError
	except (ImportError,RuntimeError,ds.DistutilsPlatformError):
		for k in origs.keys(): context.env[k]=origs[k]
		context.Result('error')
		return False
	context.Result('ok'); return True

def EnsureBoostVersion(context,version):
	context.Message('Checking that boost version is >= %d '%version)
	ret=context.TryCompile("""
		#include <boost/version.hpp>
		#if BOOST_VERSION < %d
			#error Installed boost is too old!
		#endif
		// int main() { return 0; }
    """%version,'.cpp')
	if ret: context.Result('yes')
	else: context.Result('no')
	return ret
	#os.popen('echo BOOST_VERSION | "%s" -E - -include boost/version.hpp | tail -n1'%context.env['CXX'])

def CheckBoost(context):
	context.Message('Checking boost libraries... ')
	libs=[
		# this might fail with clang, as boost_system requires boost_thread (and vice versa)
		# if boost_system fails, it will be added in checkLib_MaybeMT for boost_thread
		# as workaround, so everybody will be happy in the end
		# no idea what is clang doing
		('boost_system','boost/system/error_code.hpp','boost::system::error_code();',False),
		('boost_thread','boost/thread/thread.hpp','boost::thread();',True),
		('boost_date_time','boost/date_time/posix_time/posix_time.hpp','boost::posix_time::time_duration();',True),
		('boost_filesystem','boost/filesystem/path.hpp','boost::filesystem::path();',True),
		('boost_iostreams','boost/iostreams/device/file.hpp','boost::iostreams::file_sink("");',True),
		('boost_regex','boost/regex.hpp','boost::regex("");',True),
		('boost_chrono','boost/chrono/chrono.hpp','boost::chrono::system_clock::now();',True),
		('boost_serialization','boost/archive/archive_exception.hpp','try{} catch (const boost::archive::archive_exception& e) {};',True),
		('boost_python','boost/python.hpp','boost::python::scope();',True),
	]
	failed=[]
	def checkLib_maybeMT(lib,header,func):
		for LIB in (lib,lib+'-mt'):
			LIBS=env['LIBS'][:]; context.env.Append(LIBS=[LIB])
			if context.TryLink('#include<'+header+'>\nint main(void){'+func+';}','.cpp'): return True
			# with boost_thread, try to link boost_system (or boost_system-mt) in addition
			# as it might fail with clang otherwise (DSO not specified on the command-line)
			if lib=='boost_thread':
				LIBS=env['LIBS'][:]; context.env.Append(LIBS=[LIB,'boost_system'+('-mt' if LIB.endswith('-mt') else '')])
				if context.TryLink('#include<'+header+'>\nint main(void){'+func+';}','.cpp'): return True
			env['LIBS']=LIBS
		return False
	for lib,header,func,mandatory in libs:
		ok=checkLib_maybeMT(lib,header,func)
		if not ok and mandatory: failed.append(lib)
	if failed: context.Result('Failures: '+', '.join(failed)); return False
	context.Result('all ok'); return True

def CheckPythonModules(context):
	context.Message("Checking for required python modules... ")
	mods=[('IPython','ipython'),('numpy','python-numpy'),('matplotlib','python-matplotlib'),('genshi','python-genshi'),('xlwt','python-xlwt'),('xlrd','python-xlrd'),('h5py','python-h5py'),('lockfile','python-lockfile'),('pkg_resources','python-pkg-resources')]
	if 'qt4' in context.env['features']: mods.append(('PyQt4.QtGui','python-qt4'))
	if 'qt4' in context.env['features'] or 'opengl' in context.env['features']: mods.append(('Xlib','python-xlib'))
	failed=[]
	for m,pkg in mods:
		try:
			exec("import %s"%m)
		except ImportError:
			failed.append(m+' (package %s)'%pkg)
	if failed: context.Result('Failures: '+', '.join(failed)); return False
	context.Result('all ok'); return True

	

if not env.GetOption('clean'):
	conf=env.Configure(custom_tests={'CheckCXX':CheckCXX,'EnsureBoostVersion':EnsureBoostVersion,'CheckBoost':CheckBoost,'CheckPython':CheckPython,'CheckPythonModules':CheckPythonModules}, # 'CheckQt':CheckQt
		conf_dir='$buildDir/.sconf_temp',log_file='$buildDir/config.log'
	)
	ok=True
	ok&=conf.CheckCXX()
	if not ok:
			print "\nYour compiler is broken, no point in continuing. See `%s' for what went wrong and use the CXX/CXXFLAGS parameters to change your compiler."%(buildDir+'/config.log')
			Exit(1)
	ok&=conf.CheckLibWithHeader('pthread','pthread.h','c','pthread_exit(NULL);',autoadd=1)
	ok&=(conf.CheckPython() and conf.CheckCXXHeader(['Python.h','numpy/ndarrayobject.h'],'<>'))
	ok&=conf.CheckPythonModules()
	ok&=conf.EnsureBoostVersion(14800)
	ok&=conf.CheckBoost()
	env['haveForeach']=conf.CheckCXXHeader('boost/foreach.hpp','<>')
	if not env['haveForeach']: print "(OK, local version will be used instead)"
	ok&=conf.CheckCXXHeader('Eigen/Core')

	if not ok:
		print "\nOne of the essential libraries above was not found, unable to continue.\n\nCheck `%s' for possible causes, note that there are options that you may need to customize:\n\n"%(buildDir+'/config.log')+opts.GenerateHelpText(env)
		Exit(1)
	def featureNotOK(featureName,note=None):
		print "\nERROR: Unable to compile with optional feature `%s'.\n\nIf you are sure, remove it from features (scons features=featureOne,featureTwo for example) and build again."%featureName
		if note: print "Note:",note
		Exit(1)
	# check "optional" libs
	if 'opengl' in env['features']:
		ok=conf.CheckLibWithHeader('GL','GL/gl.h','c++','glFlush();',autoadd=1)
		ok&=conf.CheckLibWithHeader('GLU','GL/glu.h','c++','gluNewTess();',autoadd=1)
		ok&=conf.CheckLibWithHeader('glut','GL/glut.h','c++','glutGetModifiers();',autoadd=1)
		ok&=conf.CheckLibWithHeader('gle','GL/gle.h','c++','gleSetNumSides(20);',autoadd=1)
		# TODO ok=True for darwin platform where openGL (and glut) is native
		if not ok: featureNotOK('opengl')
		if 'qt4' in env['features']:
			env['ENV']['PKG_CONFIG_PATH']='/usr/bin/pkg-config'
			env.Tool('qt4')
			env.EnableQt4Modules(['QtGui','QtCore','QtXml','QtOpenGL'])
			if not conf.TryAction(env.Action('pyrcc4'),'','qrc'): featureNotOK('qt4','The pyrcc4 program is not operational (package pyqt4-dev-tools)')
			if not conf.TryAction(env.Action('pyuic4'),'','ui'): featureNotOK('qt4','The pyuic4 program is not operational (package pyqt4-dev-tools)')
			if conf.CheckLibWithHeader(['qglviewer-qt4'],'QGLViewer/qglviewer.h','c++','QGLViewer();',autoadd=1):
				env['QGLVIEWER_LIB']='qglviewer-qt4'
			elif conf.CheckLibWithHeader(['libQGLViewer'],'QGLViewer/qglviewer.h','c++','QGLViewer();',autoadd=1):
				env['QGLVIEWER_LIB']='libQGLViewer'
			else: featureNotOK('qt4','Building with Qt4 implies the QGLViewer library installed (package libqglviewer-qt4-dev package in debian/ubuntu, libQGLViewer in RPM-based distributions)')
	if 'opencl' in env['features']:
		env.Append(CPPDEFINES=['CL_USE_DEPRECATED_OPENCL_1_1_APIS'])
		ok=conf.CheckLibWithHeader('OpenCL','CL/cl.h','c','clGetPlatformIDs(0,NULL,NULL);',autoadd=1)
		if not ok: featureNotOK('opencl','OpenCL headers not found. Install an OpenCL SDK, and add appropriate directory to CPPPATH, LDPATH if necessary (note: this is not a test that your computer has an OpenCL-capable device)')
		env['haveClHpp']=conf.CheckCXXHeader('CL/cl.hpp')
		if not env['haveClHpp']: print '(OK, local version (from Khronos website) will be used instead; some SDK\'s (Intel) are not providing cl.hpp)'
	if 'vtk' in env['features']:
		ok=conf.CheckLibWithHeader(['vtkCommon'],'vtkInstantiator.h','c++','vtkInstantiator::New();',autoadd=1)
		env.Append(LIBS=['vtkHybrid','vtkRendering','vtkIO','vtkFiltering'])
		if not ok: featureNotOK('vtk',note="Installer can`t find vtk-library. Be sure you have it installed (usually, libvtk5-dev package). Or you might have to add VTK header directory (e.g. /usr/include/vtk-5.4) to CPPPATH.")
	if 'gts' in env['features']:
		env.ParseConfig('pkg-config gts --cflags --libs');
		ok=conf.CheckLibWithHeader('gts','gts.h','c++','gts_object_class();',autoadd=1)
		env.Append(CPPDEFINES='PYGTS_HAS_NUMPY')
		if not ok: featureNotOK('gts')
	if 'log4cxx' in env['features']:
		ok=conf.CheckLibWithHeader('log4cxx','log4cxx/logger.h','c++','log4cxx::Logger::getLogger("");',autoadd=1)
		if not ok: featureNotOK('log4cxx')
	if 'gl2ps' in env['features']:
		print 'WARN: the gl2ps feature is deprecated and has no effect'
	if 'cgal' in env['features']:
		ok=conf.CheckLibWithHeader('CGAL','CGAL/Exact_predicates_inexact_constructions_kernel.h','c++','CGAL::Exact_predicates_inexact_constructions_kernel::Point_3();')
		env.Append(CXXFLAGS='-frounding-math') # required by cgal, otherwise we get assertion failure at startup
		if not ok: featureNotOK('cgal')
	#env.Append(LIBS='woo-support')

	env.Append(CPPDEFINES=['WOO_'+f.upper().replace('-','_') for f in env['features']])
	env.Append(CPPDEFINES=[('WOO_CXX_FLAVOR',env['cxxFlavor'])])

	env=conf.Finish()

##########################################################################################
############# BUILDING ###################################################################
##########################################################################################

### SCONS OPTIMIZATIONS
SCons.Defaults.DefaultEnvironment(tools = [])
env.Decider('MD5-timestamp')
env.SetOption('max_drift',5) # cache md5sums of files older than 5 seconds
SetOption('implicit_cache',0) # cache #include files etc.
env.SourceCode(".",None) # skip dotted directories
SetOption('num_jobs',env['jobs'])

### SHOWING OUTPUT
if env['brief']:
	## http://www.scons.org/wiki/HidingCommandLinesInOutput
	env.Replace(CXXCOMSTR='C ${SOURCES}', # → ${TARGET.file}')
		CCOMSTR='C ${SOURCES}',
		SHCXXCOMSTR='C ${SOURCES}', 
		SHCCCOMSTR='C ${SOURCES}', 
		SHLINKCOMSTR='L ${TARGET.file}',
		LINKCOMSTR='L ${TARGET.file}',
		INSTALLSTR='> $TARGET',
		QT_UICCOMSTR='U ${SOURCES}',
		QT_MOCCOMSTR='M ${SOURCES}',
		QT4_UICCOMSTR='U ${SOURCES}',
		QT_MOCFROMHCOMSTR='M ${SOURCES}',
		QT_MOCFROMCXXCOMSTR='M ${SOURCES}',
		)
else:
	env.Replace(INSTALLSTR='cp -f ${SOURCE} ${TARGET}')

### PREPROCESSOR FLAGS
if env['QUAD_PRECISION']: env.Append(CPPDEFINES='QUAD_PRECISION')

### COMPILER
if env['debug']: env.Append(CXXFLAGS='-ggdb2',CPPDEFINES=['WOO_DEBUG'])
# NDEBUG is used in /usr/include/assert.h: when defined, asserts() are no-ops
else: env.Append(CPPDEFINES=['NDEBUG'])
if 'openmp' in env['features']: env.Append(CXXFLAGS='-fopenmp',LIBS='gomp',CPPDEFINES='WOO_OPENMP')
if env['optimize']:
	env.Append(CXXFLAGS=['-O%d'%env['optimize']])
	# do not state architecture if not provided
	# used for debian packages, where 'native' would generate instructions outside the package architecture
	# (such as SSE instructions on i386 builds, if the builder supports them)
	if env.has_key('march') and env['march']: env.Append(CXXFLAGS=['-march=%s'%env['march']]),
else:
	pass

if env['lto']:
	env.Append(CXXFLAGS='-flto',CFLAGS='-flto')
	if 'clang' in env['CXX']: env.Append(SHLINKFLAGS=['-use-gold-plugin','-O3','-flto'])
	else: env.Append(SHLINKFLAGS=['-fuse-linker-plugin','-O3','-flto=%d'%env['jobs']])

if env['gprof']: env.Append(CXXFLAGS=['-pg'],LINKFLAGS=['-pg'],SHLINKFLAGS=['-pg'])
env.Prepend(CXXFLAGS=['-pipe','-Wall'])

if env['PGO']=='gen': env.Append(CXXFLAGS=['-fprofile-generate'],LINKFLAGS=['-fprofile-generate'])
if env['PGO']=='use': env.Append(CXXFLAGS=['-fprofile-use'],LINKFLAGS=['-fprofile-use'])

if 'clang' in env['CXX']:
	print 'Looks like we use clang, adding some flags to avoid warning flood.'
	env.Append(CXXFLAGS=['-Qunused-arguments','-Wno-empty-body','-Wno-self-assign'])
if 'g++' in env['CXX']:
	ver=os.popen('LC_ALL=C '+env['CXX']+' --version').readlines()[0].split()[-1]
	if ver.startswith('4.6'):
		print 'Using gcc 4.6, adding -pedantic to avoid ICE at string initializer_list (http://gcc.gnu.org/bugzilla/show_bug.cgi?id=50478)'
		env.Append(CXXFLAGS='-pedantic')

if 'EXTRA_SHLINKFLAGS' in env: env.Append(SHLINKFLAGS=env['EXTRA_SHLINKFLAGS'])
if not env['debug']: env.Append(SHLINKFLAGS=['-Wl,--strip-all'])

def relpath(pf,pt):
	"""Returns relative path from pf (path from) to pt (path to) as string.
	Last component of pf MUST be filename, not directory name. It can be empty, though. Legal pf's: 'dir1/dir2/something.cc', 'dir1/dir2/'. Illegal: 'dir1/dir2'."""
	from os.path import sep,join,abspath,split
	apfl=abspath(split(pf)[0]).split(sep); aptl=abspath(pt).split(sep); i=0
	while apfl[i]==aptl[i] and i<min(len(apfl),len(aptl))-1: i+=1
	return sep.join(['..' for j in range(0,len(apfl)-i)]+aptl[i:])

# 1. symlink all headers to buildDir/include before the actual build
# 2. (unused now) instruct scons to install (not symlink) all headers to the right place as well
# 3. set the "install" target as default (if scons is called without any arguments), which triggers build in turn
# Should be cleaned up.

if not env.GetOption('clean'):
	from os.path import join,split,isabs,isdir,exists,lexists,islink,isfile,sep,dirname
	def mkSymlink(link,target):
		if exists(link) and not islink(link):
			import shutil
			print 'Removing directory %s, was replaced by a symlink in post-0.60 versions.'%link
			shutil.rmtree(link)
		if not exists(link):
			if lexists(link): os.remove(link) # remove dangling symlink
			d=os.path.dirname(link)
			if not exists(d): os.makedirs(d)
			os.symlink(relpath(link,target),link)

	wooInc=join(buildDir,'include/woo')
	mkSymlink(wooInc,'.')
	import glob
	boostDir=buildDir+'/include/boost'
	if not exists(boostDir): os.makedirs(boostDir)
	if 'opencl' in env['features'] and not env['haveClHpp']:
		clDir=buildDir+'/include/CL'
		if not exists(clDir): os.makedirs(clDir)
		mkSymlink(clDir+'/cl.hpp','extra/cl.hpp_local')
	env.Default(env.Alias('install',['$EXECDIR','$LIBDIR'])) # build and install everything that should go to instDirs, which are $PREFIX/{bin,lib} (uses scons' Install)

env.Export('env');

######
### combining builder
#####
# since paths are aboslute, sometime they are not picked up
# hack it away by writing the combined files into a text file
# and when it changes, force rebuild of all combined files
# changes to $buildDir/combined-files are cheked at the end of 
# the SConscript file
import md5
combinedFiles=env.subst('$buildDir/combined-files')

if os.path.exists(combinedFiles):
	combinedFilesMd5=md5.md5(file(combinedFiles).read()).hexdigest()
	os.remove(combinedFiles)
else:
	combinedFilesMd5=None

# http://www.scons.org/wiki/CombineBuilder
import SCons.Action
import SCons.Builder
def combiner_build(target, source, env):
	"""generate a file, that's including all sources"""
	out=""
	for src in source: out+="#include \"%s\"\n"%src.abspath
	open(str(target[0]),'w').write(out)
	env.AlwaysBuild(target[0])
	return 0
def CombineWrapper(target,source):
	open(combinedFiles,'a').write(env.subst(target)+': '+' '.join([env.subst(s) for s in source])+'\n')
	return env.Combine(target,source)
env.CombineWrapper=CombineWrapper
	
env.Append(BUILDERS = {'Combine': env.Builder(action = SCons.Action.Action(combiner_build, "> $TARGET"),target_factory = env.fs.File,)})


# read top-level SConscript file. It is used only so that variant_dir is set. This file reads all necessary SConscripts
env.SConscript(dirs=['.'],variant_dir=buildDir,duplicate=0)

#################################################################################
## remove plugins that are in the target dir but will not be installed now
## only when installing without requesting special path (we would have no way
## to know what should be installed overall.
if not COMMAND_LINE_TARGETS:
	toInstall=set([str(node) for node in env.FindInstalledFiles()])
	for root,dirs,files in os.walk(env.subst('$LIBDIR/woo')):
		# do not go inside the debug directly, plugins are different there
		for f in files:
			#print 'Considering',f
			ff=os.path.join(root,f)
			# do not delete python-optimized files and symbolic links (lib_gts__python-module.so, for instance)
			if ff not in toInstall and not ff.endswith('.pyo') and not ff.endswith('.pyc') and not os.path.islink(ff) and not os.path.basename(ff).startswith('.nfs'):
				# HACK
				if os.path.basename(ff).startswith('_cxxInternal'): continue
				print "Deleting extra plugin", ff
				os.remove(ff)


### check if combinedFiles is different; if so, force rebuild of all of them
if os.path.exists(combinedFiles) and combinedFilesMd5!=md5.md5(open(combinedFiles).read()).hexdigest():
	print 'Rebuilding combined files, since the md5 has changed.'
	combs=[l.split(':')[0] for l in open(combinedFiles)]
	for c in combs:
		env.AlwaysBuild(c)
		env.Default(c)

#Progress('.', interval=100, file=sys.stderr)
