from woo.core import *
from woo.dem import *
from minieigen import *
from math import pi
import woo.utils, woo.pack

S=woo.master.scene=Scene(dtSafety=.7,fields=[DemField()])

sp=woo.pack.SpherePack()
rSph,rRelFuzz=3e-2,0
sp.makeCloud(maxCorner=(1,1,1),rMean=rSph,rRelFuzz=rRelFuzz,periodic=True)
sp.toSimulation(S)

S.cell.nextGradV=-Matrix3.Identity

S.engines=DemField.minimalEngines(damping=.4)+[PyRunner(100,'checkPoro(S)')]
S.saveTmp()

def checkPoro(S):
	Vs=sum([(4/3.)*pi*p.shape.radius**3 for p in S.dem.par])
	V=S.cell.volume
	poro=(V-Vs)/V
	print 'porosity',poro
	if poro<.5:
		sp=woo.pack.SpherePack()
		sp.fromSimulation(S)
		saveTo='pbc-spheres:N=%d,r=%g,rRelFuzz=%g.txt'%(len(S.dem.par),rSph,rRelFuzz)
		sp.save(saveTo)
		print 'Packing saved to',saveTo,'\n\\bye'
		import sys
		sys.exit(0)
S.run()
