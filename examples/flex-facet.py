from woo.core import *
from woo.dem import *
import woo
import woo.gl
woo.gl.Gl1_DemField.nodes=True
nn=[Node(pos=(1,0,0)),Node(pos=(0,1,0)),Node(pos=(0,0,1))]
for n in nn:
	n.dem=DemData(inertia=(1,1,1))
	n.dem.blocked='xyzXYZ'
#nn[0].dem.vel=(1.,0,0)
### orientation is computed WRONG:
### global x-directed angVel contributes only to LOCAL phi_x on the facet!!!
nn[0].dem.angVel=(10.,0,0.)
#nn[1].dem.vel=(0,0,0)
#nn[2].dem.vel=(0,0,0)
S=woo.master.scene=Scene(fields=[DemField()])
S.dem.par.append(Particle(shape=FlexFacet(nodes=nn),material=FrictMat()))
ff=S.dem.par[0].shape
ff.setRefConf() #update()
for n in nn: S.dem.nodes.append(n)
S.engines=[Leapfrog(reset=True),PyRunner(1,'S.dem.par[0].shape.update()')]
S.dt=1e-6
S.saveTmp()
