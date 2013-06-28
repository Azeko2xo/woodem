from minieigen import *
import woo.pack
import numpy
import math

def cylinder(A,B,radius,div=20,capA=False,capB=False,angVel=0,**kw):
	'''Return triangulated cylinder, as list of facets to be passed to :obj:`ParticleContainer.append`. ``**kw`` arguments are passed to :obj:`woo.pack.gtsSurface2Facets` (and thus to :obj:`woo.utils.facet`).
	:param angVel: axial angular velocity of the cylinder; the cylinder is always created as ``fixed``, but :obj:`woo.dem.Facet.fakeVel` is assigned.
	'''
	cylLen=(B-A).norm()
	ax=(B-A)/cylLen
	axOri=Quaternion(); axOri.setFromTwoVectors(Vector3.UnitX,ax)

	thetas=numpy.linspace(0,2*math.pi,num=div,endpoint=True)
	yyzz=[Vector2(radius*math.cos(th),radius*math.sin(th)) for th in thetas]
	xxyyzz=[[A+axOri*Vector3(x,yz[0],yz[1]) for yz in yyzz] for x in (0,cylLen)]
	# add caps, if needed; the points will be merged automatically in sweptPolylines2gtsSurface via threshold
	if capA: xxyyzz=[[A+Vector3.Zero for yz in yyzz]]+xxyyzz
	if capB: xxyyzz+=[[B+Vector3.Zero for yz in yyzz]]
	ff=woo.pack.gtsSurface2Facets(woo.pack.sweptPolylines2gtsSurface(xxyyzz,threshold=min(radius,cylLen)*1e-4),fixed=True,**kw)
	if angVel!=0: 
		for f in ff:
			f.shape.fakeVel=-radius*angVel*f.shape.getNormal().cross(ax)
	return ff

