#!/usr/bin/python
# encoding: utf-8
"""
Creates geometry objects from facets.
"""

from yade import *
from miniEigen import *
import utils,math,numpy

#facetBox===============================================================
def facetBox(center,extents,orientation=Quaternion.Identity,wallMask=63,**kw):
	"""
	Create arbitrarily-aligned box composed of facets, with given center, extents and orientation.
	If any of the box dimensions is zero, corresponding facets will not be created. The facets are oriented outwards from the box.

	:param Vector3 center: center of the box
	:param Vector3 extents: lengths of the box sides
	:param Quaternion orientation: orientation of the box
	:param bitmask wallMask: determines which walls will be created, in the order -x (1), +x (2), -y (4), +y (8), -z (16), +z (32). The numbers are ANDed; the default 63 means to create all walls
	:param **kw: (unused keyword arguments) passed to :yref:`yade.utils.facet`
	:returns: list of facets forming the box
	"""

	#Defense from zero dimensions
	if (wallMask>63):
		print "wallMask must be 63 or less"
		wallMask=63
	if (extents[0]==0):
		wallMask=1
	elif (extents[1]==0):
		wallMask=4
	elif (extents[2]==0):
		wallMask=16
	if (((extents[0]==0) and (extents[1]==0)) or ((extents[0]==0) and (extents[2]==0)) or ((extents[1]==0) and (extents[2]==0))):
		raise RuntimeError("Please, specify at least 2 none-zero dimensions in extents!");
	# ___________________________

	mn,mx=[-extents[i] for i in 0,1,2],[extents[i] for i in 0,1,2]
	def doWall(a,b,c,d):
		return [utils.facet((a,b,c),**kw),utils.facet((a,c,d),**kw)]
	ret=[]

	A=orientation*Vector3(mn[0],mn[1],mn[2])+center
	B=orientation*Vector3(mx[0],mn[1],mn[2])+center
	C=orientation*Vector3(mx[0],mx[1],mn[2])+center
	D=orientation*Vector3(mn[0],mx[1],mn[2])+center
	E=orientation*Vector3(mn[0],mn[1],mx[2])+center
	F=orientation*Vector3(mx[0],mn[1],mx[2])+center
	G=orientation*Vector3(mx[0],mx[1],mx[2])+center
	H=orientation*Vector3(mn[0],mx[1],mx[2])+center
	if wallMask&1:  ret+=doWall(A,D,H,E)
	if wallMask&2:  ret+=doWall(B,F,G,C)
	if wallMask&4:  ret+=doWall(A,E,F,B)
	if wallMask&8:  ret+=doWall(D,C,G,H)
	if wallMask&16: ret+=doWall(A,B,C,D)
	if wallMask&32: ret+=doWall(E,H,G,F)
	return ret
	
#facetCylinder==========================================================
def facetCylinder(center,radius,height,orientation=Quaternion.Identity,segmentsNumber=10,wallMask=7,angleRange=None,closeGap=False,**kw):
	"""
	Create arbitrarily-aligned cylinder composed of facets, with given center, radius, height and orientation.
	Return List of facets forming the cylinder;

	:param Vector3 center: center of the created cylinder
	:param float radius:  cylinder radius
	:param float height: cylinder height
	:param Quaternion orientation: orientation of the cylinder; the reference orientation has axis along the $+x$ axis.
	:param int segmentsNumber: number of edges on the cylinder surface (>=5)
	:param bitmask wallMask: determines which walls will be created, in the order up (1), down (2), side (4). The numbers are ANDed; the default 7 means to create all walls
	:param (θmin,Θmax) angleRange: allows to create only part of bunker by specifying range of angles; if ``None``, (0,2*pi) is assumed.
	:param bool closeGap: close range skipped in angleRange with triangular facets at cylinder bases.
	:param **kw: (unused keyword arguments) passed to utils.facet;
	"""
	# check zero dimentions
	if (radius<=0): raise RuntimeError("The radius should have the positive value");
	
	return facetCylinderConeGenerator(center=center,radiusTop=radius,height=height,orientation=orientation,segmentsNumber=segmentsNumber,wallMask=wallMask,angleRange=angleRange,closeGap=closeGap,**kw)
	

