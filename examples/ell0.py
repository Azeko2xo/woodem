import woo.core, woo
from woo.dem import *
from woo.gl import *
from minieigen import *
from math import *
S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,0))])
S.dem.par.add([
	woo.utils.ellipsoid(center=(0,0,0),ori=Quaternion.Identity,semiAxes=(.1,.2,.3),fixed=True),
	woo.utils.ellipsoid(center=(.25,0,-.08),ori=Quaternion((0,1,0),pi/4.),semiAxes=(.2,.3,.1),angVel=(0,.5,0),fixed=True),
	# woo.utils.wall(-.25,glAB=((-.5,-.5),(.5,.5)),axis=2,sense=0),
	# woo.utils.facet([(.3,0,.2),(.4,.2,0),(.4,-.2,0)])
	# woo.utils.facet([(.3,0,.2),(.4,-.2,0),(.4,.2,0)])
	Capsule.make((.3,.3,.3),radius=.2,shaft=.2,ori=Quaternion((0,1,0),pi/4),fixed=True)
])
S.dem.par[-1].angVel=(0,.4,0)
S.periodic=True
S.cell.setBox(2,2,2)
#print S.cell.trsf
#S.cell.trsf=Matrix3(1,0,-.5,.5,1,0,0,0,1)
S.cell.nextGradV=(0,0,.2,0,0,0,0,0,0)
S.dtSafety=100.
S.dem.collectNodes()
S.engines=[Leapfrog(reset=True),InsertionSortCollider([Bo1_Ellipsoid_Aabb(),Bo1_Capsule_Aabb(),Bo1_Wall_Aabb(),Bo1_Facet_Aabb()],verletDist=.0),ContactLoop([Cg2_Ellipsoid_Ellipsoid_L6Geom(),Cg2_Wall_Ellipsoid_L6Geom(),Cg2_Facet_Ellipsoid_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()])]
# if 'vtk' in woo.config.features: S.engines=S.engines+[VtkExport(out='/tmp/ell0',ascii=True,compress=False,stepPeriod=1000)]


S.any=[Gl1_Ellipsoid(wire=True),Gl1_DemField(cPhys=True,cNode=Gl1_DemField.cNodeNode,bound=True),Renderer(iniViewDir=(0,1,0))]
S.saveTmp()

S.run(60,True)
S.cell.nextGradV=Matrix3.Zero
