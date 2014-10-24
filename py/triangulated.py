from minieigen import *
import woo.pack
import numpy
import math

def cylinder(A,B,radius,div=20,axDiv=1,capA=False,capB=False,wallCaps=False,angVel=0,fixed=True,**kw):
	'''Return triangulated cylinder, as list of facets to be passed to :obj:`ParticleContainer.append`. ``**kw`` arguments are passed to :obj:`woo.pack.gtsSurface2Facets` (and thus to :obj:`woo.utils.facet`).

:param angVel: axial angular velocity of the cylinder; the cylinder has nevertheless zero velocity, only :obj:`woo.dem.Facet.fakeVel` is assigned.
:param axDiv: divide the triangulation axially as well; the default creates facets spanning between both bases. If negative, it will attempt to have that ratio with circumferential division, e.g. specifying -1 will give nearly-square division.
:param wallCaps: create caps as walls (with :obj:`woo.dem.wall.glAB` properly set) rather than triangulation; cylinder axis *must* be aligned with some global axis in this case, otherwise and error is raised.
:returns: List of :obj:`particles <woo.dem.Particle>` building up the cylinder. Caps (if any) are always at the beginning of the list, triangulated perimeter is at the end.
	'''
	cylLen=(B-A).norm()
	ax=(B-A)/cylLen
	axOri=Quaternion(); axOri.setFromTwoVectors(Vector3.UnitX,ax)

	thetas=numpy.linspace(0,2*math.pi,num=div,endpoint=True)
	yyzz=[Vector2(radius*math.cos(th),radius*math.sin(th)) for th in thetas]
	if axDiv<0:
		circum=2*radius*math.pi
		axDiv=int(abs(axDiv)*cylLen/(circum/div))
	xx=numpy.linspace(0,cylLen,num=max(axDiv+1,2))
	xxyyzz=[[A+axOri*Vector3(x,yz[0],yz[1]) for yz in yyzz] for x in xx]
	# add caps, if needed; the points will be merged automatically in sweptPolylines2gtsSurface via threshold
	caps=[]
	if wallCaps:
		# determine cylinder orientation
		cAx=-1
		for i in (0,1,2):
			 if abs(ax.dot(Vector3.Unit(i)))>(1-1e-6): cAx=i
		if cAx<0: raise ValueError("With wallCaps=True, cylinder axis must be aligned with some global axis")
		sign=int(math.copysign(1,ax.dot(Vector3.Unit(cAx)))) # reversed axis
		a1,a2=((cAx+1)%3),((cAx+2)%3)
		glAB=AlignedBox2((A[a1]-radius,A[a2]-radius),(A[a1]+radius,A[a2]+radius))
		if capA:
			caps.append(woo.utils.wall(A,axis=cAx,sense=sign,glAB=glAB,**kw))
			if angVel!=0: caps[-1].shape.nodes[0].dem.angVel[cAx]=sign*angVel
		if capB:
			caps.append(woo.utils.wall(B,axis=cAx,sense=-sign,glAB=glAB,**kw))
			if angVel!=0: caps[-1].shape.nodes[0].dem.angVel[cAx]=sign*angVel
	else:
		# add as triangulation
		if capA: xxyyzz=[[A+Vector3.Zero for yz in yyzz]]+xxyyzz
		if capB: xxyyzz+=[[B+Vector3.Zero for yz in yyzz]]
	ff=woo.pack.gtsSurface2Facets(woo.pack.sweptPolylines2gtsSurface(xxyyzz,threshold=min(radius,cylLen)*1e-4),fixed=fixed,**kw)
	if angVel!=0: 
		for f in ff:
			f.shape.fakeVel=-radius*angVel*f.shape.getNormal().cross(ax)
	return caps+ff

def quadrilateral(A,B,C,D,size=0,div=Vector2i(0,0),**kw):
	'Return triangulated `quadrilateral <http://en.wikipedia.org/wiki/Quadrilateral>`__ (or a `skew quadrilateral <http://en.wikipedia.org/wiki/Skew_polygon>`__), when points are not co-planar), where division size is at most *size* (absolute length) or *div* (number of subdivisions in the AB--CD and AC--BD directions).'
	if sum(div)>0:
		if min(div)<1: raise ValueError('Both components of div must be positive')
		if size>0: raise ValueError('only one of *div* or *size* may be given (not both)')
	else:
		l1,l2=min((A-C).norm(),(D-B).norm()),min((A-B).norm(),(C-D).norm())
		div=Vector2i(int(max(1,math.ceil(l1/size))),int(max(1,math.ceil(l2/size))))
	AB,AC,CD,BD=B-A,C-A,D-C,D-B
	aabb,aacc=numpy.linspace(0,1,div[0]),numpy.linspace(0,1,div[1])
	pts=[[A+ac*AC+ab*(B+ac*BD-(A+ac*AC)).normalized() for ac in aacc] for ab in aabb]
	return woo.pack.gtsSurface2Facets(woo.pack.sweptPolylines2gtsSurface(pts),**kw)

	
