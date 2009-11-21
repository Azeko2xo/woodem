// © 2007 Václav Šmilauer <eudoxos@arcig.cz>

#pragma once

#include<vector>

#include<yade/core/InteractingGeometry.hpp>
#include<yade/core/InteractionGeometry.hpp>
#include<yade/core/InteractionSolver.hpp>

#include<yade/pkg-common/AABB.hpp>
#include<yade/pkg-common/BoundingVolumeEngineUnit.hpp>
#include<yade/pkg-common/InteractionGeometryEngineUnit.hpp>

#include<Wm3Math.h>
#include<Wm3Vector3.h>

#ifdef YADE_SHAPE
	#include<yade/pkg-common/Tetrahedron.hpp>
	#include<yade/pkg-common/InteractingGeometryEngineUnit.hpp>
#endif


/* Our mold of tetrahedron: just 4 vertices.
 *
 * Self-contained. */
class TetraMold: public InteractingGeometry{
	public:
		//! vertices of the tetrahedron.
		// FIXME - a std::vector....
		std::vector<Vector3r> v; 
		//Vector3r v[4];
		TetraMold(){createIndex(); v.resize(0); }
		TetraMold(Vector3r v0, Vector3r v1, Vector3r v2, Vector3r v3){createIndex(); v.resize(4); v[0]=v0; v[1]=v1; v[2]=v2; v[3]=v3; }
		virtual ~TetraMold (){};
	protected:
		REGISTER_ATTRIBUTES(InteractingGeometry,(v));
		REGISTER_CLASS_AND_BASE(TetraMold,InteractingGeometry);
		REGISTER_CLASS_INDEX(TetraMold,InteractingGeometry);
};
REGISTER_SERIALIZABLE(TetraMold);


/*! Collision configuration for Tetra and something.
 * This is expressed as penetration volume properties: centroid, volume, orientation of principal axes, inertia.
 *
 * Self-contained. */

class TetraBang: public InteractionGeometry{
	public:
		Real penetrationVolume;
		Real equivalentCrossSection;
		Real maxPenetrationDepthA, maxPenetrationDepthB, equivalentPenetrationDepth;
		Vector3r contactPoint;
		Vector3r normal;

		TetraBang(): InteractionGeometry(){};
		virtual ~TetraBang(){};
	protected:
		REGISTER_ATTRIBUTES(InteractionGeometry,(penetrationVolume)(equivalentCrossSection)(contactPoint)(normal)(equivalentPenetrationDepth)(maxPenetrationDepthA)(maxPenetrationDepthB));
		FUNCTOR2D(TetraMold,TetraMold);
		REGISTER_CLASS_AND_BASE(TetraBang,InteractionGeometry);
};
REGISTER_SERIALIZABLE(TetraBang);

#ifdef YADE_SHAPE
/*! Creates TetraMold from Tetrahedron.
 *
 * Self-contained. */

class Tetrahedron2TetraMold: public InteractingGeometryEngineUnit
{
	public:
		void go(const shared_ptr<GeometricalModel>& gm,shared_ptr<InteractingGeometry>& ig,const Se3r& se3,const Body*){
			Tetrahedron* tet=static_cast<Tetrahedron*>(gm.get());
			//! @fixme this seems superfluous?!: if(!ig)
			ig=boost::shared_ptr<InteractingGeometry>(new TetraMold(tet->v[0],tet->v[1],tet->v[2],tet->v[3]));
		}
	FUNCTOR2D(Tetrahedron,TetraMold);
	REGISTER_CLASS_NAME(Tetrahedron2TetraMold);
	REGISTER_BASE_CLASS_NAME(InteractingGeometryEngineUnit);
	DEFINE_FUNCTOR_ORDER_2D(Tetrahedron,TetraMold);
};
REGISTER_SERIALIZABLE(Tetrahedron2TetraMold);
#endif

/*! Creates AABB from TetraMold. 
 *
 * Self-contained. */

class TetraAABB: public BoundingVolumeEngineUnit
{
	public:
		void go(const shared_ptr<InteractingGeometry>& ig, shared_ptr<BoundingVolume>& bv, const Se3r& se3, const Body*){
			TetraMold* t=static_cast<TetraMold*>(ig.get());
			AABB* aabb=static_cast<AABB*>(bv.get());
			Quaternionr invRot=se3.orientation.Conjugate();
			Vector3r v_g[4]; for(int i=0; i<4; i++) v_g[i]=se3.orientation*t->v[i]; // vertices in global coordinates
			#define __VOP(op,ix) op(v_g[0][ix],op(v_g[1][ix],op(v_g[2][ix],v_g[3][ix])))
				aabb->min=se3.position+Vector3r(__VOP(std::min,0),__VOP(std::min,1),__VOP(std::min,2));
				aabb->max=se3.position+Vector3r(__VOP(std::max,0),__VOP(std::max,1),__VOP(std::max,2));
			#undef __VOP
			aabb->center=(aabb->min+aabb->max)*0.5;
			aabb->halfSize=(aabb->max-aabb->min)*0.5;
		}
		FUNCTOR2D(TetraMold,AABB);
		REGISTER_CLASS_NAME(TetraAABB);
		REGISTER_BASE_CLASS_NAME(BoundingVolumeEngineUnit);
};
REGISTER_SERIALIZABLE(TetraAABB);

