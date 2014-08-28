from woo.dem import *
from woo.fem import *
from woo.core import *
import woo.utils
mat=woo.utils.defaultMaterial()

S=woo.master.scene=Scene(fields=[DemField()])

def padTetra(t,halfThick,m=None):
	if m==None: m=t.material
	ret=[t]
	nn=t.shape.nodes
	for v in ((0,1,2),(0,1,3),(1,2,3),(0,2,3)):
		ret.append(Facet.make([nn[v[0]],nn[v[1]],nn[v[2]]],mat=m,wire=True))
	return ret

t=Tetra.make([(0,0,0),(1,0,0),(0,1,0),(0,0,1)],mat=mat)
S.dem.par.add(padTetra(t,halfThick=.2))
S.engines=DemField.minimalEngines()
