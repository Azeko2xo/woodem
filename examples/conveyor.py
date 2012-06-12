from yade import *
from yade.core import *
from yade.dem import *
from math import *
from yade import log,utils
from miniEigen import *
log.setLevel('ConveyorFactory',log.TRACE)
# quasi-packing here
N=10
centers=[Vector3(.2*n,sin(n/3.),0) for n in range(N)]
radii=[.04+.02*(n%3) for n in range(N)]
s=O.scene
s.fields=[DemField()]
s.dt=1e-4
s.engines=utils.defaultEngines(damping=.4,gravity=(0,0,0))+[
	ConveyorFactory(
		stepPeriod=144,
		centers=centers,
		radii=radii,
		vel=10,
		material=utils.defaultMaterial(),
		cellLen=1.2*max([c[0] for c in centers]),
		node=Node(pos=(0,0,0)),
		startLen=10,
		barrierColor=.1,
		barrierLayer=-8.,
		color=.9,
		label='conveyor'
	)
]
O.dem.par.append(utils.wall(-.5,axis=2,sense=1,material=yade.conveyor.material))
# should be reverse-sorted by x
#print yade.conveyor.centers 
O.step()
O.saveTmp()


