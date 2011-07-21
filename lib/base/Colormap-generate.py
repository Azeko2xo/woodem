import sys,re
for f in sys.argv[1:]:
	cm=' '.join([l.strip() for l in open(f).readlines()])
	m=re.match(r'.*static\s+int\s+cmap_(?P<name>\w+)\s*\[\]\s*=\s*{(?P<nums>[^}]*)};.*',cm)
	if not m: raise RuntimeError('No match for colormap in %s'%f)
	name=m.groupdict()['name']
	nums=eval('('+m.groupdict()['nums']+')')
	isLast=(f==sys.argv[-1])
	print '\t\tColormap{"'+name+'",{'+','.join(['%.4f'%(f/255.) for f in nums])+'}}'+('' if isLast else ',')
