#include<woo/pkg/dem/ShapePack.hpp>
#include<woo/pkg/dem/Clump.hpp>

#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Capsule.hpp>
#include<woo/pkg/dem/Ellipsoid.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/Renderer.hpp>
#endif

#include<boost/filesystem/convenience.hpp>
#include<boost/tokenizer.hpp>


WOO_PLUGIN(dem,(ShapeClump)(RawShapeClump)(RawShape)(ShapePack));
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ShapePack__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_RawShape__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_RawShapeClump__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ShapeClump__CLASS_BASE_DOC_ATTRS_PY);
CREATE_LOGGER(RawShapeClump);


RawShape::RawShape(const shared_ptr<Shape>& sh){
	sh->asRaw(center,radius,raw);
	className=sh->getClassName();
}

shared_ptr<Shape> RawShape::toShape(Real density, Real scale) const {
	shared_ptr<Shape> ret;
	if(className=="Sphere") ret=make_shared<Sphere>();
	else if(className=="Ellipsoid") ret=make_shared<Ellipsoid>();
	else if(className=="Capsule") ret=make_shared<Capsule>();
	else throw std::invalid_argument("RawShape.toShape: className '"+className+"' is not supported.");
	if(scale<=0) throw std::invalid_argument("RawShape.toShape: scale must be a positive number.");
	ret->setFromRaw(center,radius,raw);
	if(scale!=1.) ret->applyScale(scale);

	if(!isnan(density)){
		for(const auto& n: ret->nodes){
			n->setData<DemData>(make_shared<DemData>());
			#ifdef WOO_OPENGL
				// to avoid crashes if renderer must resize the node's data array and reallocates it while other thread accesses those data
				n->setData<GlData>(make_shared<GlData>());
			#endif
		}
		ret->updateMassInertia(density);
	}
	return ret;
}

bool RawShapeClump::isSphereOnly() const {
	for(const auto& r: rawShapes){ if(r->className!="Sphere") return false; }
	return true;
}

shared_ptr<SphereClumpGeom> RawShapeClump::asSphereClumpGeom() const {
	auto ret=make_shared<SphereClumpGeom>();
	ret->centers.reserve(rawShapes.size());
	ret->radii.reserve(rawShapes.size());
	ret->scaleProb=scaleProb;
	for(const auto& r: rawShapes){
		if(r->className!="Sphere") throw std::runtime_error("RawShapeClump::asSphereClumpGeom: all shapes must be Sphere (not a "+r->className+").");
		ret->centers.push_back(r->center);
		ret->radii.push_back(r->radius);
	}
	return ret;
}


void RawShapeClump::recompute(int _div, bool failOk/* =false */, bool fastOnly/* =false */){
	if(rawShapes.empty()){
		makeInvalid();
		if(failOk) return;
		throw std::runtime_error("RawShapeClump.recompute: rawShapes is empty.");
	}
	vector<shared_ptr<Shape>> shsh(rawShapes.size());
	for(size_t i=0; i<rawShapes.size(); i++) shsh[i]=rawShapes[i]->toShape(1.);
	// single mononodal particle: copy things over
	if(shsh.size()==1 && shsh[0]->nodes.size()==1){
		const auto& sh(shsh[0]);
		pos=sh->nodes[0]->pos;
		ori=sh->nodes[0]->ori;
		// with unit density, volume==mass
		volume=sh->nodes[0]->getData<DemData>().mass;
		inertia=sh->nodes[0]->getData<DemData>().inertia;
		equivRad=sh->equivRadius();
		return;
	}
	// several particles
	Real volume=0;
	Vector3r Sg=Vector3r::Zero();
	Matrix3r Ig=Matrix3r::Zero();
	if(_div<=0){
		// non-intersecting: Steiner's theorem over all nodes
		for(const auto& sh: shsh){
			// const Real& r(radii[i]); const Vector3r& x(centers[i]);
			for(const auto& n: sh->nodes){
				const Real& v=n->getData<DemData>().mass;
				const Vector3r& x(n->pos);
				volume+=v;
				Sg+=v*x;
				Ig+=ClumpData::inertiaTensorTranslate(n->getData<DemData>().inertia.asDiagonal(),v,-1.*x);
			}
		}
	} else {
		// intersecting: grid sampling over all particles
		Real rMin=Inf; AlignedBox3r aabb;
		for(const auto& sh: shsh){
			aabb.extend(sh->alignedBox());
			rMin=min(rMin,sh->equivRadius());
		}
		if(!(rMin>0)){
			if(failOk){ makeInvalid(); return; }
			throw std::runtime_error("SphereClumpGeom.recompute: minimum equivalent radius must be positive (not "+to_string(rMin)+")");
		}
		Real dx=rMin/_div; Real dv=pow(dx,3);
		long nCellsApprox=(aabb.sizes()/dx).prod();
		 // don't compute anything, it would take too long
		if(fastOnly && nCellsApprox>1e5){ makeInvalid(); return; }
		if(nCellsApprox>1e8) LOG_WARN("RawShapeClump: space grid has "<<nCellsApprox<<" cells, computing inertia can take a long time.");
		Vector3r x;
		for(x.x()=aabb.min().x()+dx/2.; x.x()<aabb.max().x(); x.x()+=dx){
			for(x.y()=aabb.min().y()+dx/2.; x.y()<aabb.max().y(); x.y()+=dx){
				for(x.z()=aabb.min().z()+dx/2.; x.z()<aabb.max().z(); x.z()+=dx){
					for(const auto& sh: shsh){
						if(sh->isInside(x)){
							volume+=dv;
							Sg+=dv*x;
							Ig+=dv*(x.dot(x)*Matrix3r::Identity()-x*x.transpose())+/*along princial axes of dv; perhaps negligible?*/Matrix3r(Vector3r::Constant(dv*pow(dx,2)/6.).asDiagonal());
							break;
						}
					}
				}
			}
		}
	}
	ClumpData::computePrincipalAxes(volume,Sg,Ig,pos,ori,inertia);
	equivRad=(inertia.array()/volume).sqrt().mean(); // mean of radii of gyration
}


