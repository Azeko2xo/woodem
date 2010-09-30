# -*- encoding=utf-8 -*-

from yade import utils

## PhysicalParameters 
Young = 7e6
Poisson = 0.2
Density=2700

O.bodies.append([
        utils.sphere([0,0,0.6],0.25,young=Young,poisson=Poisson,density=Density),
        utils.facet([[-0.707,-0.707,0.1],[0,1.414,0],[1.414,0,0]],dynamic=False,color=[1,0,0],young=Young,poisson=Poisson),
        utils.facet([[0,1.414,0],[1.414,0,0],[0.707,0.707,-2.0]],dynamic=False,color=[1,0,0],young=Young,poisson=Poisson)])

## Engines 
O.engines=[
	ForceResetter(),
	InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()]),
	IGeomDispatcher([Ig2_Sphere_Sphere_ScGeom(),Ig2_Facet_Sphere_ScGeom()]),
	IPhysDispatcher([MacroMicroElasticRelationships()]),
	ElasticContactLaw(),
	GravityEngine(gravity=(0,0,-10)),
	NewtonIntegrator(damping=.3)
]

## Timestep 
O.dt=5e-6
O.saveTmp()

try:
	from yade import qt
	renderer=qt.Renderer()
	renderer['Body_wire']=True
	renderer['Interaction_physics']=True
	qt.Controller()
except ImportError: pass

