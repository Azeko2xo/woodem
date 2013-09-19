#include<woo/pkg/dem/Funcs.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/G3Geom.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/Clump.hpp>

#include<cstdint>
#include<iostream>
#include<fstream>
#include<boost/algorithm/string/predicate.hpp>
#include<boost/detail/endian.hpp>

#ifdef WOO_VTK
	#include<vtkSmartPointer.h>
	#include<vtkPoints.h>
	#include<vtkPolyData.h>
	#include<vtkIncrementalOctreePointLocator.h>
	#include<vtkPointLocator.h>
#endif

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/Renderer.hpp>
#endif

#include <boost/range/adaptor/filtered.hpp>

CREATE_LOGGER(DemFuncs);

// to be used in Python wrappers, to get the DEM field
shared_ptr<DemField> DemFuncs::getDemField(const Scene* scene){
	shared_ptr<DemField> ret;
	FOREACH(const shared_ptr<Field>& f, scene->fields){
		if(dynamic_pointer_cast<DemField>(f)){
			if(ret) throw std::runtime_error("Ambiguous: more than one DemField in Scene.fields.");
			ret=static_pointer_cast<DemField>(f);
		}
	}
	if(!ret) throw std::runtime_error("No DemField in Scene.fields.");
	return ret;
}


Real DemFuncs::pWaveDt(const shared_ptr<DemField>& field, bool noClumps/*=false*/){
	//Scene* scene=(_scene?_scene.get():Master::instance().getScene().get());
	//DemField* field=DemFuncs::getDemField(scene).get();
	Real dt=Inf;
	FOREACH(const shared_ptr<Particle>& b, *field->particles){
		if(!b || !b->material || !b->shape || b->shape->nodes.size()!=1 || !b->shape->nodes[0]->hasData<DemData>()) continue;
		const shared_ptr<ElastMat>& elMat=dynamic_pointer_cast<ElastMat>(b->material);
		const shared_ptr<Sphere>& s=dynamic_pointer_cast<Sphere>(b->shape);
		if(!elMat || !s) continue;
		const auto& n(b->shape->nodes[0]);
		const auto& dyn(n->getData<DemData>());
		// for clumps, the velocity is higher: the distance from the sphere center to the clump center
		// is traversed immediately, thus we need to increase the velocity artificially
		Real velMult=1.;
		if(dyn.isClumped() && !noClumps){
			throw std::runtime_error("utils.pWaveDt does not currently work with clumps; pass noClumps=True to ignore clumps (and treat them as spheres) at your own risk.");
			// this is probably bogus:
			// assert(dyn.master); velMult+=(n->pos-dyn.master.lock()->pos).norm()/s->radius;
		}
		dt=min(dt,s->radius/(velMult*sqrt(elMat->young/elMat->density)));
	}
	return dt;
}


