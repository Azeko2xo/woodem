from woo.core import *; from woo.dem import *; import woo; from minieigen import *; from math import pi
nn=Node(pos=(0,0,1.1),ori=Quaternion((0,1,0),5*pi/12))
S=woo.master.scene=Scene(fields=[DemField(gravity=(0,-2,-10),par=[Wall.make(0,sense=1,axis=2,mat=woo.utils.defaultMaterial())])],engines=DemField.minimalEngines(damping=.2)+[ArcInlet(glColor=0,massRate=100,stepPeriod=200,node=nn,cylBox=AlignedBox3((.7,.75*pi,0),(1,1.25*pi,.3)),generator=PsdSphereGenerator(psdPts=[(.02,0),(.03,.2),(.04,.9),(.05,1.)]),materials=[woo.utils.defaultMaterial()]),ArcOutlet(inside=True,node=nn,currRateSmooth=.5,stepPeriod=200,cylBox=AlignedBox3((.4,-pi/2,-.4),(.8,pi/4,.2)),glColor=1),BoxOutlet(inside=False,stepPeriod=200,box=((-1,-1,-.1),(1,1,2.3)),glColor=.5)])
S.saveTmp()
