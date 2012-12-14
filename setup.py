# encoding: utf-8
#import setuptools # for bdist_egg and console_scripts entry point
from setuptools import setup,Extension
#import distutils.command.install_scripts
#import distutils.command.sdist
#import distutils.command.build_ext
import os.path, os, shutil, re, subprocess, sys
from glob import glob
from os.path import sep,join,basename,dirname

DISTBUILD=None # set to None if building locally, or to a string when dist-building on a bot
if 'DEB_BUILD_ARCH' in os.environ: DISTBUILD='debian'
WIN=(sys.platform=='win32')

if not DISTBUILD: # don't do parallel at buildbot
	# monkey-patch for parallel compilation
	def parallelCCompile(self, sources, output_dir=None, macros=None, include_dirs=None, debug=0, extra_preargs=None, extra_postargs=None, depends=None):
		# those lines are copied from distutils.ccompiler.CCompiler directly
		macros, objects, extra_postargs, pp_opts, build = self._setup_compile(output_dir, macros, include_dirs, sources, depends, extra_postargs)
		cc_args = self._get_cc_args(pp_opts, debug, extra_preargs)
		# parallel code
		N=4 # number of parallel compilations by default
		import multiprocessing.pool
		def _single_compile(obj):
			try: src, ext = build[obj]
			except KeyError: return
			print obj
			self._compile(obj, src, ext, cc_args, extra_postargs, pp_opts)
		# convert to list, imap is evaluated on-demand
		list(multiprocessing.pool.ThreadPool(N).imap(_single_compile,objects))
		return objects
	import distutils.ccompiler
	distutils.ccompiler.CCompiler.compile=parallelCCompile

pathSourceTree=join('build-src-tree')
pathSources=join(pathSourceTree,'src')
pathScripts=join(pathSourceTree,'scripts')
pathHeaders=join(pathSourceTree,'woo')

## get version info
version=None
revno=None
# on debian, get version from changelog
if DISTBUILD=='debian':
	version=re.match(r'^[^(]* \(([^)]+)\).*$',open('debian/changelog').readlines()[0]).group(1)
	print 'Debian version from changelog: ',version
	revno='debian'
# get version from queryling local bzr repo
if not version and os.path.exists('.bzr'):
	try:
		# http://stackoverflow.com/questions/3630893/determining-the-bazaar-version-number-from-python-without-calling-bzr
		from bzrlib.branch import BzrBranch
		branch = BzrBranch.open_containing('.')[0]
		revno=str(branch.last_revision_info()[0])
	except:
		revno='na'
	version='0.99+r'+revno
	

##
## build options
##
features=['qt4','vtk','opengl','gts','openmp']
if 'CC' in os.environ and os.environ['CC'].endswith('clang'): features.remove('openmp')
flavor='' #('' if WIN else 'distutils')
debug=False
chunkSize=(1 if WIN else 10)
hotCxx=[] # plugins to be compiled separately despite chunkSize>1

## arch-specific optimizations
march='corei7' if WIN else 'native'
# lower, but at least some machine-specific optimizations
# FIXME: code will fail to execute on older CPUs
if DISTBUILD: march='core2' 

##
## end build options
##
if DISTBUILD=='debian':
	chunkSize=1 # be nice to the builder at launchpad
	features+=['noxml'] # this should cut to half RAM used by boost::serialization templates at compile-time


cxxFlavor=('_'+re.sub('[^a-zA-Z0-9_]','_',flavor) if flavor else '')
execFlavor=('-'+flavor) if flavor else ''
cxxInternalModule='_cxxInternal%s%s'%(cxxFlavor,'_debug' if debug else '')

if 'opengl' in features and 'qt4' not in features: raise ValueError("The 'opengl' features is only meaningful in conjunction with 'qt4'.")


#
# install headers and source (in chunks)
#
def wooPrepareHeaders():
	'Copy headers to build-src-tree/woo/ subdirectory'
	if not os.path.exists(pathHeaders): os.makedirs(pathHeaders)
	hpps=sum([glob(pat) for pat in ('lib/*/*.hpp','lib/multimethods/loki/*.h','core/*.hpp','pkg/*/*.hpp','pkg/*/*.hpp')],[])
	for hpp in hpps:
		d=join(pathHeaders,dirname(hpp))
		if not os.path.exists(d): os.makedirs(d)
		#print hpp,d
		shutil.copyfile(hpp,join(pathHeaders,hpp))
