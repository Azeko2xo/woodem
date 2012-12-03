import woo
import woo.core
import sys
import codecs
import re

def fixDocstring(s):
	s=s.replace(':ref:',':obj:')
	s=re.sub(r'(?<!\\)\$([^\$]+)(?<!\\)\$',r'\ :math:`\1`\ ',s)
	s=re.sub(r'\\\$',r'$',s)
	return s

def packageClasses(outDir='/tmp'):
	'''Generate documentation of packages in the Restructured Text format. Each package is written to file called *out*/.`woo.[package].rst` and list of files created is returned.'''

	def subImport(pkg,exclude=None):
		'Import recursively all subpackages'
		import pkgutil
		for importer, modname, ispkg in pkgutil.iter_modules(pkg.__path__):
			fqmodname=pkg.__name__+'.'+modname
			if exclude and re.match(exclude,fqmodname):
				print 'Skipping  '+fqmodname
				continue
			if fqmodname not in sys.modules:
				sys.stdout.write('Importing '+fqmodname+'... ')
				try:
					__import__(fqmodname,pkg.__name__)
					print 'ok'
				except ImportError: print '(error, ignoring)'
			if ispkg: subImport(sys.modules[fqmodname])
	subImport(woo,exclude='^woo\._cxxInternal.*$')
	for m in woo.master.compiledPyModules:
		if m not in sys.modules:
			print 'Importing',m
			__import__(m)

	allClasses=woo.system.childClasses(woo.core.Object,recurse=True)
	#core.Object._derivedCxxClasses+[woo.core.Object]
	# create class tree; top-level nodes are packages
	# each level of child nodes is section in the documentation, as requested by ClassTraits of each class
	modules=set()
	for c in allClasses: modules.add(c.__module__)
	for c in allClasses:
		if c.__doc__:
			c.__doc__=fixDocstring(c.__doc__)
	outCxx=[]
	for mod in modules:
		klasses=[c for c in allClasses if c.__module__==mod]
		klasses.sort(key=lambda x: x.__name__)
		#print klasses
		outFile=outDir+'/%s.rst'%(mod)
		outCxx.append(outFile)
		out=codecs.open(outFile,'w','utf-8')
		out.write('Module %s\n==============================\n\n'%mod)
		out.write('.. inheritance-diagram:: %s\n\n'%mod)
		out.write('.. module:: %s\n\n'%mod)
		for k in klasses:
			out.write('.. autoclass:: %s\n   :show-inheritance:\n'%k.__name__)
			#out.write('   :members: %s\n'%(','.join([m for m in dir(k) if (not m.startswith('_') and m not in set(trait.name for trait in k._attrTraits))])))
			out.write('   :members:\n')
			exclude=[t.name for t in k._attrTraits]
			if exclude:
				out.write('   :exclude-members: %s\n\n'%(', '.join(exclude)))
			for trait in k._attrTraits:
				try:
					iniStr=' (= %s)'%(repr(trait.ini))
				except TypeError: # no converter found
					iniStr=''
				out.write('   .. attribute:: %s%s\n\n'%(trait.name,iniStr))
				for l in fixDocstring(trait.doc.decode('utf-8')).split('\n'): out.write('      '+l+'\n')
				out.write('\n')
	outPy=[]
	otherMods=[m for m in sys.modules if (
		m.startswith('woo.')
		and not m.startswith('woo._')
		and m not in modules
		and sys.modules[m]
		and m==sys.modules[m].__name__)]
	for mod in otherMods:
		outFile=outDir+'/%s.rst'%(mod)
		outPy.append(outFile)
		out=open(outFile,'w')
		out.write('Module %s\n==============================\n\n'%mod)
		out.write('.. automodule:: %s\n   :members:\n   :undoc-members:\n\n'%mod)

	return outCxx,outPy
	
		