std::tuple<shared_ptr<Node>,vector<shared_ptr<Particle>>> RawShapeClump::makeParticles(const shared_ptr<Material>& mat, const Vector3r& clumpPos, const Quaternionr& clumpOri, int mask, Real scale){
	ensureOk(); // this already failed with rawShapes.empty()
	vector<shared_ptr<Shape>> shsh(rawShapes.size());
	for(size_t i=0; i<rawShapes.size(); i++) shsh[i]=rawShapes[i]->toShape(mat->density,/*scale*/scale);
	vector<shared_ptr<Particle>> par; par.reserve(shsh.size());

	size_t nNodes=0;
	for(const auto& sh: shsh){
		auto p=make_shared<Particle>();
		p->shape=sh;
		// p->shape->color=_color;
		p->material=mat;
		p->mask=mask;
		par.push_back(p);
		for(const auto& n: p->shape->nodes){
			n->getData<DemData>().parRef.push_back(p.get());
			nNodes++;
		}
	}
	// one particle: do not clump
	if(shsh.size()==1 && shsh[0]->nodes.size()==1){
		const auto& n(shsh[0]->nodes[0]);
		if(!isnan(clumpPos.maxCoeff())){ n->pos=clumpPos; n->ori=clumpOri; }
		return std::make_tuple(n,par);
	}
	if(shsh.size()==1) LOG_WARN("RawShapeClump.makeParticle: clumping node of a single multi-nodal particle.");

	/* do all in the natural clump position, change everything at the end!! */
	// multiple particles: make clump node
	auto cn=make_shared<Node>();
	cn->pos=pos;
	cn->ori=ori;
	auto cd=make_shared<ClumpData>();
	cn->setData<DemData>(cd);
	cd->nodes.reserve(nNodes); cd->relPos.reserve(nNodes); cd->relOri.reserve(nNodes);
	for(const auto& p: par){
		for(const auto& n: p->shape->nodes){
			n->getData<DemData>().setClumped(cn);
			cd->nodes.push_back(n);
			cd->relPos.push_back((n->pos-pos)*scale);
			cd->relOri.push_back(n->ori.conjugate());
		}
	}
	// sets particles in global space based on relPos, relOri
	ClumpData::applyToMembers(cn);
	cd->setClump(); assert(cd->isClump());
	// scale = length scale (but not density scale)
	cd->mass=mat->density*volume*pow(scale,3);
	cd->inertia=mat->density*inertia*pow(scale,5);
	cd->equivRad=equivRad;

	if(!isnan(clumpPos.maxCoeff())){
		LOG_FATAL(__FILE__<<":"<<__LINE__<<": specifying clumpPos/clumpOri not yet implemented, garbage will be returned!");
		cn->pos=clumpPos;
		cn->ori=clumpOri;
	}
	return std::make_tuple(cn,par);
}




