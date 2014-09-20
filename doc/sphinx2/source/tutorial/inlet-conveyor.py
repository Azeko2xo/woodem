import woo, woo.pack, woo.utils
from woo.core import *
from woo.dem import *
from minieigen import *

# some material that we will use
mat=woo.utils.defaultMaterial()

# create the compact packing which will be used by the factory
sp=woo.pack.makeBandFeedPack(
	# dim: x-size, y-size, z-size
	# memoizeDir: cache result so that same input parameters return the result immediately
	dim=(.3,.7,.3),mat=mat,gravity=(0,0,-9.81),memoizeDir='/tmp',
	# generator which determines which particles will be there
	gen=PsdCapsuleGenerator(psdPts=[(.04,0),(.06,1.)],shaftRadiusRatio=(.4,3.))
)

# create new scene 
S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,-10))])
# inially only the plane is present
S.dem.par.add(Wall.make(0,axis=2,sense=1,mat=mat,glAB=((-1,-3),(10,3))))
S.dem.collectNodes()

S.engines=DemField.minimalEngines(damping=.2)+[
	# this is the factory engine
	ConveyorInlet(
		stepPeriod=100,     # run every 100 steps
		shapePack=sp,       # this is the packing which will be used
		vel=1.,             # linear velocity of particles
		maxMass=150,        # finish once this mass was generated
		doneHook='S.stop()',# run this once we finish
		material=mat,       # material for new particles
		# node which determines local coordinate system (particles move along +x initially)
		node=Node(pos=(0,0,.2),ori=Quaternion((0,-1,0),.5)),
	)
]

S.saveTmp()
#S.run()
#S.wait()

