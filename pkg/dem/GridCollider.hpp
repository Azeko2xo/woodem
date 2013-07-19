#pragma once
// © 2013 Václav Šmilauer <eu@doxos.eu>
#include<woo/pkg/dem/GridStore.hpp>
#include<woo/pkg/dem/GridBound.hpp>

// #define this macro to enable timing within this engine
// #define GC_TIMING

#ifdef GC_TIMING
	#define GC_CHECKPOINT(cpt) timingDeltas->checkpoint(__LINE__,cpt)
	#define GC_CHECKPOINT2(cpt) timingDeltas->checkpoint(500+__LINE__,cpt)
#else
	#define GC_CHECKPOINT(cpt)
	#define GC_CHECKPOINT2(cpt)
#endif



struct GridCollider: public Collider{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	DemField* dem;
	void postLoad(GridCollider&, void*);
	void selfTest() WOO_CXX11_OVERRIDE;
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&) WOO_CXX11_OVERRIDE;
	#endif
	void run() WOO_CXX11_OVERRIDE;
	// templated so that the compiler can optimize better
	// with sameGridCell, the cell is being compared with itself
	template<bool sameGridCell, bool addContacts=true> void processCell(const shared_ptr<GridStore>& gridA, const Vector3i& ijkA, const shared_ptr<GridStore>& gridB, const Vector3i& ijkB) const;

	// add new contact if permissible and does not yet exist
	bool tryAddContact(const Particle::id_t& idA, const Particle::id_t& idB) const;
	bool tryDeleteContact(const Particle::id_t& idA, const Particle::id_t& idB) const;

	bool allParticlesWithinPlay() const;
	void prepareGridCurr();
	void fillGridCurr();

	void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d) WOO_CXX11_OVERRIDE;
	void getLabeledObjects(std::map<std::string,py::object>& m, const shared_ptr<LabelMapper>&) WOO_CXX11_OVERRIDE;

	// forces reinitialization
	void invalidatePersistentData() WOO_CXX11_OVERRIDE { gridPrev.reset(); }

	WOO_CLASS_BASE_DOC_ATTRS_PY(GridCollider,Collider,"Grid-based collider.",
		// grid definition
		((AlignedBox3r,domain,AlignedBox3r(Vector3r::Zero(),Vector3r::Ones()),AttrTrait<Attr::triggerPostLoad>().startGroup("Grid geometry"),"Domain spanned by the grid."))
		((Real,minCellSize,1.,AttrTrait<Attr::triggerPostLoad>(),"Minimum cell size which will be used to compute :obj:`dim`."))
		((Vector3i,dim,Vector3i(-1,-1,-1),AttrTrait<Attr::readonly>(),"Number of cells along each axis"))
		((Vector3r,cellSize,Vector3r(NaN,NaN,NaN),AttrTrait<Attr::readonly>(),"Actual cell size"))
		// data
		// those are noSave, since their internal data are not saved anyway (yet?)
		((shared_ptr<GridStore>,gridPrev,,AttrTrait<Attr::noSave>().startGroup("Data"),"Previous fully populated grid."))
		((shared_ptr<GridStore>,gridCurr,,AttrTrait<Attr::noSave>(),"Current fully populated grid."))
		((shared_ptr<GridStore>,gridOld,,AttrTrait<Attr::noSave>(),"Grid containing entries in :obj:`gridPrev` but not in :obj:`gridCurr`."))
		((shared_ptr<GridStore>,gridNew,,AttrTrait<Attr::noSave>(),"Grid containing entries in :obj:`gridCurr` but not in :obj:`gridPrev`."))
		// rendering
		((Vector3r,color,Vector3r(1,1,0),AttrTrait<>().rgbColor().startGroup("Rendering"),"Color for rendering the domain"))
		((bool,renderCells,false,,"Render cells."))
		((int,minOccup,0,,"Minimum occupancy for cell to be rendered (zero cells are never rendered)."))
		((shared_ptr<ScalarRange>,occupancyRange,,,"Range for coloring grids based on occupancy (automatically created)"))
		// tunables
		((int,gridDense,6,AttrTrait<>().startGroup("Tunables"),"Length of dense storage for new :obj:`GridStore` objects."))
		((int,exIniSize,6,,":obj:`GridStore.exIniSize` for new grids."))
		((int,exNumMaps,100,,":obj:`GridStore.exNumMaps` for new grids."))
		((int,complSetMinSize,-1,,"The value of *setMinSize* when calling :obj:`GridStore.computeRelativeComplements`."))
		((bool,useDiff,true,,"Create new contacts based on set complement of :obj:`gridPrev` with respect to :obj:`gridCurr` if both contain meaningful data and are compatible; if false, always traverse *gridCurr* and try adding all possible contacts (this should be much slower)"))
		((bool,around,false,AttrTrait<Attr::triggerPostLoad>(),"If frue, particle in every cell is checked with particles in all cells around; this makes the grid storage substantially less loaded, as all particles can be shrunk by one half of the cell size.\n\n.. warning:: this feature is broken and will raise exception if enabled; the trade-off is not good, since many more cells need to be checked around every cell, and many more potential contacts are created."))
		((Real,shrink,0,AttrTrait<Attr::readonly>().noGui(),"The amount of shrinking for each particle (half of the minimum cell size if *around* is true, zero otherwise."))
		((Real,verletDist,0,AttrTrait<>().lenUnit(),"Length by which particle size is enalrged, to avoid running the collider at every timestep."))
		((shared_ptr<GridBoundDispatcher>,boundDispatcher,make_shared<GridBoundDispatcher>(),,"Dispatches :obj:`GridBound` creation to :obj:`GridBoundFuctor` based on :obj:`Shape` type."))
		, /*py*/
	);
};
WOO_REGISTER_OBJECT(GridCollider);

