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

allWooClasses,allWooMods=set(),set()

def _ensureInitialized():
	'Fill allWooClasses and allWooMods, called automatically as needed'
	global allWooClasses,allWooMods
	if allWooClasses: return  # do nothing if already filled
	def subImport(pkg,exclude=None):
		'Import recursively all subpackages'
		import pkgutil
		for importer, modname, ispkg in pkgutil.iter_modules(pkg.__path__):
			fqmodname=pkg.__name__+'.'+modname
			print fqmodname
			if exclude and re.match(exclude,fqmodname):
				print 'Skipping  '+fqmodname
				continue
			if fqmodname not in sys.modules:
				sys.stdout.write('Importing '+fqmodname+'... ')
				try:
					__import__(fqmodname,pkg.__name__)
					print 'ok'
				except (ImportError, NameError): print '(error, ignoring)'
			if fqmodname in sys.modules and ispkg:
				print 'Import submodules from ',fqmodname
				subImport(sys.modules[fqmodname])
				
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

	# global allWooClasses, allWooMods
	cc=woo.system.childClasses(woo.core.Object,includeBase=True,recurse=True)
	for c in cc:
		if re.match(modExcludeRegex,c.__module__): continue
		if c.__doc__: c.__doc__=fixDocstring(c.__doc__)
		allWooClasses.add(c)

	allWooMods=set([sys.modules[m] for m in sys.modules if m.startswith('woo') and sys.modules[m] and sys.modules[m].__name__==m])
	

def allWooPackages(outDir='/tmp',skip='^(woo|wooExtra(|\..*))$'):
	'''Generate documentation of packages in the Restructured Text format. Each package is written to file called *out*/.`woo.[package].rst` and list of files created is returned.'''

	global allWooClasses,allWooMods
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
	def maybe_decode(a): return (a if isinstance(a,unicode) else unicode(a,'utf-8'))
	ret=[]
	if trait.static: ret.append('static')
	if hasattr(trait,'pyType'):
		if isinstance(trait.pyType,(list,tuple)): ret.append('type: ['+trait.pyType[0].__name__+", …]")
		else: ret.append('type: '+trait.pyType.__name__)
	else: ret.append('type: '+trait.cxxType)
	if trait.unit:
		if len(trait.unit)==1: ret.append('unit: '+maybe_decode(trait.unit[0]))
		else: ret.append(u'units: ['+u','.join(maybe_decode(u) for u in trait.unit)+u']')
	if trait.prefUnit and trait.unit[0]!=trait.prefUnit[0][0]:
		if len(trait.unit)==1: ret.append('preferred unit: '+maybe_decode(trait.prefUnit[0][0]))
		else: ret.append('preferred units: ['+u','.join(maybe_decode(u[0]) for u in trait.prefUnit)+']')
	if trait.range: ret.append(u'range: %g−%g'%(trait.range[0],trait.range[1]))
	if trait.noGui: ret.append('not shown in the UI')
	if trait.noDump: ret.append('not dumped')
	if trait.noSave: ret.append('not saved')
	if trait.hidden: ret.append('not accessible from python')
	if trait.readonly: ret.append('read-only in python')
	if trait.choice and not trait.namedEnum: # named enums formatted differently below
		if isinstance(trait.choice[0],tuple): ret.append('choices: '+', '.join('%d = %s'%(c[0],c[1] if '|' not in c[1] else '``'+c[1]+'``') for c in trait.choice))
		else: ret.append('choices: '+', '.join(str(c) for c in trait.choice))
	if trait.namedEnum: ret.append('named enum, possible values are: '+trait.namedEnum_validValues(pre0="**'",post0="'**",pre="*'",post="'*"))
	if trait.filename: ret.append('filename')
	if trait.existingFilename: ret.append('existing filename')
	if trait.dirname: ret.append('directory name')
	if trait.bits: ret.append('bit accessors: '+', '.join(['**'+b+'**' for  b in trait.bits]))

	return ', '.join(ret)
	
