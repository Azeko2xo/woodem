#pragma once
#ifdef WOO_VTK

#include<woo/pkg/dem/Particle.hpp>

struct FlowAnalysis: public PeriodicEngine{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	DemField* dem; // temp only
	typedef boost::multi_array<Real,5> boost_multi_array_real_5;

	// return grid point lower or equal to this spatial point
	inline Vector3i xyz2ijk(const Vector3r& xyz) const { /* cast rounds down properly */ return ((xyz-box.min())/cellSize).cast<int>(); }
	// return lower grid point for this cell coordinate
	inline Vector3r ijk2xyz(const Vector3i& ijk) const { return box.min()+ijk.cast<Real>()*cellSize; }

	void setupGrid();
	void addOneParticle(const Real& radius, const shared_ptr<Node>& node);
	void addCurrentData();

	void vtkExportFractions(const string& out, const vector<size_t>& fractions);
	void vtkExport(const string& out);

	void run();

	// number of floats to store for each point
	enum {REC_FLOW_X=0, REC_FLOW_Y, REC_FLOW_Z, REC_EK, REC_COUNT, NUM_RECS};
	WOO_CLASS_BASE_DOC_ATTRS_PY(FlowAnalysis,PeriodicEngine,"Collect particle flow data in rectangular grid, watching different particle groups (radius ranges at this moment), and saving averages to as VTK uniform grid once finished.\n\n.. todo:: Only spherical particles are handled now, any other are ignored.",
		((boost_multi_array_real_5,data,boost_multi_array_real_5(boost::extents[0][0][0][0][0]),AttrTrait<Attr::hidden>(),"Grid data -- 5d since each 3d point contains multiple entries, and there are multiple grids."))
		((vector<Real>,rLim,,,"Limiting radius values, for defining fractions which are analyzed separately. Do not change when there is some data already."))
		((AlignedBox3r,box,,,"Domain in which the flow is to be analyzed; the box may glow slightly to accomodate integer number of cells. Do not change once there is some data alread. Do not change once there is some data already."))
		((Real,cellSize,NaN,,"Size of one cell in the box (in all directions); will be satisfied exactly at the expense of perhaps slightly growing :obj:`box`. Do not change once there is some data already."))
		((Vector3i,boxCells,,AttrTrait<Attr::readonly>(),"Number of cells in the box (computed automatically)"))
		((int,mask,0,,"Particles to consider in the flow analysis (0 to consider everything)."))
		((bool,cellData,false,,"Write flow rate as cell data rather than point data."))
		((Real,timeSpan,0.,,"Total time that the analysis has been running."))
		, /*py*/
			.def("vtkExport",&FlowAnalysis::vtkExport,(py::arg("out")))
			.def("vtkExportFractions",&FlowAnalysis::vtkExportFractions,(py::arg("out"),py::arg("fractions")))
	);
};
WOO_REGISTER_OBJECT(FlowAnalysis);

#endif /* WOO_VTK */
