O.bodies.append(utils.sphere([0,0,10],.5))
#O.bodies.append(utils.sphere([0,0,0],.5,dynamic=False))

O.engines=[
	BexResetter(),
	BoundDispatcher([InteractingSphere2AABB(),InteractingFacet2AABB()]),
	InsertionSortCollider(label='collider'),
	InteractionDispatchers(
		[ef2_Sphere_Sphere_Dem3DofGeom(),ef2_Facet_Sphere_Dem3DofGeom()],
		[SimpleElasticRelationships()],
		[Law2_Dem3Dof_Elastic_Elastic()],
	),
	GravityEngine(gravity=[0,0,-1e4]),
	NewtonIntegrator(damping=.1)
]
collider['sweepLength'],collider['nBins'],collider['binCoeff']=.5,2,2
O.dt=8e-2*utils.PWaveTimeStep()
O.saveTmp()
from yade import timing
O.timingEnabled=True
from yade import qt
r=qt.Renderer()
r['Body_bounding_volume']=True
v=qt.View(); qt.Controller()
v.ortho=True; #v.viewDir=O.bodies[0].phys.pos; v.lookAt=O.bodies[0].phys.pos;  v.upVector=(0,0,1); 
O.run(2,True)