def classSrcHyperlink(klass):
	'Return ReST-formatted line with hyperlinks to class headers (and implementation, if the corresponding .cpp file exists).'
	import woo.config
	if not klass._classTrait: return None
	f=klass._classTrait.file
	if f.startswith(woo.config.sourceRoot): commonRoot=woo.config.sourceRoot
	elif f.startswith(woo.config.buildRoot): commonRoot=woo.config.buildRoot
	elif not f.startswith('/'): commonRoot=''
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
			# find first line in the cpp file which contains KlassName:: -- that's most likely where the implementation begins
			numLines=[i for i,l in enumerate(file(cpp).readlines()) if klass.__name__+'::' in l]
			if numLines: ret.append(':woosrc:`implementation <%s#L%d>`'%(f2[:-4]+'.cpp',numLines[0]+1))
			# return just the cpp file if the implementation is probably not there at all?
			# don't do it for now
			# else: ret.append(':woosrc:`implementation <%s#L>`'%(f2[:-4]+'.cpp'))
	return '[ '+' , '.join(ret)+' ]'

def classDocHierarchy_topsAndDict(mod):
	'Return tuple containing list of top-level class objects, and dictionary which maps all module-contained class objects to classes which should be documented under it (derived classes, and doc-related classes specified via ClassTrait.section'
	global allWooClasses,allWooMods
	_ensureInitialized()
	# kk=woo.system.childClasses(woo.core.Object,includeBase=True) # all woo classes
	dd={}
	for k in allWooClasses:
		if k==woo.core.Object: continue # obejct itself is not documented this way
		t=k._classTrait
		if not t: continue
		if k.__module__!=mod.__name__: continue
		dd[k]=[]
		for a in t.docOther:
			if '.' in a:
				raise NotImplementedError("Cross-module documentation not yet supported.")
				dd[k].append(eval(a))
			else: dd[k].append(eval(k.__module__+'.'+a))
			# [eval('%s.%s'%(k.__module__,a)) for a in t.docOther] # perhaps not in the same module? fix here!!
		#print t.name,t.docOther,t.intro,t.title

	for k in dd:
		# if the base is documented, put it under the base class (prepend)
		b=k.__bases__[0]
		if b in dd: dd[b]=[k]+dd[b]
	tops=[]
	for k in dd:
		refs=[]
		for kk,vv in dd.items():
			if k in vv: refs+=[kk]
		if len(refs)==0: tops.append(k)
		elif len(refs)>1: raise RuntimeError('Class %s listed under multiple classes: '%k.__name__+', '.join(['%s'%(r.__name__) for r in refs]))
		# if the class has one single reference, it is not a top class
	return (tops,dd)