std::tuple</*stress*/Matrix3r,/*stiffness*/Matrix6r> DemFuncs::stressStiffness(const Scene* scene, const DemField* dem, bool skipMultinodal, Real volume){
	const int kron[3][3]={{1,0,0},{0,1,0},{0,0,1}}; // Kronecker delta

	Matrix3r stress=Matrix3r::Zero();
	Matrix6r K=Matrix6r::Zero();

	FOREACH(const shared_ptr<Contact>& C, *dem->contacts){
		FrictPhys* phys=WOO_CAST<FrictPhys*>(C->phys.get());
		const Particle *pA=C->leakPA(), *pB=C->leakPB();
		Vector3r posB=pB->shape->nodes[0]->pos;
		Vector3r posA=pA->shape->nodes[0]->pos-(scene->isPeriodic?scene->cell->intrShiftPos(C->cellDist):Vector3r::Zero());
		if(pA->shape->nodes.size()!=1 || pB->shape->nodes.size()!=1){
			if(skipMultinodal) continue;
			//else woo::ValueError("Particle "+lexical_cast<string>(pA->shape->nodes.size()!=1? pA->id : pB->id)+" has more than one node; to skip contacts with such particles, say skipMultinodal=True");
			if(pA->shape->nodes.size()!=1) posA=C->geom->node->pos;
			if(pB->shape->nodes.size()!=1) posB=C->geom->node->pos;
		}
		// use current distance here
		const Real d0=(posA-posB).norm();
		Vector3r n=C->geom->node->ori.conjugate()*Vector3r::UnitX(); // normal in global coords
		#if 1
			// g3geom doesn't set local x axis properly
			G3Geom* g3g=dynamic_cast<G3Geom*>(C->geom.get());
			if(g3g) n=g3g->normal;
		#endif
		// contact force, in global coords
		Vector3r F=C->geom->node->ori.conjugate()*C->phys->force;
		Real fN=F.dot(n);
		Vector3r fT=F-n*fN;
		//cerr<<"n="<<n.transpose()<<", fN="<<fN<<", fT="<<fT.transpose()<<endl;
		for(int i:{0,1,2}) for(int j:{0,1,2}) stress(i,j)+=d0*(fN*n[i]*n[j]+.5*(fT[i]*n[j]+fT[j]*n[i]));
		//cerr<<"stress="<<stress<<endl;
		const Real& kN=phys->kn; const Real& kT=phys->kt;
		// only upper triangle used here
		for(int p=0; p<6; p++) for(int q=p;q<6;q++){
			int i=voigtMap[p][q][0], j=voigtMap[p][q][1], k=voigtMap[p][q][2], l=voigtMap[p][q][3];
			K(p,q)+=d0*d0*(kN*n[i]*n[j]*n[k]*n[l]+kT*(.25*(n[j]*n[k]*kron[i][l]+n[j]*n[l]*kron[i][k]+n[i]*n[k]*kron[j][l]+n[i]*n[l]*kron[j][k])-n[i]*n[j]*n[k]*n[l]));
		}
	}
	for(int p=0;p<6;p++)for(int q=p+1;q<6;q++) K(q,p)=K(p,q); // symmetrize
	if(volume<=0){
		if(scene->isPeriodic) volume=scene->cell->getVolume();
		else woo::ValueError("Positive volume value must be given for aperiodic simulations.");
	}
	stress/=volume; K/=volume;
	return std::make_tuple(stress,K);
}

Real DemFuncs::unbalancedForce(const Scene* scene, const DemField* dem, bool useMaxForce){
	// get maximum force on a body and sum of all forces (for averaging)
	Real sumF=0,maxF=0;
	int nb=0;
	for(const shared_ptr<Node>& n: dem->nodes){
		DemData& dyn=n->getData<DemData>();
		if(!dyn.isBlockedNone() || dyn.isClumped()) continue;
		Real currF;
		// we suppose here the clump has not yet received any forces from its members
		// and get additional forces from particles
		if(dyn.isClump()){ 
			Vector3r F(dyn.force), T(Vector3r::Zero()) /*we don't care about torque*/; 
			ClumpData::collectFromMembers(n,F,T); 
			currF=F.norm();
		} else currF=dyn.force.norm();
		maxF=max(currF,maxF);
		sumF+=currF;
		nb++;
	}
	Real meanF=sumF/nb;
	// get mean force on interactions
	sumF=0; nb=0;
	FOREACH(const shared_ptr<Contact>& C, *dem->contacts){
		sumF+=C->phys->force.norm(); nb++;
	}
	sumF/=nb;
	return (useMaxForce?maxF:meanF)/(sumF);
}

