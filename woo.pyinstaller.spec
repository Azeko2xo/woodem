# -*- mode: python -*-

def Entrypoint(dist, group, name, scripts=None, pathex=None, **kw):
	import pkg_resources
	scripts = scripts or []
	pathex = pathex or []
	# get the entry point
	ep = pkg_resources.get_entry_info(dist, group, name)
	# insert path of the egg at the verify front of the search path
	pathex = [ep.dist.location] + pathex
	# script name must not be a valid module name to avoid name clashes on import
	script_path = os.path.join(BUILDPATH, name+'-script.py')
	print "writing script for entry point", dist, group, name
	fp = open(script_path, 'w')
	try:
		print >>fp, "import", ep.module_name
		print >>fp, "%s.%s()" % (ep.module_name, '.'.join(ep.attrs))
	finally:
		fp.close()
	return Analysis(scripts=scripts+[script_path], pathex=pathex, **kw)
	

import woo.pre, pkgutil
wooPreMods=[]
for importer, modname, ispkg in pkgutil.iter_modules(woo.pre.__path__):
	sys.stderr.write('ADDING PREPROCESSOR %s\n'%modname)
	wooPreMods.append('woo.pre.'+modname)
	

main=Entrypoint(dist='woo',group='console_scripts',name='wwoo',
	hiddenimports=['woo._cxxInternal']+wooPreMods,
	excludes=['wooExtra','Tkinter']
)

# hiddenimports already specified for main
batch=Entrypoint(dist='woo',group='console_scripts',name='wwoo_batch',
	excludes=['wooExtra','Tkinter']
)

pyz=PYZ(main.pure)

exeMain=EXE(pyz,
	main.scripts,
	exclude_binaries=1,
	name=os.path.join('build\\pyi.win32\\wwoo', 'wwoo.exe'),
	debug=False,
	strip=None,
	upx=False,
	console=True)

exeBatch=EXE(pyz,
	batch.scripts,
	exclude_binaries=1,
	name=os.path.join('build\\pyi.win32\\wwoo', 'wwoo_batch.exe'),
	debug=False,
	strip=None,
	upx=False,
	console=True)
	
import glob, sys
# f[3:] strips leading py/ from filename
resources=[(f[3:],f,'DATA') for f in glob.glob('py/resources/*')]
sys.stderr.write('RESOURCES: '+str(resources)+'\n')
	
import ctypes
bits=str(ctypes.sizeof(ctypes.c_voidp)*8)

coll=COLLECT(
	exeMain,
	exeBatch,
	main.binaries,
	main.zipfiles,
	main.datas,
	resources,
	strip=None,
	upx=False,
	name=os.path.join('dist', 'wwoo-win%s'%bits),
)