def wooPrepareChunks():
	'Make chunks from sources, and install those files to build-src-tree'
	# make chunks from sources
	global chunkSize
	if chunkSize<0: chunkSize=10000
	srcs=[glob('lib/*/*.cpp'),glob('core/*.cpp'),glob('py/*.cpp'),glob('py/*/*.cpp')]
	if WIN: srcs=[[s] for s in sum(srcs,[])] # compile each file separately even amongst base files
	if 'opengl' in features: srcs+=[glob('gui/qt4/*.cpp')+glob('gui/qt4/*.cc')]
	if 'gts' in features: srcs+=[[f] for f in glob('py/3rd-party/pygts-0.3.1/*.c')]
	pkg=glob('pkg/*.cpp')+glob('pkg/*/*.cpp')+glob('pkg/*/*/*.cpp')
	# print srcs,pkg
	for i in range(0,len(pkg),chunkSize): srcs.append(pkg[i:i+chunkSize])
	hot=[]
	for i in range(len(srcs)):
		hot+=[s for s in srcs[i] if basename(s)[:-4] in hotCxx]
		srcs[i]=[s for s in srcs[i] if basename(s)[:-4] not in hotCxx]
	srcs+=[[h] for h in hot] # add as single files
	#print srcs
	# check hash
	import hashlib; h=hashlib.new('sha1'); h.update(str(srcs))
	# exactly the same configuration does not have to be repeated again
	if not os.path.exists(join(pathSources,h.hexdigest())):
		if os.path.exists(pathSources): shutil.rmtree(pathSources)
		os.mkdir(pathSources)
		open(join(pathSources,h.hexdigest()),'w')
		#print srcs
		for i,src in enumerate(srcs):
			if len(src)==0: continue
			ext=('c' if src[0].split('.')[-1]=='c' else 'cpp')
			nameNoExt='' if len(src)>1 else '-'+basename(src[0][:-len(src[0].split('.')[-1])-1])
			f=open(join(pathSources,('chunk-%02d%s.%s'%(i,nameNoExt,ext))),'w')
			for s in src:
				f.write('#include"../%s"\n'%s) # build-src-tree
def wooPrepareQt4():
	'Generate Qt4 files (normally handled by scons); those are only neede with OpenGL'
	global features
	if 'qt4' not in features: return
	rccInOut=[('gui/qt4/img.qrc','gui/qt4/img_rc.py')]
	uicInOut=[('gui/qt4/controller.ui','gui/qt4/ui_controller.py')]
	mocInOut=[
		('gui/qt4/GLViewer.hpp','gui/qt4/moc_GLViewer.cc'),
		('gui/qt4/OpenGLManager.hpp','gui/qt4/moc_OpenGLManager.cc')
	]
	if WIN:
		# this is ugly
		# pyuic is a batch file, which is not runnable from mingw shell directly
		# find the real exacutable then
		import PyQt4.uic
		pyuic4=['python',PyQt4.uic.__file__[:-12]+'pyuic.py']
	else:
		pyuic4=['pyuic4']
	for tool,opts,inOut,enabled in [(['pyrcc4'],[],rccInOut,True),(pyuic4,[],uicInOut,True),(['moc'],['-DWOO_OPENGL','-DWOO_QT4'],mocInOut,('opengl' in features))]:
		if not enabled: continue
		for fIn,fOut in inOut:
			cmd=tool+opts+[fIn,'-o',fOut]
			print ' '.join(cmd)
			status=subprocess.call(cmd)
			if status: raise RuntimeError("Error %d returned when running %s"%(status,' '.join(cmd)))
			if not os.path.exists(fOut): RuntimeError("No output file (though exit status was zero): %s"%(' '.join(cmd)))
