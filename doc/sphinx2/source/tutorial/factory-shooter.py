import woo, woo.pack, woo.utils; from woo.core import *; from woo.dem import *; from minieigen import *

S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,-10))])
S.dem.par.append(Wall.make(0,axis=2,sense=1,mat=woo.utils.defaultMaterial(),glAB=((-2,-2),(2,2))))
S.dem.collectNodes()

S.engines=woo.utils.defaultEngines(damping=.2)+[
	BoxFactory(
		stepPeriod=100,                           # run every 100 steps
		box=((0,0,0),(1,1,1)),                    # unit box for new particles
		massRate=2.,                              # limit rate of new particles
		materials=[woo.utils.defaultMaterial()],  # some material here
		# spheres with diameters distributed uniformly in dRange
		generator=MinMaxSphereGenerator(dRange=(.02,.05)), 
		# assign initial velocity in the given direction, with the magnitude in vRange
		shooter=AlignedMinMaxShooter(vRange=(1.,1.5),dir=(0,.7,.5)),
		label='feed'
	)
]
S.saveTmp()
