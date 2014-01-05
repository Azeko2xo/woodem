#ifdef WOO_VTK
#include<woo/pkg/dem/VtkFlowExport.hpp>
#include<woo/pkg/dem/Tracer.hpp>

#include<vtkZLibDataCompressor.h>
#include<vtkXMLImageDataWriter.h>
#include<vtkPointData.h>
#include<vtkCellData.h>




WOO_PLUGIN(dem,(VtkFlowExport));
CREATE_LOGGER(VtkFlowExport);

void VtkFlowExport::setupGrid(){
	if(!(box.volume()>0)) throw std::runtime_error("VtkFlowExport.box: invalid box (volume not positive).");
	for(int ax:{0,1,2}){
		boxCells[ax]=std::round(box.sizes()[ax]/divSize);
		box.max()[ax]=box.min()[ax]+boxCells[ax]*divSize;
	}
	dataGrid=vtkUniformGrid::New();
	dataGrid->SetDimensions((boxCells+Vector3i::Ones()).eval().data()); // number of edge points
	dataGrid->SetOrigin(box.min().data());
	dataGrid->SetSpacing(divSize,divSize,divSize);

	size_t numData=cellData?boxCells.prod():(boxCells+Vector3i::Ones()).prod();

	#define _VTK_ARR_HELPER(arrayType,var,name,numComponents) var=vtkSmartPointer<arrayType>::New(); var->SetNumberOfComponents(numComponents); var->SetNumberOfTuples(numData); var->SetName(name); if(cellData) dataGrid->GetCellData()->AddArray(var); else dataGrid->GetPointData()->AddArray(var);

	_VTK_ARR_HELPER(vtkDoubleArray,flow,"flow (momentum density)",3)
	_VTK_ARR_HELPER(vtkDoubleArray,flowNorm,"|flow|",1)
	_VTK_ARR_HELPER(vtkDoubleArray,density,"density",1)
	_VTK_ARR_HELPER(vtkDoubleArray,ekDensity,"Ek density",1)
	_VTK_ARR_HELPER(vtkIntArray,nNear,"num near",1)

	locatorPoints=vtkPoints::New();
	pointParticle.clear();
	pointSpeed.clear(); // only used with traces, to avoid looking up trace points later
	// add particles points - current and also traces, if desired
	// keep track of what belongw to which particle via the pointParticles array
	for(const auto& p: *(dem->particles)){
		if(mask!=0 && (p->mask&mask)==0) continue;
		if(p->shape->nodes.size()!=1) continue;
		// if(!p->shape->nodes[0]->hasData<DemData>()) continue;
		if(!traces){ // one point per particle
			const Vector3r& pos=p->shape->nodes[0]->pos;
			__attribute__((unused)) vtkIdType newId=locatorPoints->InsertNextPoint(pos.data());
			pointParticle.push_back(p->id);
			assert(newId==pointParticle.size()-1);
		} else {
			// make sure that velocity is the scalar -- otherwise we won't be able to get any meaningful data
			if(Tracer::scalar!=Tracer::SCALAR_VEL) throw std::runtime_error("VtkFlowExport: when analyzing traces, the woo.dem.Tracer.scalar must be velocity (woo.dem.Tracer.scalarVel).");
			const auto& node(p->shape->nodes[0]);
			if(!node->rep || !dynamic_pointer_cast<TraceGlRep>(node->rep)) continue; // no trace data for this particle
			const auto& trace(node->rep->cast<TraceGlRep>());
			Vector3r pt; Real scalar;
			// getPointData returns false when there is nothing left
			for(int i=0; trace.getPointData(i,pt,scalar); i++){
				__attribute__((unused)) vtkIdType newId=locatorPoints->InsertNextPoint(pt.data());
				pointParticle.push_back(p->id);
				pointSpeed.push_back(scalar);
				assert(newId==pointParticle.size()-1);
				assert(pointParticle.size()==pointSpeed.size());
			}
		}
	}
	locatorGrid=vtkUnstructuredGrid::New();
	locatorGrid->SetPoints(locatorPoints);
	locator=vtkPointLocator::New();
	locator->SetDataSet(locatorGrid);
	locator->BuildLocator();
}

void VtkFlowExport::updateWeightFunc(){
	suppRad=relCrop*stDev;
	// this is perhaps bogus as we used 1d Gaussian in 3d...?!
	Real clippedQuant=boost::math::cdf(distrib,-suppRad);
	Real fullSuppVol=(4/3.)*M_PI*pow(suppRad,3); // total volume of the support
	suppVol=(1-2*clippedQuant)*fullSuppVol;
	// cross-area of the support (for flow analysis)
	// suppArea=(1-2*clippedQuant)*M_PI*pow(suppRad,2);
}

Real VtkFlowExport::pointWeight(const Vector3r& relPos){
	Real rSq=relPos.squaredNorm();
	if(rSq>suppRad*suppRad) return 0.; // discard points too far away
	return boost::math::pdf(distrib,sqrt(rSq));
}

