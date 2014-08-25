# http://youtu.be/nFC43XupOIg
#
from woo.core import *
from woo.dem import *
import woo
import woo.gl
import math
from math import pi
from minieigen import *

woo.gl.Gl1_Membrane(uScale=0,relPhi=0,refConf=False)
woo.gl.Gl1_DemField(shape=woo.gl.Gl1_DemField.shapeNonSpheres,colorBy=woo.gl.Gl1_DemField.colorDisplacement,vecAxis=-1,colorBy2=woo.gl.Gl1_DemField.colorVel)
woo.gl.Gl1_DemField.colorRange2.mnmx=(0,2.)

S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-30))],dtSafety=.8)

import woo.pack, woo.utils, numpy

xmax,ymax=1,1
xdiv,ydiv=15,15
mat=FrictMat(young=3e6,tanPhi=.55,ktDivKn=.2,density=3000)
ff=woo.pack.gtsSurface2Facets(woo.pack.sweptPolylines2gtsSurface([[(x,y,0) for x in numpy.linspace(0,xmax,num=xdiv)] for y in numpy.linspace(0,ymax,num=ydiv)]),flex=True,halfThick=.01,mat=mat)
S.dem.par.add(ff)

# a few spheres falling onto the mesh
sp=woo.pack.SpherePack()
sp.makeCloud((.3,.3,.1),(.7,.7,.6),rMean=.5*xmax/xdiv,rRelFuzz=.5)
sp.toSimulation(S,mat=mat)

S.dem.collectNodes()

# set boundary conditions and set some fake masses and inertia of mesh nodes
for n in S.dem.nodes:
	n.dem.blocked=''
	if n.pos[0]==0 or (n.pos[1]==0): n.dem.blocked='xyz'
	r=.3*xmax*1./xdiv; V=(4/3.)*pi*r**3
	n.dem.inertia=mat.density*(2/5.)*V*r**2*Vector3(1,1,1)
	n.dem.mass=V*mat.density



# split the plate along the most part of the diagonal
for n in [n for n in S.dem.nodes if n.pos[0]==n.pos[1]]:
	# make the other end of the diagonal fixed as well, just so that the split effect is better visible
	if n.pos[1]==1.: n.dem.blocked='xyz'
	# if n.pos[0]<.2: continue
	S.dem.splitNode(n,[p for p in n.dem.parRef if p.shape.getCentroid()[0]>p.shape.getCentroid()[1]])

S.engines=DemField.minimalEngines(damping=.4,verletDist=-0.01)+[IntraForce([In2_Membrane_ElastMat(thickness=.01,bending=True,bendThickness=.01)]),BoxDeleter(box=((-.1,-.1,-1),(1.1,1.1,1)),glColor=float('nan'))]


S.saveTmp()

import woo.qt
woo.qt.Controller()
woo.qt.View()
