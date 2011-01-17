// 2007 © Václav Šmilauer <eudoxos@arcig.cz>

#pragma once

#include<string>
#include<map>
#include<iostream>
#include<typeinfo>
#include<boost/any.hpp>
#include<boost/lambda/lambda.hpp>

#include<yade/lib/base/Math.hpp>
#include<yade/lib/base/Logging.hpp>
#include<yade/core/Body.hpp>

#include<boost/function.hpp>

class Scene;
class Body;
class SimpleViscoelasticBodyParameters;
class ViscElMat;
class FrictMat;
class Interaction;

using namespace std;
using boost::shared_ptr;
namespace py = boost::python;

/*! Miscillaneous utility functions which are believed to be generally useful.
 *
 * All data members are methods are static, no instance of Shop is created. It is not serializable either.
 */

class Shop{
	public:
		DECLARE_LOGGER;

		//! create default sphere, along with its bound etc. 
		static shared_ptr<Body> sphere(Vector3r center, Real radius, shared_ptr<Material> mat);
		//! create default box with everything needed
		static shared_ptr<Body> box(Vector3r center, Vector3r extents, shared_ptr<Material> mat);
		//! create default tetrahedron
		static shared_ptr<Body> tetra(Vector3r v[4], shared_ptr<Material> mat);

		//! return instance of default FrictMat
		static shared_ptr<FrictMat> defaultGranularMat();

		//! Return vector of pairs (center,radius) loaded from a file with numbers inside
		static vector<pair<Vector3r,Real> > loadSpheresFromFile(const string& fname,Vector3r& minXYZ, Vector3r& maxXYZ, Vector3r* cellSize=NULL);
		
		//! Save spheres in the current simulation into a text file
		static void saveSpheresToFile(string fileName);

		//! Compute the total volume of spheres
		static Real getSpheresVolume(const shared_ptr<Scene>& rb=shared_ptr<Scene>());
		//! Compute porosity; volume must be given for aperiodic simulations
		static Real getPorosity(const shared_ptr<Scene>& rb=shared_ptr<Scene>(),Real volume=-1);

		//! Estimate timestep based on P-wave propagation speed
		static Real PWaveTimeStep(const shared_ptr<Scene> rb=shared_ptr<Scene>());

		//! return 2d coordinates of a 3d point within plane defined by rotation axis and inclination of spiral, wrapped to the 0th period
		static boost::tuple<Real, Real, Real> spiralProject(const Vector3r& pt, Real dH_dTheta, int axis=2, Real periodStart=std::numeric_limits<Real>::quiet_NaN(), Real theta0=0);

		//! Calculate inscribed circle center of trianlge
		static Vector3r inscribedCircleCenter(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2);

		/// Get viscoelastic parameters kn,cn,ks,cs from analytical solution of
		/// a problem of interaction of pair spheres with mass 1, collision
		/// time tc and restitution coefficients en,es.
	    static void getViscoelasticFromSpheresInteraction(Real tc, Real en, Real es, shared_ptr<ViscElMat> b);

		//! Get unbalanced force of the whole simulation
		static Real unbalancedForce(bool useMaxForce=false, Scene* _rb=NULL);
		static Real kineticEnergy(Scene* _rb=NULL, Body::id_t* maxId=NULL);

		static Vector3r totalForceInVolume(Real& avgIsoStiffness, Scene *_rb=NULL);


		//! create transientInteraction between 2 bodies, using existing Dispatcher in Omega
		static shared_ptr<Interaction> createExplicitInteraction(Body::id_t id1, Body::id_t id2, bool force);

		//! apply force on contact point on both bodies (reversed on body 2)
		static void applyForceAtContactPoint(const Vector3r& force, const Vector3r& contPt, Body::id_t id1, const Vector3r& pos1, Body::id_t id2, const Vector3r& pos2, Scene* scene);

		//! map scalar variable to 1d colorscale
		static Vector3r scalarOnColorScale(Real x, Real xmin=0., Real xmax=1.);

		//! wrap floating number periodically to the given range
		static Real periodicWrap(Real x, Real x0, Real x1, long* period=NULL);

		//! Flip cell shear without affecting interactions; if flip is zeros, it will be computed such that abs of shear strain is minimal for each shear component
		//! Diagonal terms of flip are meaningless and ignored.
		static Matrix3r flipCell(const Matrix3r& flip=Matrix3r::Zero());

		//! Class for storing stresses, affected on bodies, obtained from Interactions
		struct bodyState{
				Vector3r normStress, shearStress;
				bodyState (){
					normStress = Vector3r(0.0,0.0,0.0);
					shearStress = Vector3r(0.0,0.0,0.0);
				}
		};
		//! Function of getting stresses for each body
		static void getStressForEachBody(vector<Shop::bodyState>&);
		
		//! Define the exact average stress in each particle from contour integral ("LW" stands for Love-Weber, since this is what the contour integral gives).
		static void getStressLWForEachBody(vector<Matrix3r>& bStresses, bool revertSign=false);
		static py::tuple getStressLWForEachBody(bool revertSign);

		//! Function to compute overall ("macroscopic") stress of periodic cell
		static Matrix3r stressTensorOfPeriodicCell(bool smallStrains=true);
		//! Compute overall ("macroscopic") stress of periodic cell, returning 2 tensors
		//! (contribution of normal and shear forces)
		static py::tuple normalShearStressTensors(bool compressionPositive=false);
};
