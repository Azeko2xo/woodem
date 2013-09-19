#pragma once 

#include<woo/lib/base/Math.hpp>
#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Facet.hpp>
#include<woo/lib/sphere-pack/SpherePack.hpp>

#include <boost/iterator/zip_iterator.hpp>
#include <boost/range.hpp>

struct DemFuncs{
	DECLARE_LOGGER;
	static shared_ptr<DemField> getDemField(const Scene* scene);
	static std::tuple</*stress*/Matrix3r,/*stiffness*/Matrix6r> stressStiffness(const Scene* scene, const DemField* dem, bool skipMultinodal, Real volume);
	static Real unbalancedForce(const Scene* scene, const DemField* dem, bool useMaxForce);
	static shared_ptr<Particle> makeSphere(Real radius, const shared_ptr<Material>& m);
	static vector<Particle::id_t> SpherePack_toSimulation_fast(const shared_ptr<SpherePack>& sp, const Scene* scene, const DemField* dem, const shared_ptr<Material>& mat, int mask=0, Real color=NaN);
	static vector<Vector2r> boxPsd(const Scene* scene, const DemField* dem, const AlignedBox3r& box=AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)), bool mass=false, int num=20, int mask=0, Vector2r dRange=Vector2r::Zero());

	static bool particleStress(const shared_ptr<Particle>& p, Vector3r& normal, Vector3r& shear);

	static Real pWaveDt(const shared_ptr<DemField>& field, bool noClumps=false);
	static Real spherePWaveDt(Real radius, Real density, Real young){ return radius/sqrt(young/density); }

	// sum force and torque with regards to point pt over particles with matching mask
	// if *multinodal* is true, get force/troque from contacts of multinodal particles
	// (in addition to their nodal forces/torques);
	// returns number of particles matching the mask
	static size_t reactionInPoint(const Scene* scene, const DemField* dem, int mask, const Vector3r& pt, bool multinodal, Vector3r& force, Vector3r& torque);

	#if 0
		static Vector2r radialAxialForce(const Scene* scene, const DemField* dem, int mask, Vector3r axis, bool shear);
	#endif

	# if 0
		// helper zip range adaptor, when psd should iterates over 2 sequences (of diameters and radii)
		// http://stackoverflow.com/a/8513803/761090
		auto zip_end = 
		template <typename... T>
		auto zip(const T&... containers) -> boost::iterator_range<boost::zip_iterator<decltype(boost::make_tuple(std::begin(containers)...))>>{ return boost::make_iterator_range(boost::make_zip_iterator(boost::make_tuple(std::begin(containers)...)),boost::make_zip_iterator(boost::make_tuple(std::end(containers)...))); }
	#endif

	
	template<class IteratorRange, class DiameterGetter, class WeightGetter> /* iterate over spheres */
	static vector<Vector2r> psd(const IteratorRange& particleRange,
		bool cumulative, bool normalize, int num, Vector2r dRange,
		DiameterGetter diameterGetter,
		WeightGetter weightGetter,
		bool emptyOk=false
	){
		/* determine dRange if not given */
		if(isnan(dRange[0]) || isnan(dRange[1]) || dRange[0]<0 || dRange[1]<=0 || dRange[0]>=dRange[1]){
			dRange=Vector2r(Inf,-Inf);
			for(const auto& p: particleRange){
				Real d=diameterGetter(p);
				if(d<dRange[0]) dRange[0]=d;
				if(d>dRange[1]) dRange[1]=d;
			}
			if(isinf(dRange[0])){
				if(!emptyOk) throw std::runtime_error("DemFuncs::psd: no spherical particles?");
				else return vector<Vector2r>{Vector2r::Zero(),Vector2r::Zero()};
			}
		}
		/* put particles in bins */
		vector<Vector2r> ret(num,Vector2r::Zero());
		Real weight=0;
		for(const auto& p: particleRange){
			Real d=diameterGetter(p);
			Real w=weightGetter(p);
			// particles beyong upper bound are discarded, though their weight is taken in account
			weight+=w;
			if(d>dRange[1]) continue;
			int bin=max(0,min(num-1,1+(int)((num-1)*((d-dRange[0])/(dRange[1]-dRange[0])))));
			ret[bin][1]+=w;
		}
		// set radii values
		for(int i=0;i<num;i++) ret[i][0]=dRange[0]+i*(dRange[1]-dRange[0])/(num-1);
		// cummulate and normalize
		if(normalize) for(int i=0;i<num;i++) ret[i][1]=ret[i][1]/weight;
		if(cumulative) for(int i=1;i<num;i++) ret[i][1]+=ret[i-1][1];
		return ret;
	};

	/* Create facets from STL file; scale, shift and rotation (ori) are applied in this order before
	   creating nodes in global coordinates.

		Both ASCII and binary files are supported; COLOR and MATERIAL values translate to the
		Shape::color scalar; that means that color differences are preserved, but the rendering
		will be different.

		Threshold is distance relative to maximum bounding box size if negative; if positive, it is absolute
		in the STL space (before scaling).
	*/
	static vector<shared_ptr<Particle>> importSTL(const string& filename, const shared_ptr<Material>& mat, int mask=0, Real color=0., Real scale=1., const Vector3r& shift=Vector3r::Zero(), const Quaternionr& ori=Quaternionr::Identity(), Real threshold=-1e-6, Real maxBox=0);

};


