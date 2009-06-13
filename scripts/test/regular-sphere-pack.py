from yade import pack

rad,gap=.15,.02
rho=1e3
kw={'density':rho}

O.bodies.append(
	pack.regularHexa(pack.inSphere((0,0,4),2),radius=rad,gap=gap,color=(0,1,0),density=10*rho) # head
	+[utils.sphere((.8,1.9,5),radius=.2,color=(.6,.6,.6),density=rho),utils.sphere((-.8,1.9,5),radius=.2,color=(.6,.6,.6),density=rho),utils.sphere((0,2.4,4),radius=.4,color=(1,0,0),density=rho)] # eyes and nose
	+[utils.facet([(12,0,-6),(0,12,-6,),(-12,-12,-6)],dynamic=False)] # ground
)

for part in [
	pack.regularHexa (pack.inAlignedBox((-2,-2,-2),(2,2,2)),radius=1.5*rad,gap=2*gap,color=(1,0,1),**kw), # body,
	pack.regularOrtho(pack.inCylinder((-1,0,-2),(-1,0,-6),1),radius=rad,gap=gap,color=(0,1,1),**kw), # left leg
	pack.regularHexa (pack.inCylinder((+1,1,-2.5),(0,3,-5),1),radius=rad,gap=gap,color=(0,1,1),**kw), # right leg
	pack.regularHexa (pack.inCylinder((+2,0,1),(+6,0,1),1),radius=rad,gap=gap,color=(0,0,1),**kw), # right hand
	pack.regularOrtho(pack.inCylinder((-2,0,2),(-5,0,4),1),radius=rad,gap=gap,color=(0,0,1),**kw) # left hand
	]: O.bodies.appendClumped(part)



try:
	from yade import qt
	qt.Controller()
except ImportError: pass

O.engines=[
	BexResetter(),
	BoundingVolumeMetaEngine([InteractingSphere2AABB(),InteractingFacet2AABB(),MetaInteractingGeometry2AABB()]),
	InsertionSortCollider(),
	InteractionDispatchers(
		[ef2_Sphere_Sphere_Dem3DofGeom(),ef2_Facet_Sphere_Dem3DofGeom()],
		[SimpleElasticRelationships()],
		[ef2_Dem3Dof_Elastic_ElasticLaw()],
	),
	GravityEngine(gravity=(1e-2,1e-2,-1000)),
	NewtonsDampedLaw(damping=.1)
]
# we don't care about physical accuracy here, over-critical step is fine as long as the simulation doesn't explode
O.dt=2*utils.PWaveTimeStep()
O.saveTmp('init')
#for b in O.bodies: 
