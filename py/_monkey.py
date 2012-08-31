# encoding: utf-8
# various monkey-patches for wrapped c++ classes
import woo.core
import woo.system
#import woo.dem
from miniEigen import * # for recognizing the types

import StringIO # cStringIO does not handle unicode, so stick with the slower one

from woo.core import Object
import woo._customConverters # to make sure they are loaded already

import codecs
import pickle
import json
import miniEigen

nan,inf=float('nan'),float('inf') # for values in expressions

def _Object_getAllTraits(obj):
	ret=[]; k=obj.__class__
	while k!=woo.core.Object:
		ret=k._attrTraits+ret
		k=k.__bases__[0]
	return ret

htmlHead='<head><meta http-equiv="content-type" content="text/html;charset=UTF-8" /></head><body>\n'

def _Object_dumps(obj,format,fragment=False,width=80,noMagic=False,stream=True,showDoc=False):
	if format not in ('html','expr','json','pickle','genshi'): raise IOError("Unsupported string dump format %s"%format)
	if format=='pickle':
		return pickle.dumps(obj)
	elif format=='json':
		return WooJSONEncoder().encode(obj)
	elif format=='expr':
		return SerializerToExpr(maxWd=width,noMagic=noMagic)(obj)
	elif format=='html':
		return ('' if fragment else htmlHead)+SerializerToHtmlTable(showDoc=showDoc)(obj)+('' if fragment else '</body>')
	elif format=='genshi':
		return SerializeToHtmlTable()(obj,dontRender=True,showDoc=showDoc)

def _Object_dump(obj,out,format='auto',overwrite=True,fragment=False,width=80,noMagic=False,showDoc=False):
	'''Dump an object in specified *format*; *out* can be a string (filename) or a *file* object. Supported formats are: `auto` (auto-detected from *out* extension; raises exception when *out* is an object), `html`, `expr`.'''
	if format not in ('auto','html','json','expr','pickle','boost::serialization'): raise IOError("Unsupported dump format %s"%format)
	hasFilename=isinstance(out,str)
	if hasFilename:
		import os.path
		if os.path.exists(out) and not overwrite: raise IOError("File '%s' exists (use overwrite=True)"%out)
	#if not hasFilename and not hasattr(out,'write'): raise IOError('*out* must be filename or file-like object')
	if format=='auto':
		if not isinstance(out,str): raise IOError("format='auto' is only possible when a fileName is given.")
		if out.endswith('.html'): format='html'
		if sum([out.endswith(ext) for ext in ('.pickle','pickle.gz','pickle.bz2')]): format='pickle'
		if sum([out.endswith(ext) for ext in ('.json','json.gz','json.bz2')]): format='json'
		if sum([out.endswith(ext) for ext in ('.xml','.xml.gz','.xml.bz2','.bin','.gz','.bz2')]): format='boost::serialization'
		else: IOError("Output format not deduced for filename '%s'"%out)
	if format in ('boost::serialization',) and not hasFilename: raise IOError("format='boost::serialization' needs filename.")
	# this will go away later
	if format=='boost::serialization':
		if not hasFilename: raise NotImplementedError('Only serialization to files is supported with boost::serialization.') 
		obj.save(out)
	elif format=='pickle':
		if hasFilename: pickle.dump(obj,open(out,'w'))
		else: out.write(pickle.dumps(obj))
	elif format in ('expr','html','json'): 
		if hasFilename:
			out=codecs.open(out,'w','utf-8')
		if format=='expr':
			out.write(SerializerToExpr(maxWd=width,noMagic=noMagic)(obj))
		elif format=='json':
			out.write(WooJSONEncoder().encode(obj))
		elif format=='html':
			if not fragment: out.write(htmlHead)
			out.write(SerializerToHtmlTable(showDoc=showDoc)(obj))
			if not fragment: out.write('</body>')
		
