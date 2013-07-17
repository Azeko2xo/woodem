#pragma once
// © 2013 Václav Šmilauer <eu@doxos.eu>
#include<woo/pkg/dem/GridStore.hpp>
#include<woo/pkg/dem/GridBound.hpp>

// #define this macro to enable timing within this engine
#define GC_TIMING

#ifdef GC_TIMING
	#define GC_CHECKPOINT(cpt) timingDeltas->checkpoint(__LINE__,cpt)
#else
	#define GC_CHECKPOINT(cpt)
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
	// constant on what should be done in processCell
	enum {PROCESS_FULL=0};
	// templated so that the compiler can optimize better
	template<int what> void processCell(const shared_ptr<GridStore>& gridA, const Vector3i& ijkA, const shared_ptr<GridStore>& gridB, const Vector3i& ijkB) const;

	// add new contact if permissible and does not yet exist
	bool tryAddNewContact(const Particle::id_t& idA, const Particle::id_t& idB) const;

	bool allParticlesWithinPlay() const;
	void prepareGridCurr();
	void fillGridCurr();
	


	WOO_CLASS_BASE_DOC_ATTRS_PY(GridCollider,Collider,"Grid-based collider.",
		// those are noSave, since their internal data are not saved anyway (yet?)
		((shared_ptr<GridStore>,gridPrev,,AttrTrait<Attr::noSave>(),"Previous fully populated grid."))
		((shared_ptr<GridStore>,gridCurr,,AttrTrait<Attr::noSave>(),"Current fully populated grid."))
		((shared_ptr<GridStore>,gridOld,,AttrTrait<Attr::noSave>(),"Grid containing entries in :obj:`gridPrev` but not in :obj:`gridCurr`."))
		((shared_ptr<GridStore>,gridNew,,AttrTrait<Attr::noSave>(),"Grid containing entries in :obj:`gridCurr` but not in :obj:`gridPrev`."))
		((Real,verletDist,0,AttrTrait<>().lenUnit(),"Length by which particle size is enalrged, to avoid running the collider at every timestep."))
		((bool,around,false,AttrTrait<Attr::triggerPostLoad>(),"If frue, particle in every cell is checked with particles in all cells around; this makes the grid storage substantially less loaded, as all particles can be shrunk by one half of the cell size."))
		((Real,shrink,0,AttrTrait<Attr::readonly>().noGui(),"The amount of shrinking for each particle (zero if *around* is false, otherwise half of minimum cell size"))
		// grid definition
		((AlignedBox3r,domain,AlignedBox3r(Vector3r::Zero(),Vector3r::Ones()),AttrTrait<Attr::triggerPostLoad>(),"Domain spanned by the grid."))
		((Real,minCellSize,1.,AttrTrait<Attr::triggerPostLoad>(),"Minimum cell size which will be used to compute :obj:`dim`."))
		((Vector3i,dim,Vector3i(-1,-1,-1),AttrTrait<Attr::readonly>(),"Number of cells along each axis"))
		((Vector3r,cellSize,Vector3r(NaN,NaN,NaN),AttrTrait<Attr::readonly>(),"Actual cell size"))
		((shared_ptr<GridBoundDispatcher>,gridBoundDispatcher,,,"Dispatches :obj:`GridBound` creation to :obj:`GridBoundFuctor` based on :obj:`Shape` type."))
		((Vector3r,color,Vector3r(1,1,0),AttrTrait<>().rgbColor().startGroup("Rendering"),"Color for rendering the domain"))
		((bool,renderCells,false,,"Render cells."))
		((shared_ptr<ScalarRange>,occupancyRange,,,"Range for coloring grids based on occupancy (automatically created)"))
		// tunables
		((int,gridDense,4,AttrTrait<>().startGroup("Tunables"),"Length of dense storage for new :obj:`GridStore` objects."))
		((int,exIniSize,4,,":obj:`GridStore.exIniSize` for new grids."))
		((int,exNumMaps,10,,":obj:`GridStore.exNumMaps` for new grids."))
		, /*py*/
	);
};
WOO_REGISTER_OBJECT(GridCollider);

