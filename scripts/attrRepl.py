import os, sys, re
root=sys.argv[1]
for path,dirs,files in os.walk(root,topdown=True):
	for f in files:
		if f.split('.')[-1]!='hpp': continue
		#print path+' / '+f
		lout=[]; changed=0
		for l in open(path+'/'+f).readlines():
			l=l[:-1]
			maybe='Attr::' in l
			m=re.match(r'(.*),\s*\(?\s*((Attr::[a-zA-Z]+\s*\|?\s*)+)\s*\)?\s*,(.*)',l)
			if maybe and not l: print '@@@@@@@@',l
			if not m: continue
			spec=m.group(2).replace('Attr::','').split('|')
			#print spec
			sStat,sDyn=[],[]
			for s in spec:
				if s in ('noSave','readonly','hidden','pyByRef','triggerPostLoad'): sStat.append(s)
				else: sDyn.append(s)
			print spec,'AttrTrait<%s>()%s'%('|'.join(['Attr::%s'%s for s in sStat]),('.' if sDyn else '')+'.'.join(['%s()'%s for s in sDyn]))
			


