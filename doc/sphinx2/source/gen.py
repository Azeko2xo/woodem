import woo.document
# import all modules here
from woo import utils,log,timing,pack,document,manpage,plot,post2d,runtime,ymport,WeightedAverage2d
import minieigen,re
rsts=woo.document.allWooPackages('.')
wooMods='wooMods.rst'
with open(wooMods,'w') as f:
	f.write('Woo modules\n######################\n\n')
	f.write('.. toctree::\n\n')
	for o in sorted(rsts):
		if re.match('(^|.*/)wooExtra(\..*)?$',o): continue
		f.write('    %s\n'%o)
import sys
import sphinx
sphinx.main(['','-P','-b','html','-d','../build/doctrees','../source','../build/html'])

#
# document extra modules, in separate trees
#
import pkg_resources, shutil,re 
for mName in [m for m in sys.modules if m.startswith('wooExtra.') and len(m.split('.'))==2]:
	mod=sys.modules[mName]
	srcDir='../source-extra/'+mod.KEY
	outDir='../build-extra/'+mod.KEY
	if not os.path.exists(srcDir): os.makedirs(srcDir)
	outName=srcDir+'/index.rst'
	print 'WRITING OUTPUT FOR %s TO %s'%(mName,outName)
	with open(outName,'w') as f:
		f.write('%s module\n################################\n\n'%mName)
		woo.document.oneModuleWithSubmodules(mod,f)
	# copy config over
	confName=srcDir+'/conf.py'
	shutil.copyfile('conf.py',confName) 
	# HACK: change some values in the config
	with open(confName,'a') as conf:
		conf.write("""
project = u'{mName}'
version = u'{version}'
release = u'{version}'
copyright = u'{copyright} (distributor)'
master_doc = u'index'
templates_path=['../../source/_templates'] 
html_static_path=['../../source/_static']
intersphinx_mapping={{'woo':('http://www.woodem.eu/doc',None)}}
		""".format(
			version=pkg_resources.get_distribution(mName).version,
			mName=mName,
			copyright=re.sub('<[^<]+?>','',mod.distributor.replace('<br>',', '))
		))
	sphinx.main(['','-P','-b','html','-d','../build-doctress',srcDir,outDir])

		