#if 0
void ShapePack::sort(const int& ax, const Real& trimVol){
	std::sort(rawShapes.begin(),shapes.end(),[](const shared_ptr<Shape>& a, const shared_ptr<Shape>& b)->bool{ return a.avgNodePos()[ax]<b.avgNodesPos()[ax]; });
	if(!trimVol>0) return;
	Real vol=0;
	for(size_t i=0; i<shapes.size(); i++){
		vol+=shapes[i]->volume();
		if(vol>trimVol){
			// discard remaining particles above this level
			shapes.resize(i+1);
			return;
		}
	}
};
#endif

void ShapePack::fromDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, int mask){
	raws.clear();
	for(const auto& n: dem->nodes){
		assert(n->hasData<DemData>());
		const auto& dyn(n->getData<DemData>());
		if(!dyn.isA<ClumpData>()){
			for(const Particle* p: dyn.parRef){
				if(mask!=0 && (mask&p->mask)==0) continue;
				shared_ptr<Shape> sh(p->shape);
				this->add({p->shape}); // handles sphere-only vs. non-sphere
			}
		} else {
			const auto& clump=static_pointer_cast<ClumpData>(n->getDataPtr<DemData>());
			vector<shared_ptr<Shape>> shsh;
			for(const auto& n2: clump->nodes){
				const auto& dyn2(n2->getData<DemData>());
				for(const Particle* p: dyn2.parRef){
					if(mask!=0 && (mask&p->mask)==0) continue;
					shsh.push_back(p->shape);
				}
			}
			this->add(shsh); // if sphere-only, saves as SphereClumpGeom
		}
	}
}

void ShapePack::toDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, const shared_ptr<Material>& mat, int mask, Real color){
	if(cellSize!=Vector3r::Zero()) throw std::runtime_error("ShapePack.toDem: cellSize (PBC) not supported yet.");
	for(const auto& rr: raws){
		Real _color=(isnan(color)?Mathr::UnitRandom():color);
		shared_ptr<Node> node;
		vector<shared_ptr<Particle>> pp;
		std::tie(node,pp)=rr->makeParticles(/*mat*/mat,/*clumpPos: force natural position*/Vector3r(NaN,NaN,NaN),/*ori: ignored*/Quaternionr::Identity(),/*mask*/mask,/*scale*/1.);
		for(const auto& p: pp){
			p->shape->color=_color;
			dem->particles->pyAppend(p);
		}
		dem->pyNodesAppend(node);
		#if 0
			vector<shared_ptr<Particle>> pp; pp.reserve(rr.size());
			for(size_t i=0; i<rr.size(); i++){
				auto p=make_shared<Particle>();
				p->shape=ss[i]->toShape(/*density*/mat->density);
				p->shape->color=_color;
				p->material=mat;
				p->mask=mask;
				pp.push_back(p);
				for(const auto& n: p->shape->nodes) n->getData<DemData>().parRef.push_back(p.get());
			}
			if(pp.size()==1){ // insert a solo particle
				const auto& p(pp[0]);
				dem->particles->pyAppend(p); // node not appended here
				for(const auto& n: p->shape->nodes) dem->pyNodesAppend(n);
			}
			// insert clump
			else dem->particles->pyAppendClumped(pp); // this appends node automatically
		#endif
	}
}

void ShapePack::postLoad(ShapePack&,void* attr){
	if(!loadFrom.empty()){
		if(attr==NULL){
			loadTxt(loadFrom);
			loadFrom.clear();
		} else if(attr==&loadFrom){ loadFrom.clear(); throw std::runtime_error("ShapePack.loadFrom: may only be specified at construction-time."); }
	};
}

void ShapePack::add(const vector<shared_ptr<Shape>>& shsh){
	auto rsc=make_shared<RawShapeClump>();
	for(const auto& sh: shsh) rsc->rawShapes.push_back(make_shared<RawShape>(sh));
	if(rsc->isSphereOnly()) raws.push_back(rsc->asSphereClumpGeom());
	else raws.push_back(rsc);
}

