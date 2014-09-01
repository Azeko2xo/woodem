import numpy as np
from pprint import pprint
dd={}
for l in open('timings.txt'):
	if l.startswith('#'): continue
	ll=l[:-1].split()
	if len(ll)==0: continue
	tag,cores,nPar,nSteps=ll[0],int(ll[1]),int(ll[2]),int(ll[3])
	t1,t,colliderRel=[float(i) for i in ll[4:]]
	key=(tag,cores,nPar,nSteps)
	data=[t1,t,colliderRel]
	if key not in dd: dd[key]=[data]
	else: dd[key]+=[data]
# compute averages
for k in dd: dd[k]=tuple([np.average(d) for d in zip(*dd[k])])
# nn=set()
# for k in dd: nn.add((k[1],k[2]))

out=[]

#refTag,cmpTag='par2_threadSafe','par3_oneMutex'
#refTag,cmpTag='par1','par3_oneMutex'
#refTag,cmpTag='orig','parBounds'
#refTag,cmpTag='noInvFast','par3_oneMutex'
#refTag,cmpTag='par1','par4_shortCircuit'
#refTag,cmpTag='par4_shortCircuit','parBounds'
#refTag,cmpTag='parBounds','gcc49'
#refTag,cmpTag='orig','ompTuneSort1_10k_0'
#refTag,cmpTag='r3547','r3552'
refTag,cmpTag='r3530','iniConParallel'


for k in sorted(dd.keys()):
	if k[0]==refTag: continue
	if k[0]!=cmpTag: continue
	refKey=(refTag,k[1],k[2],k[3])
	if refKey not in dd.keys(): continue
	for i,name in enumerate(['t1','t','coll%']):
		# if i==1 or i==2: continue
		# if i!=2: continue
		if i!=0: continue
		val0=dd[refKey][i]			
		val=dd[k][i]
		out+=[[k[1],k[2],k[3],name,refTag,val0,k[0],val,'%.2f%%'%(100*(val-val0)/val0)]]

# import prettytable
# print prettytable.PrettyTable(out,border=False)
# pprint(out)
# print out
for o in out:
	print '\t'.join([str(oo) for oo in o])

import pylab
cores=set([k[1] for k in dd.keys() if k[0]==cmpTag])
steps=set([k[3] for k in dd.keys() if k[0]==cmpTag])
nPar=set([k[2] for k in dd.keys() if k[0]==cmpTag])
#cores=[1]
if 0:
	for core in cores:
		for step in steps:
			nPar=sorted(list(set([k[2] for k in dd.keys() if (cmpTag,core,k[2],step) in dd.keys() and (refTag,core,k[2],step) in dd.keys()])))
			print core,step,nPar
			pylab.plot(nPar,[dd[refTag,core,N,step][1] for N in nPar],label='%s, %d cores'%(refTag,core))
			pylab.plot(nPar,[dd[cmpTag,core,N,step][1] for N in nPar],label='%s, %d cores'%(cmpTag,core),linewidth=4,alpha=.5)

	pylab.xlabel('Number of particles')
	pylab.ylabel('Time per one step [s]')
	pylab.grid(True)
	pylab.legend(loc='best')

if 1:
	pylab.figure()
	for core in cores:
		for step in steps:
			nPar=sorted(list(set([k[2] for k in dd.keys() if (cmpTag,core,k[2],step) in dd.keys() and (refTag,core,k[2],step) in dd.keys()])))
			print core,step,nPar
			pylab.plot(nPar,[dd[refTag,core,N,step][0] for N in nPar],label='%s, %d cores'%(refTag,core))
			pylab.plot(nPar,[dd[cmpTag,core,N,step][0] for N in nPar],label='%s, %d cores'%(cmpTag,core),linewidth=4,alpha=.5)
	pylab.xlabel('Number of particles')
	pylab.ylabel('Time of the intial sort [s]')
	pylab.grid(True)
	pylab.legend(loc='best')


pylab.show()
