import woo.dem
import woo.core
import woo.log
S=woo.master.scene=woo.core.Scene(fields=[woo.dem.DemField()])
psd=[(0.02,0),(.04,.8),(.1,1)]
S.engines=[
	woo.dem.InsertionSortCollider([woo.dem.Bo1_Sphere_Aabb()]),
	woo.dem.BoxInlet(
		box=((0,0,0),(1,1,2)),
		maxMass=-1,
		maxNum=-1,
		massRate=0,
		maxAttempts=1000,
		generator=woo.dem.PsdSphereGenerator(psdPts=psd,discrete=True,mass=True),
		#spatialBias=woo.dem.AxialBias(d01=(.02,.1),axis=2,fuzz=.5),
		spatialBias=woo.dem.PsdAxialBias(psdPts=psd,axis=2,fuzz=.1,discrete=True),
		materials=[woo.dem.ElastMat(density=1)], # must have some density
		shooter=None,
	)
]
S.one()
woo.gl.Gl1_DemField.colorBy='radius'
