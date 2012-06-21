#pragma once
#include<woo/core/Field.hpp>
namespace voropp{
#include<woo/lib/voro++/voro++.cc>
};

struct VoroField: public Field{
	void updateFromDem();

	voropp::container_periodic_poly* conp;
	voropp::voronoi_network* vnet;

	void postLoad(VoroField&);
	void preSave(VoroField&);

	void cellsToPov(const std::string& out);
	void particlesToPov(const std::string& out);

	DECLARE_LOGGER;

	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(VoroField,Field,"Radical voronoi tesellation, given number of points",
		// ((vector<Vector4r>,sph,,AttrTrait<Attr::readonly>(),"Positions and radius of particles; negative radius means no sphere; index the same as particles in associated DEM field"))
		((Vector3i,subDiv,Vector3i::Zero(),,"Number of internal cell subdivisions for voro++; zero terms will be computed from relSubcellSize."))
		((Real,relSubcellSize,2.5,,"Approximate size of subdivision cell relative to average particle diameter"))
		((int,initMem,8,,"How many spheres per cell to allocate by default"))
		, /*ctor*/ conp=NULL; vnet=NULL;
		, /*py*/
			.def("cellsToPov",&VoroField::cellsToPov,"Export cells to POV-Ray file.")
			.def("particlesToPov",&VoroField::particlesToPov,"Export particles to POV-Ray file.")
			.def("updateFromDem",&VoroField::updateFromDem,"Set spheres from DEM field")
	);

};
REGISTER_SERIALIZABLE(VoroField);
