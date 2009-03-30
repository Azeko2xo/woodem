#!/usr/local/bin/yade-trunk -x
# -*- encoding=utf-8 -*-

## Omega is the super-class that orchestrates the whole program.
## It holds the entire simulation (MetaBody), takes care of loading/saving,
## starting/stopping the simulation, loads plugins and more.

o=Omega() # for advaned folks: this creates default MetaBody as well

## Initializers are run before the simulation.
o.initializers=[
	## Create bounding boxes. They are needed to zoom the 3d view properly before we start the simulation.
	MetaEngine('BoundingVolumeMetaEngine',[EngineUnit('InteractingSphere2AABB'),EngineUnit('InteractingBox2AABB'),EngineUnit('MetaInteractingGeometry2AABB')])
	]

## Engines are called consecutively at each iteration. Their order matters.
##
## Some of them work by themselves (StandAloneEngine, DeusExMachina - the difference of these two is unimportant).
##
## MetaEngines act as dispatchers and based on the type of objects they operate on, different EngineUnits are called.
o.engines=[
	## Resets forces and momenta the act on bodies
	StandAloneEngine('PhysicalActionContainerReseter'),
	## associates bounding volume - in this case, AxisAlignedBoundingBox (AABB) - to each body.
	## MetaEngine calls corresponding EngineUnit, depending on whether the body is Sphere, Box, or MetaBody (rootBody).
	## AABBs will be used to detect collisions later, by PersistentSAPCollider
	MetaEngine('BoundingVolumeMetaEngine',[
		EngineUnit('InteractingSphere2AABB'),
		EngineUnit('InteractingBox2AABB'),
		EngineUnit('MetaInteractingGeometry2AABB')
	]),
	## Using bounding boxes created by the previous engine, find possible body collisions.
	## These possible collisions are inserted in Omega.interactions container (MetaBody::transientInteractions in c++).
	StandAloneEngine('PersistentSAPCollider'),
	## Decide whether the potential collisions are real; if so, create geometry information about each potential collision.
	## Here, the decision about which EngineUnit to use depends on types of _both_ bodies.
	## Note that there is no EngineUnit for box-box collision. They are not implemented.
	MetaEngine('InteractionGeometryMetaEngine',[
		EngineUnit('InteractingSphere2InteractingSphere4SpheresContactGeometry'),
		EngineUnit('InteractingBox2InteractingSphere4SpheresContactGeometry')
	]),
	## Create physical information about the interaction.
	## This may consist in deriving contact rigidity from elastic moduli of each body, for example.
	## The purpose is that the contact may be "solved" without reference to related bodies,
	## only with the information contained in contact geometry and physics.
	MetaEngine('InteractionPhysicsMetaEngine',[EngineUnit('SimpleElasticRelationships')]),
	## "Solver" of the contact, also called (consitutive) law.
	## Based on the information in interaction physics and geometry, it applies corresponding forces on bodies in interaction.
	StandAloneEngine('ElasticContactLaw'),
	## Apply gravity: all bodies will have gravity applied on them.
	## Note the engine parameter 'gravity', a vector that gives the acceleration.
	DeusExMachina('GravityEngine',{'gravity':[0,0,-9.81]}),
	## Forces acting on bodies are damped to artificially increase energy dissipation in simulation.
	## (In this model, the restitution coefficient of interaction is 1, which is not realistic.)
	## This MetaEngine acts on all PhysicalActions and selects the right EngineUnit base on type of the PhysicalAction.
	#
	# note that following 4 engines (till the end) can be replaced by an optimized monolithic version:
#	DeusExMachina('NewtonsDampedLaw',{'damping':0.0}),
	#
	MetaEngine('PhysicalActionDamper',[
		EngineUnit('CundallNonViscousForceDamping',{'damping':0.2}),
		EngineUnit('CundallNonViscousMomentumDamping',{'damping':0.2})
	]),
	## Now we have forces and momenta acting on bodies. Newton's law calculates acceleration that corresponds to them.
	MetaEngine('PhysicalActionApplier',[
		EngineUnit('NewtonsForceLaw'),
		EngineUnit('NewtonsMomentumLaw'),
	]),
	## Acceleration results in velocity change. Integrating the velocity over dt, position of the body will change.
	MetaEngine('PhysicalParametersMetaEngine',[EngineUnit('LeapFrogPositionIntegrator')]),
	## Angular acceleration changes angular velocity, resulting in position and/or orientation change of the body.
	MetaEngine('PhysicalParametersMetaEngine',[EngineUnit('LeapFrogOrientationIntegrator')]),
]


## The yade.utils module contains some handy functions, like yade.utils.box and yade.utils.sphere.
## After this import, they will be accessible as utils.box and utils.sphere.
from yade import utils

## create bodies in the simulation: one box in the origin and one floating above it.
##
## The box:
## * extents: half-size of the box. [.5,.5,.5] is unit cube
## * center: position of the center of the box
## * dynamic: it is not dynamic, i.e. will not move during simulation, even if forces are applied to it
## * color: for the 3d display; specified  within unit cube in the RGB space; [1,0,0] is, therefore, red
## * young: Young's modulus
## * poisson: Poissons's ratio

o.bodies.append(utils.box(center=[0,0,0],extents=[.5,.5,.5],dynamic=False,color=[1,0,0],young=30e9,poisson=.3,density=2400))

## The above command could be actully written without the util.box function like this:
## (will not be executed, since the condition is never True)
if False:
	# Create empty body object
	b=Body()
	# set the isDynamic body attribute
	b['isDynamic']=False
	# Assign geometrical model (shape) to the body: a box of given size
	b.shape=GeometricalModel('Box',{'extents':[.5,.5,.5],'diffuseColor':[1,0,0]})
	# Assign computational model (mold; may be simplified form of shape) to the body
	b.mold=InteractingGeometry('InteractingBox',{'extents':[.5,.5,.5],'diffuseColor':[1,0,0]})
	# physical parameters:
	# store mass to a temporary
	mass=8*.5*.5*.5*2400
	# * se3 (position & orientation) as 3 position coordinates, then 3 direction axis coordinates and rotation angle
	b.phys=PhysicalParameters('BodyMacroParameters',{'se3':[0,0,0,1,0,0,0],'mass':mass,'inertia':[mass*4*(.5**2+.5**2),mass*4*(.5**2+.5**2),mass*4*(.5**2+.5**2)],'young':30e9,'poisson':.3})
	# other information about AABB will be updated during simulation by relevant BoundingVolumeMetaEngine
	b.bound=BoundingVolume('AABB',{'diffuseColor':[0,1,0]})
	# add the body to the simulation
	o.bodies.append(b)


## The sphere
##
## * First two arguments are radius and center, respectively. They are used as "positional arguments" here:
## python will deduce based on where they are what they mean.
##
## It could also be written without using utils.sphere - see gui/py/utils.py for the code of the utils.sphere function
o.bodies.append(utils.sphere([0,0,2],1,color=[0,1,0],young=30e9,poisson=.3,density=2400))

## Estimate timestep from p-wave speed and multiply it by safety factor of .2
o.dt=.2*utils.PWaveTimeStep()

## Save the scene to file, so that it can be loaded later. Supported extension are: .xml, .xml.gz, .xml.bz2.
o.save('/tmp/a.xml.bz2');
#o.run(100000); o.wait(); print o.iter/o.realtime,'iterations/sec'

from yade import qt
qt.Controller()
