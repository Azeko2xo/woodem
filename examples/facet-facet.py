import woo
from woo.dem import *
from woo.core import *
from random import random as rnd

import woo.log, woo.utils
from minieigen import *
from math import *
# woo.log.setLevel('Cg2_Facet_Facet_L6Geom',woo.log.TRACE)

m=woo.utils.defaultMaterial()
S=woo.master.scene=Scene(
	engines=DemField.minimalEngines(damping=.4)+[IntraForce([In2_FlexFacet_ElastMat(applyBary=True)])],
	fields=[DemField(
		gravity=(0,0,-10),
		loneMask=0, # no loneMask at all
	)],
)

for i in range(200):
	o=Vector3((1.3+1e-2*i)*sin(i*.9),(1.3+1e-2*i)*cos(i*.9),.18*i+1)
	q=Quaternion(Vector3(rnd(),rnd(),rnd()),rnd())
	q.normalize();
	pp=[Vector3(0,0,0),Vector3(.3,0,0),Vector3(0,.4,0)]
	f=Facet.make([o+q*p for p in pp],halfThick=.1+.2*rnd(),mat=m,flex=True,fixed=False,wire=False)
	for n in f.shape.nodes:
		n.dem.mass=100
		n.dem.inertia=(10,10,10)
	S.dem.par.appendClumped([f])

S.dem.par.append([
	Facet.make([(-2,-2,0),(2,0,0),(0,2,0)],halfThick=.6,mat=m,fixed=True,wire=True),
	Wall.make(-.7,sense=1,axis=2,mat=m),
	InfCylinder.make((0,-2,-.5),radius=1.,axis=0,mat=m,wire=True)
])



S.dt=1e-4
S.saveTmp()
S.one()