def pkgconfig(packages):
	flag_map={'-I':'include_dirs','-L':'library_dirs','-l':'libraries'}
	ret={'library_dirs':[],'include_dirs':[],'libraries':[]}
	for token in subprocess.check_output("pkg-config --libs --cflags %s"%' '.join(packages),shell=True).split():
		if flag_map.has_key(token[:2]): ret.setdefault(flag_map.get(token[:2]),[]).append(token[2:])
		# throw others to extra_link_args
		else: ret.setdefault('extra_link_args',[]).append(token)
	# remove duplicated
	for k,v in ret.iteritems(): ret[k]=list(set(v))
	return ret

# if the following file is missing, we are being run from sdist, which has tree already prepared
# otherwise, install headers, chunks and scripts where they should be
if os.path.exists('examples'):
	wooPrepareQt4()
	wooPrepareHeaders()
	wooPrepareChunks()
# files are in chunks
cxxSrcs=['py/config.cxx']+glob(join(pathSources,'*.cpp'))+glob(join(pathSources,'*.c'))

###
### preprocessor, compiler, linker flags
###
cppDirs,cppDef,cxxFlags,cxxLibs,linkFlags,libDirs=[],[],[],[],[],[]
##
## general
##
cppDef+=[
	('WOO_REVISION',revno),
	('WOO_VERSION',version),
	('WOO_SOURCE_ROOT','' if DISTBUILD else dirname(os.path.abspath(__file__)).replace('\\','/')),
	('WOO_FLAVOR',flavor),
	('WOO_CXX_FLAVOR',cxxFlavor),
]
cppDef+=[('WOO_'+feature.upper().replace('-','_'),None) for feature in features]
cppDirs+=[pathSourceTree]

cxxFlags+=['-Wall','-fvisibility=hidden','-std=c++11','-pipe']

cxxLibs+=['m',
	'boost_python',
	'boost_system',
	'boost_thread',
	'boost_date_time',
	'boost_filesystem',
	'boost_iostreams',
	'boost_regex',
	'boost_serialization',
	'boost_chrono']
##
## Platform-specific
##
if DISTBUILD:
	# this would be nice, but gcc at launchpad ICEs with that
	# cxxFlags+=['-ftime-report','-fmem-report','-fpre-ipa-mem-report','-fpost-ipa-mem-report']
	pass
if WIN:
	cppDirs+=['c:/MinGW64/include','c:/MinGW64/include/eigen3','c:/MinGW64/include/boost-1_51']
	# avoid warnings from other headers
	# avoid hitting section limit by inlining
	cxxFlags+=['-Wno-strict-aliasing','-Wno-attributes','-finline-functions'] 	
	boostTag='-mgw47-mt-1_51'
	cxxLibs=[(lib+boostTag if lib.startswith('boost_') else lib) for lib in cxxLibs]
else:
	cppDirs+=['/usr/include/eigen3']
	
##
## Debug-specific
##
if debug:
	cppDef+=[('WOO_DEBUG',None),('WOO_CAST','dynamic_cast'),('WOO_PTR_CAST','dynamic_pointer_cast')]
	cxxFlags+=['-Os']
else:
	cppDef+=[('WOO_CAST','static_cast'),('WOO_PTR_CAST','static_pointer_cast'),('NDEBUG',None)]
	cxxFlags+=['-g0','-O3']
	if march: cxxFlags+=['-march=%s'%march]
	linkFlags+=['-Wl,--strip-all']
##
## Feature-specific
##
if 'openmp' in features:
	cxxLibs.append('gomp')
	cxxFlags.append('-fopenmp')
if 'opengl' in features:
	if WIN: cxxLibs+=['opengl32','glu32','glut','gle','QGLViewer2']
	else: cxxLibs+=['GL','GLU','glut','gle','qglviewer-qt4']
	# qt4 without OpenGL is pure python and needs no additional compile options
	if ('qt4' in features):
		cppDef+=[('QT_CORE_LIB',None),('QT_GUI_LIB',None),('QT_OPENGL_LIB',None),('QT_SHARED',None)]
		if WIN:
			cppDirs+=['c:/MinGW64/include/'+component for component in ('QtCore','QtGui','QtOpenGL','QtXml')]
			cxxLibs+=['QtCore4','QtGui4','QtOpenGL4','QtXml4']
		else: cppDirs+=['/usr/include/qt4']+['/usr/include/qt4/'+component for component in ('QtCore','QtGui','QtOpenGL','QtXml')]
		# cxxLibs+=['QtGui4','QtCore4','QtOpenGL4','