class SerializerToHtmlTableRaw:
	padding='cellpadding="2px"'
	def __init__(self,showDoc=False,maxDepth=8,hideNoGui=False):
		self.maxDepth=maxDepth
		self.hideNoGui=hideNoGui
		self.showDoc=showDoc
	def htmlSeq(self,s,indent,insideTable):
		ret=''
		indent1=indent+'\t'
		indent2=indent1+'\t'
		if hasattr(s[0],'__len__') and not isinstance(s[0],str): # 2d array
			# disregard insideTable in this case
			ret+=indent+'<table frame="box" rules="all" width="100%%" %s>'%self.padding
			for r in range(len(s)):
				ret+=indent1+'<tr>'
				for c in range(len(s[0])):
					ret+=indent2+'<td align="right" width="%g%%">'%(100./len(s[0])-1.)+('%g'%s[r][c] if isinstance(s[r][c],float) else str(s[r][c]))+'</td>'
				ret+=indent1+'</tr>'
			ret+=indent+'</table>'
			return ret
		# 1d array
		if not insideTable: ret+=indent+'<table frame="box" rules="all" width="100%%" %s>'%self.padding
		for e in s: ret+=indent1+'<td align="right" width="%g%%">'%(100./len(s)-1.)+('%g'%e if isinstance(e,float) else str(e))+'</td>'
		if not insideTable: ret+=indent+'</table>'
		return ret;
	def __call__(self,obj,depth=0):
		if depth>self.maxDepth: raise RuntimeError("Maximum nesting depth %d exceeded"%self.maxDepth)
		indent0=u'\n'+u'\t'*2*depth
		indent1=indent0+u'\t'
		indent2=indent1+u'\t'
		ret=indent0+'<table frame="box" rules="all" %s %s><th colspan="3" align="left"><b>%s</b></th>'%(self.padding, 'width="100%"' if depth>0 else '','.'.join(obj.__class__.__module__.split('.')[1:])+'.'+obj.__class__.__name__)
		# get all attribute traits first
		traits=obj._getAllTraits()
		for trait in traits:
			if trait.hidden or (self.hideNoGui and trait.noGui) or trait.noDump: continue
			# start new group (additional line)
			if trait.startGroup:
				ret+=indent1+'<tr><td colspan="3"><i>&#9656; %s</i></td></tr>'%trait.startGroup
			attr=getattr(obj,trait.name)
			ret+=indent1+'<tr>'+indent2+'<td>%s</td>'%(trait.name if not showDoc else trait.doc.decode('utf-8'))
			# nested object
			if isinstance(attr,woo.core.Object):
				ret+=indent2+'<td align="justify">'+self(attr,depth+1)+indent2+'</td>'
			# sequence of objects (no units here)
			elif hasattr(attr,'__len__') and len(attr)>0 and isinstance(attr[0],woo.core.Object):
				ret+=indent2+u'<td><ol>'+''.join(['<li>'+self(o,depth+1)+'</li>' for o in attr])+'</ol></td>'
			else:
				#ret+=indent2+'<td align="right">'
				if not trait.multiUnit: # the easier case
					if not trait.prefUnit: unit='&mdash;'
					else:
						unit=trait.prefUnit[0][0].decode('utf-8')
						# create new list, where entries are multiplied by the multiplier
						if type(attr)==list: attr=[a*trait.prefUnit[0][1] for a in attr]
						else: attr=attr*trait.prefUnit[0][1]
				else: # multiple units
					unit=[]
					wasList=isinstance(attr,list)
					if not wasList: attr=[attr] # handle uniformly
					for i in range(len(attr)):
						attr[i]=[attr[i][j]*trait.prefUnit[j][1] for j in range(len(attr[i]))]
					for pu in trait.prefUnit:
						unit.append(pu[0].decode('utf-8'))
					if not wasList: attr=attr[0]
					unit=', '.join(unit)
				# sequence type, or something similar				
				if hasattr(attr,'__len__') and not isinstance(attr,str):
					if len(attr)>0:
						ret+=indent2+'<td align="right">'
						ret+=self.htmlSeq(attr,indent=indent2+'\t',insideTable=False)
						ret+=indent2+'</td>'
					else: ret+='<td align="right"><i>[empty]</i></td>'
				else: ret+='<td align="right">%s</td>'%('%g'%attr if isinstance(attr,float) else str(attr))
				if unit: ret+=indent2+u'<td align="right">'+unit+'</td>'
			ret+=indent1+'</tr>'
		return ret+indent0+'</table>'

