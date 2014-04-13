import woo.core, woo.utils, woo
from woo.dem import *
from woo.gl import *
from minieigen import *
from math import *
import numpy
from random import random
S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,-10))])
S.dem.loneMask=0b0010
baseMask=0b0011
fallMask=0b0101

mat=FrictMat(young=1e6,density=3200)

#for x in numpy.arange(-1,1,.2):
#	for y in numpy.arange(-1,1,.3):
#		S.dem.par.append(woo.utils.capsule(center=(x,y,.2*sqrt(x**2+y**2)-.2),ori=Quaternion.Identity,radius=.1+.1*random(),shaft=.1+.1*random(),fixed=True,mask=baseMask,wire=True,color=.5))
if 1:
	import woo.pack
	cloud=woo.pack.SpherePack()
	cloud.makeCloud((-3,-3,.4),(3,3,5.5),rMean=.15,rRelFuzz=.6)
else:
	cloud=[((0,0,.3-.2),.2,Quaternion((0,1,0),pi/8.)),((.01,0,.7),.18,Quaternion(Vector3(0,1,0),pi/8.))]
for c,r in cloud:
	radius,shaft=.5*r*(.5*random()+.5),r*(.5*random()+.5)
	angVel=5*Vector3(random(),random(),random())
	ori=Quaternion(angVel.normalized(),random()*pi)
	# non-random values for testing
	# ori=Quaternion((0,1,0),pi/8.);
	#radius,shaft=r,3*r; angVel=(0,0,0)
	c=woo.utils.capsule(center=c,radius=radius,shaft=shaft,ori=ori,fixed=False,mask=fallMask,mat=mat)
	c.angVel=angVel
	S.dem.par.append(c)
# S.dem.par[0].shape.nodes[0].dem.blocked='xyzXYZ'
S.dem.par.append(woo.utils.wall(-.2,axis=2,sense=1))
S.dem.collectNodes()
S.dtSafety=.8
S.engines=woo.utils.defaultEngines(damping=.4,dynDtPeriod=10)
# S.any=[Gl1_Ellipsoid(wire=True),Gl1_DemField(cPhys=True,cNode=Gl1_DemField.cNodeNode),Renderer(iniViewDir=(0,1,0))]
# S.throttle=.03
S.saveTmp()
#S.stopAtStep=163
#S.run()
#Gl1_Capsule(quality=1,wire=False)
Renderer.allowFast=False
woo.master.timingEnabled=True
# Gl1_DemField(nodes=False,bound=False,cNode=0)
