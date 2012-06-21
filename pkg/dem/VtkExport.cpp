#ifdef WOO_VTK
#include<woo/pkg/dem/VtkExport.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Facet.hpp>
#include<woo/pkg/dem/InfCylinder.hpp>
#include<woo/pkg/dem/Wall.hpp>
#include<woo/pkg/dem/Clump.hpp>

WOO_PLUGIN(dem,(VtkExport));
CREATE_LOGGER(VtkExport);

int VtkExport::addTriangulatedObject(vector<Vector3r> pts, vector<Vector3i> tri, const vtkSmartPointer<vtkPoints>& vtkPts, const vtkSmartPointer<vtkCellArray>& cells){
	size_t id0=vtkPts->GetNumberOfPoints();
	for(const auto& pt: pts) vtkPts->InsertNextPoint(pt.data());
	for(size_t i=0; i<tri.size(); i++){
		auto vtkTri=vtkSmartPointer<vtkTriangle>::New();
		for(int j:{0,1,2}){
			vtkTri->GetPointIds()->SetId(j,id0+tri[i][j]);
		}
		cells->InsertNextCell(vtkTri);
	}
	return tri.size();
};


void VtkExport::run(){
	DemField* dem=static_cast<DemField*>(field.get());

	#define _VTK_ARR_HELPER(var,name,numComponents,arrayType) auto var=vtkSmartPointer<arrayType>::New(); var->SetNumberOfComponents(numComponents); var->SetName(name); 
	#define _VTK_POINT_ARR(grid,var,name,numComponents) _VTK_ARR_HELPER(var,name,numComponents,vtkDoubleArray); grid->GetPointData()->AddArray(var);
	#define _VTK_POINT_INT_ARR(grid,var,name,numComponents) _VTK_ARR_HELPER(var,name,numComponents,vtkIntArray); grid->GetPointData()->AddArray(var);
	#define _VTK_CELL_ARR(grid,var,name,numComponents) _VTK_ARR_HELPER(var,name,numComponents,vtkDoubleArray); grid->GetCellData()->AddArray(var);
	#define _VTK_CELL_INT_ARR(grid,var,name,numComponents) _VTK_ARR_HELPER(var,name,numComponents,vtkIntArray); grid->GetCellData()->AddArray(var);

	// contacts
	auto cPoly=vtkSmartPointer<vtkPolyData>::New();
	auto cParPos=vtkSmartPointer<vtkPoints>::New();
	auto cCells=vtkSmartPointer<vtkCellArray>::New();
	cPoly->SetPoints(cParPos);
	cPoly->SetLines(cCells);
	_VTK_CELL_ARR(cPoly,cFn,"Fn",1);
	_VTK_CELL_ARR(cPoly,cMagFt,"|Ft|",1);

	if(what&WHAT_CON){
		// holds information about cell distance between spatial and displayed position of each particle
		vector<Vector3i> wrapCellDist;
		if (scene->isPeriodic){ wrapCellDist.resize(dem->particles->size()); }
		// save particle positions, referenced by ids of vtkLine
		for(size_t id=0; id<dem->particles->size(); id++){
			const shared_ptr<Particle>& p=(*dem->particles)[id];
			if (!p) {
				// must keep ids contiguous, so that position in the array corresponds to Particle::id
				// this value is never referenced
				cParPos->InsertNextPoint(0,0,0); // do not use NaN, vtk does not read those properly
				continue;
			}
			Vector3r pos(p->shape->nodes[0]->pos);
			if(!scene->isPeriodic) cParPos->InsertNextPoint(pos[0],pos[1],pos[2]);
			else {
				pos=scene->cell->canonicalizePt(pos,wrapCellDist[id]);
				cParPos->InsertNextPoint(pos[0],pos[1],pos[2]);
			}
			assert(cParPos->GetNumberOfPoints()==p->id+1);
		}
		FOREACH(const auto& C, *dem->contacts){
			if(mask && (!(mask&C->pA->mask) || !(mask&C->pB->mask))) continue;
			Particle::id_t ids[2]={C->pA->id,C->pB->id};
			bool isSphere[]={dynamic_pointer_cast<Sphere>(C->pA->shape),dynamic_pointer_cast<Sphere>(C->pB->shape)};
			if(sphereSphereOnly && (!isSphere[0] || !isSphere[1])) continue;
			/* For the periodic boundary conditions,
				find out whether the interaction crosses the boundary of the periodic cell;
				if it does, display the interaction on both sides of the cell, with one of the
				points sticking out in each case.
				Since vtkLines must connect points with an ID assigned, we will create a new additional
				point for each point outside the cell. It might create some data redundancy, but
				let us suppose that the number of interactions crossing the cell boundary is low compared
				to total numer of interactions
			*/
			// aperiodic boundary, or interaction is inside the cell
			vector<pair<long,long>> linePtIds; // added as vtkLine later
			if(!scene->isPeriodic || (scene->isPeriodic && (C->cellDist==wrapCellDist[ids[1]]-wrapCellDist[ids[0]]))){
				// if not sphere, use contact point as the endpoint instead of sphere's center
				// since the contact might not point to the particle's node position
				linePtIds.push_back({
					isSphere[0]?ids[0]:cParPos->InsertNextPoint(C->geom->node->pos.data()),
					isSphere[1]?ids[1]:cParPos->InsertNextPoint(C->geom->node->pos.data())
				});
			} else {
				assert(scene->isPeriodic);
				// create two line objects; each of them has one endpoint inside the cell and the other one sticks outside
				// A,B are the "fake" particles outside the cell for pA and pB respectively, p1,p2 are the displayed points
				// distance in cell units for shifting A away from p1; negated value is shift of B away from p2
				for(int copy:{0,1}){ // copy contact to both sides of the periodic cell
					// copy 0: particle A and fake (shifted) particle B, copy 1 the other way around
					const Vector3r& p0=(copy==0?C->pA->shape->nodes[0]->pos:C->pB->shape->nodes[0]->pos);
					auto idOther=ids[(copy+1)%2];
					Vector3r ptFake(p0+scene->cell->hSize*(wrapCellDist[idOther]-C->cellDist).cast<Real>());
					vtkIdType idFake=cParPos->InsertNextPoint(ptFake.data());
					if(isSphere[copy]) linePtIds.push_back({ids[copy],idFake});
					else{
						linePtIds.push_back({cParPos->InsertNextPoint(Vector3r(C->geom->node->pos+scene->cell->hSize*(wrapCellDist[ids[copy]]-C->cellDist).cast<Real>()).data()),idFake});
					}
				}
			}
			// insert contact as many times as it was created
			for(const auto& ids: linePtIds){
				cFn->InsertNextValue(C->phys->force[0]);
				cMagFt->InsertNextValue(C->phys->force.tail<2>().norm());
				auto line=vtkSmartPointer<vtkLine>::New();
				line->GetPointIds()->SetId(0,ids.first);
				line->GetPointIds()->SetId(1,ids.second);
				cCells->InsertNextCell(line);
			}
		}
	}

	// spheres
	auto sGrid=vtkSmartPointer<vtkUnstructuredGrid>::New();
	auto sPos=vtkSmartPointer<vtkPoints>::New();
	auto sCells=vtkSmartPointer<vtkCellArray>::New();
	sGrid->SetPoints(sPos);
	_VTK_POINT_ARR(sGrid,sRadii,"radius",1);
	_VTK_POINT_ARR(sGrid,sMass,"mass",1)
	_VTK_POINT_INT_ARR(sGrid,sId,"id",1)
	_VTK_POINT_INT_ARR(sGrid,sMask,"mask",1)
	_VTK_POINT_INT_ARR(sGrid,sClumpId,"clumpId",1)
	_VTK_POINT_ARR(sGrid,sColor,"color",1)
	_VTK_POINT_ARR(sGrid,sVel,"vel",3)
	_VTK_POINT_ARR(sGrid,sAngVel,"angVel",3)
	_VTK_POINT_INT_ARR(sGrid,sMatId,"matId",1)
	// meshes
	auto mGrid=vtkSmartPointer<vtkUnstructuredGrid>::New();
	auto mPos=vtkSmartPointer<vtkPoints>::New();
	auto mCells=vtkSmartPointer<vtkCellArray>::New();
	mGrid->SetPoints(mPos);
	_VTK_CELL_ARR(mGrid,mColor,"color",1);
	_VTK_CELL_INT_ARR(mGrid,mMatId,"matId",1);
	_VTK_CELL_ARR(mGrid,mVel,"vel",3);
	_VTK_CELL_ARR(mGrid,mAngVel,"angVel",3);

	FOREACH(const auto& p, *dem->particles){
		if(!p->shape) continue; // this should not happen really
		if(mask && !(mask&p->mask)) continue;
		if(!p->shape->getVisible() && skipInvisible) continue;
		const auto sphere=dynamic_cast<Sphere*>(p->shape.get());
		const auto wall=dynamic_cast<Wall*>(p->shape.get());
		const auto facet=dynamic_cast<Facet*>(p->shape.get());
		const auto infCyl=dynamic_cast<InfCylinder*>(p->shape.get());
		if(sphere){
			Vector3r pos=p->shape->nodes[0]->pos;
			if(scene->isPeriodic) pos=scene->cell->canonicalizePt(pos);
			vtkIdType posId[1]={sPos->InsertNextPoint(pos.data())};
			sCells->InsertNextCell(1,posId);
			sRadii->InsertNextValue(sphere->radius);
			const auto& dyn=sphere->nodes[0]->getData<DemData>();
			sMass->InsertNextValue(dyn.mass);
			sId->InsertNextValue(p->id);
			sMask->InsertNextValue(p->mask);
			sClumpId->InsertNextValue(dyn.isClumped()?sphere->nodes[0]->getData<DemData>().cast<ClumpData>().clumpLinIx:-1);
			sColor->InsertNextValue(sphere->color);
			sVel->InsertNextTupleValue(dyn.vel.data());
			sAngVel->InsertNextTupleValue(dyn.angVel.data());
			sMatId->InsertNextValue(p->material->id);
			continue;
		}
		// no more spheres, just meshes now
		int nCells=0;
		if(facet){
			nCells=addTriangulatedObject({p->shape->nodes[0]->pos,p->shape->nodes[1]->pos,p->shape->nodes[2]->pos},{Vector3i(0,1,2)},mPos,mCells);
		}
		else if(wall){
			if(isnan(wall->glAB.volume())){
				if(infError) throw std::runtime_error("Wall #"+to_string(p->id)+" does not have Wall.glAB set and cannot be exported to VTK with VtkExport.infError=True.");
				else continue; // skip otherwise
			}
			Vector3r pos=p->shape->nodes[0]->pos;
			int ax0=wall->axis,ax1=(wall->axis+1)%3,ax2=(wall->axis+2)%3;
			Vector2r cent(pos[ax1],pos[ax2]);
			Vector2r lo(cent+wall->glAB.corner(AlignedBox2r::BottomLeft)), hi(cent+wall->glAB.corner(AlignedBox2r::TopRight));
			// construct two facets which look like this in axes (ax1,ax2)
			//
			// ax2   C---D
			// ^     | / |
			// |     A---B
			// |
			// +--------> ax1
			Vector3r A,B,C,D;
			A[ax0]=B[ax0]=C[ax0]=D[ax0]=pos[ax0];
			A[ax1]=B[ax1]=lo[0]; C[ax1]=D[ax1]=hi[0];
			A[ax2]=C[ax2]=lo[1]; B[ax2]=D[ax2]=hi[1];
			nCells=addTriangulatedObject({A,B,C,D},{Vector3i(0,1,3),Vector3i(0,3,2)},mPos,mCells);
		}
		else if(infCyl){
			if(isnan(infCyl->glAB.squaredNorm())){
				if(infError) throw std::runtime_error("InfCylinder #"+to_string(p->id)+" does not have InfCylinder.glAB set and cannot be exported to VTK with VtkExport.infError=True.");
				else continue; // skip the particles otherwise
			}
			int ax0=infCyl->axis,ax1=(infCyl->axis+1)%3,ax2=(infCyl->axis+2)%3;
			//Vector cA,cB; cA=cB=p->shape->nodes[0]->pos;
			//cA[ax0]=infCyl->glAB[0]; cB[ax0]=infCyl->glAB[1];
			Vector2r c2(infCyl->nodes[0]->pos[ax1],infCyl->nodes[0]->pos[ax2]);
			AngleAxisr aa(infCyl->nodes[0]->ori);
			Real phi0=(aa.axis()*aa.angle()).dot(Vector3r::Unit(ax0)); // current rotation
			vector<Vector3r> pts; vector<Vector3i> tri;
			pts.reserve(2*subdiv); tri.reserve(2*subdiv);
			for(int i=0;i<subdiv;i++){
				// triangle strip around cylinder
				//
				// ax    1---3---5...
				// ^     | / | / |...
				// |     0---2---4...
				// |
				// +------> φ (i)
				tri.push_back(Vector3i(2*i,2*i+1,(i==0?2*(subdiv-1):2*i-2)));
				tri.push_back(Vector3i(2*i+1,2*i,(i==(subdiv-1)?1:2*i+3)));
				Real phi=phi0+i*2*M_PI/subdiv;
				Vector2r c=c2+infCyl->radius*Vector2r(sin(phi),cos(phi));
				Vector3r A,B;
				A[ax0]=infCyl->glAB[0]; B[ax0]=infCyl->glAB[1];
				A[ax1]=B[ax1]=c[0];
				A[ax2]=B[ax2]=c[1];
				pts.push_back(A); pts.push_back(B);
			}
			nCells=addTriangulatedObject(pts,tri,mPos,mCells);
		}
		else continue; // skip unhandled shape
		assert(nCells>0);
		const auto& dyn=p->shape->nodes[0]->getData<DemData>();
		for(int i=0;i<nCells;i++){
			mColor->InsertNextValue(p->shape->color);
			mMatId->InsertNextValue(p->material->id);
			// velocity values are erroneous for multi-nodal particles (facets), don't care now
			mVel->InsertNextTupleValue(dyn.vel.data());
			mAngVel->InsertNextTupleValue(dyn.angVel.data());
		}
	}

	// set cells (must be called onces cells are complete)
	sGrid->SetCells(VTK_VERTEX,sCells);
	mGrid->SetCells(VTK_TRIANGLE,mCells);


	vtkSmartPointer<vtkDataCompressor> compressor;
	if(compress) compressor=vtkSmartPointer<vtkZLibDataCompressor>::New();

	if(!multiblock){
		if(what&WHAT_CON){
			vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
			if(compress) writer->SetCompressor(compressor);
			if(ascii) writer->SetDataModeToAscii();
			string fn=out+"con."+to_string(scene->step)+".vtp";
			writer->SetFileName(fn.c_str());
			writer->SetInput(cPoly);
			writer->Write();
		}
		if(what&WHAT_SPHERES){
			auto writer=vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if(compress) writer->SetCompressor(compressor);
			if(ascii) writer->SetDataModeToAscii();
			string fn=out+"spheres."+to_string(scene->step)+".vtu";
			writer->SetFileName(fn.c_str());
			writer->SetInput(sGrid);
			writer->Write();
		}
		if(what&WHAT_MESH){
			auto writer=vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if(compress) writer->SetCompressor(compressor);
			if(ascii) writer->SetDataModeToAscii();
			string fn=out+"mesh."+to_string(scene->step)+".vtu";
			writer->SetFileName(fn.c_str());
			writer->SetInput(mGrid);
			writer->Write();
		}
	} else {
		// multiblock
		auto multi=vtkSmartPointer<vtkMultiBlockDataSet>::New();
		int i=0;
		if(what&WHAT_SPHERES) multi->SetBlock(i++,sGrid);
		if(what&WHAT_MESH) multi->SetBlock(i++,mGrid);
		if(what&WHAT_CON) multi->SetBlock(i++,cPoly);
		auto writer=vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
		if(compress) writer->SetCompressor(compressor);
		if(ascii) writer->SetDataModeToAscii();
		string fn=out+to_string(scene->step)+".vtm";
		writer->SetFileName(fn.c_str());
		writer->SetInput(multi);
		writer->Write();	
	}

	#undef _VTK_ARR_HELPER
	#undef _VTK_POINT_ARR
	#undef _VTK_CELL_ARR
};


#endif /*WOO_VTK*/
