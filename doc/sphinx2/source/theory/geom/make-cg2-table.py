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

def type2sphinx(t,name=None):
	if name==None: return ':obj:`~%s.%s`'%(t.__module__,t.__name__)
	else: return ':obj:`%s <%s.%s>`'%(name,t.__module__,t.__name__)

t=prettytable.PrettyTable(['']+[type2sphinx(s) for s in ss])
for s1 in ss:
	row=[type2sphinx(s1)] # header column
	for s2 in ss:
		gg=[g for g in ggg if sorted(g().bases)==sorted([s1.__name__,s2.__name__])]
		cell=[]
		for g in gg:
			if g.__name__.endswith('L6Geom'): cell+=[type2sphinx(g,'l6g')]
			elif g.__name__.endswith('G3Geom'): cell+=[type2sphinx(g,'g3g')]
			else: raise RuntimError('CGeomFunctor name does not end in L6Geom or G3Geom.')
		row.append(' ,'.join(cell))
	t.add_row(row)
	# print row

print t