class SerializerToHtmlTableGenshi:
	padding=dict(cellpadding='2px')
	def __init__(self,showDoc=False,maxDepth=8,hideNoGui=False):
		self.maxDepth=maxDepth
		self.hideNoGui=hideNoGui
		self.showDoc=showDoc
	def htmlSeq(self,s,insideTable):
		from genshi.builder import tag
		table=tag.table(frame='box',rules='all',width='100%',**self.padding)
		if hasattr(s[0],'__len__') and not isinstance(s[0],str): # 2d array
			# disregard insideTable in this case
			for r in range(len(s)):
				tr=tag.tr()
				for c in range(len(s[0])):
					tr.append(tag.td('%g'%s[r][c] if isinstance(s[r][c],float) else str(s[r][c]),align='right',width='%g%%'%(100./len(s[0])-1.)))
				table.append(tr)
			return table
		# 1d array
		ret=table if not insideTable else []
		for e in s: ret.append(tag.td('%g'%e if isinstance(e,float) else str(e),align='right',width='%g%%'%(100./len(s)-1.)))
		return ret
	def __call__(self,obj,depth=0,dontRender=False):
		from genshi.builder import tag
		if depth>self.maxDepth: raise RuntimeError("Maximum nesting depth %d exceeded"%self.maxDepth)
		kw=self.padding.copy()
		if depth>0: kw.update(width='100%')
		ret=tag.table(tag.th(tag.b('.'.join(obj.__class__.__module__.split('.')[1:])+'.'+obj.__class__.__name__),colspan=3,align='left'),frame='box',rules='all',**kw)
		# get all attribute traits first
		traits=obj._getAllTraits()
		for trait in traits:
			if trait.hidden or (self.hideNoGui and trait.noGui) or trait.noDump: continue
			# start new group (additional line)
			if trait.startGroup:
				ret.append(tag.tr(tag.td(tag.i(u'▸ %s'%trait.startGroup.decode('utf-8')),colspan=3)))
			attr=getattr(obj,trait.name)
			tr=tag.tr(tag.td(trait.name if not self.showDoc else trait.doc.decode('utf-8')))
			# nested object
			if isinstance(attr,woo.core.Object):
				tr.append([tag.td(self(attr,depth+1),align='justify'),tag.td()])
			# sequence of objects (no units here)
			elif hasattr(attr,'__len__') and len(attr)>0 and isinstance(attr[0],woo.core.Object):
				tr.append(tag.td(tag.ol([tag.li(self(o,depth+1)) for o in attr])))
			else:
				if not trait.multiUnit: # the easier case
					if not trait.prefUnit: unit=u'−'
					else:
						unit=trait.prefUnit[0][0].decode('utf-8')
						# create new list, where entries are multiplied by the multiplier
						if type(attr)==list: attr=[a*trait.prefUnit[0][1] for a in attr]
						else: attr=attr*trait.prefUnit[0][1]
				else: # multiple units
					unit=[]
					wasList=isinstance(attr,list)
					if not wasList: attr=[attr] # handle uniformly
					for i in range(len(attr)):
						attr[i]=[attr[i][j]*trait.prefUnit[j][1] for j in range(len(attr[i]))]
					for pu in trait.prefUnit:
						unit.append(pu[0].decode('utf-8'))
					if not wasList: attr=attr[0]
					unit=', '.join(unit)
				# sequence type, or something similar				
				if hasattr(attr,'__len__') and not isinstance(attr,str):
					if len(attr)>0:
						tr.append(tag.td(self.htmlSeq(attr,insideTable=False),align='right'))
					else:
						tr.append(tag.td(tag.i('[empty]'),align='right'))
				else:
					tr.append(tag.td('%g'%attr if isinstance(attr,float) else str(attr),align='right'))
				if unit:
					tr.append(tag.td(unit,align='right'))
			ret.append(tr)
		if depth>0 or dontRender: return ret
		return ret.generate().render('xhtml',encoding='ascii')+'\n'

