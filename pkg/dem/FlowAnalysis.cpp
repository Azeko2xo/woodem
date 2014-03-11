#ifdef WOO_VTK

#include<woo/pkg/dem/FlowAnalysis.hpp>
#include<woo/pkg/dem/Sphere.hpp>

#include<vtkUniformGrid.h>
#include<vtkPoints.h>
#include<vtkDoubleArray.h>
#include<vtkCellData.h>
#include<vtkPointData.h>
#include<vtkSmartPointer.h>
#include<vtkZLibDataCompressor.h>
#include<vtkXMLImageDataWriter.h>

WOO_PLUGIN(dem,(FlowAnalysis));

CREATE_LOGGER(FlowAnalysis);




void FlowAnalysis::setupGrid(){
	if(!(cellSize>0)) throw std::runtime_error("FlowAnalysis.cellSize: must be positive.");
	if(!(box.volume()>0)) throw std::runtime_error("FlowAnalysis.box: invalid box (volume not positive).");
	for(int ax:{0,1,2}){
		boxCells[ax]=std::round(box.sizes()[ax]/cellSize);
		box.max()[ax]=box.min()[ax]+boxCells[ax]*cellSize;
	}
	std::sort(rLim.begin(),rLim.end()); // necessary for lower_bound
	LOG_WARN("There are "<<rLim.size()+1<<" grids "<<boxCells[0]<<"x"<<boxCells[1]<<"x"<<boxCells[2]<<"="<<boxCells.prod()<<" storing "<<NUM_RECS<<" numbers per point (total "<<boxCells.prod()*(rLim.size()+1)*NUM_RECS<<" items)");
	data.resize(boost::extents[rLim.size()+1][boxCells[0]][boxCells[1]][boxCells[2]][NUM_RECS]);
	// zero all array items
	std::fill(data.origin(),data.origin()+data.size(),0);
}


void FlowAnalysis::addOneParticle(const Real& radius, const shared_ptr<Node>& node){
	const auto& dyn(node->getData<DemData>());
	Real V(pow(cellSize,3));
	// all things saved are actually densities over the volume of a single cell
	Vector3r momentum(dyn.vel*dyn.mass/V);
	Real Ek(dyn.getEk_any(node,/*trans*/true,/*rot*/true,scene)/V);

	// determine particle fraction by radius
	// this works also when rLim is empty (returns 0)
	size_t fraction=std::lower_bound(rLim.begin(),rLim.end(),radius)-rLim.begin();
	// loop over neighboring grid points, add relevant data with weights.
	Vector3i ijk=xyz2ijk(node->pos);
	assert(ijk.minCoeff()>=0);
	assert(ijk[0]<=boxCells[0] && ijk[1]<=boxCells[1] && ijk[2]<=boxCells[1]);
	for(int ax:{0,1,2}) if(ijk[ax]==boxCells[ax]) ijk[ax]-=1; // just in case we are right at the upper edge
	// XXX: this should not happen actually?
	if(ijk.minCoeff()<0 || ijk[0]>=boxCells[0]-1 || ijk[1]>=boxCells[1]-1 || ijk[2]>=boxCells[2]-1){
		// LOG_WARN("Cell "<<ijk[0]<<","<<ijk[1]<<","<<ijk[2]<<" to "<<ijk[0]+1<<","<<ijk[1]+1<<","<<ijk[2]+1<<", shape is "<<data.shape()[1]<<","<<data.shape()[2]<<","<<data.shape()[3]);
		return;
	}
	Vector3r n=(node->pos-ijk2xyz(ijk))/cellSize; // normalized coordinate in the cube (0..1)x(0..1)x(0..1)
	const Real& x(n[0]); const Real& y(n[1]); const Real& z(n[2]);
	Real X(1-x), Y(1-y), Z(1-z);
	const int& i(ijk[0]); const int& j(ijk[1]); const int& k(ijk[2]);
	int I(i+1), J(j+1), K(k+1);
	// traverse all affected points
	Real weights[]={
		X*Y*Z,x*Y*Z,x*y*Z,X*y*Z,
		X*Y*z,x*Y*z,x*y*z,X*y*z
	};
	Vector3i pts[]={
		Vector3i(i,j,k),Vector3i(I,j,k),Vector3i(I,J,k),Vector3i(i,J,k),
		Vector3i(i,j,K),Vector3i(I,j,K),Vector3i(I,J,K),Vector3i(i,J,K)
	};
	for(int ii=0; ii<8; ii++){
		assert(fraction<data.shape()[0] && pts[ii][0]<data.shape()[1] && pts[ii][1]<data.shape()[2] && pts[ii][2]<data.shape()[3]);
		auto pt(data[fraction][pts[ii][0]][pts[ii][1]][pts[ii][2]]); // the array where to write our actual data
		const Real& w(weights[ii]);
		pt[REC_FLOW_X]+=w*momentum[0];
		pt[REC_FLOW_Y]+=w*momentum[1];
		pt[REC_FLOW_Z]+=w*momentum[2];
		pt[REC_EK]+=w*Ek;
		pt[REC_COUNT]+=w*1;
		assert(!isnan(pt[REC_FLOW_X]) && !isnan(pt[REC_FLOW_Y]) && !isnan(pt[REC_FLOW_Z]) && !isnan(pt[REC_EK]) && !isnan(pt[REC_COUNT]));
	}
};

