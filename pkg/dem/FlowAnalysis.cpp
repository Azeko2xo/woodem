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
WOO_IMPL_LOGGER(FlowAnalysis);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_FlowAnalysis__CLASS_BASE_DOC_ATTRS_PY);


void FlowAnalysis::reset(){
	data.resize(boost::extents[0][0][0][0][0]);
	timeSpan=0.;
	nDone=0; // this is used in the check below
}

void FlowAnalysis::setupGrid(){
	if(!(cellSize>0)) throw std::runtime_error("FlowAnalysis.cellSize: must be positive.");
	if(!(box.volume()>0)) throw std::runtime_error("FlowAnalysis.box: invalid box (volume not positive).");
	for(int ax:{0,1,2}){
		boxCells[ax]=std::round(box.sizes()[ax]/cellSize);
		box.max()[ax]=box.min()[ax]+boxCells[ax]*cellSize;
	}
	if(!dLim.empty() && !masks.empty()) throw std::runtime_error("FlowAnalysis: only one of dLim and masks may be given, not both.");
	nFractions=1;
	if(!dLim.empty()){
		std::sort(dLim.begin(),dLim.end()); // necessary for lower_bound
		nFractions=dLim.size()+1;
	}
	if(!masks.empty()){
		nFractions=masks.size();
	}
	LOG_WARN("There are "<<nFractions<<" grid(s) "<<boxCells[0]<<"x"<<boxCells[1]<<"x"<<boxCells[2]<<"="<<boxCells.prod()<<" storing "<<NUM_PT_DATA<<" numbers per point (total "<<boxCells.prod()*nFractions*NUM_PT_DATA<<" items)");
	data.resize(boost::extents[nFractions][boxCells[0]][boxCells[1]][boxCells[2]][NUM_PT_DATA]);
	// zero all array items
	std::fill(data.origin(),data.origin()+data.size(),0);
}


void FlowAnalysis::addOneParticle(const Real& diameter, const int& mask, const shared_ptr<Node>& node){
	const auto& dyn(node->getData<DemData>());
	Real V(pow(cellSize,3));
	// all things saved are actually densities over the volume of a single cell
	Vector3r momentum_V(dyn.vel*dyn.mass/V);
	Real Ek_V(dyn.getEk_any(node,/*trans*/true,/*rot*/true,scene)/V);

	size_t fraction=0; // default is the only fraction, which is always present
	// by diameter
	if(!dLim.empty()){
		std::lower_bound(dLim.begin(),dLim.end(),diameter)-dLim.begin();
	}
	// by particle mask
	if(!masks.empty()){
		bool found=false;
		for(size_t i=0; i<masks.size(); i++){
			if(mask&masks[i]){
				if(found){
					LOG_WARN("Particle with mask "<<mask<<" matching both masks["<<fraction<<"]="<<masks[i]<<" and masks["<<i<<"]="<<masks[i]<<"; only first match used.");
				} else {
					found=true; 
					fraction=i;
				}
			}
		}
		if(!found){ LOG_WARN("Particle not matching any mask, ignoring; set FlowAnalysis.mask to filter those out upfront."); return; }
	}
	// loop over neighboring grid points, add relevant data with weights.
	Vector3i ijk=xyz2ijk(node->pos);
	// it does not matter here if ijk are inside the grid;
	// the enlargedBox test in addCurrentData already pruned cases too far away.
	// The rest is checked in the loop below with validIjkRange;
	// that ensure that even points just touching the grid from outside are accounted for correctly.
	Vector3r n=(node->pos-ijk2xyz(ijk))/cellSize; // normalized coordinate in the cube (0..1)x(0..1)x(0..1)
	// trilinear interpolation
	const Real& x(n[0]); const Real& y(n[1]); const Real& z(n[2]);
	Real X(1-x), Y(1-y), Z(1-z);
	const int& i(ijk[0]); const int& j(ijk[1]); const int& k(ijk[2]);
	int I(i+1), J(j+1), K(k+1);
	// traverse all affected points
	Real weights[]={
		X*Y*Z,x*Y*Z,x*y*Z,X*y*Z,
		X*Y*z,x*Y*z,x*y*z,X*y*z
	};
	// the sum should be equal to one
	assert(abs(weights[0]+weights[1]+weights[2]+weights[3]+weights[4]+weights[5]+weights[6]+weights[7]-1) < 1e-5);
	Vector3i pts[]={
		Vector3i(i,j,k),Vector3i(I,j,k),Vector3i(I,J,k),Vector3i(i,J,k),
		Vector3i(i,j,K),Vector3i(I,j,K),Vector3i(I,J,K),Vector3i(i,J,K)
	};
	Eigen::AlignedBox<int,3> validIjkRange(Vector3i::Zero(),Vector3i(data.shape()[1]-1,data.shape()[2]-1,data.shape()[3]-1));
	for(int ii=0; ii<8; ii++){
		// make sure we are within bounds here
		if(!validIjkRange.contains(pts[ii])) continue;
		// subarray where we write the actual data for this point
		auto pt(data[fraction][pts[ii][0]][pts[ii][1]][pts[ii][2]]); 
		const Real& w(weights[ii]);
		pt[PT_FLOW_X]+=w*momentum_V[0];
		pt[PT_FLOW_Y]+=w*momentum_V[1];
		pt[PT_FLOW_Z]+=w*momentum_V[2];
		pt[PT_EK]+=w*Ek_V;
		pt[PT_SUM_WEIGHT]+=w*1.;
		pt[PT_SUM_DIAM]+=w*diameter; // the average is computed later, dividing by PT_SUM_WEIGHT
		pt[PT_SUM_PORO]+=w*(M_PI*pow(diameter,3)/6.)/V;
		assert(!isnan(pt[PT_FLOW_X]) && !isnan(pt[PT_FLOW_Y]) && !isnan(pt[PT_FLOW_Z]) && !isnan(pt[PT_EK]) && !isnan(pt[PT_SUM_WEIGHT]));
	}
};