def oneModuleWithSubmodules(mod,out,exclude=None,level=0,importedInto=None):
	global allWooClasses,allWooMods
	_ensureInitialized()
	if exclude==None: exclude=set() # avoid referenced empty set to be modified
	if level>=0: out.write(':mod:`%s`\n%s\n\n'%(mod.__name__,(20+len(mod.__name__))*('=-^"'[level])))
	if importedInto:
		out.write('.. note:: This module is imported into the :obj:`%s` module automatically; refer to its objects through :obj:`%s`.\n\n'%(importedInto.__name__,importedInto.__name__))

	def _docOneClass(k):
		if k in exclude: return
		exclude.add(k) # already-documented should not be documented again

		kOut.write('.. autoclass:: %s\n'%k.__name__)
		#kOut.write('   :members: %s\n'%(','.join([m for m in dir(k) if (not m.startswith('_') and m not in set(trait.name for trait in k._attrTraits))])))
		kOut.write('   :members:\n')
		ex=[t.name for t in k._attrTraits]
		if ex: kOut.write('   :exclude-members: %s\n\n'%(', '.join(ex)))

		srcXref=classSrcHyperlink(k)
		if srcXref: kOut.write('\n   '+srcXref+'\n\n')
		for trait in k._attrTraits:
			try:
				iniStr=' (= %s)'%(repr(trait.ini))
			except TypeError: # no converter found
				iniStr=''
			if trait.startGroup:
				kOut.write(u'   .. rubric:: ► %s\n\n'%(trait.startGroup))
			kOut.write('   .. attribute:: %s%s\n\n'%(trait.name,iniStr))
			for l in fixDocstring(trait.doc.decode('utf-8')).split('\n'): kOut.write('      '+l+'\n')
			traitInfo=makeTraitInfo(trait)
			if traitInfo: kOut.write(u'\n      ['+traitInfo+']\n')
			kOut.write('\n')

	klasses=[c for c in allWooClasses if (c.__module__==mod.__name__ or (hasattr(mod,'_docInlineModules') and sys.modules[c.__module__] in mod._docInlineModules))]
	klasses.sort(key=lambda x: x.__name__)
	#
	tops,klassesUnder=classDocHierarchy_topsAndDict(mod)

	#global prevLevel
	#prevLevel=0

	def _inheritanceDiagram(k,currmod):
		def nodeName(kk): return ('' if kk.__module__==currmod else kk.__module__+'.')+kk.__name__
		def mkNode(kk,style='solid',fillcolor=None): return '\t\t"%s" [shape="box",fontsize=8,style="setlinewidth(0.5),%s",%sheight=0.2,URL="%s.html#%s.%s"];\n'%(nodeName(kk),style,'fillcolor=%s,'%fillcolor if fillcolor else '','.'.join(kk.__module__.split('.')[:2]),kk.__module__,k.__name__)
		ret=".. graphviz::\n\n\tdigraph %s {\n\t\trankdir=LR;\n\t\tmargin=.2;\n"%k.__name__
		ret+=mkNode(k)
		cc=woo.system.childClasses(k,includeBase=False)
		for c in cc:
			if c.__module__.startswith('wooExtra.'): continue
			# this class will have its own diagram for inheritance
			# if hasattr(c,'_classTrait') and c._classTrait.title and c._classTrait.klassesUnder:
			if c.__module__==currmod: ret+=mkNode(c)
			else: ret+=mkNode(c,style='filled',fillcolor='grey')
			ret+='\t\t"%s" -> "%s" [arrowsize=0.5,style="setlinewidth(0.5)"]'%(nodeName(c.__bases__[0]),nodeName(c))
		return ret+'\n\t}\n\n'


	def _docOneClassWithSectioning(k,level=0):
		# print level*'\t',k.__name__
		if not hasattr(k,'_classTrait'): return # will be documented later
		t=k._classTrait
		nextLevel=level
		#global prevLevel
		#if not t.title:
		#	# decrasing level without section -- write dividing line
		#	if prevLevel>level: out.write('\n\n-----\n\n\n') #
		#prevLevel=level
		#if t.title: # write section title
		nextLevel=level+1
		tt=t.title if t.title else k.__name__
		if level<3:
			kOut.write('\n.. rst-class:: html-toggle\n\n')
			kOut.write('\n.. rst-class:: emphasized\n\n')
		kOut.write(tt+'\n'+len(tt)*'-+"%\'^_'[level]+'\n\n')
		#else:
		#	nextLevel=level+1
		#	kOut.write(k.__name__+'\n'+len(k.__name__)*('-"\'^+_%'[level])+'\n\n')
		if t.intro:
			kOut.write(t.intro+'\n\n')
			if not t.title: warnings.warn('Class %s.%s has intro but no title in class trait.'%(k.__module__,k.__name__))
		# if there was a title different from the class title and there are classes under us, repeat the class name here on the subordinate level
		if t.title:
			if nextLevel<3:
				kOut.write('\n.. rst-class:: html-toggle\n\n')
				kOut.write('\n.. rst-class:: emphasized\n\n')
			kOut.write(k.__name__+'\n'+len(k.__name__)*'-+"%\'^_'[nextLevel]+'\n\n')
		# print inheritance
		if k!=woo.core.Object:
			bb=[k]
			while True:
				bb+=[bb[-1].__bases__[0]]
				if bb[-1]==woo.core.Object: break
			kOut.write(u'\n'+u' → '.join([u':obj:`~%s.%s`'%(b.__module__,b.__name__) for b in reversed(bb)])+'\n\n')
		# print derived classes
		if klassesUnder and woo.system.childClasses(k):
			kOut.write(_inheritanceDiagram(k,mod.__name__))
		_docOneClass(k)
		for kk in klassesUnder[k]:
			assert k!=kk
			# print kk.__name__
			_docOneClassWithSectioning(kk,nextLevel)
		#if not t.title and t.intro:

	# document any c++ classes in a special way
	# defer writing that to out though so that automodule can exclude classes which are documented manually
	import StringIO
	kOut=StringIO.StringIO()
	for top in tops: _docOneClassWithSectioning(top,level)

	# document all remaining classes linearly here, if any
	for k in klasses: _docOneClass(k)

	# control whether autodocs are at the bottom or at the top
	def writeAutoMod(mm,skip=None):
		out.write('.. automodule:: %s\n   :members:\n   :undoc-members:\n'%(mm.__name__))
		if skip: out.write('   :exclude-members: %s\n'%(',  '.join([e.__name__ for e in skip])))
		out.write('\n')
	autoAtBottom=False
	if autoAtBottom:
		# HACK: we want module's docstring to appear at the top, but autodocumented stuff at the bottom
		# dump __doc__ straight to the output here, and reset it, so that automodule does not pick it up
		# autodoc however automatically creates index entry for the module;
		# that's why we end up with bunch of SEVERE: Duplicate ID: "module-..." msgs
		out.write('.. module:: %s\n\n'%mod.__name__)
		if mod.__doc__:
			out.write(mod.__doc__+'\n\n')
			mod.__doc__=None
		# if there are no classes, avoid warning from sphinx
		if klasses: out.write('.. inheritance-diagram:: %s\n\n'%mod.__name__) 
		# insert documentation of classes
		out.write(kOut.getvalue())
		# document the rest of the module here (don't recurse)
		writeAutoMod(mod,skip=exclude)
	else:
		# automodule first, including inheritance tree (are excluded classes excluded from the tree as well?)
		# exlude classes which are are then documented manually
		# if there are no classes, avoid warning from sphinx
		if klasses: out.write('.. inheritance-diagram:: %s\n   :parts: %s\n\n'%(mod.__name__,len(mod.__name__.split('.')) if mod.__name__.startswith('woo.') else 0)) # without showing module name in the inheritance diagram
		writeAutoMod(mod,skip=exclude)
		out.write(kOut.getvalue())
	# imported modules
	if hasattr(mod,'_docInlineModules'):
		# negative level will skip heading
		for m in mod._docInlineModules: oneModuleWithSubmodules(m,out,exclude=exclude,level=level+1,importedInto=mod)
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

	:param obj: object of class deriving from :obj:`woo.core.Object`, or string; if string, *attr* must be empty.
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


