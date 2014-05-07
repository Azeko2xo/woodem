# this script is shown at http://youtube.com/watch?w=-Q81oGxoz_s
from woo import *
from woo.core import *
from woo.dem import *
from math import *
from woo import log,utils,pack
from minieigen import *
import woo.pack

log.setLevel('ConveyorFactory',log.TRACE)
wd0,wd1,wd2,ht2=(.4,.7,.2,.1)
botLine=[Vector2(0,ht2),Vector2(wd2,0),Vector2(wd2+wd0,0),Vector2(wd1,ht2)]
sp=woo.pack.makeBandFeedPack(
	dim=(.3,.7,.3),psd=[],mat=woo.utils.defaultMaterial(),gravity=(0,0,-9.81),porosity=.5,damping=.3,memoizeDir='/tmp',botLine=botLine,dontBlock=False,returnSpherePack=False,
	gen=PsdCapsuleGenerator(psdPts=[(.04,0),(.06,1.)],shaftRadiusRatio=(.4,3.)),
	#gen=PsdSphereGenerator(psdPts=[(.05,0),(.07,1.)]),
)

S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,-10))])
mat=utils.defaultMaterial()
S.engines=utils.defaultEngines(damping=.2)+[
	ConveyorFactory(
		stepPeriod=50,
		shapePack=sp,
		vel=1.,
		maxMass=150,
		material=mat,
		node=Node(pos=(0,0,0),ori=Quaternion((0,-1,0),.5)),
		startLen=5,
		barrierColor=.1,
		barrierLayer=-8.,
		color=.9,
		label='conveyor'
	),
]
S.dem.par.append(utils.wall(0,axis=2,sense=1,mat=mat,glAB=((-1,-3),(10,3))))
S.saveTmp()


