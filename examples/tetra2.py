from woo.dem import *
from woo.core import *
from woo.fem import *
from woo.gl import *
import woo.utils
mat=woo.dem.FrictMat(young=1e5,ktDivKn=.2,tanPhi=1)
halfThick=.001
S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-10),loneMask=0)])

maskTri=0b0001
maskTet=0b0010

if 0:
	# ignore return value		
	woo.utils.importNmesh('tetra2.horse.nmesh',mat=mat,mask=maskTet,dem=S.dem,surf=True,surfMask=maskTri,surfHalfThick=halfThick)
else:
	with open('tetra2.horse.nmesh') as m:
		mesh=[]
		for ll in m:
			l=ll[:-1].split()
			if len(l)==1: mesh.append([])
			elif len(mesh)==1: mesh[-1].append(tuple([float(i) for i in l]))
			else: mesh[-1].append(tuple([int(i)-1 for i in l[1:]]))
	# print mesh
	nn=[Node(pos=p,dem=DemData(blocked='XYZ')) for p in mesh[0]]
	t4=[Tet4.make((nn[t[0]],nn[t[1]],nn[t[2]],nn[t[3]]),mat=mat,wire=True,fixed=None,mask=maskTet) for t in mesh[1]]
	f3=[Facet.make((nn[t[0]],nn[t[1]],nn[t[2]]),mat=mat,wire=True,halfThick=halfThick,fixed=None,mask=maskTri) for t in mesh[2]]
	S.dem.par.add(t4)
	## surface facets, for contacts
	S.dem.par.add(f3)
	S.dem.nodesAppend(nn)
	# lumped mass
	#for n in nn:
	#	n.dem.mass=sum([p.material.density*.25*p.shape.getVolume() for p in n.dem.parRef if isinstance(p.shape,Tet4)])
	for n in nn: DemData.setOriMassInertia(n)

S.dem.par.add(Wall.make(-.1,axis=2,sense=1,mat=mat,glAB=((-.4,-.4),(.4,.4))))

S.lab.sigSym,S.lab.sigSkew=ScalarRange(label="sig sym"),ScalarRange(label="sig skew")
S.lab.sigSym.log,S.lab.sigSkew.log=True,True
#S.ranges=S.ranges+[S.lab.sigSym,S.lab.sigSkew]

def showStresses(S):
	for p in S.dem.par:
		if not isinstance(p.shape,Tet4): continue
		sig=p.shape.getStressTensor()
		if not isinstance(p.shape.node.rep,TensorGlRep): p.shape.node.rep=TensorGlRep(val=sig,range=S.lab.sigSym,skewRange=S.lab.sigSkew,skewRelSz=.02,relSz=.05)
		else: p.shape.node.rep.val=sig

S.engines=DemField.minimalEngines(damping=.01)+[IntraForce([In2_Tet4_ElastMat(),In2_Facet()]),PyRunner(100,'showStresses(S)'),VtkExport(out='/tmp/tetra2-',stepPeriod=200,what=VtkExport.mesh,mask=maskTet)]
S.lab.collider.noBoundOk=True
#S.lab.collider.dead=True
#S.lab.contactLoop.dead=True

S.saveTmp()

Gl1_DemField(nodes=False,glyph=Gl1_DemField.glyphForce)
woo.gl.Gl1_Facet.slices=-1
# import woo.log
#
# woo.log.setLevel('DynDt',woo.log.TRACE)
