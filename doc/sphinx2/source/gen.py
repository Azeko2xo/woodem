import woo.document
# import all modules here
from woo import utils,log,timing,pack,document,manpage,plot,post2d,runtime,ymport,WeightedAverage2d
import minieigen,re,sys,sphinx,os,os.path
if not '--only-extras' in sys.argv:
	rsts=woo.document.allWooPackages('.')
	print '*** RST:',rsts
	wooMods='wooMods.rst'
	with open(wooMods,'w') as f:
		f.write('Woo modules\n######################\n\n')
		f.write('.. toctree::\n\n')
		for o in sorted(rsts):
			print 'USING',o
			if re.match('(^|.*/)wooExtra(\..*)?$',o):
				print '[SKIPPED]'
				continue
			f.write('    %s\n'%o)
	if 1:
		fmt='html'
		sphinx.main(['','-P','-b',fmt,'-d','../build/doctrees','../source','../build/%s'%fmt])

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
		f.write('.. note:: This page is not `documentation of Woo itself <http://www.woodem.eu/doc>`_, only of an extra module.\n\n')
		f.write('%s module\n################################\n\n'%mName)
		woo.document.oneModuleWithSubmodules(mod,f)
	# copy config over
	confName=srcDir+'/conf.py'
	shutil.copyfile('conf.py',confName) 
	## copy package resources to the source directory
	for R in ('resources','data'): # future-proof :)
		resDir=pkg_resources.resource_filename(mName,R)
		print srcDir,resDir,R
		if os.path.exists(resDir) and not os.path.exists(srcDir+'/'+R): os.symlink(resDir,srcDir+'/'+R)
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
extensions=[e for e in extensions if e!='sphinx.ext.viewcode'] # don't show code in extras, stragely wooExtra.* is included, not just the one particular extra module
		""".format(
			version=pkg_resources.get_distribution(mName).version,
			mName=mName,
			copyright=re.sub('<[^<]+?>','',mod.distributor.replace('<br>',', '))
		))
	if 1:
		sphinx.main(['','-P','-b','html','-d','../build-doctress',srcDir,outDir])

		

