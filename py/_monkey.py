# various monkey-patches for wrapped c++ classes
import yade.wrapper
import yade.core
#import yade.dem
from miniEigen import * # for recognizing the types

from yade.wrapper import Serializable

def _Serializable_getAllTraits(obj):
	ret=[]; k=obj.__class__
	while k!=yade.wrapper.Serializable:
		ret=k._attrTraits+ret
		k=k.__bases__[0]
	return ret


def _Serializable_save(obj,out,format='auto',overwrite=True,fragment=False,width=80):
	'''Save an object in specified *format*; *out* can be a string (filename) or a *file* object. Supported formats are: `auto` (auto-detected from *out* extension; raises exception when *out* is an object), `xml`, `bin`, `html`, `pyexpr`.'''
	hasFilename=isinstance(out,str)
	if hasFilename:
		import os.path
		if os.path.exists(out) and not overwrite: raise IOError("File '%s' exists (use overwrite=True)"%out)
	if not hasFilename and not hasattr(out,'write'): raise IOError('*out* must be filename or file-like object')
	if format=='auto':
		if not isinstance(out,str): raise IOError("format='auto' is only possible when a file (not a file object) is given.")
		if out.endswith('.html'): format='html'
		if sum([out.endswith(ext) for ext in ('.pickle','pickle.gz','pickle.bz2')]): format='pickle'
		if sum([out.endswith(ext) for ext in ('.xml','.xml.gz','.xml.bz2','.bin','.gz','.bz2')]): format='boost::serialization'
		else: IOError("Output format not deduced for filename '%s'"%out)
	if format in ('boost::serialization',) and not hasFilename: raise IOError("format='boost::serialization' needs filename, not file-like object")
	# this will go away later
	if format=='boost::serialization':
		raise NotImplementedError("format='boost::serialization' not yet supported")
	elif format=='pickle':
		import pickle
		if hasFilename: pickle.dump(obj,out)
		else: out.write(pickle.dumps(obj))
	elif format=='expr' or format=='html': 
		#raise NotImplementedError("format='expr' not yet implemented")
		if hasFilename:
			import codecs
			out=codecs.open(out,'w','utf-8')
		if format=='expr':
			out.write(SerializerToExpr(maxWd=width)(obj))
		elif format=='html':
			if not fragment: out.write('<head><meta http-equiv="content-type" content="text/html;charset=UTF-8" /></head><body>\n')
			out.write(SerializerToHtmlTable()(obj))
			if not fragment: out.write('</body>')
		
class SerializerToHtmlTable:
	padding='cellpadding=2px'
	def __init__(self,maxDepth=8,hideNoGui=False):
		self.maxDepth=maxDepth
		self.hideNoGui=hideNoGui
	def htmlSeq(self,s,indent,insideTable):
		ret=''
		indent1=indent+'\t'
		indent2=indent1+'\t'
		if hasattr(s[0],'__len__') and not isinstance(s[0],str): # 2d array
			# disregard insideTable in this case
			ret+=indent+'<table frame=box rules=all width=100%% %s>'%self.padding
			for r in range(len(s)):
				ret+=indent1+'<tr>'
				for c in range(len(s[0])):
					ret+=indent2+'<td align=right width=%g%%>'%(100./len(s[0])-1.)+('%g'%s[r][c] if isinstance(s[r][c],float) else str(s[r][c]))
			ret+=indent+'</table>'
			return ret
		# 1d array
		if not insideTable: ret+=indent+'<table frame=box rules=all width=100%% %s>'%self.padding
		for e in s: ret+=indent1+'<td align="right" width=%g%%>'%(100./len(s)-1.)+('%g'%e if isinstance(e,float) else str(e))
		if not insideTable: ret+=indent+'</table>'
		return ret;
	def __call__(self,obj,depth=0):
		if depth>self.maxDepth: raise RuntimeError("Maximum nesting depth %d exceeded"%self.maxDepth)
		indent0=u'\n'+u'\t'*2*depth
		indent1=indent0+u'\t'
		indent2=indent1+u'\t'
		ret=indent0+'<table frame=box rules=all %s %s><th colspan="3"><b>%s</b></th>'%(self.padding, 'width="100%"' if depth>0 else '',obj.__class__.__name__)
		# get all attribute traits first
		traits=obj._getAllTraits()
		for trait in traits:
			if trait.hidden or (self.hideNoGui and trait.noGui): continue
			# start new group (additional line)
			if trait.startGroup:
				ret+=indent1+'<tr><td colspan=3><i>&#9656; %s</i></td></tr>'%trait.startGroup
			attr=getattr(obj,trait.name)
			ret+=indent1+'<tr>'+indent2+'<td>%s'%trait.name
			# nested object
			if isinstance(attr,yade.wrapper.Serializable):
				ret+=indent2+'<td align=justify>'+self(attr,depth+1)
			# sequence of objects (no units here)
			elif hasattr(attr,'__len__') and len(attr)>0 and isinstance(attr[0],yade.wrapper.Serializable):
				ret+=indent2+u'<td><ol>'+('<li>' if len(attr) else '')+u'<li>'.join([self(o,depth+1) for o in attr])+'</ol></td>'
			else:
				ret+=indent2+'<td align=right>'
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
					if len(attr)>0: ret+=self.htmlSeq(attr,indent=indent2+'\t',insideTable=False)
				else: ret+='%g'%attr if isinstance(attr,float) else str(attr)
				if unit: ret+=indent2+u'<td align="right">'+unit
			ret+=indent1+'</tr>'
		return ret+indent0+'</table>'

class SerializerToExpr:
	unbreakableTypes=(Vector2i,Vector2,Vector3i,Vector3)
	def __init__(self,indent='\t',maxWd=120):
		self.indent=indent
		self.indentLen=len(indent.replace('\t',3*' '))
		self.maxWd=maxWd
	def __call__(self,obj,level=0):
		if isinstance(obj,Serializable):
			attrs=[(trait.name,getattr(obj,trait.name)) for trait in obj._getAllTraits() if not trait.hidden]
			delims=obj.__class__.__name__+'(',')'
		elif isinstance(obj,dict):
			attrs=obj.items()
			delims='{','}'
		# list or mutable list-like objects (NodeList, for instance)
		elif hasattr(obj,'__len__') and hasattr(obj,'__getitem__') and hasattr(obj,'append'):
			attrs=[(None,obj[i]) for i in range(len(obj))]
			delims='[',']'
		# tuple and tuple-like objects: format like tuples
		elif sum([isinstance(obj,T) for T in (tuple,Vector2i,Vector2,Vector3i,Vector3,Matrix3,Vector6,Matrix6,VectorX,MatrixX,AlignedBox2,AlignedBox3)])>0:
			attrs=[(None,obj[i]) for i in range(len(obj))]
			delims='(',')'
		# don't know what to do, use repr (unhandled or primite types
		else:
			return repr(obj)
		lst=[(((attr[0]+'=') if attr[0] else '')+self(attr[1],level+1)) for attr in attrs]
		oneLiner=(sum([len(l) for l in lst])+self.indentLen<=self.maxWd or self.maxWd<=0 or type(obj) in self.unbreakableTypes) 
		if oneLiner: return delims[0]+','.join(lst)+delims[1]
		indent0,indent1=self.indent*level,self.indent*(level+1)
		return delims[0]+'\n'+indent1+(',\n'+indent1).join(lst)+'\n'+indent0+delims[1]+('\n' if level==0 else '')

Serializable._getAllTraits=_Serializable_getAllTraits
Serializable.save=_Serializable_save
