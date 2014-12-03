# encoding: utf-8
# create craddle from rods
from math import *
import numpy
from minieigen import *
import woo.core, woo.dem, woo.utils

# minimal setup
S=woo.master.scene=woo.core.Scene(fields=[woo.dem.DemField(gravity=(0,0,-10))])
mat=woo.utils.defaultMaterial()
S.engines=woo.dem.DemField.minimalEngines(damping=.05)

# build several arcs from rods, with different origins and radii, but in parallel planes
for origin,radius in [((0,0,1),1),((0,.1,1),.95),((0,-.1,1),.95)]:
	# local coordinate system (local z-axis in the direction of global y-axis)
	loc=woo.core.Node(pos=origin,ori=Quaternion((1,0,0),-pi/2))
	# discretize by angle between 0 and π
	for i,th in enumerate(numpy.linspace(0,pi,50)):
		# node position; convert from cylindrical to local cartesian, then to global cartesian
		node=woo.core.Node(pos=loc.loc2glob(woo.comp.cyl2cart((radius,th,0))))
		# when past the first node, connect to the previous node with a rod
		if i>0: S.dem.par.add(woo.dem.Rod.make(vertices=[prevNode,node],radius=.02,mat=mat,wire=False,fixed=True))
		prevNode=node

# a sphere which will fall down
S.dem.par.add(woo.dem.Sphere.make((.7,0,1),.1,mat=mat))
# integrate motion of just this node, the other ones are static and don't move at all
# otherwise use S.dem.collectNodes() to add nodes from all particles
S.dem.nodesAppend(S.dem.par[-1].shape.nodes[0])

# run slower than full-speed
S.throttle=5e-3

S.saveTmp()

