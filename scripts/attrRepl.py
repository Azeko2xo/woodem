import os, sys, re
root=sys.argv[1]
for path,dirs,files in os.walk(root,topdown=True):
	for f in files:
		if f.split('.')[-1]!='hpp': continue
		#print path+' / '+f
		lout=[]; changed=False
		for l in open(path+'/'+f).readlines():
			l=l[:-1]
			maybe='Attr::' in l
			m=re.match(r'^(.*),\s*\(?\s*((Attr::[a-zA-Z]+\s*\|?\s*)+)\s*\)?\s*,(.*)$',l)
			if maybe and not l: print '@@@@@@@@',l
			if not m:
				lout.append(l)
				continue
			changed=True
			spec=m.group(2).replace('Attr::','').split('|')
			sStat,sDyn=[],[]
			for s in spec:
				if s in ('noSave','readonly','hidden','pyByRef','triggerPostLoad'): sStat.append(s.strip())
				else: sDyn.append(s.strip())
			newSpec='AttrTrait<%s>()%s'%('|'.join(['Attr::%s'%s for s in sStat]),('.' if sDyn else '')+'.'.join(['%s()'%s for s in sDyn]))
			lout.append(m.group(1)+','+newSpec+','+m.group(4))
			#print lout[-1]
		if changed:
			print path+'/'+f
			#out=open(path+'/'+f,'w')
			#out.write('\n'.join(lout))



			