void FlowAnalysis::addCurrentData(){
	for(const auto& p: *(dem->particles)){
		if(mask!=0 && (p->mask&mask)==0) continue;
		if(!p->shape->isA<Sphere>()) continue;
		if(!box.contains(p->shape->nodes[0]->pos)) continue;
		addOneParticle(p->shape->cast<Sphere>().radius,p->shape->nodes[0]);
	}
};

void FlowAnalysis::run(){
	dem=static_cast<DemField*>(field.get());
	if(data.size()==0) setupGrid();
	addCurrentData();
	if(nDone>0) timeSpan+=scene->time-virtPrev;
}

void FlowAnalysis::vtkExport(const string& out){
	// loop over fractions, export each of them separately
	if(!rLim.empty()){
		// i indexes the upper limit of the fraction
		for(size_t i=0; i<=rLim.size(); i++){
			string tag=(i==0?string("0"):to_string(rLim[i-1]))+"-"+(i==rLim.size()?string("inf"):to_string(rLim[i]));
			vtkExportFractions(out+"."+tag,{i});
		}
	}
	// export sum of all fractions as well
	vector<size_t> all; for(size_t i=0; i<=rLim.size(); i++) all.push_back(i);
	vtkExportFractions(out+".all",all);
}

void FlowAnalysis::vtkExportFractions(const string& out, const vector<size_t>& fractions){
	auto grid=vtkUniformGrid::New();
	Vector3r cellSize3=Vector3r::Constant(cellSize);
	// if data are in cells, we need extra items along each axes, and shift the origin by half-cell down
	if(cellData) {
		grid->SetDimensions((boxCells+Vector3i::Ones()).eval().data());
		Vector3r origin=(box.min()-.5*cellSize3);
		grid->SetOrigin(origin.data());
	} else {
		grid->SetDimensions(boxCells.data());
		grid->SetOrigin(box.min().data());
	}
	grid->SetSpacing(cellSize,cellSize,cellSize);
	size_t numData=boxCells.prod();

	#define _VTK_ARR_HELPER(arrayType,var,name,numComponents) auto var=vtkSmartPointer<arrayType>::New(); var->SetNumberOfComponents(numComponents); var->SetNumberOfTuples(numData); var->SetName(name); if(cellData) grid->GetCellData()->AddArray(var); else grid->GetPointData()->AddArray(var); /* zero-initialization */ for(int _i=0; _i<numComponents; _i++) var->FillComponent(_i,0.);

	_VTK_ARR_HELPER(vtkDoubleArray,flow,"flow (momentum density)",3)
	_VTK_ARR_HELPER(vtkDoubleArray,flowNorm,"|flow|",1)
	_VTK_ARR_HELPER(vtkDoubleArray,ek,"Ek density",1)
	_VTK_ARR_HELPER(vtkDoubleArray,count,"hit count",1)

	for(size_t fraction: fractions){
		// traverse the grid now
		for(int i=0; i<boxCells[0]; i++){
			for(int j=0; j<boxCells[1]; j++){
				for(int k=0; k<boxCells[2]; k++){
					int ijk[]={i,j,k};
					vtkIdType dataId;
					if(cellData) dataId=grid->ComputeCellId(ijk);
					else dataId=grid->ComputePointId(ijk);
					const auto ptData(data[fraction][i][j][k]);
					double* f0=flow->GetPointer(dataId);
					f0[0]+=ptData[REC_FLOW_X]/timeSpan;
					f0[1]+=ptData[REC_FLOW_Y]/timeSpan;
					f0[2]+=ptData[REC_FLOW_Z]/timeSpan;
					*(flowNorm->GetPointer(dataId))+=Vector3r(ptData[REC_FLOW_X],ptData[REC_FLOW_Y],ptData[REC_FLOW_Z]).norm()/timeSpan;
					*(ek->GetPointer(dataId))+=ptData[REC_EK]/timeSpan;
					*(count->GetPointer(dataId))+=ptData[REC_COUNT]/timeSpan;
				}
			}
		}
	}

	auto compressor=vtkSmartPointer<vtkZLibDataCompressor>::New();
	auto writer=vtkSmartPointer<vtkXMLImageDataWriter>::New();
	string fn=scene->expandTags(out)+".vti";
	writer->SetFileName(fn.c_str());
	writer->SetInput(grid);
	writer->SetDataModeToAscii();
 	writer->SetCompressor(compressor);
	writer->Write();
}

#endif
