import woo.dem
import woo.core
import woo.log
from minieigen import *
S=woo.master.scene=woo.core.Scene(fields=[woo.dem.DemField(gravity=(0,0,-1))])
#psd=[(0.04,0),(.1,.7),(.2,1)]
psd=[(.1,0),(.2,1)]
#S.dem.par.add(woo.dem.Wall.make(.1,axis=2,sense=1))
z0=.05
mat=woo.dem.FrictMat(density=1000,young=1e4,tanPhi=.9)
matFloor=woo.dem.FrictMat(density=1000,young=1e8,tanPhi=.4)
S.dem.par.add(woo.triangulated.quadrilateral(Vector3(0,0,z0),Vector3(1,0,z0),Vector3(0,1,z0),Vector3(1,1,z0),mat=matFloor,halfThick=.05,div=(4,4)))
S.periodic=True
S.cell.setBox(1,1,2)
S.engines=[
	woo.dem.InsertionSortCollider([woo.dem.Bo1_Sphere_Aabb(),woo.dem.Bo1_Wall_Aabb(),woo.dem.Bo1_Facet_Aabb()]),
	woo.dem.BoxInlet(
		box=((0,0,.1),(1,1,2)),
		maxMass=-1,
		maxNum=-1,
		massRate=0,
		maxAttempts=10000,
		generator=woo.dem.PsdSphereGenerator(psdPts=psd,discrete=False,mass=True),
		# spatialBias=woo.dem.AxialBias(d01=(.02,.1),axis=2,fuzz=.5),
		spatialBias=woo.dem.PsdAxialBias(psdPts=psd,axis=2,fuzz=.1,discrete=False),
			materials=[mat],
		shooter=None,
	)
]
S.one()
woo.gl.Gl1_DemField.colorBy='radius'
S.trackEnergy=True
S.engines=woo.dem.DemField.minimalEngines(damping=.6)+[woo.core.PyRunner(100,'e=woo.utils.unbalancedEnergy(S)\nprint "E",e\nif e<.05: S.stop()')]
S.saveTmp()
#S.run(); S.wait()
#woo.triangulated.spheroidsToSTL(stl='/tmp/big.stl',dem=S.dem,tol=.001,clipCell=True)
#print 'Saving STL'
#woo.triangulated.spheroidsToSTL(stl='/tmp/spheroids.stl',dem=S.dem,tol=.001)