bool DemFuncs::particleStress(const shared_ptr<Particle>& p, Vector3r& normal, Vector3r& shear){
	if(!p || !p->shape || p->shape->nodes.size()!=1) return false;
	normal=shear=Vector3r::Zero();
	for(const auto& idC: p->contacts){
		const shared_ptr<Contact>& C(idC.second);
		if(!C->isReal()) continue;
		L6Geom* l6g=dynamic_cast<L6Geom*>(C->geom.get());
		if(!l6g) continue;
		const Vector3r& Fl=C->phys->force;
		const Quaternionr invOri=l6g->node->ori.conjugate();
		normal+=(1/l6g->contA)*invOri*Vector3r(Fl[0],0,0);
		shear+=(1/l6g->contA)*invOri*Vector3r(0,Fl[1],Fl[2]);
	}
	return true;
}


shared_ptr<Particle> DemFuncs::makeSphere(Real radius, const shared_ptr<Material>& m){
	auto sphere=make_shared<Sphere>();
	sphere->radius=radius;
	sphere->nodes.push_back(make_shared<Node>());

	const auto& n=sphere->nodes[0];
	n->setData<DemData>(make_shared<DemData>());
	#ifdef WOO_OPENGL
		// to avoid crashes if renderer must resize the node's data array and reallocates it while other thread accesses those data
		n->setData<GlData>(make_shared<GlData>());
	#endif
	auto par=make_shared<Particle>();
	par->shape=sphere;
	par->material=m;

	auto& dyn=n->getData<DemData>();
	dyn.addParRef(par);
	dyn.mass=(4/3.)*M_PI*pow(radius,3)*m->density;
	dyn.inertia=Vector3r::Ones()*(2./5.)*dyn.mass*pow(radius,2);

	return par;
};

vector<Particle::id_t> DemFuncs::SpherePack_toSimulation_fast(const shared_ptr<SpherePack>& sp, const Scene* scene, const DemField* dem, const shared_ptr<Material>& mat, int mask, Real color){
	vector<Particle::id_t> ret; ret.reserve(sp->pack.size());
	if(sp->cellSize!=Vector3r::Zero()) throw std::runtime_error("PBC not supported.");
	for(const auto& s: sp->pack){
		if(s.clumpId>=0) throw std::runtime_error("Clumps not supported.");
		shared_ptr<Particle> p=makeSphere(s.r,mat);
		p->shape->nodes[0]->pos=s.c;
		p->shape->color=(!isnan(color)?color:Mathr::UnitRandom());
		p->mask=mask;
		ret.push_back(dem->particles->insert(p));
	}
	return ret;
}


vector<Vector2r> DemFuncs::boxPsd(const Scene* scene, const DemField* dem, const AlignedBox3r& box, bool mass, int num, int mask, Vector2r rRange){
	bool haveBox=!isnan(box.min()[0]) && !isnan(box.max()[0]);
	return psd(
		*dem->particles|boost::adaptors::filtered([&](const shared_ptr<Particle>&p){ return p && p->shape && p->shape->nodes.size()==1 && (mask?(p->mask&mask):true) && (bool)(dynamic_pointer_cast<woo::Sphere>(p->shape)) && (haveBox?box.contains(p->shape->nodes[0]->pos):true); }),
		/*cumulative*/true,/*normalize*/true,
		num,
		rRange,
		/*diameter getter*/[](const shared_ptr<Particle>&p) ->Real { return 2.*p->shape->cast<Sphere>().radius; },
		/*weight getter*/[&](const shared_ptr<Particle>&p) -> Real{ return mass?p->shape->nodes[0]->getData<DemData>().mass:1.; }
	);
}

