# this script is shown at http://youtube.com/watch?w=-Q81oGxoz_s
from woo import *
from woo.core import *
from woo.dem import *
from math import *
from woo import log,utils,pack
from minieigen import *
log.setLevel('ConveyorFactory',log.TRACE)
if 0:
	# quasi-packing here
	N=10
	centers=[Vector3(.2*n,sin(n/3.),0) for n in range(N)]
	radii=[.04+.02*(n%3) for n in range(N)]
	cellLen=1.2*max([c[0] for c in centers])
else:
	p=pack.randomPeriPack(radius=.1,rRelFuzz=.3,memoizeDb='/tmp/wooPackCache.sqlite',initSize=(2.,2.,1.))
	cellLen=p.cellSize[0]
	centers=[c for c,r in p]
	radii=[.95*r for c,r in p] # avoid overlaps by making smaller artificially
	
s=O.scene
s.fields=[DemField()]
mat=utils.defaultMaterial()
s.dt=utils.spherePWaveDt(min(radii),mat.density,mat.young)
s.engines=utils.defaultEngines(damping=.2,gravity=(0,0,-10))+[
	ConveyorFactory(
		stepPeriod=10,
		centers=centers,
		radii=radii,
		vel=10,
		material=mat,
		cellLen=cellLen,
		node=Node(pos=(0,0,0),ori=Quaternion((0,0,1),0)),
		startLen=5,
		barrierColor=.1,
		barrierLayer=-8.,
		color=.9,
		label='conveyor'
	),
	VtkExport(out='/tmp/conv-',what=VtkExport.spheres|VtkExport.mesh,stepPeriod=20)
]
s.dem.par.append(utils.wall(-2.,axis=2,sense=1,mat=mat,glAB=((-1,-3),(10,3))))
# should be reverse-sorted by x
s.saveTmp()


