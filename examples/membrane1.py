from woo.core import *
from woo.dem import *
import woo
import woo.gl
import math
from math import pi
from minieigen import *
woo.gl.Gl1_DemField.nodes=True
woo.gl.Gl1_Node.wd=4
woo.gl.Gl1_Node.len=.05
woo.gl.Gl1_FlexFacet.node=False
woo.gl.Gl1_FlexFacet.phiScale=0.
woo.gl.Gl1_DemField.glyph=woo.gl.Gl1_DemField.glyphForce

if 0:
	nn=[Node(pos=(0,0,1)),Node(pos=(0,1,1)),Node(pos=(0,1,0))]
	for n in nn:
		n.dem=DemData(inertia=(1,1,1))
		n.dem.blocked='xyz'
		rotvec=Vector3.Random()
		n.ori=Quaternion(rotvec.norm(),rotvec.normalized())
		n.dem.mass=1e3
	nn[2].dem.vel=(0,-1,0)
	nn[2].dem.blocked=''

	S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,0))])
	S.dem.par.append(Particle(shape=FlexFacet(nodes=nn),material=FrictMat(young=1e6)))
	for n in nn: n.dem.addParRef(S.dem.par[-1])
	ff=S.dem.par[0].shape
	ff.setRefConf() #update()
	for n in nn: S.dem.nodesAppend(n)
	S.engines=[Leapfrog(reset=True),IntraForce([In2_FlexFacet_ElastMat(thickness=.01)])]
	S.dt=1e-5
	S.saveTmp()
	S.one()
	print S.dem.par[0].shape.KK
else:
	# see http://www.youtube.com/watch?v=jimWu0_8oLc
	woo.gl.Gl1_DemField.nodes=False
	woo.gl.Gl1_FlexFacet.uScale=0.
	woo.gl.Gl1_FlexFacet.relPhi=0.
	woo.gl.Gl1_DemField.colorBy=woo.gl.Gl1_DemField.colorVel
	S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-30))])
	import woo.pack, woo.utils, numpy
	xmax,ymax=1,1
	xdiv,ydiv=20,20
	ff=woo.pack.gtsSurface2Facets(woo.pack.sweptPolylines2gtsSurface([[(x,y,0) for x in numpy.linspace(0,xmax,num=xdiv)] for y in numpy.linspace(0,ymax,num=ydiv)]),flex=True)
	S.dem.par.append(ff)
	S.dem.collectNodes()
	for n in S.dem.nodes:
		n.dem.inertia=(1.,1.,1.)
		n.dem.blocked=''
		if n.pos[0] in (0,xmax) or n.pos[1] in (0,ymax): n.dem.blocked='xyzXYZ'
		n.dem.mass=5.
	# this would be enough without spheres
	#S.engines=[Leapfrog(reset=True,damping=.05),IntraForce([In2_FlexFacet_ElastMat(thickness=.01)])]
	S.engines=[
		Leapfrog(reset=True,damping=.1),
		InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()],verletDist=0.01),
		ContactLoop([Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()],applyForces=False), # forces are applied in IntraForce
		IntraForce([In2_FlexFacet_ElastMat(thickness=.01,bending=False,bendThickness=.2),In2_Sphere_ElastMat()]),
		# VtkExport(out='/tmp/membrane',stepPeriod=100,what=VtkExport.spheres|VtkExport.mesh)
	]
	
	# a few spheres falling onto the mesh
	if 1:
		sp=woo.pack.SpherePack()
		sp.makeCloud((.3,.3,.1),(.7,.7,.3),rMean=.3*xmax/xdiv,rRelFuzz=.3,periodic=False)
		sp.toSimulation(S,mat=FrictMat(young=1e6,density=3000))
		for s in S.dem.par:
			if type(s.shape)==Sphere: S.dem.nodesAppend(s.shape.nodes[0])

	S.dt=min(1e-4,.7*woo.utils.pWaveDt(S))

	S.saveTmp()

import woo.qt
woo.qt.Controller()
woo.qt.View()
