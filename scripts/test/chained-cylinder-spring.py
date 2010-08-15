#--- bruno.chareyre@hmg.inpg.fr ---
#!/usr/bin/python
# -*- coding: utf-8 -*-

# Experiment beam-like behaviour with chained cylinders + CohFrict connexions

from yade import utils
young=1.0e5
poisson=5
density=2.60e3 
frictionAngle=radians(30)
O.materials.append(CohFrictMat(young=young,poisson=poisson,density=density,frictionAngle=frictionAngle,label='mat'))
O.dt=1e-4

O.engines=[
	ForceResetter(),
	InsertionSortCollider([
		Bo1_ChainedCylinder_Aabb(),
		Bo1_Sphere_Aabb()
	]),
	InteractionDispatchers(
		[Ig2_ChainedCylinder_ChainedCylinder_ScGeom(),Ig2_Sphere_ChainedCylinder_CylScGeom()],
		[Ip2_2xCohFrictMat_CohFrictPhys(setCohesionNow=True,setCohesionOnNewContacts=True,normalCohesion=1e13,shearCohesion=1e13)],
		[Law2_ScGeom_CohFrictPhys_ElasticPlastic(momentRotationLaw=True,label='law')]
	),
	## Apply gravity
	GravityEngine(gravity=[0,-9.81,0]),
	## Motion equation
	NewtonIntegrator(damping=0.15),
	PeriodicPythonRunner(iterPeriod=500,command='history()'),
	PeriodicPythonRunner(iterPeriod=5000,command='if O.iter<21000 : yade.qt.center()')
]

#Generate a spiral
Ne=400
for i in range(0, Ne):
	omega=60.0/float(Ne); hy=0.10; hz=0.15;
	px=float(i)*(omega/60.0); py=sin(float(i)*omega)*hy; pz=cos(float(i)*omega)*hz;
	px2=float(i+1.)*(omega/60.0); py2=sin(float(i+1.)*omega)*hy; pz2=cos(float(i+1.)*omega)*hz;
	O.bodies.append(utils.chainedCylinder(begin=Vector3(pz,py,px), radius=0.005,end=Vector3(pz2,py2,px2),color=Vector3(0.6,0.5,0.5)))

def outp(id=1):
	for i in O.interactions:
		if i.id1 == 1:
			print i.phys.shearForce
			print i.phys.normalForce
			return  i

O.bodies[Ne-1].state.blockedDOFs=['x','y','z','rx','ry','rz']
yade.qt.View();


 #plot some results
from math import *
from yade import plot
plot.plots={'t':('pos1',None,'vel1')}
def history():
  	plot.addData(pos1=O.bodies[0].state.pos[1], # potential elastic energy
		     vel1=O.bodies[0].state.vel[1],
		     t=O.time)

#yade.qt.Renderer().bound=True
plot.plot()
O.saveTmp()
#O.bodies[0].state.angVel=Vector3(0.05,0,0)

