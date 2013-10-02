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
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void run();

	enum{WHAT_SPHERES=1,WHAT_MESH=2,WHAT_CON=4 /*,WHAT_PELLET=8*/ };
	enum{WHAT_ALL=WHAT_SPHERES|WHAT_MESH|WHAT_CON};

	static int addTriangulatedObject(vector<Vector3r> pts, vector<Vector3i> tri, const vtkSmartPointer<vtkPoints>& vtkPts, const vtkSmartPointer<vtkCellArray>& cells);

	void postLoad(VtkExport&,void*){
		if(what>WHAT_ALL || what<0) throw std::runtime_error("VtkExport.what="+to_string(what)+", but should be at most "+to_string(WHAT_ALL)+".");
	}

	// write the PVD file for easy opening of everything
	void writePvd(const string&);

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(VtkExport,PeriodicEngine,"Export DEM simulation to VTK files for post-processing.",
		((string,out,,
			AttrTrait<>().buttons({"Open PVD in Paraview","from subprocess import call; call(['paraview',self.pvd])",""},/*showBefore*/true),
			"Filename prefix to write into; :obj:`woo.core.Scene.tags` written as {tagName} are expanded at the first run."))
		((bool,compress,true,,"Compress output XML files"))
		((bool,ascii,false,,"Store data as readable text in the XML file (sets `vtkXMLWriter <http://www.vtk.org/doc/nightly/html/classvtkXMLWriter.html>`__ data mode to ``vtkXMLWriter::Ascii``, while the default is ``Appended``"))
		((bool,multiblock,false,,"Write to multi-block VTK files, rather than separate files; currently borken, do not use."))
		((int,mask,0,,"If non-zero, only particles matching the mask will be exported."))
		((int,what,WHAT_ALL,AttrTrait<Attr::triggerPostLoad>(),"Select data to be saved (e.g. VtkExport.spheres|VtkExport.mesh, or use VtkExport.all for everything)"))
		((bool,sphereSphereOnly,false,,"Only export contacts between two spheres (not sphere+facet and such)"))
		((bool,infError,true,,"Raise exception for infinite objects which don't have the glAB attribute set properly"))
		((bool,skipInvisible,true,,"Skip invisible particles"))
		((int,subdiv,16,AttrTrait<>().noGui(),"Subdivision fineness for circular objects (such as cylinders).\n\n.. note::  are rendered without rounded edges (they are closed flat)."))
		((int,thickFacetDiv,1,AttrTrait<>().noGui(),"Subdivision for :obj:`woo.dem.Facet` objects with non-zero :obj:`woo.dem.Facet.halfThick`; the value of -1 will use :obj:`subdiv`; 0 will render only faces, without edges; 1 will close the edge flat."))
		((bool,cylCaps,true,,"Render caps of :obj:`InfCylinder` (at :obj:`InfCylinder.glAB`)."))
		((Real,nanValue,0.,,"Use this number instead of NaN in entries, since VTK cannot read NaNs properly"))
		((vector<vector<string>>,outFiles,,AttrTrait<>().noGui().readonly(),"Files which have been written out."))
		((vector<Real>,outTimes,,AttrTrait<>().noGui().readonly(),"Times at which files were written."))
		((vector<int>,outSteps,,AttrTrait<>().noGui().readonly(),"Steps at which files were written."))
		((string,pvd,,AttrTrait<>().readonly(),"PVD file (if already written)"))

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
			// _classObj.attr("pellet")=(int)VtkExport::WHAT_PELLET;
	);
};
WOO_REGISTER_OBJECT(VtkExport);

#endif /*WOO_VTK*/
