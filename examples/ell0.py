import woo.core, woo
from woo.dem import *
from woo.gl import *
from minieigen import *
from math import *
S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,0))])
S.dem.par.append([
	woo.utils.ellipsoid(center=(0,0,0),ori=Quaternion.Identity,semiAxes=(.1,.2,.3),fixed=True),
	woo.utils.ellipsoid(center=(.25,0,-.08),ori=Quaternion((0,1,0),pi/4.),semiAxes=(.2,.1,.1),angVel=(0,.5,0),fixed=True),
	woo.utils.wall(-.25,axis=2,sense=0)
])
S.dem.collectNodes()
S.engines=[Leapfrog(reset=True),InsertionSortCollider([Bo1_Ellipsoid_Aabb(),Bo1_Wall_Aabb()],verletDist=0.),ContactLoop([Cg2_Ellipsoid_Ellipsoid_L6Geom(),Cg2_Wall_Ellipsoid_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()])]

S.any=[Gl1_Ellipsoid(wire=True),Gl1_DemField(cPhys=True,cNode=Gl1_DemField.cNodeNode,bound=True),Renderer(iniViewDir=(0,1,0))]
