import itertools
from numpy import *
from woo.dem import *
from woo.core import *
from woo.gl import *
from woo import plot,pack,qt,utils
from minieigen import *
import woo

div=5
uv=mgrid[0.001:1:div*1j,0.001:1:div*1j,0.001:1:div*1j]

sp=woo.pack.SpherePack()
sp.makeCloud((0,0,0),(1,1,1),.04,.9,periodic=True)

S=woo.master.scene=woo.core.Scene(fields=[DemField()],dtSafety=.5)
sp.toSimulation(S)
#S.periodic=True
S.engines=DemField.minimalEngines(damping=.6)+[PyRunner(100,'addPlotData(S)')]
#[
#	ForceResetter(),
#	InsertionSortCollider([Bo1_Sphere_Aabb()]),
#	ContactLoop([Cg2_Sphere_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl(label='law')]),
#	IntraForce([In2_Sphere_ElastMat(label='inFunc')]),
#	PyRunner('addPlotData(S)',100),
#	Leapfrog(damping=.6,reset=False,kinSplit=True),
#	PyRunner('updateRep()',100,dead=False),
#	#PyRunner('colorFastRot()',2),
#	#PyRunner('O.scene.cell.velGrad=Matrix3.Zero',5000,initRun=False)
#]

S.lab.apa=AnisoPorosityAnalyzer()
S.cell.nextGradV=Vector3(-.5,0,0).asDiagonal()
S.trackEnergy=True

def addPlotData(S):
	dta={}
	pts={(0,0):'x',(1,0):'xy',(2,0):'y',(0,1):'xz',(1,1):'xyz',(2,1):'yz',(0,2):'z'}
	dta=dict([('p-%s'%(name),S.lab.apa.oneRay(theta=pt[0]*pi/4,phi=pt[1]*pi/4,vis=False)) for pt,name in pts.items()])
	dta.update(**S.energy)
	S.plot.addData(i=S.step,**dta)

#def colorFastRot():
#	Ekmax=max([p.Ekr for p in O.dem.par])
#	if Ekmax==0: return
#	for p in O.dem.par: p.shape.color=1.8*p.Ekr/Ekmax

S.plot.plots={'i':('p-x','p-xy','p-y','p-xz','p-xyz','p-yz','p-z'),
	' i':('**S.energy',),
}

from woo.gl import *
rr=qt.Renderer()
rr.extraDrawers=[GlExtra_AnisoPorosityAnalyzer(analyzer=S.lab.apa,num=0)]
rr.nodes=False
#rr.shape
#rr.wire=True
#forceRange=ScalarRange(label='contF')
#torqueRange=ScalarRange(label='T')
#scene.ranges=[Gl1_CPhys.range,Gl1_CPhys.shearRange,forceRange,torqueRange]
#Gl1_CPhys.range.label='Fn'
#Gl1_CPhys.shearRange.label='Fs'
#Gl1_CPhys.shearColor=True

S.any=[Gl1_CPhys()]

# from yade import log
#log.setLevel('AnisoPorosityAnalyzer',log.TRACE)

def showSphere(S):
	import pylab
	from mpl_toolkits.mplot3d import Axes3D
	fig = pylab.figure()
	ax=fig.add_subplot(111,projection='3d')
	nSteps=300 # number of steps per full circle
	xx,yy,zz,pp,tth,pphi=[],[],[],[],[],[]
	S.lab.apa.clearVis()
	for phi in linspace(-pi/2,pi/2,num=.5*nSteps,endpoint=True):
		for theta in linspace(0,2*pi,num=nSteps*cos(phi),endpoint=True):
			x,y,z=cos(theta)*cos(phi),sin(theta)*cos(phi),sin(phi); xx+=[x]; yy+=[y]; zz+=[z]; tth+=[theta]; pphi+=[phi]
			pp+=[S.lab.apa.relSolid(theta,phi,vis=False)]
	sc=ax.scatter(xx,yy,zz,c=pp,edgecolor='none')
	pylab.colorbar(sc)
	pylab.figure()
	pylab.scatter(tth,pphi,c=pp,marker='o',alpha=.5,edgecolor='none')
	pylab.show()
	
#def onSelection(S,obj):
#	print 'Selected',obj
#	import woo
#	if isinstance(obj,Contact):
#		for p in O.dem.par: p.shape.visible=(p in (obj.pA,obj.pB))
#		yade.law.watch=(obj.pA.id,obj.pB.id)
#		yade.inFunc.watch=(obj.pA.id,obj.pB.id)
#globals()['onSelection']=onSelection
	

S.stopAtStep=1000

S.saveTmp()

#for nu,nv in itertools.product(range(div),range(div)):
#	print uv[:,nu,nv]
#	print mkSegment(*uv[:,nu,nv])