SerializerToHtmlTable=SerializerToHtmlTableGenshi


#class SerializerToHTML2:
#	unbreakableTypes=(Vector2i,Vector2,Vector3i,Vector3)
#	indent='\t'
#	def __init__(self,indent='\t',maxWd=120): pass
#	def __call__(self,obj,level=0):
#		ret=''
#		ind0=level*indent
#		ind1=(level+1)*indent
#		if isinstance(obj,Object):
#			attrs=[(trait.name,getattr(obj,trait.name)) for trait in obj._getAllTraits() if not trait.hidden]
#			ret+='<table frame=box rules=all %s %s><th><b>%s</b></th>\n'%(self.padding,'width="100%"' if depth>0 else '',obj.__class__.name)
#			for a in attrs:

class SerializerToExpr:
	unbreakableTypes=(Vector2i,Vector2,Vector3i,Vector3)
	def __init__(self,indent='\t',maxWd=120,noMagic=True):
		self.indent=indent
		self.indentLen=len(indent.replace('\t',3*' '))
		self.maxWd=maxWd
		self.noMagic=noMagic
	def __call__(self,obj,level=0):
		if isinstance(obj,Object):
			attrs=[(trait.name,getattr(obj,trait.name)) for trait in obj._getAllTraits() if not (trait.hidden or trait.noDump)]
			delims=(obj.__class__.__module__)+'.'+obj.__class__.__name__+'(',')'
		elif isinstance(obj,dict):
			attrs=obj.items()
			delims='{','}'
		# list or mutable list-like objects (NodeList, for instance)
		elif hasattr(obj,'__len__') and hasattr(obj,'__getitem__') and hasattr(obj,'append'):
			attrs=[(None,obj[i]) for i in range(len(obj))]
			delims='[',']'
		# tuple and tuple-like objects: format like tuples
		## were additionally: Matrix3,Vector6,Matrix6,VectorX,MatrixX,AlignedBox2,AlignedBox3
		elif sum([isinstance(obj,T) for T in (tuple,Vector2i,Vector2,Vector3i,Vector3)])>0:
			attrs=[(None,obj[i]) for i in range(len(obj))]
			delims='(',')'
		# don't know what to do, use repr (unhandled or primive types)
		else:
			return repr(obj)
		lst=[(((attr[0]+'=') if attr[0] else '')+self(attr[1],level+1)) for attr in attrs]
		oneLiner=(sum([len(l) for l in lst])+self.indentLen<=self.maxWd or self.maxWd<=0 or type(obj) in self.unbreakableTypes)
		magic=('##woo-expression##\n' if not self.noMagic and level==0 else '')
		if oneLiner: return magic+delims[0]+', '.join(lst)+delims[1]
		indent0,indent1=self.indent*level,self.indent*(level+1)
		return magic+delims[0]+'\n'+indent1+(',\n'+indent1).join(lst)+'\n'+indent0+delims[1]+('\n' if level==0 else '')

