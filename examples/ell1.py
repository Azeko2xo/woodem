import woo.core, woo
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

for x in numpy.arange(-1,1,.2):
	for y in numpy.arange(-1,1,.3):
		S.dem.par.append(woo.utils.ellipsoid(center=(x,y,.2*sqrt(x**2+y**2)-.2),ori=Quaternion.Identity,semiAxes=(.1+.1*random(),.15+.1*random(),.1+.1*random()),fixed=True,mask=baseMask,wire=True,color=.5))
import woo.pack
cloud=woo.pack.SpherePack()
cloud.makeCloud((-.8,-.8,.4),(.8,.8,5.5),rMean=.15,rRelFuzz=.6)
for c,r in cloud:
	semiAxes=r*Vector3(.5*random()+.5,.5*random()+.5,.5*random()+.5)
	angVel=.5*Vector3(random(),random(),random())
	ori=Quaternion(angVel.normalized(),random()*pi)
	S.dem.par.append(
		woo.utils.ellipsoid(center=c,ori=ori,semiAxes=semiAxes,angVel=angVel,fixed=False,mask=fallMask)
	)
S.dem.collectNodes()
S.dtSafety=.1
S.engines=[Leapfrog(reset=True,damping=.4),InsertionSortCollider([Bo1_Ellipsoid_Aabb()]),ContactLoop([Cg2_Ellipsoid_Ellipsoid_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()]),DynDt(stepPeriod=1000)]
S.dt=1e-4
S.saveTmp()
# S.any=[Gl1_Ellipsoid(wire=True),Gl1_DemField(cPhys=True,cNode=Gl1_DemField.cNodeNode),Renderer(iniViewDir=(0,1,0))]
Gl1_Ellipsoid(quality=3)

