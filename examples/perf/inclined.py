from woo.core import *; from woo.dem import *
import woo, woo.pack, woo.timing
import os.path, sys
from minieigen import *
import math
import numpy as np
periodic=False
outName='timings.txt'
r=.1
if len(sys.argv)==1:
	tag,N,steps='',20,1000
else:
	if len(sys.argv)!=4: raise RuntimeError("Exactly 3 argument must be given: tag N steps")
	tag,N,steps=sys.argv[1],int(sys.argv[2]),int(sys.argv[3])
	
S=woo.master.scene=Scene(fields=[DemField(gravity=Quaternion((.3,.7,0),math.radians(15))*Vector3(0,0,-9.81))])
mat=FrictMat(young=1e7,ktDivKn=.2,density=2500)
S.dem.par.append(Wall.make(-r,axis=2,sense=1,mat=mat))
S.dem.par.append(woo.pack.regularOrtho(woo.pack.inAlignedBox((0,0,0),(2*N+1)*r*Vector3.Ones),radius=r,gap=0,mat=mat))
S.dem.collectNodes()
S.engines=DemField.minimalEngines(damping=.5)
if periodic:
	S.periodic=True
	S.cell.setBox(4*N*r*Vector3.Ones)
#S.lab.dynDt.dead=True # disable dynamic dt update
#S.dt=1e-3
#S.lab.collider.ompTuneSort=(1,1000,10000)
print 'Number of spheres',len(S.dem.par)-1


woo.master.timingEnabled=True
S.one()

#woo.timing.stats()
t1=1e-9*sum([e.execTime for e in S.engines])
woo.timing.reset()
if tag:
	S.run(steps,True)
	# print S.lab.collider.ompTuneSort
	# print S.lab.collider.sortChunks
	t=1e-9*sum([e.execTime for e in S.engines])/(S.step-1)
	colliderRel=S.lab.collider.execTime*1./sum([e.execTime for e in S.engines])

	newOut=not os.path.exists(outName)
	out=open(outName,'a')
	if newOut: out.write("#tag\tcores\tnPar\tnSteps\tt1\tt\tcolliderRel\n")
	out.write('%s\t%d\t%d\t%d\t%f\t%.8f\t%f\n'%(tag,woo.master.numThreads,len(S.dem.par)-1,S.step-1,t1,t,colliderRel))
