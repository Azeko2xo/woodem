# encoding: utf-8
from distutils.core import setup,Extension
import glob, os.path


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
	version='r'+revno
## build options
features=[]
flavor=''
debug=False
cxxFlavor=('_'+re.sub('[^a-zA-Z0-9_]','_',flavor) if flavor else '')
execFlavor=('-'+flavor) if flavor else ''
cxxInternalModule='_cxxInternal%s%s'%(cxxFlavor,'_debug' if debug else '')
chunkSize=10
hotCxx=[]


#
# isntall headers and source (in chunks)
#
def wooPrepareHeaders():
	'''copy headers to woo/ subdirectory'''
	import glob, shutil, os
	hpps=sum([glob.glob(pat) for pat in ('lib/*/*.hpp','core/*.hpp','pkg/*/*.hpp','pkg/*/*.hpp')],[])
	for hpp in hpps:
		d=os.path.join('woo',os.path.dirname(hpp))
		if not os.path.exists(d): os.makedirs(d)
		#print hpp,os.path.join('woo',hpp),d
		shutil.copyfile(hpp,os.path.join('woo',hpp))
def wooPrepareChunks():
	# make chunks from sources
	global chunkSize
	if chunkSize<0: chunkSize=10000
	srcs=[glob.glob('lib/*/*.cpp'),glob.glob('core/*.cpp'),glob.glob('py/*.cpp')+glob.glob('py/*/*.cpp')]
	pkg=glob.glob('pkg/*.cpp')+glob.glob('pkg/*/*.cpp')+glob.glob('pkg/*/*/*.cpp')
	for i in range(0,len(pkg),chunkSize): srcs.append(pkg[i:i+chunkSize])
	hot=[]
	for i in range(len(srcs)):
		srcs[i]=[s for s in srcs[i] if os.path.basename(s)[:-4] not in hotCxx]
		hot+=[s for s in srcs[i] if os.path.basename(s)[:-4] in hotCxx]
	srcs+=hot # add as single files
	#print srcs
	# check hash
	import hashlib; h=hashlib.new('sha1'); h.update(str(srcs))
	# exactly the same configuration does not have to be repeated again
	if not os.path.exists(os.path.join('src',h.hexdigest())):
		if os.path.exists('src'): shutil.rmtree('src')
		os.mkdir('src')
		open(os.path.join('src',h.hexdigest()),'w')
		for i,src in enumerate(srcs):
			f=open(os.path.join('src',('chunk-%02d%s.cpp'%(i,'' if len(src)>1 else os.path.basename(src[0][:-4])))),'w')
			for s in src:
				print f.name,s
				f.write('#include"../%s"\n'%s)

# if MANIFEST.in is missing, we are being run from sdist, which has tree already prepared
# otherwise, install headers and chunks where they should be
if os.path.exists('MANIFEST.in'):
	wooPrepareHeaders()
	wooPrepareChunks()


#
cxxSrcs=['py/config.cxx']
#rawSrcs=sum([glob.glob(pattern) for pattern in ('lib/*/*.cpp','core/*.cpp','py/*.cpp','py/*/*.cpp','pkg/*.cpp','pkg/*/*.cpp','pkg/*/*/*.cpp')],[])
#if chunkSize==1: cxxSrcs+=rawSrcs
#else:
# those are created in WooSdist.get_file_list
cxxSrcs+=glob.glob('src/*.cpp')

#
# preprocessor, compiler, linker flags
#
cppDef,cxxFlags,linkFlags=[],[],[]
cxxFlags+=['-pipe','-Wall','-fvisibility=hidden','-std=c++11']
cppDef+=[
	#('EIGEN_DONT_ALIGN',None),
	('WOO_REVISION',revno),
	('WOO_VERSION',version),
	('WOO_SOURCE_ROOT',os.path.dirname(__file__)),
	('WOO_FLAVOR',flavor),
	('WOO_CXX_FLAVOR',cxxFlavor),
]
cppDef+=[('WOO_'+feature.upper().replace('-','_'),None) for feature in features]
if debug:
	cppDef+=[('WOO_DEBUG',None),('WOO_CAST','dynamic_cast'),('WOO_PTR_CAST','dynamic_pointer_cast')]
	linkFlags+=['-Wl,--strip-all']
	cxxFlags+=['-ggdb2']
else:
	cppDef+=[('WOO_CAST','static_cast'),('WOO_PTR_CAST','static_pointer_cast'),('NDEBUG',None)]
	cxxFlags+=['-O2','-march=native']

cxxLibs=['boost_python','dl','m','rt']
if 'openmp' in features:
	cxxLibs.append('gomp')
	cxxFlags.append('-fompenmp')
if 'gts' in features: cppDef.append(('PYGTS_HAS_NUMPY',None))

wooModules=['woo.'+os.path.basename(py)[:-3] for py in glob.glob('py/*.py') if os.path.basename(py)!='__init__.py']

## HACK: install executable scripts
## http://stackoverflow.com/a/11400431/761090
import distutils.command.install_scripts
import shutil
class WooExecInstall(distutils.command.install_scripts.install_scripts):
	def run(self):
		distutils.command.install_scripts.install_scripts.run(self)
		for script in self.get_outputs():
			if os.path.basename(script)=='main.py': shutil.move(script,os.path.join(os.path.dirname(script),'woo'+execFlavor))
			elif os.path.basename(script)=='batch.py': shutil.move(script,os.path.join(os.path.dirname(script),'woo'+execFlavor+'-batch'))
			else: raise ValueError("WooExecInstall only handles main.py and batch.py, not %s."%script)
import distutils.command.sdist
class WooSdist(distutils.command.sdist.sdist):
	def get_file_list(self):
		wooPrepareHeaders()
		wooPrepareChunks()
		return distutils.command.sdist.sdist.get_file_list(self)

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
	package_dir={'woo':'py','':'core/main'},
	packages=['woo'],
	py_modules=wooModules+['wooOptions'],
	ext_modules=[Extension('woo.'+cxxInternalModule,
		sources=cxxSrcs,
		include_dirs=['/usr/include/eigen3','.'],
		define_macros=cppDef,
		extra_compile_args=cxxFlags,
		libraries=cxxLibs,
		extra_link_args=linkFlags,
	)],
	#headers=sum([glob.glob(pattern) for pattern in ('lib/*/*.hpp','core/*.hpp','py/*.hpp','pkg/*.hpp','pkg/*/*.hpp','pkg/*/*/*.hpp')],[]),
	scripts=['core/main/main.py','core/main/batch.py'],
	cmdclass={'install_scripts':WooExecInstall,'sdist':WooSdist},
	#install_requires=['distribute'],
)

