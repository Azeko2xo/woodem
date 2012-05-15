from yade.core import *

padding='cellpadding=2px'

def htmlSeq(s,indent,insideTable):
	ret=''
	indent1=indent+'\t'
	indent2=indent1+'\t'
	if hasattr(s[0],'__len__') and not isinstance(s[0],str): # 2d array
		# disregard insideTable in this case
		ret+=indent+'<table frame=box rules=all width=100%% %s>'%padding
		for r in range(len(s)):
			ret+=indent1+'<tr>'
			for c in range(len(s[0])):
				ret+=indent2+'<td align=right width=%g%%>'%(100./len(s[0])-1.)+('%g'%s[r][c] if isinstance(s[r][c],float) else str(s[r][c]))
		ret+=indent+'</table>'
		return ret
	# 1d array
	if not insideTable: ret+=indent+'<table frame=box rules=all width=100%% %s>'%padding
	for e in s: ret+=indent1+'<td align="right" width=%g%%>'%(100./len(s)-1.)+('%g'%e if isinstance(e,float) else str(e))
	if not insideTable: ret+=indent+'</table>'
	return ret;
		
def htmlTable(obj,depth=0,maxDepth=8):
	if depth>maxDepth: raise RuntimeError("Maximum nesting depth %d exceeded"%maxDepth)
	indent0=u'\n'+u'\t'*2*depth
	indent1=indent0+u'\t'
	indent2=indent1+u'\t'
	ret=indent0+'<table frame=box rules=all %s %s><th colspan="3"><b>%s</b></th>'%(padding, 'width="100%"' if depth>0 else '',obj.__class__.__name__)
	# get all attribute traits first
	traits=[];k=obj.__class__
	while k!=yade.wrapper.Serializable:
		traits=k._attrTraits+traits
		k=k.__bases__[0]
	for trait in traits:
		if trait.hidden or trait.noGui: continue
		# start new group (additional line)
		if trait.startGroup:
			ret+=indent1+'<tr><td colspan=3><i>&#9656; %s</i></td></tr>'%trait.startGroup
		attr=getattr(obj,trait.name)
		ret+=indent1+'<tr>'+indent2+'<td>%s'%trait.name
		# nested object
		if isinstance(attr,yade.wrapper.Serializable):
			ret+=indent2+'<td align=justify>'+htmlTable(attr,depth+1,maxDepth)
		# sequence of objects (no units here)
		elif hasattr(attr,'__len__') and len(attr)>0 and isinstance(attr[0],yade.wrapper.Serializable):
			ret+=indent2+u'<td>'+u'<br>'.join([htmlTable(o,depth+1,maxDepth) for o in attr])
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
				if len(attr)>0: ret+=htmlSeq(attr,indent=indent2+'\t',insideTable=False)
			else: ret+='%g'%attr if isinstance(attr,float) else str(attr)
			if unit: ret+=indent2+u'<td align="right">'+unit
		ret+=indent1+'</tr>'
	return ret+indent0+'</table>'
import codecs
out=codecs.open('/tmp/rr.html','w','utf-8')
out.write('<head><meta http-equiv="content-type" content="text/html;charset=UTF-8" /></head><body>')
out.write('<h1>Preprocessor</h1>')
import yade.pre
rr=yade.pre.Roro()
for p in rr.psd: p[1]*=1e-1 # to fake percents
out.write(htmlTable(rr))

O.scene=rr()

# wait till there is a contact
while len([c for c in O.dem.con])==0: O.run(20,True)

out.write('<h1>Particles</h1>')
out.write(htmlTable(O.dem.par[10]))

out.write('<h1>Contacts</h1>')
c=[c for c in O.dem.con][0]
out.write(htmlTable(c))


#out.write('<h1>Engines</h1>')
#for e in O.scene.engines: out.write(htmlTable(e).encode('utf-8))

out.write('</body>')
out.close()
