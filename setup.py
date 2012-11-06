# encoding: utf-8
import setuptools # for bdist_egg and console_scripts entry point
from distutils.core import setup,Extension
import distutils.command.install_scripts
import distutils.command.sdist
import distutils.command.build_ext
import os.path, os, shutil, re, subprocess, sys
from glob import glob
from os.path import sep,join,basename,dirname

if 1:
	# monkey-patch for parallel compilation
	def parallelCCompile(self, sources, output_dir=None, macros=None, include_dirs=None, debug=0, extra_preargs=None, extra_postargs=None, depends=None):
		# those lines are copied from distutils.ccompiler.CCompiler directly
		macros, objects, extra_postargs, pp_opts, build = self._setup_compile(output_dir, macros, include_dirs, sources, depends, extra_postargs)
		cc_args = self._get_cc_args(pp_opts, debug, extra_preargs)
		# parallel code
		N=3 # number of parallel compilations
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
pathHeaders=join(pathSourceTree,'woo')

## get version info
version=None
revno=None
if not version:
	try:
		# http://stackoverflow.com/questions/3630893/determining-the-bazaar-version-number-from-python-without-calling-bzr
		from bzrlib.branch import BzrBranch
		branch = BzrBranch.open_containing('.')[0]
		revno=str(branch.last_revision_info()[0])
	except:
		revno='na'
	version='0.99+r'+revno
## build options
features=([] if ('CC' in os.environ and os.environ['CC'].endswith('clang')) else ['openmp'])
features+=['qt4','vtk','opengl'] #,'opengl'] 
##
import sys
WIN=(sys.platform=='win32')
##
if WIN:
	features=[]
	flavor=''
	debug=True
else:
	flavor='distutils'
	debug=False
cxxFlavor=('_'+re.sub('[^a-zA-Z0-9_]','_',flavor) if flavor else '')
execFlavor=('-'+flavor) if flavor else ''
cxxInternalModule='_cxxInternal%s%s'%(cxxFlavor,'_debug' if debug else '')
chunkSize=(10 if WIN else 1)
hotCxx=['Factory']

if 'opengl' in features and 'qt4' not in features: raise ValueError("The 'opengl' features is only meaningful in conjunction with 'qt4'.")


#
# isntall headers and source (in chunks)
#
def wooPrepareHeaders():
	'Copy headers to build-src-tree/woo/ subdirectory'
	if not os.path.exists(pathHeaders): os.makedirs(pathHeaders)
	hpps=sum([glob(pat) for pat in ('lib/*/*.hpp','core/*.hpp','pkg/*/*.hpp','pkg/*/*.hpp')],[])
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
	#if WIN: srcs=[[s] for s in sum(srcs,[]) if '_utils2.cpp' not in s]
	if 'opengl' in features: srcs+=[glob('gui/qt4/*.cpp')+glob('gui/qt4/*.cc')]
	pkg=glob('pkg/*.cpp')+glob('pkg/*/*.cpp')+glob('pkg/*/*/*.cpp')
	#if WIN: pkg=[]
	#print srcs,pkg
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
			f=open(join(pathSources,('chunk-%02d%s.%s'%(i,'' if len(src)>1 else ('-'+basename(src[0][:-4])),'cpp'))),'w')
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
	if 'opengl' not in features:
		for __,out in mocInOut:
			if os.path.exists(out): os.unlink(out) # delete files which would be useless
		mocInOut=[]
	for tool,opts,inOut in [('pyrcc4',[],rccInOut),('pyuic4',[],uicInOut),('moc',['-DWOO_OPENGL','-DWOO_QT4'],mocInOut)]:
		for fIn,fOut in inOut:
			cmd=[tool]+opts+[fIn,'-o',fOut]
			print ' '.join(cmd)
			status=subprocess.call(cmd)
			if status: raise RuntimeError("Error %d returned when running %s"%(status,' '.join(cmd)))
			if not os.path.exists(fOut): RuntimeError("No output file (though exit status was zero): %s"%(' '.join(cmd)))

# if the following file is missing, we are being run from sdist, which has tree already prepared
# otherwise, install headers and chunks where they should be
if os.path.exists('examples'):
	wooPrepareQt4()
	wooPrepareHeaders()
	wooPrepareChunks()