# roughly following http://www.doughellmann.com/PyMOTW/json/, thanks!
class WooJSONEncoder(json.JSONEncoder,indent=3,sort_keys=True):
	def __init__(self):
		json.JSONEncoder.__init__(self,sort_keys=sort_keys,indent=indent)
	def default(self,obj):
		# Woo objects
		if isinstance(obj,woo.core.Object):
			d={'__class__':obj.__class__.__module__+'.'+obj.__class__.__name__}
			# assign woo attributes
			d.update(dict([(trait.name,getattr(obj,trait.name)) for trait in obj._getAllTraits() if not (trait.hidden or trait.noDump)]))
			return d
		# vectors, matrices: those can be assigned from tuples
		elif obj.__class__.__module__=='woo._customConverters':
			if hasattr(obj,'__len__'): return list(obj)
			else: raise TypeError("Unhandled type for JSON: "+obj.__class__.__module__+'.'+obj.__class__.__name__)
		elif obj.__class__.__module__=='miniEigen':
			if isinstance(obj,miniEigen.Quaternion): return obj.toAxisAngle()
			else: return tuple(obj[i] for i in range(len(obj)))
		# other types, handled by the json module natively
		else:
			return super(WooJSONEncoder,self).default(obj)
class WooJSONDecoder(json.JSONDecoder):
	def __init__(self):
		json.JSONDecoder.__init__(self,object_hook=self.dictToObject)
	def dictToObject(self,d):
		if '__class__' in d:
			klass=d.pop('__class__')
			__import__(klass.rsplit('.',1)[0]) # in case the module has not been imported yet
			return eval(klass)(**dict((key.encode('ascii'),value.encode('ascii') if isinstance(value,unicode) else value) for key,value in d.items()))
		else: return d

def _Object_loads(typ,data,format='auto'):
	def typeChecked(obj,type):
		if not isinstance(obj,typ): raise TypeError('Loaded object of type '+obj.__class__.__name__+' is not a '+typ.__name__)
		return obj
	if format not in ('auto','pickle','expr','json'): raise ValueError('Invalid format %s'%format)
	elif format=='auto':
		if data.startswith('##woo-expression##'): format='expr'
		else:
			# try pickle
			try: return typeChecked(pickle.loads(data),typ)
			except (IOError,KeyError): pass
			# try json
			try: return typeChecked(WooJSONDecoder().decode(data),typ)
			except (IOError,ValueError,KeyError): pass
	if format=='auto': IOError("Format detection failed on data: "%data)
	## format detected now
	if format=='expr':
		return typeChecked(eval(data),typ)
	elif format=='pickle':
		return typeChecked(pickle.loads(data,typ))
	elif format=='json':
		return typeChecked(WooJSONDecoder().decode(data),typ)
	assert(False) # impossible


def _Object_load(typ,inFile,format='auto'):
	def typeChecked(obj,type):
		if not isinstance(obj,typ): raise TypeError('Loaded object of type '+obj.__class__.__name__+' is not a '+typ.__name__)
		return obj
	validFormats=('auto','boost::serialization','expr','pickle','json')
	if format not in validFormats: raise ValueError('format must be one of '+', '.join(validFormats)+'.')
	if format=='auto':
		format=None
		## DO NOT use extensions to determine type, that is evil
		# check for compression first
		head=open(inFile).read(100)
		# we might want to pass this data to ObjectIO, which currently determines format by looking at extension
		if head[:2]=='\x1f\x8b':
			import gzip
			head=gzip.open(inFile).read(100)
		elif head[:2]=='BZ':
			import bz2
			head=bz2.BZ2File(inFile).read(100)
		# detect real format here (head is uncompressed already)
		# the string between nulls is 'serialization::archive'
		# see http://stackoverflow.com/questions/10614215/magic-for-detecting-boostserialization-file
		if head.startswith('\x16\x00\x00\x00\x00\x00\x00\x00\x73\x65\x72\x69\x61\x6c\x69\x7a\x61\x74\x69\x6f\x6e\x3a\x3a\x61\x72\x63\x68\x69\x76\x65\x09\x00'):
			format='boost::serialization'
		elif head.startswith('<?xml version="1.0"'):
			format='boost::serialization'
		elif head.startswith('##woo-expression##'):
			format='expr'
		else:
			# test pickling by trying to load
			try: return typeChecked(pickle.load(open(inFile)),typ) # open again to seek to the beginning
			except (IOError,KeyError): pass
			try: return typeChecked(WooJSONDecoder().decode(open(inFile).read()),typ)
			except (IOError,ValueError): pass
		if not format: raise RuntimeError('File format detection failed on %s (head: %s)'%(inFile,''.join(["\\x%02x"%ord(x) for x in head])))
	if format not in validFormats: raise RuntimeError("format='%s'??"%format)
	assert format in validFormats
	if format==None:
		raise IOError('Input file format not detected')
	elif format=='boost::serialization':
		# ObjectIO takes care of detecting binary, xml, compression independently
		return typeChecked(Object._boostLoad(inFile),typ)
	elif format=='expr':
		buf=codecs.open(inFile,'r','utf-8').read()
		return typeChecked(eval(buf),typ)
	elif format=='pickle':
		return typeChecked(pickle.load(open(inFile)),typ)
	elif format=='json':
		return typeChecked(WooJSONDecoder().decode(open(inFile).read()),typ)
	assert(False)



