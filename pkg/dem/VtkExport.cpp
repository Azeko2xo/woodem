#ifdef WOO_VTK
#include<woo/pkg/dem/VtkExport.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Facet.hpp>
#include<woo/pkg/dem/InfCylinder.hpp>
#include<woo/pkg/dem/Ellipsoid.hpp>
#include<woo/pkg/dem/Wall.hpp>
#include<woo/pkg/dem/Capsule.hpp>
#include<woo/pkg/fem/Tetra.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/dem/Funcs.hpp>

WOO_PLUGIN(dem,(VtkExport));
WOO_IMPL_LOGGER(VtkExport);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_VtkExport__CLASS_BASE_DOC_ATTRS_CTOR_PY);


int VtkExport::addTriangulatedObject(const vector<Vector3r>& pts, const vector<Vector3i>& tri, const vtkSmartPointer<vtkPoints>& vtkPts, const vtkSmartPointer<vtkCellArray>& cells){
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

/* triangulate strip given two equal-length indices of points, return indices of triangles; adds triangulation to an existing *tri* triangulation */
int VtkExport::triangulateStrip(const vector<int>::iterator& ABegin, const vector<int>::iterator& AEnd, const vector<int>::iterator& BBegin, const vector<int>::iterator& BEnd, bool close, vector<Vector3i>& tri){
	size_t ASize=AEnd-ABegin, BSize=BEnd-BBegin;
	if(ASize!=BSize) throw std::logic_error("VtkExport::triangulateStrip: point lengths not equal.");
	if(ASize<2) throw std::logic_error("VtkExport::triangulateStrip: at least 2 points must be given.");
	int ret=0;
	// vector<Vector3i> ret;
	tri.reserve(tri.size()+2*ASize+(close?2:0));
	for(size_t i=0; i<ASize-1; i++){
		const int& a1=*(ABegin+i); const int& a2=*(ABegin+i+1);
		const int& b1=*(BBegin+i); const int& b2=*(BBegin+i+1);
		if(a1==b1 && a2==b2){ throw std::logic_error("VtkExport::triangulateStrip: indices "+to_string(i)+", "+to_string(i+1)+" are both co-incident."); }
		else if(a1==b1){ tri.push_back(Vector3i(a1,b2,a2)); ret+=1; }
		else if(a2==b2){ tri.push_back(Vector3i(a1,b1,a2)); ret+=1; }
		else{ tri.push_back(Vector3i(b1,b2,a1)); tri.push_back(Vector3i(a1,b2,a2)); ret+=2; }
	}
	if(close){
		// don't check for co-indicent vertices here
		tri.push_back(Vector3i(*(BEnd-1),*BBegin,*(AEnd-1)));
		tri.push_back(Vector3i(*(AEnd-1),*BBegin,*ABegin));
		ret+=2;
	}
	return ret;
}

/* triangulate close fan around point a, adds to an existing triangulation *tri*. */
int VtkExport::triangulateFan(const int& a, const vector<int>::iterator& BBegin, const vector<int>::iterator& BEnd, bool invert, vector<Vector3i>& tri){
	size_t BSize(BEnd-BBegin);
	tri.reserve(tri.size()+(BEnd-BBegin));
	for(size_t i=0; i<BSize; i++){
		int b1=*(BBegin+i), b2=*(BBegin+((i+1)%BSize));
		if(!invert) tri.push_back(Vector3i(a,b1,b2));
		else tri.push_back(Vector3i(a,b2,b1));
	}
	return BSize;
}

std::tuple<vector<Vector3r>,vector<Vector3i>>
VtkExport::triangulateCapsule(const shared_ptr<Capsule>& capsule, int subdiv){
	vector<Vector3r> vert; vector<Vector3i> tri;

	const Real& rad(capsule->radius); const Real& shaft(capsule->shaft);
	const auto& node=capsule->nodes[0];
	//int capPos=0, capNeg=1;
	//vert.push_back(node->loc2glob(Vector3r( rad+.5*shaft,0,0)));
	//vert.push_back(node->loc2glob(Vector3r(-rad-.5*shaft,0,0)));
	Real phiStep=2*M_PI/subdiv;
	int thetaDiv=int(ceil(subdiv/4));
	Real thetaStep=.5*M_PI/thetaDiv;
	// create points around; cap at +x first
	for(int i=1; i<=thetaDiv; i++){
		Real theta=i*thetaStep;
		for(int j=0; j<subdiv; j++){
			Real phi=j*phiStep;
			vert.push_back(node->loc2glob(Vector3r(.5*shaft+rad*cos(theta),rad*sin(theta)*cos(phi),rad*sin(theta)*sin(phi))));
		}
	}
	for(int i=0; i<thetaDiv; i++){
		Real theta=.5*M_PI-i*thetaStep;
		for(int j=0; j<subdiv; j++){
			Real phi=j*phiStep;
			vert.push_back(node->loc2glob(Vector3r(-.5*shaft-rad*cos(theta),rad*sin(theta)*cos(phi),rad*sin(theta)*sin(phi))));
		}
	}
	// dummy array of sequential indices to pass to triangulateStrip and triangulateFan 
	vector<int> ii(vert.size()); for(size_t i=0; i<vert.size(); i++) ii[i]=i;
	// connect successive groups of subdiv points
	assert((vert.size()%subdiv)==0);
	for(size_t i=0; i<=vert.size()-2*subdiv; i+=subdiv){
		auto i0=ii.begin()+i;
		triangulateStrip(i0,i0+subdiv,i0+subdiv,i0+2*subdiv,/*close*/true,tri);
	}
	// connect endpoints (caps)
	size_t lastGroupAt=vert.size()-subdiv;
	vert.push_back(node->loc2glob(Vector3r( rad+.5*shaft,0,0)));
	triangulateFan(vert.size()-1,ii.begin(),ii.begin()+subdiv,/*invert*/false,tri);
	vert.push_back(node->loc2glob(Vector3r(-rad-.5*shaft,0,0)));
	triangulateFan(vert.size()-1,ii.begin()+lastGroupAt,ii.begin()+lastGroupAt+subdiv,/*invert*/true,tri);

	return std::make_tuple(vert,tri);
}


py::dict VtkExport::pyOutFiles() const {
	py::dict ret;
	for(auto& item: outFiles){
		ret[item.first]=py::object(item.second);
	}
	return ret;
}


void VtkExport::run(){
	DemField* dem=static_cast<DemField*>(field.get());
	out=scene->expandTags(out);

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
			const Particle *pA=C->leakPA(), *pB=C->leakPB();
			if(mask && (!(mask&pA->mask) || !(mask&pB->mask))) continue;
			Particle::id_t ids[2]={pA->id,pB->id};
			bool isSphere[]={(bool)dynamic_pointer_cast<Sphere>(pA->shape),(bool)dynamic_pointer_cast<Sphere>(pB->shape)};
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
					const Vector3r& p0=(copy==0?pA->shape->nodes[0]->pos:pB->shape->nodes[0]->pos);
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
	// _VTK_POINT_INT_ARR(sGrid,sClumpId,"clumpId",1)
	_VTK_POINT_ARR(sGrid,sColor,"color",1)
	_VTK_POINT_ARR(sGrid,sVel,"vel",3)
	_VTK_POINT_ARR(sGrid,sAngVel,"angVel",3)
	_VTK_CELL_ARR(sGrid,sSigT,"sigT",3);
	_VTK_CELL_ARR(sGrid,sSigN,"sigN",3);
	_VTK_POINT_INT_ARR(sGrid,sMatId,"matId",1)
	// meshes
	auto mGrid=vtkSmartPointer<vtkUnstructuredGrid>::New();
	auto mPos=vtkSmartPointer<vtkPoints>::New();
	auto mCells=vtkSmartPointer<vtkCellArray>::New();
	mGrid->SetPoints(mPos);
	_VTK_CELL_ARR(mGrid,mColor,"color",1);
	_VTK_CELL_ARR(mGrid,mMatState,"matState",1);
	_VTK_CELL_INT_ARR(mGrid,mMatId,"matId",1);
	_VTK_CELL_ARR(mGrid,mVel,"vel",3);
	_VTK_CELL_ARR(mGrid,mAngVel,"angVel",3);
	_VTK_CELL_ARR(mGrid,mSigNorm,"|sigma|",1);
	// triangulated particles
	auto tGrid=vtkSmartPointer<vtkUnstructuredGrid>::New();
	auto tPos=vtkSmartPointer<vtkPoints>::New();
	auto tCells=vtkSmartPointer<vtkCellArray>::New();
	tGrid->SetPoints(tPos);
	_VTK_POINT_ARR(tGrid,tEqRad,"eqRadius",1);
	_VTK_CELL_ARR(tGrid,tColor,"color",1);
	_VTK_CELL_ARR(tGrid,tMatState,"matState",1);
	_VTK_CELL_INT_ARR(tGrid,tMatId,"matId",1);
	_VTK_CELL_ARR(tGrid,tVel,"vel",3);
	_VTK_CELL_ARR(tGrid,tAngVel,"angVel",3);

	for(const auto& p: *dem->particles){
		if(!p->shape) continue; // this should not happen really
		if(mask && !(mask&p->mask)) continue;
		if(!p->shape->getVisible() && skipInvisible) continue;
		const auto sphere=dynamic_cast<Sphere*>(p->shape.get());
		const auto wall=dynamic_cast<Wall*>(p->shape.get());
		const auto facet=dynamic_cast<Facet*>(p->shape.get());
		const auto tetra=dynamic_cast<Tetra*>(p->shape.get());
		const auto tet4=dynamic_cast<Tet4*>(p->shape.get());
		const auto infCyl=dynamic_cast<InfCylinder*>(p->shape.get());
		const auto ellipsoid=dynamic_cast<Ellipsoid*>(p->shape.get());
		const auto capsule=dynamic_cast<Capsule*>(p->shape.get());
		Real sigNorm=0;
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
			// sClumpId->InsertNextValue(dyn.isClumped()?sphere->nodes[0]->getData<DemData>().cast<ClumpData>().clumpLinIx:-1);
			sColor->InsertNextValue(sphere->color);
			sVel->InsertNextTupleValue(dyn.vel.data());
			sAngVel->InsertNextTupleValue(dyn.angVel.data());
			sMatId->InsertNextValue(p->material->id);
			Vector3r sigT,sigN;
			__attribute__((unused)) bool sig=DemFuncs::particleStress(p,sigT,sigN); 
			assert(sig); // should not fail for spheres
			sSigN->InsertNextTupleValue(sigN.data());
			sSigT->InsertNextTupleValue(sigT.data());
			continue;
		}
		// no more spheres, just meshes now
		int mCellNum=0;
		int tCellNum=0;
		if(tetra){
			const Vector3r &A(tetra->nodes[0]->pos), &B(tetra->nodes[1]->pos), &C(tetra->nodes[2]->pos), &D(tetra->nodes[3]->pos);
			mCellNum=addTriangulatedObject({A,B,C,D},{Vector3i(0,2,1),Vector3i(0,1,3),Vector3i(0,3,2),Vector3i(1,2,3)},mPos,mCells);
			if(tet4) sigNorm=tet4->getStressTensor().norm();
		}
		else if(facet){
			const Vector3r &A(facet->nodes[0]->pos), &B(facet->nodes[1]->pos), &C(facet->nodes[2]->pos);
			if(facet->halfThick==0.){
				mCellNum=addTriangulatedObject({A,B,C},{Vector3i(0,1,2)},mPos,mCells);
			} else {
				int fDiv=max(0,thickFacetDiv>=0?thickFacetDiv:subdiv);
				const Vector3r dz=facet->getNormal()*facet->halfThick;
				if(fDiv<=1){
					// fDiv==0: open edges; fDiv==1: closed edges
					vector<Vector3i> pts=(fDiv==0?vector<Vector3i>({Vector3i(0,1,2),Vector3i(3,4,5)}):vector<Vector3i>({
						Vector3i(0,1,2),Vector3i(3,4,5),
						Vector3i(0,1,3),Vector3i(3,1,4),
						Vector3i(1,2,4),Vector3i(4,2,5),
						Vector3i(2,0,5),Vector3i(5,0,3),
					}));
					mCellNum=addTriangulatedObject({A+dz,B+dz,C+dz,A-dz,B-dz,C-dz},pts,mPos,mCells);
				} else {
					// with rounded edges
					vector<Vector3r> vertices={A+dz,B+dz,C+dz,A-dz,B-dz,C-dz};
					vector<Vector3i> pts={Vector3i(0,1,2),Vector3i(3,4,5)}; // connectivity data
					Vector3r fNorm=facet->getNormal();
					const Real& rad=facet->halfThick;
					for(int edge:{0,1,2}){
						const Vector3r& K(facet->nodes[edge]->pos);
						const Vector3r& L(facet->nodes[(edge+1)%3]->pos);
						const Vector3r& M(facet->nodes[(edge+2)%3]->pos);
						Vector3r KL=(L-K).normalized(), LM=(M-L).normalized();
						Real capAngle=acos(KL.dot(LM));
						int capDiv=max(1.,round(fDiv*(capAngle/M_PI)));
						Real capStep=capAngle/capDiv;
						// edge plus subsequent cap
						for(int edgePos=-1; edgePos<=capDiv; edgePos++){
							// arc center
							const Vector3r C(edgePos==-1?K:L);
							// vector in arc plane, normal to fNorm
							Vector3r arcPlane=KL.cross(fNorm);
							// rotate when on the cap
							if(edgePos>0) arcPlane=AngleAxisr(capStep*edgePos,fNorm)*arcPlane;
							// define vertices
							int v0=vertices.size();
							for(int i=1;i<fDiv;i++){
								Real phi=i*(M_PI/fDiv);
								vertices.push_back(C+rad*cos(phi)*fNorm+rad*sin(phi)*arcPlane);
							}
							// connect vertices with the next arc
							for(int i=0;i<fDiv;i++){
								int a=v0+i-1; int b=a+(fDiv-1);
								// on the last arc -- connect to beginning
								if(edgePos==capDiv && edge==2){ b=6+i-1; }
								int c=a+1; int d=b+1;
								// upper piece
								if(i==0){
									if(edgePos==-1){ a=edge; b=(edge+1)%3; }
									else a=b=(edge+1)%3; // upper cap triangle
								}
								// lower piece
								if(i==fDiv-1){
									if(edgePos==-1){ c=edge+3; d=(edge+1)%3+3; }
									else c=d=((edge+1)%3)+3; // lower cap triangle
								}
								if(a==b) pts.push_back(Vector3i(a,c,d));
								else if(c==d) pts.push_back(Vector3i(a,c,b));
								else{
									pts.push_back(Vector3i(a,c,b));
									pts.push_back(Vector3i(b,c,d));
								}
							}
						}
					}
					mCellNum=addTriangulatedObject(vertices,pts,mPos,mCells);
				}
			}
		}
		else if(capsule){
			vector<Vector3r> vert; vector<Vector3i> tri;
			std::tie(vert,tri)=triangulateCapsule(static_pointer_cast<Capsule>(p->shape),subdiv);
			tCellNum=addTriangulatedObject(vert,tri,tPos,tCells);
		}
		else if(wall){
			if(isnan(wall->glAB.volume())){
				if(infError) throw std::runtime_error("Wall #"+to_string(p->id)+" does not have Wall.glAB set and cannot be exported to VTK with VtkExport.infError=True.");
				else continue; // skip otherwise
			}
			const auto& node=p->shape->nodes[0];
			int ax0=wall->axis,ax1=(wall->axis+1)%3,ax2=(wall->axis+2)%3;
			Vector2r lo(wall->glAB.corner(AlignedBox2r::BottomLeft)), hi(wall->glAB.corner(AlignedBox2r::TopRight));
			// construct two facets which look like this in axes (ax1,ax2)
			//
			// ax2   C---D
			// ^     | / |
			// |     A---B
			// |
			// +--------> ax1
			Vector3r A,B,C,D;
			A[ax0]=B[ax0]=C[ax0]=D[ax0]=0;
			A[ax1]=B[ax1]=lo[0]; C[ax1]=D[ax1]=hi[0];
			A[ax2]=C[ax2]=lo[1]; B[ax2]=D[ax2]=hi[1];
			mCellNum=addTriangulatedObject({node->loc2glob(A),node->loc2glob(B),node->loc2glob(C),node->loc2glob(D)},{Vector3i(0,1,3),Vector3i(0,3,2)},mPos,mCells);
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
			pts.reserve(2*subdiv+(cylCaps?2:0)); tri.reserve((cylCaps?3:2)*subdiv);
			// centers have indices after points at circumference, and are added after the loop
			int centA=2*subdiv, centB=2*subdiv+1;
			for(int i=0;i<subdiv;i++){
				// triangle strip around cylinder
				//
				//           J   K    
				// ax    1---3---5...
				// ^     | / | / |...
				// |     0---2---4...
				// |     L   M
				// |      i=^^^  
				// +------> φ (i)
				int J=2*i+1, K=(i==(subdiv-1))?1:2*i+3, L=(i==0)?2*(subdiv-1):2*i-2, M=2*i;
				tri.push_back(Vector3i(M,J,L));
				tri.push_back(Vector3i(J,M,K));
				if(cylCaps){ tri.push_back(Vector3i(M,L,centA)); tri.push_back(Vector3i(J,K,centB)); }
				Real phi=phi0+i*2*M_PI/subdiv;
				Vector2r c=c2+infCyl->radius*Vector2r(sin(phi),cos(phi));
				Vector3r A,B;
				A[ax0]=infCyl->glAB[0]; B[ax0]=infCyl->glAB[1];
				A[ax1]=B[ax1]=c[0];
				A[ax2]=B[ax2]=c[1];
				pts.push_back(A); pts.push_back(B);
			}
			if(cylCaps){
				Vector3r cA, cB;
				cA[ax0]=infCyl->glAB[0]; cB[ax0]=infCyl->glAB[1];
				cA[ax1]=cB[ax1]=c2[0];
				cA[ax2]=cB[ax2]=c2[1];
				pts.push_back(cA); pts.push_back(cB);
			}
			mCellNum=addTriangulatedObject(pts,tri,mPos,mCells);
		}
		else if(ellipsoid){
			const Vector3r& semiAxes(ellipsoid->semiAxes);
			const Vector3r& pos(ellipsoid->nodes[0]->pos);
			const Quaternionr& ori(ellipsoid->nodes[0]->ori);
			auto uSphTri(CompUtils::unitSphereTri20(ellLev));
			vector<Vector3r> pts; pts.reserve(std::get<0>(uSphTri).size());
			for(const Vector3r& p: std::get<0>(uSphTri)){ pts.push_back(pos+(ori*(p.array()*semiAxes.array()).matrix())); }
			tCellNum=addTriangulatedObject(pts,std::get<1>(uSphTri),tPos,tCells);
		}
		else continue; // skip unhandled shape
		const auto& dyn=p->shape->nodes[0]->getData<DemData>();
		assert(mCellNum>0 || tCellNum>0);

		// mesh and mesh-like particles (facets, walls, infCylinders)
		for(int i=0;i<mCellNum;i++){
			mColor->InsertNextValue(p->shape->color);
			mMatId->InsertNextValue(p->material->id);
			// velocity values are erroneous for multi-nodal particles (facets), don't care now
			mVel->InsertNextTupleValue(dyn.vel.data());
			mAngVel->InsertNextTupleValue(dyn.angVel.data());
			Real scalar=NaN;
			if(p->matState){
				scalar=p->matState->getScalar(0,scene->step);
				if(facet) scalar/=p->shape->cast<Facet>().getArea();
			}
			mMatState->InsertNextValue(isnan(scalar)?nanValue:scalar);
			mSigNorm->InsertNextValue(sigNorm);
		}
		// triangulated particles (ellipsoids, capsules)
		for(int i=0;i<tCellNum;i++){
			Real r=p->shape->equivRadius();
			tEqRad->InsertNextValue(isnan(r)?-1:r);
			tColor->InsertNextValue(p->shape->color);
			tMatId->InsertNextValue(p->material->id);
			// velocity values are erroneous for multi-nodal particles (facets), don't care now
			tVel->InsertNextTupleValue(dyn.vel.data());
			tAngVel->InsertNextTupleValue(dyn.angVel.data());
			Real scalar=NaN;
			if(p->matState){ scalar=p->matState->getScalar(0,scene->step);	/* if(facet) scalar/=p->shape->cast<Facet>().getArea(); */ }
			tMatState->InsertNextValue(isnan(scalar)?nanValue:scalar);
		}
	}

	// set cells (must be called onces cells are complete)
	sGrid->SetCells(VTK_VERTEX,sCells);
	mGrid->SetCells(VTK_TRIANGLE,mCells);
	tGrid->SetCells(VTK_TRIANGLE,tCells);


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
			outFiles["con"].push_back(fn);
		} 
		if(what&WHAT_SPHERES){
			auto writer=vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if(compress) writer->SetCompressor(compressor);
			if(ascii) writer->SetDataModeToAscii();
			string fn=out+"spheres."+to_string(scene->step)+".vtu";
			writer->SetFileName(fn.c_str());
			writer->SetInput(sGrid);
			writer->Write();
			outFiles["spheres"].push_back(fn);
		}
		if(what&WHAT_MESH){
			auto writer=vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if(compress) writer->SetCompressor(compressor);
			if(ascii) writer->SetDataModeToAscii();
			string fn=out+"mesh."+to_string(scene->step)+".vtu";
			writer->SetFileName(fn.c_str());
			writer->SetInput(mGrid);
			writer->Write();
			outFiles["mesh"].push_back(fn);
		}
		if(what&WHAT_TRI){
			auto writer=vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if(compress) writer->SetCompressor(compressor);
			if(ascii) writer->SetDataModeToAscii();
			string fn=out+"tri."+to_string(scene->step)+".vtu";
			writer->SetFileName(fn.c_str());
			writer->SetInput(tGrid);
			writer->Write();
			outFiles["tri"].push_back(fn);
		}
	} else {
		// multiblock
		auto multi=vtkSmartPointer<vtkMultiBlockDataSet>::New();
		int i=0;
		if(what&WHAT_SPHERES) multi->SetBlock(i++,sGrid);
		if(what&WHAT_MESH) multi->SetBlock(i++,mGrid);
		if(what&WHAT_CON) multi->SetBlock(i++,cPoly);
		if(what&WHAT_TRI) multi->SetBlock(i++,tGrid);
		auto writer=vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
		if(compress) writer->SetCompressor(compressor);
		if(ascii) writer->SetDataModeToAscii();
		string fn=out+to_string(scene->step)+".vtm";
		writer->SetFileName(fn.c_str());
		writer->SetInput(multi);
		writer->Write();	
	}

	outTimes.push_back(scene->time);
	outSteps.push_back(scene->step);

	#undef _VTK_ARR_HELPER
	#undef _VTK_POINT_ARR
	#undef _VTK_POINT_INT_ARR
	#undef _VTK_CELL_ARR
	#undef _VTK_CELL_INT_ARR
};

#endif /*WOO_VTK*/