void FlowAnalysis::addCurrentData(){
	// point which would even just touch the box are interesting for us
	AlignedBox3r enlargedBox(box.min()-Vector3r::Ones()*cellSize,box.max()+Vector3r::Ones()*cellSize);
	for(const auto& p: *(dem->particles)){
		if(mask!=0 && (p->mask&mask)==0) continue;
		//if(!p->shape->isA<Sphere>()) continue;
		Real radius=p->shape->equivRadius();
		if(isnan(radius)) continue;
		if(!enlargedBox.contains(p->shape->nodes[0]->pos)) continue;
		addOneParticle(radius*2.,p->mask,p->shape->nodes[0]);
	}
};

void FlowAnalysis::run(){
	dem=static_cast<DemField*>(field.get());
	if(data.size()==0) setupGrid();
	addCurrentData();
	if(nDone>0) timeSpan+=scene->time-virtPrev;
}

vector<string> FlowAnalysis::vtkExport(const string& out){
	vector<string> ret;
	// loop over fractions, export each of them separately
	if(!dLim.empty()){
		// i indexes the upper limit of the fraction
		for(size_t i=0; i<=dLim.size(); i++){
			string tag=(i==0?string("0"):to_string(dLim[i-1]))+"-"+(i==dLim.size()?string("inf"):to_string(dLim[i]));
			ret.push_back(vtkExportFractions(out+"."+tag,{i}));
		}
	}
	// export over all masks, export those separately
	if(!masks.empty()){
		for(size_t i=0; i<masks.size(); i++){
			string tag("mask="+to_string(masks[i]));
			ret.push_back(vtkExportFractions(out+"."+tag,{i}));
		}
	}
	// export sum of all fractions as well
	vector<size_t> all; for(int i=0; i<nFractions; i++) all.push_back((size_t)i);
	ret.push_back(vtkExportFractions(out+".all",all));
	return ret;
}