def makeCGeomFunctorsMatrix():
	import woo, woo.dem, woo.system
	import prettytable
	ggg0=woo.system.childClasses(woo.dem.CGeomFunctor)
	ggg=set()
	for g in ggg0:
		# handle derived classes which don't really work as functors (Cg2_Any_Any_L6Geom__Base)
		try: 
			g().bases
			ggg.add(g)
		except: pass 

	ss=list(woo.system.childClasses(woo.dem.Shape))
	ss.sort(key=lambda s: s.__name__)
	ss=[s for s in ss if s.__name__ not in ('Membrane','Tet4')] # Membrane is useless here, as it is the same as Facet

	def type2sphinx(t,name=None):
		if name==None: return ':obj:`~%s.%s`'%(t.__module__,t.__name__)
		else: return ':obj:`%s <%s.%s>`'%(name,t.__module__,t.__name__)

	t=prettytable.PrettyTable(['']+[type2sphinx(s) for s in ss],border=True,header=True,hrules=prettytable.ALL)
	for s1 in ss:
		row=[type2sphinx(s1)] # header column
		for s2 in ss:
			gg=[g for g in ggg if sorted(g().bases)==sorted([s1.__name__,s2.__name__])]
			cell=[]
			for g in gg:
				if g.__name__.endswith('L6Geom'): cell+=[type2sphinx(g,'l6g')]
				elif g.__name__.endswith('G3Geom'): cell+=[type2sphinx(g,'g3g')]
				else: raise RuntimError('CGeomFunctor name does not end in L6Geom or G3Geom.')
			cell.sort(reverse=True) # so that l6g comes first
			if cell: row.append(', '.join(cell))
			else: row.append(u'×')
		t.add_row(row)
	tt=t.get_string().split('\n')
	tt=[(tt[i] if i!=2 else tt[i].replace('-','=')) for i in range(len(tt))]
	# return '.. tabularcolumns:: |l|'+'|'.join(len(ss)*['c'])+'\n\n'
	return '\n'.join(tt)+'\n\n'