size_t DemFuncs::reactionInPoint(const Scene* scene, const DemField* dem, int mask, const Vector3r& pt, bool multinodal, Vector3r& force, Vector3r& torque){
	force=torque=Vector3r::Zero();
	size_t ret=0;
	for(const shared_ptr<Particle>& p: *dem->particles){
		if(!(p->mask & mask) || !p->shape) continue;
		ret++;
		const auto& nn=p->shape->nodes;
		for(const auto& n: nn){
			const Vector3r& F(n->getData<DemData>().force), T(n->getData<DemData>().torque);
			force+=F; torque+=(n->pos-pt).cross(F)+T;
		}
		if(multinodal && nn.size()>1){
			// traverse contacts with other particles
			for(const auto& idC: p->contacts){
				const shared_ptr<Contact>& C(idC.second);
				if(!C->isReal()) continue;
				assert(C->geom && C->phys);
				int forceSign=C->forceSign(p); // +1 if we are pA, -1 if pB
				// force and torque at the contact point in global coords
				const auto& n=C->geom->node;
				Vector3r F(n->ori.conjugate()*C->phys->force *forceSign);
				Vector3r T(n->ori.conjugate()*C->phys->torque*forceSign);
				force+=F;
				torque+=(n->pos-pt).cross(F)+T;
			}
		}
	}
	return ret;
}

#if 0
size_t DemFuncs::radialAxialForce(const Scene* scene, const DemField* dem, int mask, Vector3r axis, bool shear, Vector2r& radAxF){
	size_t ret=0;
	radAxF=Vector2r::Zero();
	axis.normalize();
	for(const shared_ptr<Particle>& p: *dem->particles){
		if(!(p->mask & mask) || !p->shape) continue;
		ret++;
		for(const auto& idC: p->contacts){
			const shared_ptr<Contact>& C(idC.second);
			if(!C->isReal()) continue;
			Vector3r F=C->geom->node->ori.conjugate()*((shear?C->phys->force:Vector3r(C->phys->force[0],0,0))*C->forceSign(p));
			Vector3r axF=F.dot(axis);
			radAxF+=Vector2r(axF,(F-axF).norm());
		}
	}
	return ret;
}
#endif