string FlowAnalysis::vtkExportFractions(const string& out, /*copy, since we may modify locally*/ vector<size_t> fractions){ 
	if(timeSpan==0.) throw std::runtime_error("FlowAnalysis.timeSpan==0, no data was ever collected.");
	if(nFractions<=0) throw std::runtime_error("FlowAnalysis.nFractions<=0?!!");
	auto grid=vtkMakeGrid();
	auto flow=vtkMakeArray(grid,"flow (momentum density)",3,/*fillZero*/true);
	auto flowNorm=vtkMakeArray(grid,"|flow|",1,true);
	auto ek=vtkMakeArray(grid,"Ek density",1,true);
	auto hitRate=vtkMakeArray(grid,"hit rate",1,true);
	auto diam=vtkMakeArray(grid,"avg. diameter",1,true);
	auto poro=vtkMakeArray(grid,"avg. porosity",1,true);
	// no fractions given, export all of them together
	if(fractions.empty()){ for(size_t i=0; i<(size_t)nFractions; i++) fractions.push_back(i); }

	for(size_t fraction: fractions){
		if((int)fraction>=nFractions) throw std::runtime_error("FlowAnalysis.vtkExportFraction: fraction="+to_string(fraction)+" out of range 0.."+to_string(nFractions-1));
		// traverse the grid now
		for(int i=0; i<boxCells[0]; i++){
			for(int j=0; j<boxCells[1]; j++){
				for(int k=0; k<boxCells[2]; k++){
					int ijk[]={i,j,k};
					vtkIdType dataId;
					if(cellData) dataId=grid->ComputeCellId(ijk);
					else dataId=grid->ComputePointId(ijk);
					const auto ptData(data[fraction][i][j][k]);
					// increment flow vector
					double f[3];
					flow->GetTupleValue(dataId,f);
					f[0]+=ptData[PT_FLOW_X]/timeSpan;
					f[1]+=ptData[PT_FLOW_Y]/timeSpan;
					f[2]+=ptData[PT_FLOW_Z]/timeSpan;
					flow->SetTupleValue(dataId,f);
					// increment scalars
					*(ek->GetPointer(dataId))+=ptData[PT_EK]/timeSpan;
					*(hitRate->GetPointer(dataId))+=ptData[PT_SUM_WEIGHT]/timeSpan;
					// avoid NaNs due to division
					if(ptData[PT_SUM_WEIGHT]!=0.){
						*(diam->GetPointer(dataId))+=ptData[PT_SUM_DIAM]/ptData[PT_SUM_WEIGHT];
						*(poro->GetPointer(dataId))+=ptData[PT_SUM_PORO]/ptData[PT_SUM_WEIGHT];
					}
				}
			}
		}
	}
	// extra loop to fill the |flow| array
	// (cannot be done incrementally above, as sum or norms does not equal norm of sums)
	for(int i=0; i<boxCells[0]; i++){
		for(int j=0; j<boxCells[1]; j++){
			for(int k=0; k<boxCells[2]; k++){
				int ijk[]={i,j,k};
				vtkIdType dataId;
				if(cellData) dataId=grid->ComputeCellId(ijk);
				else dataId=grid->ComputePointId(ijk);
				double f[3];
				flow->GetTupleValue(dataId,f);
				flowNorm->SetValue(dataId,sqrt(f[0]*f[0]+f[1]*f[1]+f[2]*f[2]));
			}
		}
	}
	return vtkWriteGrid(out,grid);
}

vtkSmartPointer<vtkUniformGrid> FlowAnalysis::vtkMakeGrid(){
	auto grid=vtkSmartPointer<vtkUniformGrid>::New();
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
	return grid;
}

template<class vtkArrayType>
vtkSmartPointer<vtkArrayType> FlowAnalysis::vtkMakeArray(const vtkSmartPointer<vtkUniformGrid>& grid, const string& name, size_t numComponents, bool fillZero){
	auto arr=vtkSmartPointer<vtkArrayType>::New();
	arr->SetNumberOfComponents(numComponents);
	arr->SetNumberOfTuples(boxCells.prod());
	arr->SetName(name.c_str());
	if(cellData) grid->GetCellData()->AddArray(arr);
	else grid->GetPointData()->AddArray(arr);
	if(fillZero){ for(int _i=0; _i<(int)numComponents; _i++) arr->FillComponent(_i,0.); }
	return arr;
}

string FlowAnalysis::vtkWriteGrid(const string& out, vtkSmartPointer<vtkUniformGrid>& grid){
	auto compressor=vtkSmartPointer<vtkZLibDataCompressor>::New();
	auto writer=vtkSmartPointer<vtkXMLImageDataWriter>::New();
	string fn=scene->expandTags(out)+".vti";
	writer->SetFileName(fn.c_str());
	writer->SetInput(grid);
	// writer->SetDataModeToAscii();
 	writer->SetCompressor(compressor);
	writer->Write();
	return fn;
}

Real FlowAnalysis::avgFlowNorm(const vector<size_t> &fractions){
	long double ret=0.;
	for(int i=0; i<boxCells[0]; i++){ for(int j=0; j<boxCells[1]; j++){ for(int k=0; k<boxCells[2]; k++){
		for(size_t frac: fractions){
			ret+=sqrt(pow(data[frac][i][j][k][PT_FLOW_X],2)+pow(data[frac][i][j][k][PT_FLOW_Y],2)+pow(data[frac][i][j][k][PT_FLOW_Z],2));
		}
	}}}
	return ret/boxCells.prod();
}