# files are in chunks
cxxSrcs=['py/config.cxx']+glob(join(pathSources,'*.cpp'))+glob(join(pathSources,'*.c'))

#
# preprocessor, compiler, linker flags
#
cppDirs,cppDef,cxxFlags,linkFlags,libDirs=[],[],[],[],[]
cxxFlags+=['-Wall','-fvisibility=hidden','-std=c++11']
cppDef+=[
	('WOO_REVISION',revno),
	('WOO_VERSION',version),
	('WOO_SOURCE_ROOT',dirname(__file__)),
	('WOO_FLAVOR',flavor),
	('WOO_CXX_FLAVOR',cxxFlavor),
]
cppDef+=[('WOO_'+feature.upper().replace('-','_'),None) for feature in features]
cppDirs+=[pathSourceTree]+['/usr/include/eigen3']
if debug:
	cppDef+=[('WOO_DEBUG',None),('WOO_CAST','dynamic_cast'),('WOO_PTR_CAST','dynamic_pointer_cast')]
	cxxFlags+=['-g']
else:
	cppDef+=[('WOO_CAST','static_cast'),('WOO_PTR_CAST','static_pointer_cast'),('NDEBUG',None)]
	cxxFlags+=['-O2']+([] if WIN else ['-march=native'])
	linkFlags+=['-Wl,--strip-all']

cxxLibs=['m',
	'boost_python',
	'boost_system',
	'boost_thread',
	'boost_date_time',
	'boost_filesystem',
	'boost_iostreams',
	'boost_regex',
	'boost_serialization'
]+(['ws2_32'] # boost::asio::ip on Windows
	if WIN else
	['rt']) # realtime clock 
if WIN:
	boostSuffix='-mgw47-mt-1_51'
	cxxLibs=[(lib+boostSuffix if lib.startswith('boost_') else lib) for lib in cxxLibs]
if 'openmp' in features:
	cxxLibs.append('gomp')
	cxxFlags.append('-fopenmp')
if 'opengl' in features:
	cxxLibs+=['GL','GLU','glut','gle','qglviewer-qt4']
	# qt4 without OpenGL is pure python and needs no additional compile options
	if ('qt4' in features):
		cppDef+=[('QT_CORE_LIB',None),('QT_GUI_LIB',None),('QT_OPENGL_LIB',None),('QT_SHARED',None)]
		cppDirs+=['/usr/include/qt4']
		cppDirs+=['/usr/include/qt4/'+component for component in ('QtCore','QtGui','QtOpenGL','QtXml')]
if 'vtk' in features:
	cxxLibs+=['vtkCommon','vtkHybrid','vtkRendering','vtkIO','vtkFiltering']
	vtks=glob('/usr/include/vtk-*')
	if not vtks: raise ValueError("No header directory for VTK detected.")
	elif len(vtks)>1: raise ValueError("Multiple header directories for VTK detected: "%','.join(vtks))
	cppDirs+=[vtks[0]]
if WIN:
	cppDirs+=[r'c:\src\boost_1_51_0',r'c:\src\eigen-3.1.1',r'c:\src\loki-0.1.7\include']
	libDirs+=[r'c:\src\boost_1_51_0\stage\lib']
	cxxFlags+=['-Wno-strict-aliasing','-Wno-attributes'] # warnings from other headers # -g0 for less debugging info?
	# see http://stackoverflow.com/questions/4702732/the-program-cant-start-because-libgcc-s-dw2-1-dll-is-missing for discussion of this
	# does not work anyway
	# linkFlags+=['-static-libgcc','-static-libstdc++']
	cppDef+=[('EIGEN_DONT_VECTORIZE',None)] # this might be the cause for "Invalid access to memory location"?
else:
	cxxFlags+=['-pipe']

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
	description='Platform for discree dynamic compuations, especially granular mechanics.',
	long_description='''TODO''',
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
	package_dir={'woo':'py','':'core/main','woo.qt':'gui/qt4','woo.pre':'pkg/pre','woo.gts':'py/3rd-party/pygts-0.3.1'},
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
	entry_points={
		'console_scripts':[
			'woo%s = wooMain:main'%execFlavor,
			'woo%s-batch = wooMain:batch'%execFlavor,
		]
	},
	# woo.__init__ makes symlinks to _cxxInternal
	# see http://stackoverflow.com/a/10618900/761090
	zip_safe=False, 
)