/*
	The following code is based on GPL-licensed K-3d importer module from
	https://github.com/K-3D/k3d/blob/master/modules/stl_io/mesh_reader.cpp

	TODO: read color/material, convert to scalar color in Woo
*/
vector<shared_ptr<Particle>> DemFuncs::importSTL(const string& filename, const shared_ptr<Material>& mat, int mask, Real color, Real scale, const Vector3r& shift, const Quaternionr& ori, Real threshold, Real maxBox){
	vector<shared_ptr<Particle>> ret;
	std::ifstream in(filename,std::ios::in|std::ios::binary);
	if(!in) throw std::runtime_error("Error opening "+filename+" for reading (STL import).");

	char buffer[80]; in.read(buffer, 80); in.seekg(0, std::ios::beg);
	bool isAscii=boost::algorithm::starts_with(buffer,"solid");

	// linear array of vertices, each triplet is one face
	// this is filled from ASCII and binary formats as intermediary representation
	vector<Vector3r> vertices;
	if(isAscii){
		LOG_TRACE("STL: ascii format detected");
		string lineBuf;
		long lineNo=-1;
		int fVertsNum=0;  // number of vertices in this facet (for checking)
		for(std::getline(in,lineBuf); in; getline(in,lineBuf)){
			lineNo++;
			string tok;
			std::istringstream line(lineBuf);
			line>>tok;
			if(tok=="facet"){
				string tok2; line>>tok2;
				if(tok2!="normal") LOG_WARN("STL: 'normal' expected after 'facet' (line "+to_string(lineNo)+")");
				// we ignore normal values:
				// Vector3r normal; line>>normal.x(); line>>normal.y(); line>>normal.z();
			} else if(tok=="vertex"){
				Vector3r p; line>>p.x(); line>>p.y(); line>>p.z();
				vertices.push_back(p);
				fVertsNum++;
			} else if(tok=="endfacet"){
				if(fVertsNum!=3){
					LOG_WARN("STL: face has "+to_string(fVertsNum)+" vertices instead of 3, skipping face (line "+to_string(lineNo)+")");
					vertices.resize(vertices.size()-fVertsNum);
				}
				fVertsNum=0;
			}
		}
	} else { // binary format
		/* make sure we're on little-endian machine, because that's what STL uses */
		#ifdef BOOST_LITTLE_ENDIAN
			LOG_TRACE("STL: binary format detected");
			char header[80];
			in.read(header,80);
			if(boost::algorithm::contains(header,"COLOR=") || boost::algorithm::contains(header,"MATERIAL=")){
				LOG_WARN("STL: global COLOR/MATERIAL not imported (not implemented yet).");
			}
			int32_t numFaces;
			in.read(reinterpret_cast<char*>(&numFaces),sizeof(int32_t));
			LOG_TRACE("binary STL: number of faces "<<numFaces);
			struct bin_face{ // 50 bytes total
				float normal[3], v[9];
				uint16_t color;
			};
			// longer struct is OK (padding?), but not shorter
			static_assert(sizeof(bin_face)>=50,"One face in the STL binary format must be at least 50 bytes long. !?");
			vector<bin_face> faces(numFaces);
			for(int i=0; i<numFaces; i++){
				in.read(reinterpret_cast<char*>(&faces[i]),50);
				const auto& f(faces[i]);
				LOG_TRACE("binary STL: face #"<<i<<" @ "<<(int)in.tellg()-50<<": normal ("<<f.normal[0]<<", "<<f.normal[1]<<", "<<f.normal[2]<<"), "
					" vertex 0 ("<<f.v[0]<<", "<<f.v[1]<<", "<<f.v[2]<<"), "
					" vertex 1 ("<<f.v[3]<<", "<<f.v[4]<<", "<<f.v[5]<<"), "
					" vertex 2 ("<<f.v[6]<<", "<<f.v[7]<<", "<<f.v[8]<<"), "
					" color "<<f.color
				);
				for(int j:{0,3,6}) vertices.push_back(Vector3r(f.v[j+0],f.v[j+1],f.v[j+2]));
				if(f.color!=0) LOG_WARN("STL: face #"+to_string(i)+": color not imported (not implemented yet).");
			}
		#else
			throw std::runtime_error("Binary STL import not supported on big-endian machines.");
		#endif
	}

	assert(vertices.size()%3==0);
	if(vertices.empty()) return ret;

	AlignedBox3r bbox;
	for(const Vector3r& v: vertices) bbox.extend(v);
	threshold*=-bbox.sizes().maxCoeff();

	// tesselate faces so that their smalles bbox dimension (in rotated & scaled space) does not exceed maxBox
	// this is useful to avoid faces with voluminous bboxs, creating many spurious potential contacts
	if(maxBox>0){
		for(int i=0; i<(int)vertices.size(); i+=3){
			AlignedBox3r b; // in resulting (simulation) space
			for(int j:{0,1,2}) b.extend(ori*(vertices[i+j]*scale+shift));
			if(b.sizes().minCoeff()<=maxBox) continue;
			// too big, tesselate (in STL space)
			vertices.reserve(vertices.size()+9); // 3 new faces will be added
			/*         A
			          +
			         / \
			        /   \
			    CA + --- + AB
			      / \   / \
              /   \ /   \
			  C + --- + --- + B
                   BC

			*/
			Vector3r A(vertices[i]), B(vertices[i+1]), C(vertices[i+2]);  
			Vector3r AB(.5*(A+B)), BC(.5*(B+C)), CA(.5*(C+A));
			vertices[i+1]=AB; vertices[i+2]=CA;
			vertices.push_back(CA); vertices.push_back(BC); vertices.push_back(C );
			vertices.push_back(CA); vertices.push_back(AB); vertices.push_back(BC);
			vertices.push_back(AB); vertices.push_back(B ); vertices.push_back(BC);

			i-=3; // re-run on the same triangle
		}
	}

	// Incremental neighbor search from VTK fails to work, see
	// http://stackoverflow.com/questions/15173310/removing-point-from-cloud-which-are-closer-than-threshold-distance
	#if 0 and defined(WOO_VTK)
		//	http://markmail.org/thread/zrfkbazg3ljed2mj clears up confucion on BuildLocator vs. InitPointInsert
		// in short: call InitPointInsertion with empty point set, and don't call BuildLocator at all

		// point locator for fast point merge lookup
		auto locator=vtkSmartPointer<vtkPointLocator>::New(); // was vtkIncrementalOctreeLocator, but that one crashed
		auto points=vtkSmartPointer<vtkPoints>::New();
		auto polydata=vtkSmartPointer<vtkPolyData>::New();
		// add the first face so that the locator can be built
		// those points are handled specially in the loop below
		//QQ for(int i:{0,1,2}) points->InsertNextPoint(vertices[i].data());
		polydata->SetPoints(points);
		locator->SetDataSet(polydata);
		Real bounds[]={bbox.min()[0],bbox.max()[0],bbox.min()[1],bbox.max()[1],bbox.min()[2],bbox.max()[2]};
		locator->InitPointInsertion(points,bounds);
	#endif


	

	vector<shared_ptr<Node>> nodes;
	for(size_t v0=0; v0<vertices.size(); v0+=3){
		size_t vIx[3];
		__attribute__((unused)) bool isNew[3]={false,false,false};
		for(size_t v: {0,1,2}){
			vIx[v]=nodes.size(); // this value means the point was not found
			const Vector3r& pos(vertices[v0+v]);
			#if 0 and defined(WOO_VTK)
				// for the first face, v0==0, pretend the point was not found, so that the node is create below (without inserting additional point)
				//QQ if(v0>0){
					if(points->GetNumberOfPoints()>0){
						double realDist;
						vtkIdType id=locator->FindClosestPointWithinRadius(threshold,pos.data(),realDist);
						if(id>=0) vIx[v]=id; // point was found
					}
					// if not found, keep vIx[v]==nodes.size()
				//QQ };
			#else
				for(size_t i=0; i<nodes.size(); i++){
					if((pos-nodes[i]->pos).squaredNorm()<pow(threshold,2)){
						vIx[v]=i;
						break;
					}
				}
			#endif
			// create new node
			if(vIx[v]==nodes.size()){
				auto n=make_shared<Node>();
				n->pos=ori*(pos*scale+shift); // change coordinate system here
				n->setData<DemData>(make_shared<DemData>());
				// block all DOFs
				n->getData<DemData>().setBlockedAll();
				#ifdef WOO_OPENGL
					// see comment in DemFuncs::makeSphere
					n->setData<GlData>(make_shared<GlData>());
				#endif
				nodes.push_back(n);
				#if 0 and defined(WOO_VTK)
					//QQ if(v0>0){ // don't add points for the first face, which were added to the locator above already
						LOG_TRACE("Face #"<<v0/3<<"; new vertex "<<n->pos<<" (number of vertices: "<<points->GetNumberOfPoints()<<")");
						__attribute__((unused)) vtkIdType id=locator->InsertNextPoint(n->pos.data());
						assert(id==nodes.size()-1); // assure same index of node and point
					//QQ }
				#endif
				isNew[v]=true;
			}
		}
		LOG_TRACE("STL: Face #"<<v0/3<<", node indices "<<vIx[0]<<(isNew[0]?"*":"")<<", "<<vIx[1]<<(isNew[1]?"*":"")<<", "<<vIx[2]<<(isNew[2]?"*":"")<<" ("<<nodes.size()<<" nodes)");
		// create facet
		auto facet=make_shared<Facet>();
		auto par=make_shared<Particle>();
		par->shape=facet;
		par->material=mat;
		for(auto ix: vIx){
			facet->nodes.push_back(nodes[ix]);
			nodes[ix]->getData<DemData>().addParRef(par);
		}
		facet->color=color; // set per-face color, once this is imported form the binary STL
		ret.push_back(par);
	}
	return ret;
}

