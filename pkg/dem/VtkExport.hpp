#pragma once
#ifdef WOO_VTK

#include<woo/core/Engine.hpp>
#include<woo/pkg/dem/Particle.hpp>

#pragma GCC diagnostic push
	// avoid warnings in VTK headers for using sstream
	#pragma GCC diagnostic ignored "-Wdeprecated"
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
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void run();

	enum{WHAT_SPHERES=1,WHAT_MESH=2,WHAT_CON=4};
	enum{WHAT_ALL=WHAT_SPHERES|WHAT_MESH|WHAT_CON};

	static int addTriangulatedObject(vector<Vector3r> pts, vector<Vector3i> tri, const vtkSmartPointer<vtkPoints>& vtkPts, const vtkSmartPointer<vtkCellArray>& cells);

	void postLoad(VtkExport&){
		if(what>WHAT_ALL || what<0) throw std::runtime_error("VtkExport.what="+to_string(what)+", but should be at most "+to_string(WHAT_ALL)+".");
	}

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(VtkExport,PeriodicEngine,"Export DEM simulation to VTK files for post-processing.",
		((string,out,,,"Filename prefix to write into"))
		((bool,compress,true,,"Compress output XML files"))
		((bool,ascii,false,,"Store data as readable text in the XML file (sets `vtkXMLWriter <http://www.vtk.org/doc/nightly/html/classvtkXMLWriter.html>`__ data mode to ``vtkXMLWriter::Ascii``, while the default is ``Appended``"))
		((bool,multiblock,false,,"Write to multi-block VTK files, rather than separate files; currently borken, do not use."))
		((int,mask,0,,"If non-zero, only particles matching the mask will be exported."))
		((int,what,WHAT_ALL,AttrTrait<>().triggerPostLoad(),"Select data to be saved (e.g. VtkExport.spheres|VtkExport.mesh, or use VtkExport.all for everything)"))
		((bool,sphereSphereOnly,false,,"Only export contacts between two spheres (not sphere+facet and such)"))
		((int,subdiv,16,AttrTrait<>().noGui(),"Subdivision fineness for circular objects (such as cylinders)."))
		((bool,infError,true,,"Raise exception for infinite objects which don't have the glAB attribute set properly"))
		((bool,skipInvisible,true,,"Skip invisible particles"))
		,/*ctor*/
		initRun=false; // do not run at the very first step
		,/*py*/
			;
			// casting to (int) necessary, since otherwise it is a special enum type
			// which is not registered in python and we get error
			//
			//    TypeError: No to_python (by-value) converter found for C++ type: VtkExport::$_2
			//
			// at boot.
			_classObj.attr("spheres")=(int)VtkExport::WHAT_SPHERES;
			_classObj.attr("mesh")=(int)VtkExport::WHAT_MESH;
			_classObj.attr("con")=(int)VtkExport::WHAT_CON;
			_classObj.attr("all")=(int)VtkExport::WHAT_ALL;
	);
};
REGISTER_SERIALIZABLE(VtkExport);

#endif /*WOO_VTK*/