#facetCone==============================================================
def facetCone(center,radiusTop,radiusBottom,height,orientation=Quaternion.Identity,segmentsNumber=10,wallMask=7,angleRange=None,closeGap=False,**kw):
	"""
	Create arbitrarily-aligned cone composed of facets, with given center, radius, height and orientation.
	Return List of facets forming the cone;

	:param Vector3 center: center of the created cylinder
	:param float radiusTop:  cone top radius
	:param float radiusBottom:  cone bottom radius
	:param float height: cone height
	:param Quaternion orientation: orientation of the cone; the reference orientation has axis along the $+x$ axis.
	:param int segmentsNumber: number of edges on the cone surface (>=5)
	:param bitmask wallMask: determines which walls will be created, in the order up (1), down (2), side (4). The numbers are ANDed; the default 7 means to create all walls
	:param (θmin,Θmax) angleRange: allows to create only part of cone by specifying range of angles; if ``None``, (0,2*pi) is assumed.
	:param bool closeGap: close range skipped in angleRange with triangular facets at cylinder bases.
	:param **kw: (unused keyword arguments) passed to utils.facet;
	"""
	# check zero dimentions
	if ((radiusBottom<=0) and (radiusTop<=0)): raise RuntimeError("The radiusBottom or radiusTop should have the positive value");
	
	return facetCylinderConeGenerator(center=center,radiusTop=radiusTop,radiusBottom=radiusBottom,height=height,orientation=orientation,segmentsNumber=segmentsNumber,wallMask=wallMask,angleRange=angleRange,closeGap=closeGap,**kw)

#facetPolygon===========================================================
def facetPolygon(center,radiusOuter,orientation=Quaternion.Identity,segmentsNumber=10,angleRange=None,radiusInner=0,**kw):
	"""
	Create arbitrarily-aligned polygon composed of facets, with given center, radius (outer and inner) and orientation.
	Return List of facets forming the polygon;

	:param Vector3 center: center of the created cylinder
	:param float radiusOuter:  outer radius
	:param float radiusInner: inner height (can be 0)
	:param Quaternion orientation: orientation of the polygon; the reference orientation has axis along the $+x$ axis.
	:param int segmentsNumber: number of edges on the polygon surface (>=3)
	:param (θmin,Θmax) angleRange: allows to create only part of polygon by specifying range of angles; if ``None``, (0,2*pi) is assumed.
	:param **kw: (unused keyword arguments) passed to utils.facet;
	"""
	# check zero dimentions
	if (abs(angleRange[1]-angleRange[0])>2.0*math.pi): raise RuntimeError("The |angleRange| cannot be larger 2.0*math.pi");
	
	return facetPolygonHelixGenerator(center=center,radiusOuter=radiusOuter,orientation=orientation,segmentsNumber=segmentsNumber,angleRange=angleRange,radiusInner=radiusInner,**kw)

#facetHelix===========================================================
def facetHelix(center,radiusOuter,pitch,orientation=Quaternion.Identity,segmentsNumber=10,angleRange=None,radiusInner=0,**kw):
	"""
	Create arbitrarily-aligned helix composed of facets, with given center, radius (outer and inner), pitch and orientation.
	Return List of facets forming the helix;

	:param Vector3 center: center of the created cylinder
	:param float radiusOuter:  outer radius
	:param float radiusInner: inner height (can be 0)
	:param Quaternion orientation: orientation of the helix; the reference orientation has axis along the $+x$ axis.
	:param int segmentsNumber: number of edges on the helix surface (>=3)
	:param (θmin,Θmax) angleRange: range of angles; if ``None``, (0,2*pi) is assumed.
	:param **kw: (unused keyword arguments) passed to utils.facet;
	"""
	
	# check zero dimentions
	if (pitch<=0): raise RuntimeError("The pitch should have the positive value");
	return facetPolygonHelixGenerator(center=center,radiusOuter=radiusOuter,orientation=orientation,segmentsNumber=segmentsNumber,angleRange=angleRange,radiusInner=radiusInner,pitch=pitch,**kw)
	
