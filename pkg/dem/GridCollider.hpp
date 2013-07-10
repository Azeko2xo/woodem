#pragma once
// © 2013 Václav Šmilauer <eu@doxos.eu>
#include<woo/pkg/dem/GridStore.hpp>
#include<woo/pkg/dem/GridBound.hpp>

struct GridCollider: public Collider{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	DemField* dem;
	void postLoad(GridCollider&, void*);
	void selfTest() WOO_CXX11_OVERRIDE;
	// say in which cell lies given spatial coordinate
	// no bound checking is done, caller must assure the result makes sense
	Vector3i xyz2ijk(const Vector3r& xyz) const;
	// point nearest to given point *from* in cell *ijk*
	Vector3r xyzNearXyz(const Vector3r& xyz, const Vector3i& ijk) const;
	// point nearest to given cell *from* in cell *ijk*
	Vector3r xyzNearIjk(const Vector3i& from, const Vector3i& ijk) const;
	// bounding box for given cell
	AlignedBox3r ijk2box(const Vector3i& ijk) const;
	AlignedBox3r ijk2boxShrink(const Vector3i& ijk, const Real& shrink) const;
	// lower corner of given cell
	Vector3r ijk2boxMin(const Vector3i& ijk) const;
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&) WOO_CXX11_OVERRIDE;
	#endif
	void run() WOO_CXX11_OVERRIDE;
	// constant on what should be done in processCell
	enum {PROCESS_FULL=0};
	void processCell(const int& what, const shared_ptr<GridStore>& gridA, const Vector3i& ijkA, const shared_ptr<GridStore>& gridB, const Vector3i& ijkB);
	// add new contact if permissible and does not yet exist
	bool tryAddNewContact(const Particle::id_t& idA, const Particle::id_t& idB);


	WOO_CLASS_BASE_DOC_ATTRS_PY(GridCollider,Collider,"Grid-based collider.",
		// those are noSave, since their internal data are not saved anyway (yet?)
		((shared_ptr<GridStore>,gridPrev,,AttrTrait<Attr::noSave>(),"Previous fully populated grid."))
		((shared_ptr<GridStore>,gridCurr,,AttrTrait<Attr::noSave>(),"Current fully populated grid."))
		((shared_ptr<GridStore>,gridOld,,AttrTrait<Attr::noSave>(),"Grid containing entries in :obj:`gridPrev` but not in :obj:`gridCurr`."))
		((shared_ptr<GridStore>,gridNew,,AttrTrait<Attr::noSave>(),"Grid containing entries in :obj:`gridCurr` but not in :obj:`gridPrev`."))
		((Real,verletDist,0,AttrTrait<>().lenUnit(),"Length by which particle size is enalrged, to avoid running the collider at every timestep."))
		((bool,around,false,,"If frue, particle in every cell is checked with particles in all cells around; this makes the grid storage substantially less loaded, as all particles can be shrunk by one half of the cell size."))
		((Real,shrink,0,AttrTrait<Attr::readonly>().noGui(),"The amount of shrinking for each particle (zero if *around* is false, otherwise half of minimum cell size"))
		// grid definition
		((AlignedBox3r,domain,AlignedBox3r(Vector3r::Zero(),Vector3r::Ones()),AttrTrait<Attr::triggerPostLoad>(),"Domain spanned by the grid."))
		((Real,minCellSize,1.,AttrTrait<Attr::triggerPostLoad>(),"Minimum cell size which will be used to compute :obj:`dim`."))
		((Vector3i,dim,Vector3i(-1,-1,-1),AttrTrait<Attr::readonly>(),"Number of cells along each axis"))
		((Vector3r,cellSize,Vector3r(NaN,NaN,NaN),AttrTrait<Attr::readonly>(),"Actual cell size"))
		((shared_ptr<GridBoundDispatcher>,gridBoundDispatcher,,,"Dispatches :obj:`GridBound` creation to :obj:`GridBoundFuctor` based on :obj:`Shape` type."))
		((Vector3r,color,Vector3r(1,1,0),AttrTrait<>().rgbColor(),"Color for rendering the domain"))
		((shared_ptr<ScalarRange>,occupancyRange,,,"Range for coloring grids based on occupancy (automatically created)"))
		// tunables
		((int,gridDense,4,,"Length of dense storage for new :obj:`GridStore` objects."))
		((int,exIniSize,4,,":obj:`GridStore.exIniSize` for new grids."))
		((int,exNumMaps,10,,":obj:`GridStore.exNumMaps` for new grids."))
		, /*py*/
			.def("xyz2ijk",&GridCollider::xyz2ijk,"Convert spatial coordinates to cell coordinate (no bound checking is done)")
			.def("xyzNearXyz",&GridCollider::xyzNearXyz,(py::arg("from"),py::arg("ijk")),"Return point nearest to *from*(given in spatial coords) in cell *ijk* (corner, at a face, on an edge, inside)")
			.def("xyzNearIjk",&GridCollider::xyzNearIjk,(py::arg("from"),py::arg("ijk")),"Return point nearest to *from* (given in cell coords) in cell *ijk*")
			.def("ijk2box",&GridCollider::ijk2box,(py::arg("ijk")),"Box for given cell")
	);
};
WOO_REGISTER_OBJECT(GridCollider);

