import sys
from minieigen import *
from math import *
IIR=['1.1','1.2','1.3','1.4','1.5','1.6','1.8','2','2.2','2.4','2.5','2.8']
devNull=file('/dev/null','w')
cout=sys.stdout

errFileName='pbc-Ir-simErr-latErr.txt';
errFile=file(errFileName,'w')
# write header, for pgfplots
errFile.write('Ir avgCont simErr latErr\n')

stiffFileName='pbc-stiffnesses.txt';
stiffFile=file(stiffFileName,'w')
# write header, for pgfplots
stiffFile.write('Ir C11mp C33mp C13mp C12mp C44mp C11lat C33lat C13lat C12lat C44lat C11sim C33sim C13sim C12sim C44sim\n')

for Ir in IIR:
	sys.stdout=cout
	doLaTeX=(Ir=='1.8')
	print 'Ir=%s, LaTeX: %s'%(Ir,str(doLaTeX))
	EE=Vector3()
	GG=Vector3()
	nu={}

	nuMarker={'zx':r'\dag','zy':r'\dag','xy':r'\ddag','yx':r'\ddag','xz':r'*','yz':'*'}
	eMarker={'xx':r'\circ','yy':r'\circ','zz':r''}
	gMarker={'yz':r'+','zx':r'+','xy':r''}

	sys.stdout=devNull

	if doLaTeX: sys.stdout=file('pbc-latex-parts-sims.tex','w')
	#sys.stderr.write('sys.stdout=%s'%str(sys.stdout))
	
	missingLogs=False
	print "\t\t\\begin{align}"
	for axis in ('xx','yy','zz','yz','zx','xy'):
		isShear=(axis[0]!=axis[1])
		try:
			log='logs/pbc-deform-sample.py.axis=%s,Ir=%.1f.log'%(axis,float(Ir))  # log files get different formatting from the table
			sys.stderr.write('LOG %s\n'%log)
			ll=file(log).readlines()
		except IOError:
			sys.stderr.write('ERROR: FILE %s not found, skipping\n'%log)
			if doLaTeX:
				sys.stderr.write('Since LaTeX output was requested, missing log file is fatal. Aborting.\n')
				sys.exit(1)
			missingLogs=True
			break
		if isShear:
			if axis=='yz': print '\t\t\t',r'\intertext{while three pure shear simulations gave}'
			try:
				sh=map(float,[l for l in ll if l.startswith('SHEAR')][0].split()[1:])
			except IndexError: # lines not found
				sys.stderr.write('Expected values not found in %s, will skip this Ir.\n'%log)
				missingLogs=True
				break
			ax=('yz','zx','xy').index(axis)
			GG[ax]=sh[1]/(sh[0])
			print '\t\t\t'+r'G_{%s}&=%.4g\,{\rm MPa}^{%s}'%(axis,1e-6*GG[ax],gMarker[axis]),(r', & ' if axis!='xy' else '.')
		else:
			try:
				sig=map(float,[l for l in ll if l.startswith('STRESSES')][0].split()[1:])
				eps=map(float,[l for l in ll if l.startswith('STRAINS')][0].split()[1:])
			except IndexError: #lines not found
				sys.stderr.write('Expected values not found in %s, will skip this Ir'%log)
				missingLogs=True
				break
			ax=('x','y','z').index(axis[0])
			ax0,ax1,ax2=ax,(ax+1)%3,(ax+2)%3
			ax01='xyz'[ax0]+'xyz'[ax1]
			ax02='xyz'[ax0]+'xyz'[ax2]
			nu[ax01]=-eps[ax1]/eps[ax0]
			nu[ax02]=-eps[ax2]/eps[ax0]
			EE[ax]=sig[ax]/eps[ax]
			print '\t\t\t'+r'E_{%s}&=%.4g\,{\rm MPa}^{%s}, & \nu_{%s}&=%.4g^{%s}, & \nu_{%s}&=%.4g^{%s},\\'%(axis[0],1e-6*EE[ax],eMarker[axis],ax01,nu[ax01],nuMarker[ax01],ax02,nu[ax02],nuMarker[ax02])
	if missingLogs: continue
	print "\t\t\\end{align}"
	E1,E2=.5*(EE[0]+EE[1]),EE[2]
	G1,G2=GG[2],.5*(GG[0]+GG[1])
	e=E1/E2
	nu1=.5*(nu['xy']+nu['yx'])
	nu2=.5*(nu['zx']+nu['zy'])
	nu3=.5*(nu['yz']+nu['xz'])

	print "\t\tDependent values can be written (we used averages for symmetric couples):"
	print '\t\t\\begin{align}'
	print '\t\t\t'+r'\nu_{xz}=\nu_{yz}=\frac{E_x}{E_z}\nu_{zx}&=%.3g, \\'%((E1/E2)*nu2)
	print '\t\t\t'+r'G_{xy}=\frac{E_x}{2(1+\nu_{xy})}&=%.3g\,{\rm MPa}. '%(1e-6*E1/(2*(1+nu1)))
	print '\t\t\\end{align}'
	#print 'dependent nu3=%g'%nu3,'computed',(E1/E2)*nu2
	#print 'dependent G1=G_xy=%g'%G1,'computed',E1/(2*(1+nu1))

	from woo.core import *
	import woo.utils
	import numpy
	from numpy import matrix,array
	S=woo.master.scene=Scene.load('pbc-deform-sample-xx-saved.Ir=%s.gz'%Ir)
	table=S.lab.table

	N,V=S.dem.con.countReal(),S.cell.volume
	rrPrime=numpy.array([.5*(c.pA.pos-c.pB.pos-(S.cell.hSize*Matrix3(c.cellDist)).diagonal()).norm() for c in S.dem.con])
	rr=numpy.array([min(c.pA.shape.radius,c.pB.shape.radius) for c in S.dem.con])
	pihat=pi/2.
	#mu=(2/3.)*(N*pihat/V)*numpy.average(rrPrime**2)*numpy.average(rr**2/rrPrime)
	mu=woo.utils.muStiffnessScaling(piHat=pi)
	#sys.stdout=cout
	#print mu,utils.muStiffnessScaling(piHat=pi/2.)
	#mu*=1.1

	if doLaTeX: sys.stdout=file('pbc-latex-parts-params.tex','w')

	def printMPa(name,val): print name,'&',r'%.3g\,MPa \\'%(val*1e-6)
	print r'number of spheres & %d \\'%len(S.dem.par)
	print r'sphere radius & %.3g$\,{\rm m}$ \\'%(S.dem.par[0].shape.radius)
	print r'interaction radius $I_r$ & %.3g \\'%table.Ir
	print r'$\hat\pi$ parameter & $\pi$ \\'
	printMPa('$C_{11}$',table.C11)
	printMPa('$C_{33}$',table.C33)
	printMPa('$C_{13}$',table.C13)
	printMPa('$C_{12}$',table.C12)
	print r'stress tolerance $t_r$ & $10^{%g}$ \\'%(log10(table.relStressTol))
	print r'prescribed strain $\hat\eps$ & %.3g\%% \\'%(table.maxStrain*1e2)
	print r'\midrule'
	avgNumCont=(2*S.dem.con.countReal()/len(S.dem.par))
	print r'average number of contacts & %.3g \\'%avgNumCont
	print r'lattice density scaling $\mu$ (\ref{eq-mu}) & %.3g \\'%mu
	printMPa("${E_N^a}'$",S.lab.xiso.E1)
	printMPa("${E_N^b}'$",S.lab.xiso.E2)
	printMPa("${E_T^a}'$",S.lab.xiso.G1)
	printMPa("${E_T^b}'$",S.lab.xiso.G2)

	S.one()
	kPackMat=woo.utils.stressStiffnessWork()[1]
	voigtIndices=[(0,0),(2,2),(0,2),(0,1),(3,3)] # subtract 1
	kPack=array([kPackMat[vi] for vi in voigtIndices])
	#kPack*=(sqrt(3)/2.)
	ENT=matrix([[S.lab.xiso.E1,S.lab.xiso.E2,S.lab.xiso.G1,S.lab.xiso.G2]]).T
	kMicroplane=mu*(1/35.)*matrix([[36,6,20,8],[12,30,16,12],[8,6,-8,-6],[12,2,-12,-2],[8,6,13,8]])*ENT


	# Anastasia's thesis, (2.14)
	m=(1-nu1-2*e*nu2**2)
	C11=E1*(1-e*nu2**2)/((1+nu1)*m)
	C33=E2*(1-nu1)/m
	C13=E1*nu2/m
	C12=E1*(nu1+e*nu2**2)/((1+nu1)*m)
	C44=G2
	if 0:
		sys.stdout=cout
		print 'microplane',', '.join(['%.3g'%(k*1e-6) for k in kMicroplane]),r'\\'
		print 'from-stiff',', '.join(['%.3g'%(k*1e-6) for k in kPack]),r'\\'
		relDiff=array(kMicroplane).T/array(kPack).T
		print '[ratio] avg.',numpy.average(relDiff),':',relDiff
		print 'measured',', '.join(['%.3g'%(k*1e-6) for k in (C11,C33,C13,C12,C44)]),r'\\'

	if doLaTeX: sys.stdout=file('pbc-latex-parts-CC.tex','w')

	print r'\begin{tabular}{lccccc}\toprule'
	print '\t& $C_{11}$ & $C_{33}$ & $C_{13}$ & $C_{12}$ & $C_{44}$ ',r'\\ \midrule'
	print '\tmicroplane (\\ref{eq-Celliptic-micro}) &',' & '.join([(r'\textbf{%.3g}'%(k*1e-6) if k!=kMicroplane[4] else '%.3g'%(k*1e-6)) for k in kMicroplane]),r'\\'
	CCC=array([C11,C33,C13,C12,C44])
	###print '\tlattice (\\ref{eq-tE-knkt}) &',' & '.join(['%.3g'%(k*1e-6) for k in kPack]),r'\\'
	print '\tlattice (\\ref{eq-tE-knkt}) &',' & '.join(['%.3g'%(k*1e-6) for k in kPack]),r'\\'
	latErr=numpy.average(kPack.T/kMicroplane.T-1)
	print '\t\\quad error & ',' & '.join(['%.1g\\%% %s'%(e,'\\cellcolor{red}' if e>10 else '') for e in (1e2*(kPack.T/kMicroplane.T-1)).flat]),r'\\'
	print '\tsimulations (\\ref{eq-sim-Ex},\\ref{eq-sim-Gij},\\ref{eq-CC-from-EGnu}) & ',' & '.join(['%.3g'%(k*1e-6) for k in CCC]),r'\\'
	simErr=numpy.average(CCC.T/kMicroplane.T-1)
	print '\t\\quad error & ',' & '.join(['%2d\\%%'%e for e in (1e2*(CCC.T/kMicroplane.T-1)).flat]),r'\\'
	print r'\bottomrule\end{tabular}'

	sys.stdout=errFile
	print '%g\t%g\t%g\t%g'%(table.Ir,avgNumCont,100*simErr,100*latErr)

	# use more intelligent names
	kLattice,kSim=kPack,CCC
	sys.stdout=stiffFile
	print '%g\t'%(table.Ir)+'\t'.join(['\t'.join(['%.3g'%(1e-6*k[i]) for i in range(0,5)]) for k in kMicroplane.flat,kLattice.flat,kSim.flat])

	#errFile.write('C11mp C33mp C13mp C12mp C44mp C11lat C33lat C13lat C12lat C44lat C11sim C33sim C13sim C12sim C44sim\n')

	#print 'theoretical',(mu*array(Klattice)*uu).ravel().tolist()
	#	global microplaneK,microplaneMu
	#	microplaneK=(mu*array(Klattice)).ravel().tolist()
	#	microplaneMu=mu

sys.exit(0)

		
