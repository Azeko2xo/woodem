#ifndef __MESH3D_H__
#define __MESH3D_H__

#include "CollisionGeometry.hpp"

class Polyhedron : public CollisionGeometry
{
	public : string mshFileName;

	public : vector<Vector3r> vertices;
	public : vector<pair<int,int> > edges;
	public : vector<vector<int> > tetrahedrons;

	public : vector<vector<int> > faces;
	public : vector<Vector3r> fNormals;
	public : vector<Vector3r> vNormals;
 	public : vector<vector<int> > triPerVertices;
	// construction
	public : Polyhedron ();
	public : ~Polyhedron ();

	public : bool collideWith(CollisionGeometry* collisionGeometry);
	public : bool loadFromFile(char * fileName);
	public : void glDraw();
	public : void computeNormals();
	public : void loadGmshMesh(const string& fileName);

	protected : virtual void postProcessAttributes(bool deserializing);
	public : void registerAttributes();

	REGISTER_CLASS_NAME(Polyhedron);
	REGISTER_CLASS_INDEX(Polyhedron);

};

#include "ArchiveTypes.hpp"
using namespace ArchiveTypes;

REGISTER_SERIALIZABLE(Polyhedron,false);

#endif // __MESH3D_H__