void ShapePack::saveTxt(const string& out) const {
	std::ofstream f(out.c_str());
	if(!f.good()) throw std::runtime_error("SpherePack.saveTxt: unable to open '"+out+"'.");
	if(cellSize!=Vector3r::Zero()){ f<<"##PERIODIC:: "<<cellSize[0]<<" "<<cellSize[1]<<" "<<cellSize[2]<<std::endl; }
	if(!userData.empty()) {
		if(userData.find('\n')!=string::npos) throw std::runtime_error("SpherePack.saveTxt: userData must not contain newline.");
		f<<"##USER-DATA:: "<<userData<<std::endl;
	}
	for(size_t i=0; i<raws.size(); i++){
		if(raws[i]->isA<SphereClumpGeom>()){
			const auto& scg(raws[i]->cast<SphereClumpGeom>());
			for(size_t j=0; j<scg.radii.size(); j++) f<<i<<" Sphere "<<scg.centers[j][0]<<" "<<scg.centers[j][1]<<" "<<scg.centers[j][2]<<" "<<scg.radii[j]<<endl;
		} else if(raws[i]->isA<RawShapeClump>()){
			const auto& rsc(raws[i]->cast<RawShapeClump>());
			for(const auto& rs: rsc.rawShapes){
				f<<i<<" "<<rs->className<<" "<<rs->center[0]<<" "<<rs->center[1]<<" "<<rs->center[2]<<" "<<rs->radius;
				for(const Real& r: rs->raw) f<<" "<<r;
				f<<endl;
			}
		} else {
			throw std::logic_error("ShapePack.raws["+to_string(i)+"] is an unhandled type "+raws[i]->pyStr()+".");
		}
	}
}

void ShapePack::loadTxt(const string& in) {
	if(!boost::filesystem::exists(in)) throw std::invalid_argument("ShapePack.loadTxt: '"+in+"' doesn't exist.");
	std::ifstream f(in);
	if(!f.good()) throw std::runtime_error("ShapePack.loadTxt: '"+in+"' couldn't be opened (but exists).");
	raws.clear();
	movable=false;
	cellSize=Vector3r::Zero();

	string line;
	size_t lineNo=0;
	int lastId=-1;
	while(std::getline(f,line,'\n')){
		lineNo++;
		if(boost::algorithm::starts_with(line,"##USER-DATA:: ")){
			userData=line.substr(14,string::npos); // strip the line header
			continue;
		}
		boost::tokenizer<boost::char_separator<char> > toks(line,boost::char_separator<char>(" \t"));
		vector<string> tokens; for(const string& s: toks) tokens.push_back(s);
		if(tokens[0]=="##PERIODIC::"){
			if(tokens.size()!=4) throw std::invalid_argument(in+":"+to_string(lineNo)+": starts with ##PERIODIC:: but the line is malformed.");
			cellSize=Vector3r(lexical_cast<Real>(tokens[1]),lexical_cast<Real>(tokens[2]),lexical_cast<Real>(tokens[3]));
			continue;
		}
		// strip all tokens after first comment
		for(size_t i=0; i<tokens.size(); i++){ if(tokens[i][0]=='#'){ tokens.resize(i); break; }}
		if(tokens.empty()) continue;
		if(tokens.size()<6) throw std::invalid_argument(in+":"+to_string(lineNo)+": insufficient number of columns ("+to_string(tokens.size())+")");
		int id=lexical_cast<int>(tokens[0]);
		if(id!=lastId && id!=raws.size()){
			LOG_WARN(in<<":"<<lineNo<<": shape numbers not contiguous.");
		}
		// creating a new particle
		if(id!=lastId){
			raws.push_back(make_shared<RawShapeClump>());
			lastId=id;
		}
		// read particle data, no checking whatsoever
		auto ss=make_shared<RawShape>();
		ss->className=tokens[1];
		ss->center=Vector3r(lexical_cast<Real>(tokens[2]),lexical_cast<Real>(tokens[3]),lexical_cast<Real>(tokens[4]));
		ss->radius=lexical_cast<Real>(tokens[5]);
		ss->raw.reserve(tokens.size()-6);
		for(size_t r=6; r<tokens.size(); r++) ss->raw.push_back(lexical_cast<Real>(tokens[r]));
		(*raws.rbegin())->cast<RawShapeClump>().rawShapes.push_back(ss);
	}
	/* convert sphere-only RawShapeClumps to SphereClumpGeoms */
	for(size_t i=0; i<raws.size(); i++){
		if(raws[i]->cast<RawShapeClump>().isSphereOnly()){
			raws[i]=(raws[i]->cast<RawShapeClump>().asSphereClumpGeom());
		}
	}
}


