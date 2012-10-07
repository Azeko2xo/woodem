# vim: set filetype=python:
Import('*')

import os.path

pyModules=[]
pyObjects=[]
pySharedObject='$LIBDIR/libEverything.so'

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

SConscript(dirs=['resources'],duplicate=0)

#
# LIB (support)
#
pyObjects.append(
	env.SharedObject('woo-support',
		## ['voro++/voro++.cc']+ # included directly in sources, due to templates
		env.Combine('woo-support.cpp',env.Glob('lib/*/*.cpp'))
	)
)

pyModules.append('woo.boot')
pyObjects.append(env.SharedObject('boot',['core/main/pyboot.cpp']))

pyObjects.append(env.SharedObject('core',env.Combine('core.cpp',env.Glob('core/*.cpp'))))


pyObjects+=[env.SharedObject(f) for f in env.Glob('py/*.cpp')+env.Glob('py/*/*.cpp')]
pyModules+=['woo.WeightedAverage2d','woo.log','woo._utils2','woo._packPredicates','woo._packSpheres','woo._packObb','woo._customConverters']

if 'qt4' in env['features']:
	pyModules.append('woo.qt._GLViewer')
	pyObjects+=['gui/qt4/GLViewer.cpp','gui/qt4/_GLViewer.cpp','gui/qt4/OpenGLManager.cpp']

if 'gts' in env['features']:
	pyModules.append('gts._gts')
	pyObjects+=[env.SharedObject(f) for f in env.Glob('py/3rd-party/pygts-0.3.1/*.c')]
	env.Install('$LIBDIR/py/gts',[
		env.File('py/3rd-party/pygts-0.3.1/__init__.py'),
		env.File('py/3rd-party/pygts-0.3.1/pygts.py')
	])

#
# install shared lib
#
env.Install(
env.SharedLibrary(pySharedObject,pyObjects,
	LIBS=['dl','m','rt']+[l for l in env['LIBS'] if l!='woo-support']+(['glut','GL','GLU']+[env['QGLVIEWER_LIB']] if 'opengl' in env['features'] else []),CXXFLAGS=env['CXXFLAGS']+['-fPIC']
))
#
# link python modules
#
for name in pyModules:
	# link python module
	f='$LIBDIR/py/'+'/'.join(name.split('.'))+'.so'
	rp=os.path.relpath(pySharedObject,os.path.dirname(f))
	ff=env.subst(f)
	if os.path.exists(ff) and os.path.islink(ff) and os.path.realpath(ff)==os.path.realpath(env.subst(pySharedObject)): continue
	env.Command(f,pySharedObject,'ln -s -f "%s" $TARGET'%rp)

#
# SCRIPTS HERE
#
pyMain='$PREFIX/bin/woo$SUFFIX'
main=env.ScanReplace('core/main/main.py.in')
batch=env.ScanReplace('core/main/batch.py.in')
env.AlwaysBuild(main)
env.AlwaysBuild(batch)
env.InstallAs(pyMain,main)
env.InstallAs(pyMain+'-batch',batch)
env.AddPostAction(pyMain,Chmod(pyMain,0755))
env.AddPostAction(pyMain+'-batch',Chmod(pyMain+'-batch',0755))

env.Install('$LIBDIR/py','core/main/wooOptions.py')
## for --rebuild
if 'execCheck' in env and env['execCheck']!=os.path.abspath(env.subst(pyMain)):
	raise RuntimeError('execCheck option (%s) does not match what is about to be installed (%s)'%(env['execCheck'],env.subst(pyMain)))

#install preprocessor scripts
env.Install('$LIBDIR/py/woo/pre',
	env.File(env.Glob('pkg/pre/*.py'),'pkg/pre'),
)

env.Install('$LIBDIR/py/woo',[
	env.AlwaysBuild(env.ScanReplace('#/py/__init__.py.in')),
	env.AlwaysBuild(env.ScanReplace('#/py/config.py.in')),
	env.File(env.Glob('#/py/*.py')),
	env.File('#py/pack/pack.py'),
])
env.Install('$LIBDIR/py/woo/tests',[env.File(env.Glob('#/py/tests/*.py'),'tests'),])
env.Install('$LIBDIR/py/woo/_monkey',[env.File(env.Glob('#/py/_monkey/*.py'),'_monkey')])


if 'qt4' in env['features']:
	env.Install('$LIBDIR/py/woo/qt',[
		env.File('gui/qt4/img_rc.py'),
		env.File('gui/qt4/ui_controller.py'),
		env.File('gui/qt4/SerializableEditor.py'),
		env.File('gui/qt4/Inspector.py'),
		env.File('gui/qt4/ExceptionDialog.py'),
		env.File('gui/qt4/__init__.py'),
	])
	env.Command('gui/qt4/img_rc.py','gui/qt4/img.qrc','pyrcc4 -o $buildDir/gui/qt4/img_rc.py gui/qt4/img.qrc')
	env.Command('gui/qt4/ui_controller.py','gui/qt4/controller.ui','pyuic4 -o $buildDir/gui/qt4/ui_controller.py gui/qt4/controller.ui')