void VtkFlowExport::fillGrid(){
	// traverse the grid
	#ifdef WOO_OPENMP
		#pragma omp parallel for
	#endif
	for(int i=0; i<boxCells[0]+(cellData?0:1); i++){
		for(int j=0; j<boxCells[1]+(cellData?0:1); j++){
			for(int k=0; k<boxCells[2]+(cellData?0:1); k++){
				Vector3i ijk(i,j,k);
				// cell mid-point or grid point
				Vector3r P=box.min()+divSize*(ijk.cast<Real>()+(cellData?.5:0.)*Vector3r::Ones());
				vtkIdList* ids=vtkIdList::New();
				locator->FindPointsWithinRadius(suppRad,P.data(),ids);
				fillOnePoint(ijk,P,ids);
			}
		}
	}
}

void VtkFlowExport::fillOnePoint(const Vector3i& ijk, const Vector3r& P, vtkIdList* ids){
	int ijkArr[]={ijk[0],ijk[1],ijk[2]}; // vtk needs non-const pointer, make it here
	vtkIdType dataId;
	if(cellData) dataId=dataGrid->ComputeCellId(ijkArr);
	else dataId=dataGrid->ComputePointId(ijkArr);
	int numIds=ids->GetNumberOfIds();
	nNear->SetValue(dataId,numIds);
	Vector3r mom; Real ekDens, dens;
	// one neighbor does not count (makes fuzzy domain boundaries, not good)
	if(numIds<1) goto blank_cell;

	// compute the velocity average
	if(traces){ // each point is one particle
		throw std::runtime_error("VtkFlowExport.traces: not yet implemented.");
	} else {
		// FIXME: dimensionality??
		// kg*(m/s)/m³=kg/m² but is that right?
		Real weightSum=0.; Vector3r velWSum=Vector3r::Zero();
		Real massSum=0., ekSum=0.;
		Vector3r relPosPrev;
		// bool allSameDir=true;
		for(int i=0; i<numIds; i++){
			const auto& id=ids->GetId(i);
			const auto parId=pointParticle[id];
			const auto& p=(*(dem->particles))[parId];
			const auto& dyn=p->shape->nodes[0]->getData<DemData>();
			Vector3r relPos=p->shape->nodes[0]->pos-P;
			Real weight=pointWeight(relPos);
			weightSum+=weight;
			velWSum+=weight*dyn.vel*dyn.mass;
			massSum+=weight*dyn.mass;
			ekSum+=weight*.5*dyn.vel.squaredNorm()*dyn.mass;
			// if dot-product of coors is always positive, all points are on one side from the central point
			// in that case, we discard that one also, as it is beyond the really active area anyway
			//if(relPos.dot(relPosPrev)<0) allSameDir=false;
			if(i>0) relPosPrev=relPos;
		}
		if(weightSum==0.) goto blank_cell; // perhaps just at the edge; avoid NaNs which Paraview chokes on
		// if(allSameDir) goto blank_cell;
		mom=velWSum/weightSum/suppVol;
		ekDens=ekSum/weightSum/suppVol;
		dens=massSum/weightSum/suppVol;
	}

	// write flow vector to the grid
	flow->SetTuple3(dataId,mom[0],mom[1],mom[2]);
	flowNorm->SetValue(dataId,mom.norm());
	density->SetValue(dataId,dens);
	ekDensity->SetValue(dataId,ekDens);
	return;

	// invalid cell data (too little points, or unsuitable arrangement)
	blank_cell:
		flow->SetTuple3(dataId,0.,0.,0.);
		flowNorm->SetValue(dataId,0.);
		if(cellData) dataGrid->BlankCell(dataId);
		else dataGrid->BlankPoint(dataId);
		return;
}

void VtkFlowExport::writeGrid(){
	auto compressor=vtkSmartPointer<vtkZLibDataCompressor>::New();
	auto writer=vtkSmartPointer<vtkXMLImageDataWriter>::New();
	string fn=out+to_string(scene->step)+".vti";
	writer->SetFileName(fn.c_str());
	writer->SetInput(dataGrid);
	// writer->SetDataModeToAscii();
 	writer->SetCompressor(compressor);
	writer->Write();
	lastOut=fn;
}


void VtkFlowExport::run(){
	dem=static_cast<DemField*>(field.get());
	out=scene->expandTags(out);

	if(!(stDev>0)) throw std::runtime_error("VtkFlowExport: stDev must be positive (not "+to_string(stDev)+")");
	if(!(relCrop>0)) throw std::runtime_error("VtkFlowExport: relCrop must be positive (not "+to_string(relCrop)+")");
	if(!(divSize>0)) throw std::runtime_error("VtkFlowExport: divSize must be positive (not "+to_string(divSize)+")");

	setupGrid();
	updateWeightFunc();
	fillGrid();
	writeGrid();
}

#endif /* WOO_VTK */
