import gts
import woo.pack,woo.dem,woo.log,woo.core
from minieigen import *
S=woo.master.scene=woo.core.Scene(fields=[woo.dem.DemField()])
surf=gts.sphere(4)
surf.translate(1.2,1.2,1.2)
S.dem.par.add(woo.pack.gtsSurface2Facets(surf))
S.dem.collectNodes()
S.periodic=True
S.cell.setBox(2.4,2.4,2.4)
S.cell.nextGradV=Matrix3(0.,.1,.1, -.1,0,.1, .1,-.1,-.01)
woo.log.setLevel('MeshVolume',woo.log.TRACE)
S.engines=[
	woo.dem.MeshVolume(stepPeriod=1,label='meshVolume'),
	woo.dem.Leapfrog(reset=True),
	woo.core.PyRunner(1,'S.plot.addData(i=S.step,V=S.lab.meshVolume.vol,Vcell=S.cell.volume,ratio=S.lab.meshVolume.vol/S.cell.volume)'),
]
S.dt=1e-4
S.plot.plots={'i':('V','Vcell',None,'ratio')}
S.one()
print 'Sphere volume is ',S.lab.meshVolume.vol
S.saveTmp()
