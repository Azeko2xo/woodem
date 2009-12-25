O.periodic=True
O.cell.refSize=Vector3(20,20,10)
from yade import pack,log,timing
O.materials.append(GranularMat(young=30e9,density=2400))
p=pack.SpherePack()
p.makeCloud(Vector3().ZERO,O.cell.refSize,1,.5,700,True)
for sph in p:
	O.bodies.append(utils.sphere(sph[0],sph[1]))

#log.setLevel("PeriIsoCompressor",log.TRACE)
O.timingEnabled=True
O.engines=[
	ForceResetter(),
	BoundDispatcher([Bo1_Sphere_Aabb()]),
	InsertionSortCollider(),
	InteractionDispatchers(
		[ef2_Sphere_Sphere_Dem3DofGeom()],
		[SimpleElasticRelationships()],
		[Law2_Dem3Dof_Elastic_Elastic()],
	),
	PeriIsoCompressor(charLen=.5,stresses=[-50e9,-1e8],doneHook="print 'FINISHED'; O.pause() ",keepProportions=True),
	NewtonIntegrator(damping=.4)
]
O.dt=utils.PWaveTimeStep()
O.saveTmp()
print O.cell.refSize
from yade import qt; qt.Controller(); qt.View()
O.run()
O.wait()
timing.stats()

# now take that packing and pad some larger volume with it
#sp=pack.SpherePack()
#sp.fromSimulation() # take spheres from simulation; cellSize is set as well
#O.reset()
#print sp.cellSize
#sp.cellFill((30,30,30))
#print sp.cellSize
#for s in sp:
#	O.bodies.append(utils.sphere(s[0],s[1]))
