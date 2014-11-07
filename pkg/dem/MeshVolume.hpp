
#pragma once

#ifdef WOO_GTS

#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Facet.hpp>

#include<gts.h>

struct MeshVolume: public PeriodicEngine{
	vector<std::unique_ptr<GtsVertex>> vertices;
	vector<std::unique_ptr<GtsEdge>> edges;
	vector<std::unique_ptr<GtsFace>> faces;
	std::unique_ptr<GtsSurface> surface;
	void init();
	void updateVertices();
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void run();
	Real pyNetVol() const{ return vol-thickVol; }
	WOO_DECL_LOGGER;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(MeshVolume,PeriodicEngine,"Compute volume of (possibly deforming) closed triangulated surface; depends on the *gts* feature.",
		((int,mask,0,,"Mask for finding surface triangles"))
		((bool,reinit,false,,"If true, recreate internal data from scratch"))
		((vector<shared_ptr<Node>>,nodes,,AttrTrait<Attr::noSave>().noGui(),"List of nodes, in the same order as the GTS surface structure."))
		((Real,vol,NaN,,"Volume as computed when last run"))
		((Real,thickVol,NaN,,"Volume of the inner side of the mesh: the mesh is defined by :obj:`facets' <Facet>` midplanes, but some facets may have non-zero :obj:`Facet.halfThick`. This number is the sum of (initial!) facet area times :obj:`Facet.halfThick`. To get the volume with this part subtracted, use :obj:`netVol`."))
		, /*ctor*/
		,/*py*/ .add_property("netVol",&MeshVolume::pyNetVol,"Net volume: :obj:`volume` minus :obj:`thickVol`.")
	);
};
WOO_REGISTER_OBJECT(MeshVolume);

#endif
