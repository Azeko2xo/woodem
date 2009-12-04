"""
Import geometry from various formats ('import' is python keyword, hence the name 'ymport').
"""

from yade.wrapper import *
from miniWm3Wrap import *
from yade import utils


def ascii(filename,scale=1.,wenjieFormat=False,**kw):
	"""Load sphere coordinates from file, create spheres, insert them to the simulation.

	filename is the file holding ASCII numbers (at least 4 colums that hold x_center, y_center, z_center, radius).
	All remaining arguments are passed the the yade.utils.sphere function that creates the bodies.

	wenjieFormat will skip all lines that have exactly 5 numbers and where the 4th one is exactly 1.0 -
	this was used by a fellow developer called Wenjie to mark box elements.
	
	Returns list of body ids that were inserted into simulation."""
	from yade.utils import sphere
	o=Omega()
	ret=[]
	for l in open(filename):
		ss=[float(i) for i in l.split()]
		if wenjieFormat and len(ss)==5 and ss[4]==1.0: continue
		id=o.bodies.append(sphere([scale*ss[0],scale*ss[1],scale*ss[2]],scale*ss[3],**kw))
		ret.append(id)
	return ret

def stl(file, dynamic=False,wire=True,color=None,highlight=False,noBound=False,material=0):
	""" Import geometry from stl file, create facets and return list of their ids."""
	imp = STLImporter()
	imp.open(file)
	begin=len(O.bodies)
	imp.import_geometry(O.bodies)
	imported=range(begin,begin+imp.number_of_facets)
	for i in imported:
		b=O.bodies[i]
		b['isDynamic']=dynamic
		b.mold.postProcessAttributes(True)
		b.mold['diffuseColor']=color if color else utils.randomColor()
		b.mold['wire']=wire
		b.mold['highlight']=highlight
		utils._commonBodySetup(b,0,Vector3(0,0,0),noBound=noBound,material=material,resetState=False)
	return imported

def gmsh(meshfile="file.mesh",**kw):
	""" Imports geometry from mesh file and creates facets.
	Remaining **kw arguments are passed to utils.facet; 
	mesh files can be easily created with GMSH http://www.geuz.org/gmsh/
	Example added to scripts/test/regular-sphere-pack.py"""
	infile = open(meshfile,"r")
	lines = infile.readlines()
	infile.close()

	nodelistVector3=[]
	numNodes = int(lines[4].split()[0])
	for i in range(numNodes):
		nodelistVector3.append(Vector3(0.0,0.0,0.0))
	id = 0
	for line in lines[5:numNodes+5]:
		data = line.split()
		X = float(data[0])
		Y = float(data[1])
		Z = float(data[2])
		nodelistVector3[id] = Vector3(X,Y,Z)
		id += 1
	numTriangles = int(lines[numNodes+6].split()[0])
	triList = []
	for i in range(numTriangles):
		triList.append([0,0,0,0])
	 
	tid = 0
	for line in lines[numNodes+7:numNodes+7+numTriangles]:
		data = line.split()
		id1 = int(data[0])-1
		id2 = int(data[1])-1
		id3 = int(data[2])-1
		triList[tid][0] = tid
		triList[tid][1] = id1
		triList[tid][2] = id2
		triList[tid][3] = id3
		tid += 1
		ret=[]
	for i in triList:
		a=nodelistVector3[i[1]]
		b=nodelistVector3[i[2]]
		c=nodelistVector3[i[3]]
		ret.append(utils.facet((nodelistVector3[i[1]],nodelistVector3[i[2]],nodelistVector3[i[3]]),**kw))
	return ret

def gengeoFile(fileName="file.geo",moveTo=[0.0,0.0,0.0],scale=1.0,**kw):
	""" Imports geometry from LSMGenGeo .geo file and creates spheres.
	moveTo[X,Y,Z] parameter moves the specimen.
	Remaining **kw arguments are passed to utils.sphere; 
	
	LSMGenGeo library allows to create pack of spheres
	with given [Rmin:Rmax] with null stress inside the specimen.
	Can be usefull for Mining Rock simulation.
	
	Example added to scripts/test/regular-sphere-pack.py
	Example of LSMGenGeo library using is added to genCylLSM.py
	
	http://www.access.edu.au/lsmgengeo_python_doc/current/pythonapi/html/GenGeo-module.html
	https://svn.esscc.uq.edu.au/svn/esys3/lsm/contrib/LSMGenGeo/"""
	from yade.utils import sphere

	infile = open(fileName,"r")
	lines = infile.readlines()
	infile.close()

	numSpheres = int(lines[6].split()[0])
	ret=[]
	for line in lines[7:numSpheres+7]:
		data = line.split()
		ret.append(utils.sphere([moveTo[0]+scale*float(data[0]),moveTo[1]+scale*float(data[1]),moveTo[2]+scale*float(data[2])],scale*float(data[3]),**kw))
	return ret

def gengeo(mntable,moveTo=[0.0,0.0,0.0],scale=1.0,**kw):
	""" Imports geometry from LSMGenGeo library and creates spheres.
	moveTo[X,Y,Z] parameter moves the specimen.
	Remaining **kw arguments are passed to utils.sphere; 
	
	LSMGenGeo library allows to create pack of spheres
	with given [Rmin:Rmax] with null stress inside the specimen.
	Can be usefull for Mining Rock simulation.
	
	Example added to scripts/test/regular-sphere-pack.py
	Example of LSMGenGeo library using is added to genCylLSM.py
	
	http://www.access.edu.au/lsmgengeo_python_doc/current/pythonapi/html/GenGeo-module.html
	https://svn.esscc.uq.edu.au/svn/esys3/lsm/contrib/LSMGenGeo/"""
	from GenGeo import MNTable3D,Sphere
	
	ret=[]
	sphereList=mntable.getSphereListFromGroup(0)
	for i in range(0, len(sphereList)):
		r=sphereList[i].Radius()
		c=sphereList[i].Centre()
		ret.append(utils.sphere([moveTo[0]+scale*float(c.X()),moveTo[1]+scale*float(c.Y()),moveTo[2]+scale*float(c.Z())],scale*float(r),**kw))
	return ret
