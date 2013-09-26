# encoding: utf-8
import woo
import woo.core
import sys
import codecs
import re


sphinxOnlineDocPath='http://www.woodem.eu/doc/'
"Base URL for the documentation. Packaged versions should change to the local installation directory."

import os.path
# find if we have docs installed locally from package
sphinxLocalDocPath=woo.config.prefix+'/share/doc/woo'+woo.config.suffix+'/html/'
# FIXME: unfortunately file:// links don't seem to work with anchors, so just make this invalid
sphinxBuildDocPath=woo.config.sourceRoot+'/doc/sphinx2/build/html/REMOVE_THIS_TO_USE_LOCAL_DOCS'
# we prefer the packaged documentation for this version, if installed
if   os.path.exists(sphinxLocalDocPath+'/index.html'): sphinxPrefix='file://'+sphinxLocalDocPath
# otherwise look for documentation generated in the source tree
elif  os.path.exists(sphinxBuildDocPath+'/index.html'): sphinxPrefix='file://'+sphinxBuildDocPath
# fallback to online docs
else: sphinxPrefix=sphinxOnlineDocPath


def fixDocstring(s):
	s=s.replace(':ref:',':obj:')
	s=re.sub(r'(?<!\\)\$([^\$]+)(?<!\\)\$',r'\ :math:`\1`\ ',s)
	s=re.sub(r'\\\$',r'$',s)
	return s

cxxClasses,allWooMods=set(),set()

def _ensureInitialized():
	'Fill cxxClasses and allWooMods, called automatically as needed'
	global cxxClasses,allWooMods
	if cxxClasses: return  # do nothing if already filled
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
					if ispkg:
						subImport(sys.modules[fqmodname])
				except ImportError: print '(error, ignoring)'
	modExcludeRegex='^woo\.(_cxxInternal.*)(\..*|$)'
	subImport(woo,exclude=modExcludeRegex)
	try:
		import wooExtra
		subImport(wooExtra)
	except ImportError:
		print 'No wooExtra modules imported.'

	for m in woo.master.compiledPyModules:
		if m not in sys.modules:
			print 'Importing',m
			__import__(m)

	# global cxxClasses, allWooMods
	cc=woo.system.childClasses(woo.core.Object,includeBase=True,recurse=True)
	for c in cc:
		if re.match(modExcludeRegex,c.__module__): continue
		if c.__doc__: c.__doc__=fixDocstring(c.__doc__)
		cxxClasses.add(c)

	allWooMods=set([sys.modules[m] for m in sys.modules if m.startswith('woo') and sys.modules[m] and sys.modules[m].__name__==m])
	

def allWooPackages(outDir='/tmp',skip='^(woo|wooExtra(|\..*))$'):
	'''Generate documentation of packages in the Restructured Text format. Each package is written to file called *out*/.`woo.[package].rst` and list of files created is returned.'''

	global cxxClasses,allWooMods
	_ensureInitialized()
	
	modsElsewhere=set()
	for m in allWooMods: modsElsewhere|=set(m._docInlineModules if hasattr(m,'_docInlineModules') else [])
	# woo.foo.* modules go insides woo.foo
	toplevMods=set([m for m in allWooMods if (m not in modsElsewhere) and len(m.__name__.split('.'))<3])
	rsts=[]
	print 'TOPLEVEL MODULES',[m.__name__ for m in toplevMods]
	print 'MODULES DOCUMENTED ELSEWHERE',[m.__name__ for m in modsElsewhere]

	for mod in toplevMods:
		if re.match(skip,mod.__name__):
			print '[SKIPPING %s]'%(mod.__name__)
			continue
		outFile=outDir+'/%s.rst'%(mod.__name__)
		print 'WRITING',outFile,mod.__name__
		rsts.append(outFile)
		out=codecs.open(outFile,'w','utf-8')
		oneModuleWithSubmodules(mod,out)

	return rsts

