botWd=.3; sideWd=.2; sideHt=.1; x1=2; xDiv=5
import woo; from woo.core import *; from woo.dem import*; import woo.utils; import numpy; import math
S=woo.master.scene=Scene(fields=[DemField()])
xx=numpy.linspace(0,x1,num=xDiv)
pts=[[(x,-.5*botWd-sideWd,sideHt),(x,-.5*botWd,0),(x,.5*botWd,0),(x,.5*botWd+sideWd,sideHt)] for x in xx]
node=Node(pos=(.2,.2,.2),ori=((0,0,1),math.pi/3))
surf=woo.pack.sweptPolylines2gtsSurface(pts,localCoords=node)
S.dem.par.append(woo.pack.gtsSurface2Facets(surf))
