## http://youtu.be/cOLMNqtCy1c

from woo.core import *
from woo.dem import *
from woo.fem import *
import woo, woo.triangulated
from math import pi
from minieigen import *
S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-10),loneMask=0)],dtSafety=0.9)
S.engines=DemField.minimalEngines(damping=.5)+[IntraForce([In2_Tet4_ElastMat(),In2_Facet(),In2_Membrane_FrictMat(bending=True)])]

m0=woo.utils.defaultMaterial()
m0.young*=5
mask=0b001
mask2=0b011
S.dem.par.add([
	# InfCylinder.make((0,0,0),radius=.5,axis=1,mat=m0,mask=mask,glAB=(-4,4)),
	#InfCylinder.make((2,0,0),radius=.5,axis=1,mat=m0,mask=mask,glAB=(-4,4)),
	#Wall.make((0,0,-1.),sense=1,axis=2,mat=m0,mask=mask),
])
cc=woo.triangulated.cylinder(Vector3(-1,0,3),Vector3(9,0,3),radius=1.5,div=30,axDiv=-1,halfThick=.1,flex=True,fixed=False,wire=False,mat=m0,mask=mask2)
# TODO: lumped mass and inertia autocomputed
for c in cc:
	for n in c.nodes:
		n.dem.mass=500
		n.dem.inertia=(1,1,1)
S.dem.par.add(cc)

m1=m0.deepcopy()
nn,pp=woo.utils.importNmesh('tube.beam-long.nmesh',mat=m1,mask=mask,trsf=lambda v: v+Vector3(4,-4,-.5),dem=S.dem,surfHalfThick=0.05)
for n in nn:
	if abs(n.pos[1])>3.9: n.dem.blocked='xyzXYZ'
for p in pp:
	if isinstance(p.shape,Facet): p.shape.visible=False
	else: p.shape.wire=False

S.lab.collider.noBoundOk=True

S.dem.collectNodes()
try:
	from woo.gl import *
	Gl1_Membrane(refConf=False,slices=5,uScale=0,relPhi=0)
	Gl1_DemField(shape='mask',mask=0b010,colorBy='disp',shape2=True,colorBy2='disp')
except ImportError: pass

S.engines=S.engines+[VtkExport(out='/tmp/abcdef',what=VtkExport.mesh|VtkExport.spheres,thickFacetDiv=6,stepPeriod=400),PyRunner(1200,'addSpheres(S)',nDo=1,initRun=False)]

def addSpheres(S):
	sp=woo.pack.SpherePack()
	sp.makeCloud((3.5,-1,2.5),(6,1,3.5),rMean=.15,rRelFuzz=.5)
	sp.toSimulation(S,mat=m0,mask=mask)
	S.dem.collectNodes()

S.saveTmp()
