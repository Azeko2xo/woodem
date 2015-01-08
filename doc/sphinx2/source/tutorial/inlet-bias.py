import woo.dem, woo.core, woo.log, math
from minieigen import *
S=woo.master.scene=woo.core.Scene(fields=[woo.dem.DemField()])

psd=[(.05,0),(.08,.4),(.12,.7),(.2,1)]

# attributes common to all inlets
inletKw=dict(maxMass=-1,maxNum=-1,massRate=0,maxAttempts=5000,materials=[woo.dem.FrictMat(density=1000,young=1e4,tanPhi=.9)])

S.engines=[
	woo.dem.InsertionSortCollider([woo.dem.Bo1_Sphere_Aabb()]),
	# regular bias
	woo.dem.BoxInlet(
		box=((0,0,0),(1,1,2)),
		generator=woo.dem.PsdSphereGenerator(psdPts=psd,discrete=False,mass=True),
		spatialBias=woo.dem.PsdAxialBias(psdPts=psd,axis=2,fuzz=.1,discrete=False),
		**inletKw
	),
	# inverted bias, with capsules
	woo.dem.BoxInlet(
		box=((1.5,0,0),(2.5,1,2)),
		generator=woo.dem.PsdCapsuleGenerator(psdPts=psd,shaftRadiusRatio=(.8,1.5)),
		spatialBias=woo.dem.PsdAxialBias(psdPts=psd,axis=2,fuzz=.1,invert=True),
		**inletKw
	),
	# discrete PSD
	woo.dem.BoxInlet(
		box=((3,0,0),(4,1,2)),
		generator=woo.dem.PsdSphereGenerator(psdPts=psd,discrete=True),
		spatialBias=woo.dem.PsdAxialBias(psdPts=psd,axis=2,fuzz=.1,discrete=True),
		**inletKw
	),
	# reordered discrete PSD
	woo.dem.BoxInlet(
		box=((4.5,0,0),(5.5,1,2)),
		generator=woo.dem.PsdSphereGenerator(psdPts=psd,discrete=True),
		spatialBias=woo.dem.PsdAxialBias(psdPts=psd,axis=2,discrete=True,reorder=[2,0,1,3]),
		**inletKw
	),
	# radial bias in arc inlet
	woo.dem.ArcInlet(
		node=woo.core.Node(pos=(7,1,.65),ori=Quaternion((1,0,0),math.pi/2)),cylBox=((.2,-math.pi/2,0),(1.3,.75*math.pi,1)),
		generator=woo.dem.PsdSphereGenerator(psdPts=psd),
		spatialBias=woo.dem.PsdAxialBias(psdPts=psd,axis=0,fuzz=.7),
		**inletKw
	),
]

# actually generate the particles
S.one()

woo.gl.Gl1_DemField.colorBy='radius'
woo.gl.Renderer.iniViewDir=(0,-1,0)

# abuse DEM nodes to stick labels on the top
for i,t in enumerate(['graded','inverted (capsules)','discrete','reordered','radial']):
	S.dem.nodesAppend(woo.core.Node((1+i*1.5,0,2.2),dem=woo.dem.DemData(),rep=woo.gl.LabelGlRep(text=t)))