def makeTraitInfo(trait):
	ret=[]
	if trait.static: ret.append('static')
	if hasattr(trait,'pyType'):
		if isinstance(trait.pyType,list): ret.append('type: ['+trait.pyType[0].__name__+", …]")
		else: ret.append('type: '+trait.pyType.__name__)
	else: ret.append('type: '+trait.cxxType)
	if trait.unit:
		if len(trait.unit)==1: ret.append('unit: '+unicode(trait.unit[0]))
		else: ret.append(u'units: ['+u','.join(u for u in trait.unit)+u']')
	if trait.prefUnit and trait.unit[0]!=trait.prefUnit[0][0]:
		if len(trait.unit)==1: ret.append('preferred unit: '+unicode(trait.prefUnit[0][0]))
		else: ret.append('preferred units: ['+u','.join(u[0] for u in trait.prefUnit)+']')
	if trait.range: ret.append(u'range: %g−%g'%(trait.range[0],trait.range[1]))
	if trait.noGui: ret.append('not shown in the UI')
	if trait.noDump: ret.append('not dumped')
	if trait.noSave: ret.append('not saved')
	if trait.hidden: ret.append('not accessible from python')
	if trait.readonly: ret.append('read-only in python')
	if trait.choice:
		if isinstance(trait.choice[0],tuple): ret.append('choices: '+', '.join('%d = %s'%(c[0],c[1]) for c in trait.choice))
		else: ret.append('choices: '+', '.join(str(c) for c in trait.choice))
	if trait.filename: ret.append('filename')
	if trait.existingFilename: ret.append('existing filename')
	if trait.dirname: ret.append('directory name')

	return ', '.join(ret)
	
def classSrcHyperlink(klass):
	'Return ReST-formatted line with hyperlinks to class headers (and implementation, if the corresponding .cpp file exists).'
	import woo.config
	f=klass._classTrait.file
	if f.startswith(woo.config.sourceRoot): commonRoot=woo.config.sourceRoot
	elif f.startswith(woo.config.buildRoot): commonRoot=woo.config.buildRoot
	else:
		print 'File where class is defined (%s) does not start with source root (%s) or build root (%s)'%(f,woo.config.sourceRoot,woo.config.buildRoot)
		return None
	# +1 removes underscore in woo/...
	f2=f[len(commonRoot)+1:] 
	# if this header was copied into include/, get rid of that now
	m=re.match('include/woo/(.*)',f2)
	if m: f2=m.group(1)
	ret=[':woosrc:`header <%s#L%d>`'%(f2,klass._classTrait.line)]
	if f2.endswith('.hpp'):
		cpp=woo.config.sourceRoot+'/'+f2[:-4]+'.cpp'
		# print 'Trying ',cpp
		if os.path.exists(cpp):	
			ret.append(':woosrc:`implementation <%s>`'%(f2[:-4]+'.cpp'))
	return '[ '+' , '.join(ret)+' ]'

def oneModuleWithSubmodules(mod,out,exclude=None,level=0):
	global cxxClasses,allWooMods
	_ensureInitialized()
	if exclude==None: exclude=set() # avoid referenced empty set to be modified
	if level>=0: out.write('Module %s\n%s\n\n'%(mod.__name__,(20+len(mod.__name__))*('=-^"'[level])))
	out.write('.. module:: %s\n\n'%mod.__name__)
	# HACK: we wan't module's docstring to appear at the top, but autodocumented stuff at the bottom
	# dump __doc__ straight to the output here, and reset it, so that automodule does not pick it up
	if mod.__doc__:
		out.write(mod.__doc__+'\n\n')
		mod.__doc__=None
	out.write('.. inheritance-diagram:: %s\n\n'%mod.__name__)
	klasses=[c for c in cxxClasses if (c.__module__==mod.__name__ or (hasattr(mod,'_docInlineModules') and sys.modules[c.__module__] in mod._docInlineModules))]
	klasses.sort(key=lambda x: x.__name__)
	# document any c++ classes in a special way
	for k in klasses:
		if k in exclude: continue
		exclude.add(k) # already-documented should not be documented again
		out.write('.. autoclass:: %s\n   :show-inheritance:\n'%k.__name__)
		#out.write('   :members: %s\n'%(','.join([m for m in dir(k) if (not m.startswith('_') and m not in set(trait.name for trait in k._attrTraits))])))
		out.write('   :members:\n')
		ex=[t.name for t in k._attrTraits]
		if ex: out.write('   :exclude-members: %s\n\n'%(', '.join(ex)))
		srcXref=classSrcHyperlink(k)
		if srcXref: out.write('\n   '+srcXref+'\n\n')
		for trait in k._attrTraits:
			try:
				iniStr=' (= %s)'%(repr(trait.ini))
			except TypeError: # no converter found
				iniStr=''
			if trait.startGroup:
				out.write(u'   .. rubric:: ► %s\n\n'%(trait.startGroup))
			out.write('   .. attribute:: %s%s\n\n'%(trait.name,iniStr))
			for l in fixDocstring(trait.doc.decode('utf-8')).split('\n'): out.write('      '+l+'\n')
			traitInfo=makeTraitInfo(trait)
			if traitInfo: out.write(u'\n      ['+traitInfo+']\n')
			out.write('\n')
	def writeAutoMod(mm,skip=None):
		out.write('.. automodule:: %s\n   :members:\n   :undoc-members:\n'%(mm.__name__))
		if skip: out.write('   :exclude-members: %s\n'%(',  '.join([e.__name__ for e in skip])))
		out.write('\n')
	# document the rest of the module here (don't recurse)
	writeAutoMod(mod,skip=exclude)
	# imported modules
	if hasattr(mod,'_docInlineModules'):
		# negative level will skip heading
		for m in mod._docInlineModules: oneModuleWithSubmodules(m,out,exclude=exclude,level=-1)
	# nested modules
	# with nested heading
	for m in [m for m in allWooMods if m.__name__.startswith(mod.__name__+'.') and len(mod.__name__.split('.'))+1==len(m.__name__.split('.'))]: oneModuleWithSubmodules(m,out,exclude=exclude,level=level+1)



