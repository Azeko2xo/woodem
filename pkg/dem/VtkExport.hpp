#pragma once
#ifdef WOO_VTK

#include<woo/core/Engine.hpp>
#include<woo/pkg/dem/Particle.hpp>

#pragma GCC diagnostic push
	// avoid warnings in VTK headers for using sstream
	#pragma GCC diagnostic ignored "-Wdeprecated"
	// work around error in vtkMath.h?
	bool _isnan(double x){ return std::isnan(x); }
	#include<vtkCellArray.h>
	#include<vtkPoints.h>
	#include<vtkPointData.h>
	#include<vtkCellData.h>
	#include<vtkSmartPointer.h>
	#include<vtkFloatArray.h>
	#include<vtkDoubleArray.h>
	#include<vtkIntArray.h>
	#include<vtkUnstructuredGrid.h>
	#include<vtkPolyData.h>
	#include<vtkXMLUnstructuredGridWriter.h>
	#include<vtkXMLPolyDataWriter.h>
	#include<vtkZLibDataCompressor.h>
	#include<vtkTriangle.h>
	#include<vtkLine.h>
	#include<vtkXMLMultiBlockDataWriter.h>
	#include<vtkMultiBlockDataSet.h>
#pragma GCC diagnostic pop

struct VtkExport: public PeriodicEngine{
	WOO_DECL_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void run();

	enum{WHAT_SPHERES=1,WHAT_MESH=2,WHAT_CON=4,WHAT_TRI=8 /*,WHAT_PELLET=8*/ };
	enum{WHAT_ALL=WHAT_SPHERES|WHAT_MESH|WHAT_CON|WHAT_TRI};

	static int addTriangulatedObject(const vector<Vector3r>& pts, const vector<Vector3i>& tri, const vtkSmartPointer<vtkPoints>& vtkPts, const vtkSmartPointer<vtkCellArray>& cells);

	// TODO: convert to boost::range instead of 2 iterators
	int triangulateStrip(const vector<int>::iterator& ABegin, const vector<int>::iterator& AEnd, const vector<int>::iterator& BBegin, const vector<int>::iterator& BEnd, bool close, vector<Vector3i>& tri);
	int triangulateFan(const int& a, const vector<int>::iterator& BBegin, const vector<int>::iterator& BEnd, bool invert, vector<Vector3i>& tri);


	void postLoad(VtkExport&,void*){
		if(what>WHAT_ALL || what<0) throw std::runtime_error("VtkExport.what="+to_string(what)+", but should be at most "+to_string(WHAT_ALL)+".");
	}

	py::dict pyOutFiles() const;

	typedef map<string,vector<string>> map_string_vector_string;

	#define woo_dem_VtkExport__CLASS_BASE_DOC_ATTRS_CTOR_PY \
		VtkExport,PeriodicEngine,ClassTrait().doc("Export DEM simulation to VTK files for post-processing.").section("Export","TODO",{"FlowAnalysis","VtkFlowExport"}), \
		((string,out,,AttrTrait<>().buttons({"Open in Paraview","import woo.paraviewscript\nwith self.scene.paused(): woo.paraviewscript.fromVtkExport(self,launch=True)",""},/*showBefore*/true),"Filename prefix to write into; :obj:`woo.core.Scene.tags` written as {tagName} are expanded at the first run.")) \
		((bool,compress,true,,"Compress output XML files")) \
		((bool,ascii,false,,"Store data as readable text in the XML file (sets `vtkXMLWriter <http://www.vtk.org/doc/nightly/html/classvtkXMLWriter.html>`__ data mode to ``vtkXMLWriter::Ascii``, while the default is ``Appended``")) \
		((bool,multiblock,false,,"Write to multi-block VTK files, rather than separate files; currently borken, do not use.")) \
		((int,mask,0,,"If non-zero, only particles matching the mask will be exported.")) \
		((int,what,WHAT_ALL,AttrTrait<Attr::triggerPostLoad>(),"Select data to be saved (e.g. VtkExport.spheres|VtkExport.mesh, or use VtkExport.all for everything)")) \
		((bool,sphereSphereOnly,false,,"Only export contacts between two spheres (not sphere+facet and such)")) \
		((bool,infError,true,,"Raise exception for infinite objects which don't have the glAB attribute set properly.")) \
		((bool,skipInvisible,true,,"Skip invisible particles")) \
		((int,subdiv,16,AttrTrait<>(),"Subdivision fineness for circular objects (such as cylinders).\n\n.. note:: :obj:`Facets <woo.dem.Facet>` are rendered without rounded edges (they are closed flat).\n\n.. note:: :obj:`Ellipsoids <woo.dem.Ellipsoid>` triangulation is controlled via the :obj:`ellLev` parameter.")) \
		((int,ellLev,0,AttrTrait<>().range(Vector2i(0,CompUtils::unitSphereTri20_maxTesselationLevel)),"Tesselation level for exporting ellipsoids (0 = icosahedron, each level subdivides one triangle into three.")) \
		((int,thickFacetDiv,1,AttrTrait<>(),"Subdivision for :obj:`woo.dem.Facet` objects with non-zero :obj:`woo.dem.Facet.halfThick`; the value of -1 will use :obj:`subdiv`; 0 will render only faces, without edges; 1 will close the edge flat; higher values mean the number of subdivisions.")) \
		((bool,cylCaps,true,,"Render caps of :obj:`InfCylinder` (at :obj:`InfCylinder.glAB`).")) \
		((Real,nanValue,0.,,"Use this number instead of NaN in entries, since VTK cannot read NaNs properly")) \
		((map_string_vector_string,outFiles,,AttrTrait<>().noGui().readonly(),"Files which have been written out, keyed by what they contain: 'spheres','mesh','con'.")) \
		((vector<Real>,outTimes,,AttrTrait<>().noGui().readonly(),"Times at which files were written.")) \
		((vector<int>,outSteps,,AttrTrait<>().noGui().readonly(),"Steps at which files were written.")) \
		,/*ctor*/ initRun=false; /* do not run at the very first step */ \
		,/*py*/ \
			/* this overrides the c++ map above which won't convert to python automatically */ \
			.add_property("outFiles",&VtkExport::pyOutFiles)  \
			; \
			/* casting to (int) necessary, since otherwise it is a special enum type which is not registered in python and we get error: "TypeError: No to_python (by-value) converter found for C++ type: VtkExport::$_2" at boot. */ \
			_classObj.attr("spheres")=(int)VtkExport::WHAT_SPHERES; \
			_classObj.attr("mesh")=(int)VtkExport::WHAT_MESH; \
			_classObj.attr("tri")=(int)VtkExport::WHAT_TRI; \
			_classObj.attr("con")=(int)VtkExport::WHAT_CON; \
			_classObj.attr("all")=(int)VtkExport::WHAT_ALL; \
			/* _classObj.attr("pellet")=(int)VtkExport::WHAT_PELLET; */

	WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_VtkExport__CLASS_BASE_DOC_ATTRS_CTOR_PY);
};
WOO_REGISTER_OBJECT(VtkExport);

#endif /*WOO_VTK*/
