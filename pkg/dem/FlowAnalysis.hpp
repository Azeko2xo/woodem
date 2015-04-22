#pragma once
#ifdef WOO_VTK

#include<woo/pkg/dem/Particle.hpp>
#include<vtkUniformGrid.h>
#include<vtkDoubleArray.h>
#include<vtkSmartPointer.h>



struct FlowAnalysis: public PeriodicEngine{
	WOO_DECL_LOGGER;
	bool acceptsField(Field* f) WOO_CXX11_OVERRIDE { return dynamic_cast<DemField*>(f); }
	DemField* dem; // temp only
	typedef boost::multi_array<Real,5> boost_multi_array_real_5;
	typedef boost::multi_array<Vector3r,3> boost_multi_array_Vector3_3;
	typedef boost::multi_array<Real,3> boost_multi_array_real_3;

	// return grid point lower or equal to this spatial point
	inline Vector3i xyz2ijk(const Vector3r& xyz) const { /* cast rounds down properly */ return ((xyz-box.min())/cellSize).cast<int>(); }
	// return lower grid point for this cell coordinate
	inline Vector3r ijk2xyz(const Vector3i& ijk) const { return box.min()+ijk.cast<Real>()*cellSize; }

	void setupGrid();
	void addOneParticle(const Real& radius, const int& mask, const shared_ptr<Node>& node);
	void addCurrentData();
	Real avgFlowNorm(const vector<size_t> &fractions);


	string vtkExportFractions(const string& out, /*modifiable copy*/ vector<size_t> fractions);
	vector<string> vtkExport(const string& out);
	string vtkWriteGrid(const string& out, vtkSmartPointer<vtkUniformGrid>& grid);

	vtkSmartPointer<vtkUniformGrid> vtkMakeGrid();

	template<class vtkArrayType=vtkDoubleArray>
	vtkSmartPointer<vtkArrayType> vtkMakeArray(const vtkSmartPointer<vtkUniformGrid>& grid, const string& name, size_t numComponents, bool fillZero=true);

	string vtkExportVectorOps(const string& out, const vector<size_t>& fracA, const vector<size_t>& fracB);




	void run() WOO_CXX11_OVERRIDE;
	void reset();

	#ifdef WOO_OPENGL
		void render(const GLViewInfo&) WOO_CXX11_OVERRIDE;
	#endif

	// number of floats to store for each point
	enum {PT_FLOW_X=0, PT_FLOW_Y, PT_FLOW_Z, PT_EK, PT_SUM_WEIGHT, PT_SUM_DIAM, PT_SUM_PORO, NUM_PT_DATA};
	enum {OP_CROSS=0,OP_WEIGHTED_DIFF=1};

	#define woo_dem_FlowAnalysis__CLASS_BASE_DOC_ATTRS_PY \
		FlowAnalysis,PeriodicEngine,"Collect particle flow data in rectangular grid, watching different particle groups (radius ranges via :obj:`dLim` or groups by mask via :obj:`masks` -- only one of them may be specified), and saving averages to as VTK uniform grid once finished.\n\n.. note:: Only particles returning meaningful :obj:`woo.dem.Shape.equivRad` are considered, all other are ignored.", \
		((AlignedBox3r,box,,AttrTrait<>().buttons({"Open in Paraview","import woo.paraviewscript\nwith self.scene.paused(): woo.paraviewscript.fromFlowAnalysis(self,launch=True)",""},/*showBefore*/true),"Domain in which the flow is to be analyzed; the box may glow slightly to accomodate integer number of cells. Do not change once there is some data alread. Do not change once there is some data already.")) \
		((boost_multi_array_real_5,data,boost_multi_array_real_5(boost::extents[0][0][0][0][0]),AttrTrait<Attr::hidden>(),"Grid data -- 5d since each 3d point contains multiple entries, and there are multiple grids.")) \
		((vector<Real>,dLim,,,"Limiting diameter values, for defining fractions which are analyzed separately. Do not change when there is some data already.")) \
		((vector<int>,masks,,,"Mask values for fractions; it is an error if a particle matches multiple masks. Do not change when there is some data already.")) \
		((int,nFractions,-1,AttrTrait<Attr::readonly>(),"Number of fractions, defined via :obj:`dLim` or :obj:`masks`; set automatically.")) \
		((Real,cellSize,NaN,,"Size of one cell in the box (in all directions); will be satisfied exactly at the expense of perhaps slightly growing :obj:`box`. Do not change once there is some data already.")) \
		((Vector3i,boxCells,Vector3i::Zero(),AttrTrait<Attr::readonly>(),"Number of cells in the box (computed automatically)")) \
		((int,mask,0,,"Particles to consider in the flow analysis (0 to consider everything).")) \
		((bool,cellData,false,,"Write flow rate as cell data rather than point data.")) \
		((Real,timeSpan,0.,,"Total time that the analysis has been running.")) \
		((Vector3r,color,Vector3r(1,1,0),AttrTrait<>().rgbColor(),"Color for rendering the domain")) \
		, /*py*/ \
			.def("vtkExport",&FlowAnalysis::vtkExport,(py::arg("out")),"Export all fractions separately, and also an overall flow (sum). *out* specifies prefix for all export filenames, the rest is created to describe the fraction or ``all`` for the sum. Exported file names are returned, the sum being at the very end. Internally calls :obj:`vtkExportFractions` for all fractions.") \
			.def("vtkExportFractions",&FlowAnalysis::vtkExportFractions,(py::arg("out"),py::arg("fractions")),"Export one single fraction to file named *out*. The extension ``.vti`` is added automatically. *fractions* specifies existing fraction numbers to export. If *fractions* are an empty list (``[]``), all fractions are exported at once.") \
			.def("vtkExportVectorOps",&FlowAnalysis::vtkExportVectorOps,(py::arg("out"),py::arg("fracA"),py::arg("fracB")),"Export operations on two fraction sets: cross-product, weighted differences and such, for segregation analysis. Appends ``.opt.vti`` to the output filename in *out*.") \
			.def("reset",&FlowAnalysis::reset,"Reset all data so that next analysis will be run from virgin state.")

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_FlowAnalysis__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(FlowAnalysis);

#endif /* WOO_VTK */