#facetBunker============================================================
def facetBunker(center,dBunker,dOutput,hBunker,hOutput,hPipe=0.0,orientation=Quaternion.Identity,segmentsNumber=10,wallMask=4,angleRange=None,closeGap=False,**kw):
	"""
	Create arbitrarily-aligned bunker, composed of facets, with given center, radii, heights and orientation.
	Return List of facets forming the bunker;
	::
		   dBunker
		______________
		|            |
		|            |
		|            | hBunker
		|            |
		|            |
		|            |
		|____________|
		\            /
		 \          /
		  \        /   hOutput
		   \      /
		    \____/
		    |    |
		    |____|     hPipe
		    dOutput
	
	:param Vector3 center: center of the created bunker
	:param float dBunker: bunker diameter, top
	:param float dOutput: bunker output diameter
	:param float hBunker: bunker height 
	:param float hOutput: bunker output height 
	:param float hPipe: bunker pipe height 
	:param Quaternion orientation: orientation of the bunker; the reference orientation has axis along the $+x$ axis.
	:param int segmentsNumber: number of edges on the bunker surface (>=5)
	:param bitmask wallMask: determines which walls will be created, in the order up (1), down (2), side (4). The numbers are ANDed; the default 7 means to create all walls
	:param (θmin,Θmax) angleRange: allows to create only part of bunker by specifying range of angles; if ``None``, (0,2*pi) is assumed.
	:param bool closeGap: close range skipped in angleRange with triangular facets at cylinder bases.
	:param **kw: (unused keyword arguments) passed to utils.facet;
	"""
	# check zero dimentions
	if (dBunker<=0): raise RuntimeError("The diameter dBunker should have the positive value");
	if (dOutput<=0): raise RuntimeError("The diameter dOutput should have the positive value");
	if (hBunker<=0): raise RuntimeError("The height hBunker should have the positive value");
	if (hOutput<=0): raise RuntimeError("The height hOutput should have the positive value");
	if (hPipe<0): raise RuntimeError("The height hPipe should have the positive value or zero");
	
	ret=[]
	if ((hPipe>0) or (wallMask&2)):
		centerPipe = Vector3(0,0,hPipe/2.0)
		ret+=facetCylinder(center=centerPipe,radius=dOutput/2.0,height=hPipe,segmentsNumber=segmentsNumber,wallMask=wallMask&6,angleRange=angleRange,closeGap=closeGap,**kw)
	
	centerOutput = Vector3(0.0,0.0,hPipe+hOutput/2.0)
	ret+=facetCone(center=centerOutput,radiusTop=dBunker/2.0,radiusBottom=dOutput/2.0,height=hOutput,segmentsNumber=segmentsNumber,wallMask=wallMask&4,angleRange=angleRange,closeGap=closeGap,**kw)
	
	centerBunker = Vector3(0.0,0.0,hPipe+hOutput+hBunker/2.0)
	ret+=facetCylinder(center=centerBunker,radius=dBunker/2.0,height=hBunker,segmentsNumber=segmentsNumber,wallMask=wallMask&5,angleRange=angleRange,closeGap=closeGap,**kw)
	
	for i in ret:
		i.state.pos=orientation*(i.state.pos)+Vector3(center)
		i.state.ori=orientation
	
	return ret

#facetPolygonHelixGenerator==================================================
def facetPolygonHelixGenerator(center,radiusOuter,pitch=0,orientation=Quaternion.Identity,segmentsNumber=10,angleRange=None,radiusInner=0,**kw):
	"""
	Please, do not use this function directly! Use geom.facetPloygon and geom.facetHelix instead.
	This is the base function for generating polygons and helixes from facets.
	"""
	# check zero dimentions
	if (segmentsNumber<3): raise RuntimeError("The segmentsNumber should be at least 3");
	if (radiusOuter<=0): raise RuntimeError("The radiusOuter should have the positive value");
	if (radiusInner<0): raise RuntimeError("The radiusInner should have the positive value or 0");
	if angleRange==None: angleRange=(0,2*math.pi)
	
	anglesInRad = numpy.linspace(angleRange[0], angleRange[1], segmentsNumber+1, endpoint=True)
	heightsInRad = numpy.linspace(0, pitch*(abs(angleRange[1]-angleRange[0])/(2.0*math.pi)), segmentsNumber+1, endpoint=True)
	
	POuter=[];
	PInner=[];
	PCenter=[];
	z=0;
	for i in anglesInRad:
		XOuter=radiusOuter*math.cos(i); YOuter=radiusOuter*math.sin(i); 
		POuter.append(Vector3(XOuter,YOuter,heightsInRad[z]))
		PCenter.append(Vector3(0,0,heightsInRad[z]))
		if (radiusInner<>0):
			XInner=radiusInner*math.cos(i); YInner=radiusInner*math.sin(i); 
			PInner.append(Vector3(XInner,YInner,heightsInRad[z]))
		z+=1
	
	for i in range(0,len(POuter)):
		POuter[i]=orientation*POuter[i]+center
		PCenter[i]=orientation*PCenter[i]+center
		if (radiusInner<>0):
			PInner[i]=orientation*PInner[i]+center
	
	ret=[]
	for i in range(1,len(POuter)):
		if (radiusInner==0):
			ret.append(utils.facet((PCenter[i],POuter[i],POuter[i-1]),**kw))
		else:
			ret.append(utils.facet((PInner[i-1],POuter[i-1],POuter[i]),**kw))
			ret.append(utils.facet((PInner[i],PInner[i-1],POuter[i]),**kw))
	
	return ret


