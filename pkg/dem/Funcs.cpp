#include<woo/pkg/dem/Funcs.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/G3Geom.hpp>
#include<woo/pkg/dem/FrictMat.hpp>

#include<cstdint>
#include<iostream>
#include<fstream>
#include<boost/algorithm/string/predicate.hpp>
#include<boost/detail/endian.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/Renderer.hpp>
#endif

#include <boost/range/adaptor/filtered.hpp>

CREATE_LOGGER(DemFuncs);

std::tuple</*stress*/Matrix3r,/*stiffness*/Matrix6r> DemFuncs::stressStiffness(const Scene* scene, const DemField* dem, bool skipMultinodal, Real volume){
	const int kron[3][3]={{1,0,0},{0,1,0},{0,0,1}}; // Kronecker delta

	Matrix3r stress=Matrix3r::Zero();
	Matrix6r K=Matrix6r::Zero();

	FOREACH(const shared_ptr<Contact>& C, *dem->contacts){
		FrictPhys* phys=WOO_CAST<FrictPhys*>(C->phys.get());
		const Particle *pA=C->leakPA(), *pB=C->leakPB();
		if(pA->shape->nodes.size()!=1 || pB->shape->nodes.size()!=1){
			if(skipMultinodal) continue;
			else woo::ValueError("Particle "+lexical_cast<string>(pA->shape->nodes.size()!=1? pA->id : pB->id)+" has more than one node; to skip contacts with such particles, say skipMultinodal=True");
		}
		// use current distance here
		const Real d0=(pB->shape->nodes[0]->pos-pA->shape->nodes[0]->pos+(scene->isPeriodic?scene->cell->intrShiftPos(C->cellDist):Vector3r::Zero())).norm();
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
	FOREACH(const shared_ptr<Node>& n, dem->nodes){
		DemData& dyn=n->getData<DemData>();
		if(!dyn.isBlockedNone() || dyn.isClumped()) continue;
		Real currF=dyn.force.norm();
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
	auto& dyn=n->getData<DemData>();
	dyn.parCount=1;
	dyn.mass=(4/3.)*M_PI*pow(radius,3)*m->density;
	dyn.inertia=Vector3r::Ones()*(2./5.)*dyn.mass*pow(radius,2);

	auto par=make_shared<Particle>();
	par->shape=sphere;
	par->material=m;
	return par;
};

vector<Vector2r> DemFuncs::boxPsd(const Scene* scene, const DemField* dem, const AlignedBox3r& box, bool mass, int num, int mask, Vector2r rRange){
	bool haveBox=!isnan(box.min()[0]) && !isnan(box.max()[0]);
	return psd(
		*dem->particles|boost::adaptors::filtered([&](const shared_ptr<Particle>&p){ return p && p->shape && p->shape->nodes.size()==1 && (mask?(p->mask&mask):true) && (bool)(dynamic_pointer_cast<woo::Sphere>(p->shape)) && (haveBox?box.contains(p->shape->nodes[0]->pos):true); }),
		/*cumulative*/true,/*normalize*/true,
		num,
		rRange,
		/*radius getter*/[](const shared_ptr<Particle>&p) ->Real { return p->shape->cast<Sphere>().radius; },
		/*weight getter*/[&](const shared_ptr<Particle>&p) -> Real{ return mass?p->shape->nodes[0]->getData<DemData>().mass:1.; }
	);
}

#if 0
vector<Vector2r> DemFuncs::psd(const vector<shared_ptr<Particle>>& pp, int num, Vector2r rRange){
	if(isnan(rRange[0]) || isnan(rRange[1]) || rRange[0]<0 || rRange[1]<=0 || rRange[0]>=rRange[1]){
		rRange=Vector2r(Inf,-Inf);
		for(const shared_ptr<Particle>& p: pp){
			if(!p || !p->shape || !dynamic_pointer_cast<woo::Sphere>(p->shape)) continue;
			Real r=p->shape->cast<Sphere>().radius;
			if(r<rRange[0]) rRange[0]=r;
			if(r>rRange[1]) rRange[1]=r;
		}
		if(isinf(rRange[0])) throw std::runtime_error("DemFuncs::boxPsd: no spherical particles?");
	}
	vector<Vector2r> ret(num,Vector2r::Zero());
	size_t nPar=0;
	for(const shared_ptr<Particle>& p: pp){
		if(!p || !p->shape || !dynamic_pointer_cast<woo::Sphere>(p->shape)) continue;
		Real r=p->shape->cast<Sphere>().radius;
		if(r>rRange[1]) continue;
		nPar++;
		int bin=max(0,min(num-1,1+(int)((num-1)*((r-rRange[0])/(rRange[1]-rRange[0])))));
		ret[bin][1]+=1;
	}
	for(int i=0;i<num;i++){
		ret[i][0]=2*(rRange[0]+i*(rRange[1]-rRange[0])/(num-1));
		ret[i][1]=ret[i][1]/pp.size()+(i>0?ret[i-1][1]:0.);
	}
	// for(int i=0; i<num; i++) ret[i][1]/=ret[num-1][1];
	return ret;
};
#endif



/*
	The following code is based on GPL-licensed K-3d importer module from
	https://github.com/K-3D/k3d/blob/master/modules/stl_io/mesh_reader.cpp

	TODO: read color/material, convert to scalar color in Woo
*/
vector<shared_ptr<Particle>> DemFuncs::importSTL(const string& filename, const shared_ptr<Material>& mat, int mask, Real color, Real scale, const Vector3r& shift, const Quaternionr& ori, Real threshold){
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
				LOG_TRACE("binary STL: face #"<<i<<" @ "<<in.tellg()-50<<": normal ("<<f.normal[0]<<", "<<f.normal[1]<<", "<<f.normal[2]<<"), "
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
	
	if(threshold<0){
		AlignedBox3r box;
		for(const Vector3r& v: vertices) box.extend(v);
		threshold*=-box.sizes().maxCoeff();
	}

	vector<shared_ptr<Node>> nodes;
	for(size_t v0=0; v0<vertices.size(); v0+=3){
		size_t vIx[3];
		for(size_t v: {0,1,2}){
			vIx[v]=nodes.size();
			const Vector3r& pos(vertices[v0+v]);
			for(size_t i=0; i<nodes.size(); i++){
				if((pos-nodes[i]->pos).squaredNorm()<pow(threshold,2)){
					vIx[v]=i;
					break;
				}
			}
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
			}
			LOG_TRACE("STL: Face #"<<v0/3<<", node indices "<<vIx[0]<<", "<<vIx[1]<<", "<<vIx[2]<<" ("<<nodes.size()<<" nodes)");
		}
		// create facet
		auto facet=make_shared<Facet>();
		for(auto ix: vIx){
			facet->nodes.push_back(nodes[ix]);
			nodes[ix]->getData<DemData>().parCount++;
		}
		facet->color=color; // set per-face color, once this is imported form the binary STL
		auto par=make_shared<Particle>();
		par->shape=facet;
		par->material=mat;
		ret.push_back(par);
	}
	return ret;
}