string FlowAnalysis::vtkExportVectorOps(const string& out, const vector<size_t>& fracA, const vector<size_t>& fracB){
	if(timeSpan==0.) throw std::runtime_error("FlowAnalysis.timeSpan==0, no data was ever collected.");
	auto grid=vtkMakeGrid();
	auto cross=vtkMakeArray(grid,"cross",3,/*fillZero*/false);
	auto crossNorm=vtkMakeArray(grid,"|cross|",1,/*fillZero*/false);
	auto diff=vtkMakeArray(grid,"diff",3,/*fillZero*/false);
	auto diffNorm=vtkMakeArray(grid,"|diff|",1,/*fillZero*/false);
	auto diffA=vtkMakeArray(grid,"diffA",3,/*fillZero*/false);
	auto diffANorm=vtkMakeArray(grid,"|diffA|",1,/*fillZero*/false);
	auto diffB=vtkMakeArray(grid,"diffB",3,/*fillZero*/false);
	auto diffBNorm=vtkMakeArray(grid,"|diffB|",1,/*fillZero*/false);
	for(size_t i=0; i<fracA.size(); i++) if((int)fracA[i]>=nFractions) throw std::runtime_error("FlowAnalysis.vtkExportVectorOps: fracA["+to_string(i)+"]="+to_string(fracA[i])+" out of range 0.."+to_string(nFractions-1));
	for(size_t i=0; i<fracB.size(); i++) if((int)fracB[i]>=nFractions) throw std::runtime_error("FlowAnalysis.vtkExportVectorOps: fracB["+to_string(i)+"]="+to_string(fracB[i])+" out of range 0.."+to_string(nFractions-1));

	Real weightB=avgFlowNorm(fracA)/avgFlowNorm(fracB);
	if(isnan(weightB)){
		LOG_WARN("Weighting coefficient is NaN (no flow in fraction B), using 1 instead; the result will be still mathematically correct but useless.")
		weightB=1.;
	}
	Vector3r _cross, _diff, _diffA, _diffB;
	// other conditions: resize sRes
	for(int i=0; i<boxCells[0]; i++){ for(int j=0; j<boxCells[1]; j++){ for(int k=0; k<boxCells[2]; k++){
		int ijk[]={i,j,k};
		Vector3r A(Vector3r::Zero()), B(Vector3r::Zero());
		for(size_t frac: fracA) A+=Vector3r(data[frac][i][j][k][PT_FLOW_X],data[frac][i][j][k][PT_FLOW_Y],data[frac][i][j][k][PT_FLOW_Z]);
		for(size_t frac: fracB) B+=Vector3r(data[frac][i][j][k][PT_FLOW_X],data[frac][i][j][k][PT_FLOW_Y],data[frac][i][j][k][PT_FLOW_Z]);
		// compute the result
		_cross=A.cross(B);
		_diff=A-B*weightB;
		if(_diff.dot(A+B)>=0.){ _diffA=_diff; _diffB=Vector3r::Zero(); }
		else{ _diffA=Vector3r::Zero(); _diffB=-_diff; }

		vtkIdType dataId=grid->ComputePointId(ijk);

		cross->SetTuple(dataId,_cross.data());
		diff->SetTuple(dataId,_diff.data());
		diffA->SetTuple(dataId,_diffA.data());
		diffB->SetTuple(dataId,_diffB.data());

		crossNorm->SetValue(dataId,_cross.norm());
		diffNorm->SetValue(dataId,_diff.norm());
		diffANorm->SetValue(dataId,_diffA.norm());
		diffBNorm->SetValue(dataId,_diffB.norm());
	}}}
	return vtkWriteGrid(out+".ops",grid);
}

#ifdef WOO_OPENGL
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/pkg/gl/Renderer.hpp>
	void FlowAnalysis::render(const GLViewInfo&){
		if(Renderer::fastDraw) GLUtils::AlignedBox(box,color);
		else GLUtils::AlignedBoxWithTicks(box,Vector3r::Constant(cellSize),Vector3r::Constant(cellSize),color);
	}
#endif /* WOO_OPENGL */

#endif /* WOO_VTK */