def _Object_loadTmp(typ,name=''):
	obj=woo.master.loadTmpAny(name)
	if not isinstance(obj,typ): raise TypeError('Loaded object of type '+obj.__class__.__name__+' is not a '+typ.__name__)
	return obj
def _Object_saveTmp(obj,name='',quiet=False):
	woo.master.saveTmpAny(obj,name,quiet)
def _Object_deepcopy(obj):
	'Make object deepcopy by serializing to memory and deserializing.'
	prefix='_deepcopy'
	tmps=woo.master.lsTmp()
	# FIXME: possible race condition here?
	i=0
	while prefix+'%d'%i in tmps: i+=1
	name=prefix+'%d'%i
	obj.saveTmp(name)
	ret=Object.loadTmp(name)
	woo.master.rmTmp(name)
	return ret
	

Object._getAllTraits=_Object_getAllTraits
Object.dump=_Object_dump
Object.dumps=_Object_dumps
Object.saveTmp=_Object_saveTmp
Object.deepcopy=_Object_deepcopy
Object.load=classmethod(_Object_load)
Object.loads=classmethod(_Object_loads)
Object.loadTmp=classmethod(_Object_loadTmp)


def _Master_save(o,*args,**kw):
	o.scene.save(*args,**kw)
def _Master_load(o,*args,**kw):
	o.scene=woo.core.Scene.load(*args,**kw)
def _Master_reload(o,quiet=None,*args,**kw): # this arg is deprecated
	f=o.scene.lastSave
	if not f: raise ValueError("Scene.lastSave is empty.")
	if f.startswith(':memory:'): o.scene=woo.core.Scene.loadTmp(f[8:])
	else: o.scene=woo.core.Scene.load(f,*args,**kw)
def _Master_loadTmp(o,name='',quiet=None): # quiet deprecated
	o.scene=woo.core.Scene.loadTmp(name)
def _Master_saveTmp(o,name='',quiet=False):
	o.scene.lastSave=':memory:'+name
	o.scene.saveTmp(name,quiet)

woo.core.Master.save=_Master_save
woo.core.Master.load=_Master_load
woo.core.Master.reload=_Master_reload
woo.core.Master.loadTmp=_Master_loadTmp
woo.core.Master.saveTmp=_Master_saveTmp

def _Master_run(o,*args,**kw): return o.scene.run(*args,**kw)
def _Master_pause(o,*args,**kw): return o.scene.stop(*args,**kw)
def _Master_step(o,*args,**kw): return o.scene.one(*args,**kw)
def _Master_wait(o,*args,**kw): return o.scene.wait(*args,**kw)
def _Master_reset(o):
	o.scene=woo.core.Scene()

#def _Master_running(o.,*args,**kw): return o.scene.running
woo.core.Master.run=_Master_run
woo.core.Master.pause=_Master_pause
woo.core.Master.step=_Master_step
woo.core.Master.wait=_Master_wait
woo.core.Master.running=property(lambda o: o.scene.running)
woo.core.Master.reset=_Master_reset
