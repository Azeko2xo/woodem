# encoding: utf-8
# vim: set filetype=python:
Import('*')

import os.path,re

pyModules=[]
pyObjects=[]
pySharedObject='$LIBDIR/woo/_cxxInternal%s%s.so'%(env['cxxFlavor'],('_debug' if env['debug'] else ''))

# we need relative paths here, to keep buildDir
pkgSrcs=[os.path.relpath(str(s),env.Dir('#').abspath) for s in env.Glob('pkg/*.cpp')+env.Glob('pkg/*/*.cpp')+env.Glob('pkg/*/*/*.cpp')]
## compile hotPlugs separately
hotPlugins=[s for s in pkgSrcs if os.path.basename(s[:-4]) in env['hotPlugins']]
pyObjects+=[env.SharedObject(s) for s in pkgSrcs if s in hotPlugins]
# and remove them from pkgSrcs
pkgSrcs=[s for s in pkgSrcs if s not in hotPlugins]
#
# handle chunkSize here
chunkSize=env['chunkSize']
if chunkSize==1: pass  # compile each seaparately
if chunkSize<=0 or len(pkgSrcs)<chunkSize: pkgSrcs=[env.CombineWrapper('$buildDir/pkg.cpp',pkgSrcs)] # compile all together
else: # compile by chunks
	# thanks to http://stackoverflow.com/questions/312443/how-do-you-split-a-list-into-evenly-sized-chunks-in-python :
	pkgSrcs=[env.CombineWrapper('$buildDir/pkg-%d.cpp'%j,pkgSrcs[i:i+chunkSize]) for j,i in enumerate(range(0,len(pkgSrcs),chunkSize))]
pyObjects+=[env.SharedObject(s) for s in pkgSrcs]


## LIB
pyObjects.append(
	env.SharedObject('woo-support',
		## ['voro++/voro++.cc']+ # included directly in sources, due to templates
		env.Combine('woo-support.cpp',env.Glob('lib/*/*.cpp'))
	)
)
## CORE
pyObjects.append(env.SharedObject('core',env.Combine('core.cpp',env.Glob('core/*.cpp'))))
## PY
pyObjects+=[env.SharedObject(f) for f in env.Glob('py/*.cpp')+env.Glob('py/*/*.cpp')]
## CONFIG
pyObjects.append(env.SharedObject('config','py/config.cxx',
	CPPDEFINES=env['CPPDEFINES']+[
		('WOO_REVISION',env['realVersion']),
		('WOO_VERSION',env['version']),
		('WOO_SOURCE_ROOT',env['sourceRoot']),
		('WOO_FLAVOR',env['flavor'])
	]
))

if 'qt4' in env['features']:
	pyObjects+=['gui/qt4/GLViewer.cpp','gui/qt4/_GLViewer.cpp','gui/qt4/OpenGLManager.cpp']

if 'gts' in env['features']:
	# HACK HACK HACK!!!
	# this is VERY ugly, but we need resolve those symbols for ourselves
	# so we basically have the same lib twice
	pyObjects+=[env.SharedObject(f) for f in env.Glob('py/3rd-party/pygts-0.3.1/*.c')]
	env.Install('$LIBDIR/woo/gts',[
		env.File('py/3rd-party/pygts-0.3.1/__init__.py'),
		env.File('py/3rd-party/pygts-0.3.1/pygts.py'),
		# this module is faked in py/__init__.py
		# env.SharedLibrary('_gts',env.Glob('py/3rd-party/pygts-0.3.1/*.c'),SHLIBPREFIX='',LIBS=['gts'])
	])


#
# install shared lib
#
env.Install(
env.SharedLibrary(pySharedObject,pyObjects,
	SHLIBPREFIX='',
	LIBS=['dl','m','rt']+env['LIBS']+(['glut','GL','GLU']+[env['QGLVIEWER_LIB']] if 'opengl' in env['features'] else []),CXXFLAGS=env['CXXFLAGS']+['-fPIC']
))

#
# EXECUTABLES HERE
#
pyMain='$EXECDIR/woo'+('-'+env['flavor'] if env['flavor'] else '')
env.InstallAs(pyMain,env.Textfile('main.py','#!/usr/bin/python\nimport wooMain,sys; sys.exit(wooMain.main())\n'))
env.InstallAs(pyMain+'-batch',env.Textfile('batch.py','#!/usr/bin/python\nimport wooMain,sys; sys.exit(wooMain.batch())\n'))
env.AddPostAction(pyMain,Chmod(pyMain,0755))
env.AddPostAction(pyMain+'-batch',Chmod(pyMain+'-batch',0755))

env.Install('$LIBDIR','core/main/wooMain.py')
## for --rebuild
if 'execCheck' in env and env['execCheck']!=os.path.abspath(env.subst(pyMain)):
	raise RuntimeError('execCheck option (%s) does not match what is about to be installed (%s)'%(env['execCheck'],env.subst(pyMain)))


env.Install('$LIBDIR/woo',[
	env.File(env.Glob('py/*.py')),
])
env.Install('$LIBDIR/woo/tests',[env.File(env.Glob('py/tests/*.py'),'tests'),])
env.Install('$LIBDIR/woo/pre',[env.File(env.Glob('py/pre/*.py'),'pre'),])
env.Install('$LIBDIR/woo/_monkey',[env.File(env.Glob('py/_monkey/*.py'),'_monkey')])

# install any locally-linked extra modules
# using their respective setup.py
import subprocess, sys
for s in env.Glob('wooExtra/*/setup.py'):
	s=str(s)
	subprocess.check_call([sys.executable,os.path.abspath(s),'install'],cwd=os.path.dirname(s))

if 'qt4' in env['features']:
	env.Install('$LIBDIR/woo/qt',[
		env.Glob('gui/qt4/*.py'),
		# mention generated files explicitly			
		env.File('gui/qt4/img_rc.py'),
		env.File('gui/qt4/ui_controller.py'),
		#env.File('gui/qt4/SerializableEditor.py'),
		#env.File('gui/qt4/Inspector.py'),
		#env.File('gui/qt4/ExceptionDialog.py'),
		#env.File('gui/qt4/__init__.py'),
	])
	env.Command('gui/qt4/img_rc.py','gui/qt4/img.qrc','pyrcc4 -o $buildDir/gui/qt4/img_rc.py gui/qt4/img.qrc')
	env.Command('gui/qt4/ui_controller.py','gui/qt4/controller.ui','pyuic4 -o $buildDir/gui/qt4/ui_controller.py gui/qt4/controller.ui')

# install .egg-info so that pkg_resources work with woo when installed via scons
# http://svn.python.org/projects/sandbox/trunk/setuptools/doc/formats.txt
env.Install('$LIBDIR',[env.Textfile('woo.egg-info','''
Metadata-Version: 1.1
Name: woo
Version: {version}
Summary: Discrete dynamic compuations, especially granular mechanics.
Home-page: http://www.woodem.eu
Author: Václav Šmilauer
Author-email: eu@doxos.eu
'''.format(version='0.99-'+env['realVersion']))])

# install data files
# accessible through pkg_resources
env.Install('$LIBDIR/woo/data',[env.File(env.Glob('py/data/*'))])



