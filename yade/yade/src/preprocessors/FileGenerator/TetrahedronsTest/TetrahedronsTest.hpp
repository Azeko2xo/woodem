
#ifndef __TETRAHEDRONSTEST_HPP__
#define __TETRAHEDRONSTEST_HPP__

#include <yade/FileGenerator.hpp>
#include <yade-common/Tetrahedron.hpp>

class TetrahedronsTest : public FileGenerator
{
	private : Vector3r nbTetrahedrons;
	private : Real minRadius,density;
	private : Vector3r groundSize,gravity;
	private : Real maxRadius;
	private : Real dampingForce;
	private	: Real disorder;
	private : Real dampingMomentum;
	private : int timeStepUpdateInterval;
	private : bool rotationBlocked;
	private : Real sphereYoungModulus,spherePoissonRatio,sphereFrictionDeg;
	// construction
	public : TetrahedronsTest ();
	public : ~TetrahedronsTest ();


	private : void createBox(shared_ptr<Body>& body, Vector3r position, Vector3r extents);
	private : void createTetrahedron(shared_ptr<Body>& body, int i, int j, int k);
	private : void createActors(shared_ptr<MetaBody>& rootBody);
	private : void positionRootBody(shared_ptr<MetaBody>& rootBody);
	
	private : void loadTRI(shared_ptr<Tetrahedron>& tet, const string& fileName);

	protected : virtual void postProcessAttributes(bool deserializing);
	public : void registerAttributes();

	public : string generate();

	REGISTER_CLASS_NAME(TetrahedronsTest);
};

REGISTER_SERIALIZABLE(TetrahedronsTest,false);

#endif // __TETRAHEDRONSTEST_HPP__ 

