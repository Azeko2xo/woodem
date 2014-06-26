#
# TODO: S.loneGroup
#
from woo.core import *
from woo.dem import *
import woo
import woo.gl
import math
from math import pi
from minieigen import *

import woo.log
#woo.log.setLevel('In2_FlexFacet_ElastMat',woo.log.TRACE)
#woo.log.setLevel('DynDt',woo.log.DEBUG)

woo.gl.Gl1_Node.wd=2
woo.gl.Gl1_Node.len=.05
#woo.gl.Gl1_DemField.glyph=woo.gl.Gl1_DemField.glyphForce

woo.gl.Gl1_FlexFacet.uScale=1.
woo.gl.Gl1_FlexFacet.relPhi=.1
woo.gl.Gl1_FlexFacet.phiSplit=False
woo.gl.Gl1_FlexFacet.phiWd=5


woo.gl.Gl1_DemField.colorBy=woo.gl.Gl1_DemField.colorVel
S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-30))])
if 1:
	import woo.pack, woo.utils, numpy
	scenario=['youtube','plain','inverse'][0]
	if scenario=='youtube':
		# http://www.youtube.com/watch?v=KmQWD_MfR8M
		xmax,ymax=1,1
		xdiv,ydiv=10,10
		ff=woo.pack.gtsSurface2Facets(woo.pack.sweptPolylines2gtsSurface([[(x,y,0) for x in numpy.linspace(0,xmax,num=xdiv)] for y in numpy.linspace(0,ymax,num=ydiv)]),flex=True)
		S.dem.par.append(ff)
		woo.gl.Renderer.dispScale=(10,10,10)
		woo.gl.Gl1_FlexFacet.node=False
		woo.gl.Gl1_FlexFacet.refConf=False
		woo.gl.Gl1_FlexFacet.uScale=0.
		woo.gl.Gl1_FlexFacet.relPhi=0.
		woo.gl.Gl1_DemField.shape=woo.gl.Gl1_DemField.shapeNonSpheres
		woo.gl.Gl1_DemField.colorBy=woo.gl.Gl1_DemField.colorDisplacement
		woo.gl.Gl1_DemField.vecAxis=-1
		woo.gl.Gl1_DemField.colorBy2=woo.gl.Gl1_DemField.colorVel
		woo.gl.Gl1_DemField.colorRange2.mnmx=(0,2.)
	else:
		woo.gl.Renderer.dispScale=(1000,1000,10000)
		woo.gl.Renderer.scaleOn=True
		woo.gl.Gl1_DemField.nodes=True
		woo.gl.Gl1_DemField.glyphRelSz=.3
		woo.gl.Gl1_FlexFacet.node=True
		## this triggers weirdness in rotation computation!! (quaternion wrapping around?!)
		if scenario=='inverse':
			f=woo.utils.facet([(0,0,0),(0,.2,0),(.2,0,0)],flex=True) #!!!
			import woo.log
			#woo.log.setLevel('In2_FlexFacet_ElastMat',woo.log.TRACE)
			woo.log.setLevel('FlexFacet',woo.log.TRACE)
		else: f=woo.utils.facet([(.2,.1,0),(0,.2,0),(0,0,0)],flex=True) 
		S.dem.par.append(f)
	S.dem.collectNodes()
	for n in S.dem.nodes:
		n.dem.blocked=''
		if n.pos[0]==0 or (n.pos[1]==0 and scenario=='youtube'): n.dem.blocked='xyzXYZ'
		r=.02; V=(4/3.)*pi*r**3
		n.dem.inertia=1e3*(2/5.)*V*r**2*Vector3(1,1,1)
		n.dem.mass=V*1e3
	# this would be enough without spheres
	#S.engines=[Leapfrog(reset=True,damping=.05),IntraForce([In2_FlexFacet_ElastMat(thickness=.01)])]
	S.engines=[
		Leapfrog(reset=True,damping=.2),
		InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()],verletDist=0.01),
		ContactLoop([Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()],applyForces=False), # forces are applied in IntraForce
		IntraForce([In2_FlexFacet_ElastMat(thickness=.1,bending=True,bendThickness=.03),In2_Sphere_ElastMat()]),
		DynDt(stepPeriod=100),
		# VtkExport(out='/tmp/membrane',stepPeriod=100,what=VtkExport.spheres|VtkExport.mesh)
	]
	
	# a few spheres falling onto the mesh
	if scenario=='youtube':
		sp=woo.pack.SpherePack()
		sp.makeCloud((.3,.3,.1),(.7,.7,.3),rMean=.3*xmax/xdiv,rRelFuzz=.3,periodic=False)
		sp.toSimulation(S,mat=FrictMat(young=1e6,density=3000))
		for s in S.dem.par:
			if type(s.shape)==Sphere: S.dem.nodesAppend(s.shape.nodes[0])

	S.saveTmp()

import woo.qt
woo.qt.Controller()
woo.qt.View()
