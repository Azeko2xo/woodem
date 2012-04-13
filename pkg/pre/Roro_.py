from yade import utils,pack
from yade.core import *
from yade.dem import *
import yade.gl
def run(roro):
	print 'Roro_.run(...)'
	s=Scene()
	de=DemField();
	s.fields=[de]
	mat=utils.defaultMaterial()
	de.par.append([
		utils.wall(-3,axis=2,material=mat),
		utils.wall(0,axis=1,material=mat),
		utils.wall(10,axis=1,material=mat)
	])
	for i in range(0,roro.cylNum):
		c=utils.infCylinder((2*i,0,-1),radius=.8,axis=1,material=mat)
		c.angVel=(0,2*(i+1),0)
		de.par.append(c)
	sp=pack.SpherePack()
	sp.makeCloud((0,0,0),(10,10,5),roro.psd[0][0],rRelFuzz=.5)
	de.par.append([utils.sphere(c,r,material=mat) for c,r in sp])
	s.engines=utils.defaultEngines(damping=.4,gravity=(0,0,-10))
	s.dt=.3*utils.pWaveDt(s)
	s.any=[yade.gl.Gl1_InfCylinder(wire=True),yade.gl.Gl1_Wall(div=5)]
	print 'Generated Rollenrost with %d particles.'%len(de.par)
	de.collectNodes()
	return s
