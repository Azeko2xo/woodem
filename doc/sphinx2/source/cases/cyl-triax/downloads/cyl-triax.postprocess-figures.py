# encoding: utf-8
from woo.pre.cylTriax import plotBatchResults
sim='sim4d'
db='%s.batch.hdf5'%sim

def naturalTitleSorter(resultList):
	def naturalKey(sim):
		import re
		m=re.match('([0-9.]+)(:)([0-9.]+)(.*)',sim['title'])
		if m: return (float(m.group(1)),m.group(2),float(m.group(3)),m.group(4))
		else: return sim['title']
	return sorted(resultList,key=naturalKey)
	

for regex,out in [
	('[0-9.]+:[0-9.]+a','%s.res.a.pdf'%(sim)),
	('[0-9.]+:[0-9.]+b','%s.res.b.pdf'%(sim)),
	('[0-9.]+:[0-9.]+[abA]','%s.res.all.pdf'%(sim)),
	('1:1[aA]','%s.res.num.pdf'%sim),
	('1:1[ab]','%s.res.1_1.pdf'%sim),
	(r'1\.5:1[ab]','%s.res.15_1.pdf'%sim),
	('2:1[ab]','%s.res.2_1.pdf'%sim),
	('3:1[ab]','%s.res.3_1.pdf'%sim),
	]:
	try:
		plotBatchResults(db,titleRegex=regex,out=out,stressPath=False,sorter=naturalTitleSorter)
	except RuntimeError:
		print u'No figure generated for %s → %s'%(regex,out)