#facetCylinderConeGenerator=============================================
def facetCylinderConeGenerator(center,radiusTop,height,orientation=Quaternion.Identity,segmentsNumber=10,wallMask=7,angleRange=None,closeGap=False,radiusBottom=-1,**kw):
	"""
	Please, do not use this function directly! Use geom.facetCylinder and geom.facetCone instead.
	This is the base function for generating cylinders and cones from facets.
	:param float radiusTop:  top radius
	:param float radiusBottom:  bottom radius
	:param **kw: (unused keyword arguments) passed to utils.facet;
	"""
	
	#For cylinders top and bottom radii are equal
	if (radiusBottom == -1):
		radiusBottom = radiusTop
	# check zero dimentions
	if (segmentsNumber<3): raise RuntimeError("The segmentsNumber should be at least 3");
	if (height<0): raise RuntimeError("The height should have the positive value");
	if angleRange==None: angleRange=(0,2*math.pi)
	if (abs(angleRange[1]-angleRange[0])>2.0*math.pi): raise RuntimeError("The |angleRange| cannot be larger 2.0*math.pi");
	
	if isinstance(angleRange,float):
		print u'WARNING: geom.facetCylinder,angleRange should be (Θmin,Θmax), not just Θmax (one number), update your code.'
		angleRange=(0,angleRange)
		
	anglesInRad = numpy.linspace(angleRange[0], angleRange[1], segmentsNumber+1, endpoint=True)
	
	PTop=[]; PTop.append(Vector3(0,0,+height/2))
	PBottom=[]; PBottom.append(Vector3(0,0,-height/2))

	for i in anglesInRad:
		XTop=radiusTop*math.cos(i); YTop=radiusTop*math.sin(i); 
		PTop.append(Vector3(XTop,YTop,+height/2))
		
		XBottom=radiusBottom*math.cos(i); YBottom=radiusBottom*math.sin(i); 
		PBottom.append(Vector3(XBottom,YBottom,-height/2))
		
	for i in range(0,len(PTop)):
		PTop[i]=orientation*PTop[i]+center
		PBottom[i]=orientation*PBottom[i]+center

	ret=[]
	for i in range(2,len(PTop)):
		if (wallMask&1)and(radiusTop!=0):
			ret.append(utils.facet((PTop[0],PTop[i],PTop[i-1]),**kw))
			
		if (wallMask&2)and(radiusBottom!=0):
			ret.append(utils.facet((PBottom[0],PBottom[i-1],PBottom[i]),**kw))
			
		if wallMask&4:
			if (radiusBottom!=0):
				ret.append(utils.facet((PTop[i],PBottom[i],PBottom[i-1]),**kw))
			if (radiusTop!=0):
				ret.append(utils.facet((PBottom[i-1],PTop[i-1],PTop[i]),**kw))
				
	if closeGap and (angleRange[0]%(2*math.pi))!=(angleRange[1]%(2*math.pi)and(abs(angleRange[1]-angleRange[0])>math.pi)): # some part is skipped
		if (wallMask&1)and(radiusTop!=0):
			pts=[(radiusTop*math.cos(angleRange[i]),radiusTop*math.sin(angleRange[i])) for i in (0,1)]
			pp=[(pts[0][0],pts[0][1],+height/2.0),(pts[1][0],pts[1][1],+height/2.0),(0,0,+height/2.0)]
			pp=[orientation*p+center for p in pp]
			ret.append(utils.facet(pp,**kw))
			
		if (wallMask&2)and(radiusBottom!=0):
			pts=[(radiusBottom*math.cos(angleRange[i]),radiusBottom*math.sin(angleRange[i])) for i in (0,1)]
			pp=[(pts[0][0],pts[0][1],-height/2.0),(pts[1][0],pts[1][1],-height/2.0),(0,0,-height/2.0)]
			pp=[orientation*p+center for p in pp]
			ret.append(utils.facet(pp,**kw))
	return ret
