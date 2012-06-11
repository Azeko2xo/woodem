#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Small showcase posted at http://www.youtube.com/watch?v=KUv26xlh89I,
in response to pfc3d's http://www.youtube.com/watch?v=005rdDBoe4w.
Physical correctness is not the focus, the geometry and similar look is.

You can take this file as instruction on how to build parametric surfaces,
and how to make videos as well.
"""
from yade import *
from yade.dem import *
from yade.core import *
from miniEigen import *
from yade import pack,utils
from numpy import linspace
# geometry parameters
bumpNum=20
bumpHt,bumpTipAngle=0.07,60*pi/180
millRad,millDp=1,1 # radius and depth (cylinder length) of the mill
sphRad,sphRadFuzz=0.03,.8 # mean radius and relative fuzz of the radius (random, uniformly distributed between sphRad*(1-.5*sphRadFuzz)…sphRad*(1+.5*sphRadFuzz))
dTheta=pi/24 # circle division angle



###
### mill geometry (parameteric)
###
bumpPeri=2*bumpHt*tan(.5*bumpTipAngle) # length of a bump on the perimeter of the mill
bumpAngle=bumpPeri/millRad # angle of one bump from the axis of the mill
interBumpAngle=2*pi/bumpNum
bumpRad=millRad-bumpHt
pts=[]; thMin=0
for i in range(0,bumpNum):
	thMin+=interBumpAngle
	thMax=thMin+interBumpAngle-bumpAngle
	thTip=thMax+.5*bumpAngle
	# the circular parts spanning from thMin to thMax
	for th0 in linspace(thMin,thMax,interBumpAngle/dTheta,endpoint=True):
		pts.append(Vector3(-.5*millDp,millRad*cos(th0),millRad*sin(th0)))
	# tip of the bump
	pts.append(Vector3(-.5*millDp,bumpRad*cos(thTip),bumpRad*sin(thTip)))
# close the curve
pts+=[pts[0]]
# make the second contour, just shifted by millDp; ppts contains both
ppts=[pts,[p+Vector3(millDp,0,0) for p in pts]]
mill=pack.sweptPolylines2gtsSurface(ppts,threshold=.01*min(dTheta*millRad,bumpHt))
millPar=pack.gtsSurface2Facets(mill,color=.5,wire=False) # add triangles, save their ids
# make the caps less comfortably, but looking better as two triangle couples over the mill
mrs2=millRad*sqrt(2)
cap1,cap2=[Vector3(0,0,mrs2),Vector3(0,-mrs2,0),Vector3(0,0,-mrs2)],[Vector3(0,0,mrs2),Vector3(0,0,-mrs2),Vector3(0,mrs2,0)] # 2 triangles at every side
for xx in -.5*millDp,.5*millDp: millPar.extend([utils.facet([p+Vector3(xx,0,0) for p in cap1],color=0),utils.facet([p+Vector3(xx,0,0) for p in cap2],color=0)])
centralNode=Node(pos=(0,0,0))
O.dem.par.appendClumped(millPar,centralNode=centralNode)

# define domains for initial cloud of red and blue spheres
packHt=.8*millRad # size of the area
bboxes=[(Vector3(-.5*millDp,-.5*packHt,-.5*packHt),Vector3(.5*millDp,0,.5*packHt)),(Vector3(-.5*millDp,0,-.5*packHt),Vector3(.5*millDp,.5*packHt,.5*packHt))]
colors=.2,.7
for i in (0,1): # red and blue spheres
	sp=pack.SpherePack(); bb=bboxes[i]; vol=(bb[1][0]-bb[0][0])*(bb[1][1]-bb[0][1])*(bb[1][2]-bb[0][2])
	sp.makeCloud(bb[0],bb[1],sphRad,sphRadFuzz,int(.25*vol/((4./3)*pi*sphRad**3)),False)
	O.bodies.append([utils.sphere(s[0],s[1],color=colors[i]) for s in sp])

O.collectNodes()

#print "Numer of grains",len(O.dem.par)-len(millIds)

O.dt=.5*utils.pWaveDt()
O.engines=utils.defaultEngines(gravity=(0,0,-50),damping=.3)

O.saveTmp()
from yade import qt
v=qt.View()
v.eyePosition=(3,.8,.96); v.upVector=(-.4,-.4,.8); v.viewDir=(-.9,-.25,-.3); v.axes=True; v.sceneRadius=1.9
#O.run(20000); O.wait()
#utils.encodeVideoFromFrames(snapshooter['savedSnapshots'],out='/tmp/mill.ogg',fps=30)
