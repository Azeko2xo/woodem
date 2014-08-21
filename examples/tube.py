## http://youtu.be/cOLMNqtCy1c

from woo.core import *
from woo.dem import *
import woo, woo.triangulated
from math import pi
from minieigen import *
S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-10),loneMask=0)],dtSafety=0.9)
S.engines=DemField.minimalEngines(damping=.5)+[IntraForce([In2_FlexFacet_ElastMat(bending=True)])]

m0=woo.utils.defaultMaterial()
mask=0b001
S.dem.par.append([
	InfCylinder.make((0,0,0),radius=.5,axis=1,mat=m0,mask=mask),
	InfCylinder.make((2,0,0),radius=.5,axis=1,mat=m0,mask=mask),
	Wall.make((0,0,-.8),sense=1,axis=2,mat=m0,mask=mask),
])
cc=woo.triangulated.cylinder(Vector3(-1,0,3),Vector3(9,0,3),radius=2,div=40,axDiv=-1,halfThick=.1,flex=True,fixed=False,wire=False,mat=m0)
# TODO: lumped mass and inertia autocomputed
for c in cc:
	for n in c.nodes:
		n.dem.mass=100
		n.dem.inertia=(1,1,1)
S.dem.par.append(cc)
S.dem.collectNodes()
try:
	from woo.gl import *
	Gl1_FlexFacet(refConf=False,slices=-1,uScale=0,relPhi=0)
	Gl1_DemField(colorBy=Gl1_DemField.colorDisplacement)
except ImportError: pass

S.saveTmp()
