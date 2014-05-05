# this script is shown at http://youtube.com/watch?w=-Q81oGxoz_s
from woo import *
from woo.core import *
from woo.dem import *
from math import *
from woo import log,utils,pack
from minieigen import *
import woo.pack

log.setLevel('ConveyorFactory',log.TRACE)
if 0:
	# quasi-packing here
	N=10
	centers=[Vector3(.2*n,sin(n/3.),0) for n in range(N)]
	radii=[.04+.02*(n%3) for n in range(N)]
	cellLen=1.2*max([c[0] for c in centers])
elif 0:
	p=pack.randomPeriPack(radius=.1,rRelFuzz=.3,memoizeDb='/tmp/wooPackCache.sqlite',initSize=(2.,2.,1.))
	cellLen=p.cellSize[0]
	centers=[c for c,r in p]
	radii=[.95*r for c,r in p] # avoid overlaps by making smaller artificially
else:
	# sp=woo.pack.makeBandFeedPack(dim=
	wd0,wd1,wd2,ht2=(.4,.7,.2,.1)
	botLine=[Vector2(0,ht2),Vector2(wd2,0),Vector2(wd2+wd0,0),Vector2(wd1,ht2)]
	sp=woo.pack.makeBandFeedPack(
		dim=(.3,.7,.3),psd=[],mat=woo.utils.defaultMaterial(),gravity=(0,0,-9.81),porosity=.5,damping=.3,memoizeDir='/tmp',botLine=botLine,dontBlock=False,returnSpherePack=False,
		# gen=PsdCapsuleGenerator(psdPts=[(.02,0),(.04,1.)],discrete=False,mass=True,shaftRadiusRatio=(.4,3.)),
		gen=PsdSphereGenerator(psdPts=[(.03,0),(.045,1.)]),
	)
	# woo.master.scene=S=sp
	# S.saveTmp()

if 1:
	S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,-10))])
	mat=utils.defaultMaterial()
	S.engines=utils.defaultEngines(damping=.2)+[
		ConveyorFactory(
			stepPeriod=10,
			shapePack=sp,
			#centers=centers,
			#radii=radii,
			vel=10,
			material=mat,
			# cellLen=cellLen,
			node=Node(pos=(0,0,0),ori=Quaternion((0,0,1),0)),
			startLen=5,
			barrierColor=.1,
			barrierLayer=-8.,
			color=.9,
			label='conveyor'
		),
		# VtkExport(out='/tmp/conv-',what=VtkExport.spheres|VtkExport.mesh,stepPeriod=20)
	]
	S.dem.par.append(utils.wall(-2.,axis=2,sense=1,mat=mat,glAB=((-1,-3),(10,3))))
	S.saveTmp()