#ifdef YADE_OPENGL
	#include<yade/pkg-common/GLDrawFunctors.hpp>
	/*! Draw TetraMold using OpenGL */
	class TetraDraw: public GLDrawInteractingGeometryFunctor
	{	
		public:
			virtual void go(const shared_ptr<InteractingGeometry>&, const shared_ptr<State>&,bool,const GLViewInfo&);

			RENDERS(TetraMold);
			REGISTER_CLASS_NAME(TetraDraw);
			REGISTER_BASE_CLASS_NAME(GLDrawInteractingGeometryFunctor);
	};
	REGISTER_SERIALIZABLE(TetraDraw);
#endif

/*! Calculate physical response based on penetration configuration given by TetraBang. */

class TetraLaw: public InteractionSolver {
	public:
		TetraLaw():InteractionSolver(){};

		int sdecGroupMask; // probably unused?!

		void action(MetaBody*);

	DECLARE_LOGGER;
	REGISTER_ATTRIBUTES(InteractionSolver,/* nothing*/);
	REGISTER_CLASS_NAME(TetraLaw);
	REGISTER_BASE_CLASS_NAME(InteractionSolver);
};
REGISTER_SERIALIZABLE(TetraLaw);



/*! @fixme implement Tetra2BoxBang by representing box as 6 tetrahedra. */

/*! Create TetraBang (collision geometry) from colliding TetraMolds. */
class Tetra2TetraBang: public InteractionGeometryEngineUnit
{
	public:
		virtual bool go(const shared_ptr<InteractingGeometry>& cm1, const shared_ptr<InteractingGeometry>& cm2, const Se3r& se31, const Se3r& se32, const shared_ptr<Interaction>& c);
		virtual bool goReverse(	const shared_ptr<InteractingGeometry>& cm1, const shared_ptr<InteractingGeometry>& cm2, const Se3r& se31, const Se3r& se32, const shared_ptr<Interaction>& c);

		FUNCTOR2D(TetraMold,TetraMold);
		REGISTER_CLASS_NAME(Tetra2TetraBang);
		REGISTER_BASE_CLASS_NAME(InteractionGeometryEngineUnit);
		DEFINE_FUNCTOR_ORDER_2D(TetraMold,TetraMold);
		DECLARE_LOGGER;
	private:
		list<TetraMold> Tetra2TetraIntersection(const TetraMold& A, const TetraMold& B);
		list<TetraMold> TetraClipByPlane(const TetraMold& T, const Vector3r& P, const Vector3r& n);
		//! Intersection of line given by points A, B and plane given by P and its normal.
		Vector3r PtPtPlaneIntr(const Vector3r& A, const Vector3r& B, const Vector3r& P, const Vector3r& normal){const double t=(P-A).Dot(normal) / (B-A).Dot(normal); /* TRWM3VEC(A); TRWM3VEC(B); TRWM3VEC(P); TRWM3VEC(normal); LOG_TRACE("t="<<t); TRWM3VEC((A+t*(B-A))); */ return A+t*(B-A); }
};

REGISTER_SERIALIZABLE(Tetra2TetraBang);

// Miscillaneous functions
//! Tetrahedron's volume.
/// http://en.wikipedia.org/wiki/Tetrahedron#Surface_area_and_volume
Real TetrahedronVolume(const Vector3r v[4]){ return fabs((Vector3r(v[3])-Vector3r(v[0])).Dot((Vector3r(v[3])-Vector3r(v[1])).Cross(Vector3r(v[3])-Vector3r(v[2]))))/6.; }
Real TetrahedronVolume(const vector<Vector3r>& v){ return fabs(Vector3r(v[1]-v[0]).Dot(Vector3r(v[2]-v[0]).Cross(v[3]-v[0])))/6.; }
Matrix3r TetrahedronInertiaTensor(const vector<Vector3r>& v);
//Matrix3r TetrahedronInertiaTensor(const Vector3r v[4]);
Matrix3r TetrahedronCentralInertiaTensor(const vector<Vector3r>& v);
//Matrix3r TetrahedronCentralInertiaTensor(const Vector3r v[4]);
Quaternionr TetrahedronWithLocalAxesPrincipal(shared_ptr<Body>& tetraBody);


