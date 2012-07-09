from woo import utils,pack
from woo.dem import *
from woo.core import *

woo.master.scene=S=Scene(fields=[DemField()])
mat=utils.defaultMaterial()
sp=pack.SpherePack()
sp.makeCloud((0,0,0),(10,10,10),.4,rRelFuzz=.5)
sp.toSimulation(S,mat=mat)
S.dem.par.append([
	utils.wall(-3,axis=2,sense=1,mat=mat,glAB=((-10,-1),(20,11))),
	utils.wall(0,axis=1,sense=1,mat=mat,glAB=((-5,-5),(5,15))),
	utils.wall(10,axis=1,sense=-1,mat=mat,glAB=((-5,-5),(5,15))),
])
S.engines=utils.defaultEngines(damping=.4)
S.dt=.3*utils.pWaveDt()
# create cylinders
for i,x in enumerate((-2,0,2,4,6,8,10.5,12,14)):
	c=utils.infCylinder((x,0,-1),radius=.8,axis=1,mat=mat,glAB=(-1,11))
	c.angVel=(0,2.*(i+1),0)
	# each of cylinders will move haronically along global x and z axes (not y)
	c.impose=AlignedHarmonicOscillations(freqs=(1./(10000.*S.dt),float('nan'),1/(((i%3)+3)*1000.*S.dt)),amps=(.3*(i%2+1),0,.4*(i%4+1)))
	S.dem.par.append(c)
S.dem.collectNodes()
S.saveTmp()

from woo import gl
gl.Gl1_Wall.div=10
gl.Gl1_InfCylinder.wire=True
