from woo.core import *
from woo.dem import *
import woo
from minieigen import *

import woo.log
woo.log.setLevel('Cg2_Shape_Shape_L6Geom__Potential',woo.log.TRACE)

S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,-10))])
S.engines=[Leapfrog(reset=True),InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Wall_Aabb()]),ContactLoop(
	#[],
	[Cg2_Shape_Shape_L6Geom__Potential([Pot1_Sphere(),Pot1_Wall()])],
	[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()]),DynDt(stepPeriod=100)]
mat=woo.utils.defaultMaterial()
S.dem.par.append([
	Sphere.make((0,0,.201),radius=.2,mat=mat),
	Wall.make(0,sense=1,axis=2,mat=mat)
])
S.dem.collectNodes()
