#pragma once
#ifdef WOO_VTK

// #include<woo/core/Field.hpp>
// #include<woo/core/Scene.hpp>
// #include<woo/core/Field-templates.hpp>
#include<woo/pkg/dem/Particle.hpp>

#include<boost/math/distributions/normal.hpp>


#include<vtkPointLocator.h>
#include<vtkIdList.h>
#include<vtkUniformGrid.h>
#include<vtkUnstructuredGrid.h>
#include<vtkPoints.h>
#include<vtkPointData.h>
#include<vtkDoubleArray.h>
#include<vtkIntArray.h>
#include<vtkSmartPointer.h>



struct VtkFlowExport: public PeriodicEngine{
	vtkPointLocator* locator;
	vtkPoints* locatorPoints;
	vtkUnstructuredGrid* locatorGrid;

	vtkUniformGrid *dataGrid;
	vtkSmartPointer<vtkDoubleArray> flow, flowNorm, density, ekDensity, radius;
	vtkSmartPointer<vtkIntArray> nNear;
	vector<Particle::id_t> pointParticle;
	vector<Real> pointSpeed;

	void setupGrid();
	void updateWeightFunc();
	Real pointWeight(const Vector3r& relPos);
	void fillGrid();
	void fillOnePoint(const Vector3i& ijk, const Vector3r& P, vtkIdList* ids);
	void writeGrid();

	void run(); 

	DECLARE_LOGGER;

	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }

	// distribution parameters
	boost::math::normal_distribution<Real> distrib;
	Real suppRad, clipQuant, fullSuppVol, suppVol;

	DemField* dem;

	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(VtkFlowExport,PeriodicEngine,"Export DEM simulation to VTK files for post-processing.\n\n.. warning:: This engine is not fully functional, use :obj:`FlowAnalysis` instead. ",
		((string,out,,,"Filename prefix to write into; :obj:`woo.core.Scene.tags` written as {tagName} are expanded the first time run."))
		((string,lastOut,,AttrTrait<>().buttons({"Open in Paraview","from subprocess import call; call(['paraview',self.lastOut])",""},/*showBefore*/true),"Last output file written."))
		((AlignedBox3r,box,,,"Domain where the flow is to be analyzed."))
		((Vector3i,boxCells,,AttrTrait<Attr::readonly>(),"Number of cells in the box (computed automatically)."))
		// ((Vector3r,cellSize,,AttrTrait<Attr::readonly>(),"Size of a cell in the box (computed automatically)."))
		((Real,divSize,NaN,,"Cell size in the box."))
		((Real,stDev,NaN,,":math:`\\sigma^2` of the Gaussian weight function; the search range is :obj:`stDev` × :obj:`relCrop`."))
		((Real,relCrop,3,,"Crop the weight function to zero after this amount (see :obj:`stDev`)."))
		((bool,traces,false,,"Use traces of particles instead of particles themselves."))
		((int,mask,0,,"Particles to consider in the flow analysis (0 to consider everything)."))
		((Vector2r,rRange,Vector2r::Zero(),,"Range for radii of particles to consider, used if rRange[0]<rRange[1]; in that case, non-spherical particles are ignored."))
		((bool,cellData,false,,"Write flow rate as cell data rather than point data."))
		, /*ctor*/ initRun=false; // do not run at the very first step
		, /*py*/ 
	);
};
WOO_REGISTER_OBJECT(VtkFlowExport);


#endif /* WOO_VTK */
