#--- bruno.chareyre@hmg.inpg.fr ---
#!/usr/bin/python
# -*- coding: utf-8 -*-

# Experiment beam-like behaviour with chained cylinders + CohFrict connexions

from yade import utils,pack
import math

young=4.0e6
poisson=3
density=1e3
frictionAngle1=radians(15)
frictionAngle2=radians(15)
frictionAngle3=radians(15)
O.dt=5e-5

O.materials.append(FrictMat(young=4000000.0,poisson=0.5,frictionAngle=frictionAngle1,density=1600,label='spheremat'))
O.materials.append(CohFrictMat(young=1.0e6,poisson=0.2,density=2.60e3,frictionAngle=frictionAngle2,label='walllmat'))

Ns=10
sp=pack.SpherePack()

#cohesive spheres crash because sphere-cylnder functor geneteragets ScGeom3D
#O.materials.append(CohFrictMat(young=1.0e5,poisson=0.03,density=2.60e2,frictionAngle=frictionAngle,label='spheremat'))

if os.path.exists("cloud4cylinders"):
 	print "loading spheres from file"
	sp.load("cloud4cylinders")
	Ns=0
	for x in sp: Ns=Ns+1 #is there another way?
else:
 	print "generating spheres"
 	Ns=sp.makeCloud(Vector3(-0.3,0.2,-1.0),Vector3(+0.3,+0.5,+1.0),-1,.2,Ns,False,0.9)
	sp.save("cloud4cylinders")

O.bodies.append([utils.sphere(center,rad,material='spheremat') for center,rad in sp])

#Ns=1
#O.bodies.append(utils.sphere(Vector3(0.,0.4,0.0),0.1,material='spheremat'))

#O.materials.append(FrictMat(young=150e6,poisson=.4,frictionAngle=.2,density=2600,label='wallmat'))#this one crash
walls=utils.aabbWalls((Vector3(-0.3,-0.15,-1),Vector3(+0.3,+1.0,+1)),thickness=.1,material='walllmat')
wallIds=O.bodies.append(walls)

O.initializers=[
	## Create bounding boxes. They are needed to zoom the 3d view properly before we start the simulation.
	BoundDispatcher([Bo1_Sphere_Aabb(),Bo1_ChainedCylinder_Aabb(),Bo1_Box_Aabb()])
]

O.engines=[
	ForceResetter(),
	InsertionSortCollider([
		Bo1_ChainedCylinder_Aabb(),
		Bo1_Sphere_Aabb(),
		Bo1_Box_Aabb()
	]),
	InteractionLoop(
		[Ig2_ChainedCylinder_ChainedCylinder_ScGeom6D(), Ig2_Sphere_ChainedCylinder_CylScGeom(), Ig2_Sphere_Sphere_ScGeom6D(),Ig2_Box_Sphere_ScGeom6D()],
		#[Ig2_ChainedCylinder_ChainedCylinder_ScGeom6D()],
		[Ip2_2xCohFrictMat_CohFrictPhys(setCohesionNow=True,setCohesionOnNewContacts=False,label='ipf'),Ip2_FrictMat_FrictMat_FrictPhys()],
		[Law2_ScGeom6D_CohFrictPhys_CohesionMoment(label='law'),Law2_ScGeom_FrictPhys_CundallStrack(),Law2_CylScGeom_FrictPhys_CundallStrack()]
	),
	## Apply gravity
	GravityEngine(gravity=[1,-9.81,0],label='gravity'),
	## Motion equation
	NewtonIntegrator(damping=0.10,label='newton'),
	PyRunner(iterPeriod=500,command='history()'),
	#PyRunner(iterPeriod=5000,command='if O.iter<21000 : yade.qt.center()')
]

#Generate a spiral
O.materials.append(CohFrictMat(young=young,poisson=poisson,density=3.0*density,frictionAngle=frictionAngle3,normalCohesion=1e40,shearCohesion=1e40,momentRotationLaw=True,label='cylindermat'))
Ne=30
dy=0.03
dx=0.2
dz=0.2
Nc=1 #nb. of additional chains
for j in range(-Nc, Nc+1):
	dyj = abs(float(j))*dy
	dxj = abs(float(j))*dx
	dzj = float(j)*dz
	for i in range(0, Ne):
		omega=20/float(Ne); hy=0.0; hz=0.05; hx=1.5;
		px=float(i)*hx/float(Ne)-0.8+dxj; py=sin(float(i)*omega)*hy+dyj; pz=cos(float(i)*omega)*hz+dzj;
		px2=float(i+1.)*hx/float(Ne)-0.8+dxj; py2=sin(float(i+1.)*omega)*hy+dyj; pz2=cos(float(i+1.)*omega)*hz+dzj;
		utils.chainedCylinder(begin=Vector3(pz,py,px), radius=0.02,end=Vector3(pz2,py2,px2),color=Vector3(0.6,0.5,0.5),material='cylindermat')
		if (i == Ne-1): #close the chain with a node of size 0
			print "closing chain"
			b=utils.chainedCylinder(begin=Vector3(pz2,py2,px2), radius=0.02,end=Vector3(pz2,py2,px2),color=Vector3(0.6,0.5,0.5),material='cylindermat')
			b.state.blockedDOFs=['x','y','z','rx','ry','rz']
	ChainedState.currentChain=ChainedState.currentChain+1

def outp(id=1):
	for i in O.interactions:
		if i.id1 == 1:
			print i.phys.shearForce
			print i.phys.normalForce
			return  i
#from yade import qt
#qt.View()
#qt.Controller()
yade.qt.View();


 #plot some results
#from math import *
from yade import plot
plot.plots={'t':('pos1',None,'vel1')}
def history():
  	plot.addData(pos1=O.bodies[6+Ns].state.pos[1], # potential elastic energy
		     vel1=O.bodies[6+Ns].state.vel[1],
		     t=O.time)

O.run(1,True)
#ipf.setCohesiononNewContacts=False
#yade.qt.Renderer().bound=True
#plot.plot()
#O.bodies[0].state.angVel=Vector3(0.05,0,0)

