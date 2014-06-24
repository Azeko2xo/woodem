import woo.core, woo.dem
from woo.dem import *
import woo.utils
from minieigen import *
from math import *
from woo import utils

m=woo.utils.defaultMaterial()
zeroSphere=woo.utils.sphere((0,0,0),.4) # sphere which is entirely inside the thing
for p in [woo.utils.sphere((0,0,0),1,mat=m),woo.utils.ellipsoid((0,0,0),semiAxes=(.8,1,1.2),mat=m),woo.utils.ellipsoid((0,0,0),semiAxes=(1.,1.,1.),mat=m),woo.utils.capsule((0,0,0),radius=.8,shaft=.6,mat=m)]:
	print 100*'#'
	print p.shape
	#S=woo.core.Scene(fields=[DemField()])
	#S.dem.par.append(p)
	sp=woo.dem.ShapePack()
	sp.add([p.shape,zeroSphere.shape])
	r=sp.raws[0]
	if isinstance(r,SphereClumpGeom):
		for i in range(len(r.radii)): print r.centers[i],r.radii[i]
	else:
		for rr in r.rawShapes: print rr,rr.className,rr.center,rr.radius,rr.raw
	# print [i for i in r.rawShapes]
	r.recompute(div=10)

	print 'equivRad',r.equivRad,p.shape.equivRadius
	print 'volume',r.volume,p.mass/m.density
	print 'inertia',r.inertia,p.inertia/m.density
	print 'pos',r.pos,p.pos
	print 'ori',r.ori,p.ori

	print 50*'='
	ee=p.shape
	print ee
	print 'volume',ee.volume
	print 'equivRadius',ee.equivRadius
	rr=(ee.volume/((4/3.)*pi))**(1/3.)
	print 'sphere radius of the same volume',rr
	print 'sphere volume',(4/3.)*pi*rr**3