def makeSphinxHtml(k):
	'Given a class, try to guess name of the HTML page where it is documented by Sphinx'
	if not k.__module__.startswith('woo.'): return k.__module__
	mod=k.__module__.split('.')[1] # sphinx does not make the hierarchy any deeper than 2
	for start,repl in [('_pack','pack'),('_qt','qt'),('_utils','utils')]:
		if mod.startswith(start): return 'woo.'+repl
	return 'woo.'+mod

def makeClassAttrDocUrl(klass,attr=None):
	'''Return URL to documentation of Woo class or its attribute in http://woodem.eu/doc.
	:param klass: class object
	:param attr:  attribute to link to. If given, must exist directly in given *klass* (not its parent); if not given or empty, link to the class itself is created and *attr* is ignored.
	:return: URL as text
	'''
	dotAttr=(('.'+attr) if attr else '')
	if klass.__module__.startswith('wooExtra.'):
		KEY=sys.modules['.'.join(klass.__module__.split('.')[:2])].KEY
		return 'http://www.woodem.eu/private/{KEY}/doc/index.html#{module}.{klass}{dotAttr}'.format(KEY=KEY,module=klass.__module__,klass=klass.__name__,dotAttr=dotAttr)
	return '{sphinxPrefix}/{sphinxHtml}.html#{module}.{klass}{dotAttr}'.format(sphinxPrefix=sphinxPrefix,sphinxHtml=makeSphinxHtml(klass),module=klass.__module__,klass=klass.__name__,dotAttr=dotAttr)

def makeObjectUrl(obj,attr=None):
	"""Return HTML href to a *obj* optionally to the attribute *attr*.
	The class hierarchy is crawled upwards to find out in which parent class is *attr* defined,
	so that the href target is a valid link. In that case, only single inheritace is assumed and
	the first class from the top defining *attr* is used.

	:param obj: object of class deriving from :ref:`Object`, or string; if string, *attr* must be empty.
	:param attr: name of the attribute to link to; if empty, linke to the class itself is created.

	:returns: HTML with the hyperref.
	"""
	if attr:
		klass=obj.__class__
		while attr in dir(klass.__bases__[0]): klass=klass.__bases__[0]
	else:
		klass=obj.__class__
	return makeClassAttrDocUrl(klass,attr)

def makeObjectHref(obj,attr=None,text=None):
	'''Create HTML hyperlink, wrapping :obj:`makeObjectUrl`. adding ``<a href="...">text</a>``.

	:param text: visible text of the hyperlink; if not given, either class name or attribute name without class name (when *attr* is given) is used.
	'''
	if not text:
		if attr: text=attr
		else: text=obj.__class__.__name__
	return '<a href="%s">%s</a>'%(makeObjectUrl(obj,attr),text)


