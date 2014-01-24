import woo
from woo import utils,pack
from woo.dem import *
from woo.core import *
from minieigen import *

woo.master.scene=S=Scene(fields=[DemField(gravity=(0,0,-10))])
mat=utils.defaultMaterial()
sp=pack.SpherePack()
sp.makeCloud((4,4,4),(14,14,14),.4,rRelFuzz=.5)
sp.toSimulation(S,mat=mat)
S.dem.par.append([
	utils.wall(1,axis=2,sense=0,mat=mat,glAB=((-10,-1),(20,11))),
	#utils.wall(0,axis=1,sense=1,mat=mat,glAB=((-5,-5),(5,15))),
	#utils.wall(10,axis=1,sense=-1,mat=mat,glAB=((-5,-5),(5,15))),
])
S.periodic=True
S.cell.setBox(20,20,20)
# S.cell.nextGradV=Vector3(0,-.1,-.1).asDiagonal()
S.engines=utils.defaultEngines(damping=.4)
S.dtSafety=.5
S.dt=.5*utils.pWaveDt() # to compute oscillation freqs below
# create cylinders
for i,x in enumerate([-2,0,2,4,6,8,10.5,12,14]):
	c=utils.infCylinder((x,0,3),radius=.8,axis=1,mat=mat,glAB=(-1,11))
	c.angVel=(0,2.*(i+1),0)
	# each of cylinders will move haronically along global x and z axes (not y)
	c.impose=AlignedHarmonicOscillations(freqs=(1./(10000.*S.dt),float('nan'),1/(((i%3)+3)*1000.*S.dt)),amps=(.3*(i%2+1),0,.4*(i%4+1)))
	S.dem.par.append(c)
S.dem.collectNodes()
S.saveTmp()

try:
	from woo import gl
	gl.Gl1_Wall.div=10
	gl.Gl1_InfCylinder.wire=True
except ImportError: pass
