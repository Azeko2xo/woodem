from woo.dem import *
from woo.fem import *
from woo.core import *
import woo.utils
mat=woo.utils.defaultMaterial()

S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-10))])

# block rotations for now
nn=[Node(dem=DemData(mass=100,blocked='XYZ'),pos=p) for p in [(0,0,0),(1,0,0),(0,1,0),(0,0,1),(1,1,1)]]

t=Tet4.make(nn[0:4],mat=mat,wire=True,fixed=None)
t2=Tet4.make(nn[1:5],mat=mat,wire=True,fixed=None)
for n in nn[0:3]: n.dem.blocked='xyzXYZ'
#
#nn[3].dem.blocked='xyzXYZ'
#nn[3].dem.vel=(0,0,1)
# S.dtSafety=1e-1
#S.dt=1e-5

S.dem.par.add([t,t2])
for p in S.dem.par: p.shape.setRefConf()
S.dem.collectNodes()
#S.dt=1e-6
S.engines=DemField.minimalEngines(damping=.4)+[IntraForce([In2_Tet4_ElastMat()])]
	#PyRunner(1,'for p in S.dem.par: p.shape.update()')]
S.lab.collider.dead=True

S.saveTmp()

from woo.gl import *
Gl1_DemField(nodes=True,glyph=Gl1_DemField.glyphForce)