if 'vtk' in features:
	cxxLibs+=['vtkCommon','vtkHybrid','vtkRendering','vtkIO','vtkFiltering']
	if WIN:
		libDirs+=glob('c:/MinGW64/lib/vtk-*')
		cxxLibs+=['vtksys']
	vtks=(glob('/usr/include/vtk-*') if not WIN else glob('c:/MinGW64/include/vtk-*'))
	if not vtks: raise ValueError("No header directory for VTK detected.")
	elif len(vtks)>1: raise ValueError("Multiple header directories for VTK detected: "%','.join(vtks))
	cppDirs+=[vtks[0]]
if 'gts' in features:
	c=pkgconfig(['gts'])
	cxxLibs+=['gts']+c['libraries']
	cppDirs+=c['include_dirs']
	libDirs+=c['library_dirs']

wooModules=['woo.'+basename(py)[:-3] for py in glob('py/*.py') if basename(py)!='__init__.py']

# compiler-specific flags, if ever needed:
# 	http://stackoverflow.com/a/5192738/761090
#class WooBuildExt(distutils.command.build_ext.build_ext):
#	def build_extensions(self):
#		c=self.compiler.compiler_type
#		if re.match(r'.*(gcc|g\+\+)^'):
#			for e in self.extensions:
#				e.extra_compile_args=['-fopenmp']
#				e.extra_link_args=['-lgomp']

setup(name='woo',
	version=version,
	author='Václav Šmilauer',
	author_email='eu@doxos.eu',
	url='http://www.woodem.eu',
	description='Discrete dynamic compuations, especially granular mechanics.',
	long_description='''Extesible and portable framework primarily for mechanics
granular materials. Computation parts are written in c++ parallelized using
OpenMP, fully accessible and modifiable from python (ipython console or
scripts). Arbitrarily complex scene can be scripted. Qt-based user interface
is provided, featuring flexible OpenGL display, inspection of all objects
and runtime modification. Parametric preprocessors can be written in pure
python, and batch system can be used to drive parametric studies. New
material models and particle shapes can be added (in c++) without modifying
existing classes.
	
Woo is an evolution of the Yade package
(http://www.launchpad.net/yade), aiming at more flexibility, extensibility,
tighter integration with python and user-friendliness.
	''',
	classifiers=[
		'License :: OSI Approved :: GNU General Public License v2 or later (GPLv2+)',
		'Programming Language :: C++',
		'Programming Language :: Python',
		'Operating System :: POSIX',
		'Operating System :: Microsoft :: Windows',
		'Topic :: Scientific/Engineering :: Mathematics',
		'Intended Audience :: Science/Research',
		'Development Status :: 4 - Beta'
	],
	# '' must use join(...) to use native separator
	# otherwise scripts don't get installed!
	# http://stackoverflow.com/questions/13271085/console-scripts-entry-point-ignored
	package_dir={'woo':'py','':join('core','main'),'woo.qt':'gui/qt4','woo.pre':'py/pre','woo.gts':'py/3rd-party/pygts-0.3.1'},
	packages=(
		['woo','woo._monkey','woo.tests','woo.pre']
		+(['woo.qt'] if 'qt4' in features else [])
		+(['woo.gts'] if 'gts' in features else [])
	),
	py_modules=wooModules+['wooOptions','wooMain'],
		ext_modules=[
		Extension('woo.'+cxxInternalModule,
			sources=cxxSrcs,
			include_dirs=cppDirs,
			define_macros=cppDef,
			extra_compile_args=cxxFlags,
			libraries=cxxLibs,
			library_dirs=libDirs,
			extra_link_args=linkFlags,
		),
	],
	# works under windows as well now
	# http://stackoverflow.com/questions/13271085/console-scripts-entry-point-ignored
	entry_points={
		'console_scripts':[
			'%swoo%s = wooMain:main'%('w' if WIN else '',execFlavor),
			'%swoo%s_batch = wooMain:batch'%('w' if WIN else '',execFlavor),
		],
	},
	# woo.__init__ makes symlinks to _cxxInternal, which would not be possible if zipped
	# see http://stackoverflow.com/a/10618900/761090
	zip_safe=False, 
)

