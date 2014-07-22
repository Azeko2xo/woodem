import woo.utils, woo.pack, woo.batch
from woo.dem import *
from woo.core import *
from woo.gl import *
from minieigen import *
from math import pi
import numpy

S=woo.master.scene=woo.core.Scene(fields=[DemField()])

mat=FrictMat(density=1e3,young=1e7)

woo.batch.readParamsFromTable(S,unknownOk=True,
	relStressTol=5e-4,
	#E1=1e8,E2=1e7,G1=5e6,G2=2e6,
	C11=130e6,C33=55e6,C13=28e6,C12=40e6,
	#C11=130e6,C33=130e6,C13=28e6,C12=28e6,
	Ir=1.6,
	axis='xx',
	maxStrain=.01,
	packScale=1.,
)

sp=woo.pack.SpherePack()
sp.load('pbc-spheres:N=2556,r=0.03,rRelFuzz=0.txt')
sp.scale(S.lab.table.packScale)
sp.toSimulation(S,mat=mat)
S.dem.collectNodes()

axis=S.lab.axis={'xx':0,'yy':1,'zz':2,'yz':3,'zy':3,'zx':4,'xz':4,'xy':5,'yx':5}[S.lab.table.axis]
isShear=axis>=3

S.engines=[
	Leapfrog(damping=.5,label='leapfrog',reset=True),
	InsertionSortCollider([Bo1_Sphere_Aabb(label='bo1',distFactor=S.lab.table.Ir)],label='collider'),
	ContactLoop([Cg2_Sphere_Sphere_L6Geom(label='cg2',distFactor=S.lab.table.Ir)],
		[Cp2_FrictMat_FrictPhys_CrossAnisotropic(E1=1,E2=1,G1=1,G2=1,alpha=0,beta=0,label='xiso')],
		[Law2_L6Geom_FrictPhys_IdealElPl(label='law',noBreak=True,noSlip=True,iniEqlb=True)],
		label='loop',evalStress=True
	),
	DynDt(stepPeriod=1000),
]

initPos=numpy.array([Vector3(p.pos) for p in S.dem.par])
strainRate=1./(S.cell.size[0])


# just create initial contacts and deactivate the collider, don't move yet
S.dt=0 # a very small timestep would do as well
# create initial contacts, deactivate the collider
S.one()
S.dem.con.removeNonReal()
import woo, numpy, numpy.linalg
S.lab.collider.dead=True

# compute lattice parameters and reassign stiffnesses
mu=woo.utils.muStiffnessScaling(piHat=pi)
#
Ace=mu*(1/35.)*numpy.matrix([[36,6,20,8],[12,30,16,12],[8,6,-8,-6],[12,2,-12,-2]])
EEGG=numpy.linalg.inv(Ace)*numpy.matrix([S.lab.table.C11,S.lab.table.C33,S.lab.table.C13,S.lab.table.C12]).T
print 'microplane stiffnesses ENa, ENb, ETa, ETb',EEGG.T
# reassign correect stiffness
S.lab.xiso.E1,S.lab.xiso.E2,S.lab.xiso.G1,S.lab.xiso.G2=tuple(EEGG.flat)
S.lab.loop.updatePhys=True
S.one(); S.one()
S.lab.loop.updatePhys=False
S.dt=float('nan') # force recomputation of timestep
S.dtSafety=.9

print 'CC components (input  ):',S.lab.table.C11,S.lab.table.C33,S.lab.table.C13,S.lab.table.C12
kPackMat=woo.utils.stressStiffnessWork()[1]
CClattice=numpy.array([kPackMat[vi] for vi in ((0,0),(2,2),(0,2),(0,1),(3,3))])
print 'CC components (lattice):',' '.join([str(x) for x in CClattice.flat])

if axis==0: S.save('pbc-deform-sample-xx-saved.Ir=%g.gz'%S.lab.table.Ir)
S.saveTmp()
print 'initial cell dimensions',S.cell.size

def shearLoading(S):
	ax=S.lab.axis
	trsf=S.cell.trsf
	eps=2*trsf[S.lab.shearAxes]
	print 'should be the same:',eps,trsf[S.lab.shearAxes]+trsf[S.lab.shearAxes[1],S.lab.shearAxes[0]]
	if abs(eps)<S.lab.table.maxStrain: return
	print 'SHEAR',eps,S.lab.loop.stress[S.lab.shearAxes]
	sys.exit(0)

def axialDone(S):
	sig=S.lab.loop.stress.diagonal()
	print 'STRESSES',sig[0],sig[1],sig[2]
	eps=S.cell.trsf.diagonal()-Vector3.Ones
	print 'STRAINS',eps[0],eps[1],eps[2]
	import sys
	sys.exit(0)

#
# engines controlling deformation
#
if isShear:
	S.lab.shearAxes={3:(1,2),4:(0,2),5:(0,1)}[axis]
	S.cell.nextGradV=Matrix3.Zero
	S.cell.nextGradV[S.lab.shearAxes]=.5*strainRate
	S.cell.nextGradV[S.lab.shearAxes[1],S.lab.shearAxes[0]]=.5*strainRate
	S.engines=S.engines+[PyRunner(100,'shearLoading(S)')]
else:
	goal=Vector3(0,0,0)
	goal[axis]=-S.lab.table.maxStrain
	ax1,ax2=(axis+1)%3,(axis+2)%3
	stressMask=(1<<ax1 | 1<<ax2)
	S.engines=[WeirdTriaxControl(maxStrainRate=(1.,1.,1.),goal=goal,stressMask=stressMask,globUpdate=1,relStressTol=-1e-4,maxUnbalanced=.05,doneHook='axialDone(S)',mass=.1*S.cell.volume*mat.density,label='triax',growDamping=.6),PyRunner(200,'print S.lab.triax.stress.diagonal()')]+S.engines
	#S.cell.nextGradV=-strainRate*Vector3.Unit(axis).asDiagonal()


def showLattice(constRad=True,constCol=False,relSz=.1):
	rg=ScalarRange(label='kn')
	S.ranges=[rg]
	nan=float('nan')
	for c in S.dem.con:
		r1,r2=c.pA.shape.radius,c.pB.shape.radius
		col=c.phys.kn if not constCol else nan
		xx=(-(r1+.5*c.geom.uN),(r2+.5*c.geom.uN))
		if constRad: c.geom.node.rep=CylGlRep(xx=xx,rad=nan,col=col,rangeCol=rg,relSz=relSz)
		else: c.geom.node.rep=CylGlRep(xx=xx,rad=c.phys.kn,col=col,rangeRad=rg,rangeCol=rg,relSz=relSz)

if not woo.batch.inBatch():
	S.one()
	#Gl1_DemField(cPhys=True,shape=0,shape2=0)
	#S.ranges=S.ranges+[Gl1_CPhys.range,Gl1_CPhys.shearRange]
	S.run()
else:
	S.run()
	woo.batch.wait()
