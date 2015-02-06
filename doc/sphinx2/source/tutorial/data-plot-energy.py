import woo; from woo.dem import *; from woo.core import *
S=woo.master.scene=woo.core.Scene(fields=[DemField(gravity=(0,0,-9.81),par=[Wall.make(0,axis=2),Sphere.make((0,0,2),radius=.2)])],engines=DemField.minimalEngines(damping=.2)+[PyRunner(10,'S.plot.autoData()')],trackEnergy=True)
S.plot.plots={'t=S.time':('$z_1$=S.dem.par[1].pos[2]','$z_0$=S.dem.par[0].pos[2]',),' t=S.time':('**S.energy',r'$\sum$energy=S.energy.total()',None,('relative error=S.energy.relErr()','g--'))}
S.run(2000,True)
S.plot.saveGnuplot('/tmp/aaa')
