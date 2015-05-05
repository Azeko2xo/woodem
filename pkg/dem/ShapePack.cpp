#include<woo/pkg/dem/ShapePack.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/lib/base/Volumetric.hpp>

#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Capsule.hpp>
#include<woo/pkg/dem/Ellipsoid.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/GlData.hpp>
#endif

#include<boost/filesystem/convenience.hpp>
#include<boost/tokenizer.hpp>


WOO_PLUGIN(dem,(ShapeClump)(RawShapeClump)(RawShape)(ShapePack));
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ShapePack__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_RawShape__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_RawShapeClump__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ShapeClump__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL_LOGGER(RawShapeClump);
WOO_IMPL_LOGGER(ShapePack);


RawShape::RawShape(const shared_ptr<Shape>& sh){
	vector<shared_ptr<Node>> nn; // unused
	sh->asRaw(center,radius,nn,raw);
	className=sh->getClassName();
}


void RawShape::translate(const Vector3r& off){ center+=off; }

#if 0
void RawShape::rotate(const Quaternionr& rot){
	// convert to shape, rotate, convert back
	const auto sh=this->toShape();
	if(sh->nodes.size()>1) sh->nodes[0]->ori=rot*sh->nodes[0]->ori;
	else throw std::runtime_error("RawShape::rotate: rotation of multinodal shapes not supported.");
	sh->asRaw(center,radius,raw);
}
#endif

shared_ptr<RawShape> RawShape::copy() const {
	auto ret=make_shared<RawShape>();
	ret->className=className;
	ret->center=center;
	ret->radius=radius;
	ret->raw=raw;
	return ret;
}

shared_ptr<Shape> RawShape::toShape(Real density, Real scale) const {
	shared_ptr<Shape> ret;
	if(className=="Sphere") ret=make_shared<Sphere>();
	else if(className=="Ellipsoid") ret=make_shared<Ellipsoid>();
	#ifndef WOO_NOCAPSULE
		else if(className=="Capsule") ret=make_shared<Capsule>();
	#endif
	else throw std::invalid_argument("RawShape.toShape: className '"+className+"' is not supported.");
	if(scale<=0) throw std::invalid_argument("RawShape.toShape: scale must be a positive number.");
	vector<shared_ptr<Node>> nodes; // initially empty
	ret->setFromRaw(center,radius,nodes,raw);
	if(scale!=1.) ret->applyScale(scale);
	if(nodes.empty()) throw std::runtime_error("Programming error: RawShape::toShape: "+className+"::setFromRaw did not set any nodes.");

	if(!isnan(density)){
		for(const auto& n: nodes){
			n->setData<DemData>(make_shared<DemData>());
			#ifdef WOO_OPENGL
				// to avoid crashes if renderer must resize the node's data array and reallocates it while other thread accesses those data
				n->setData<GlData>(make_shared<GlData>());
			#endif
			// n->getData<DemData>().setOriMassInertia(n);
		}
		// ret->updateMassInertia(density);
	}
	return ret;
}

void RawShapeClump::translate(const Vector3r& offset){
	for(const auto& r: rawShapes) r->translate(offset);
}

shared_ptr<ShapeClump> RawShapeClump::copy() const {
	auto ret=make_shared<RawShapeClump>();
	ret->rawShapes.reserve(rawShapes.size());
	ret->div=div;
	for(const auto& r: rawShapes) ret->rawShapes.push_back(r->copy());
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

#if 0
void RawShapeClump::ensureApproxPos(){
	if(isOk()) return;
	pos=Vector3r::Zero();
	for(const auto& r: rawShapes){
		pos+=r.center;
	}
	pos/=rawShapes.size();
}
#endif

void RawShapeClump::recompute(int _div, bool failOk/* =false */, bool fastOnly/* =false */){
	if(rawShapes.empty()){
		makeInvalid();
		if(failOk) return;
		throw std::runtime_error("RawShapeClump.recompute: rawShapes is empty.");
	}
	div=_div;
	vector<shared_ptr<Shape>> shsh(rawShapes.size());
	for(size_t i=0; i<rawShapes.size(); i++) shsh[i]=rawShapes[i]->toShape(1.); // unit density
	// single mononodal particle: copy things over
	if(shsh.size()==1 && shsh[0]->nodes.size()==1){
		const auto& sh(shsh[0]);
		sh->updateMassInertia(1.); // unit density
		pos=sh->nodes[0]->pos;
		ori=sh->nodes[0]->ori;
		// with unit density, volume==mass
		volume=sh->nodes[0]->getData<DemData>().mass;
		inertia=sh->nodes[0]->getData<DemData>().inertia;
		equivRad=sh->equivRadius();
		return;
	}
	// several particles
	volume=0;
	Vector3r Sg=Vector3r::Zero();
	Matrix3r Ig=Matrix3r::Zero();
	if(_div<=0){
		// non-intersecting: Steiner's theorem over all nodes
		for(const auto& sh: shsh){
			sh->updateMassInertia(1.); // unit density
			// const Real& r(radii[i]); const Vector3r& x(centers[i]);
			for(const auto& n: sh->nodes){
				const Real& v=n->getData<DemData>().mass;
				const Vector3r& x(n->pos);
				volume+=v;
				Sg+=v*x;
				Ig+=woo::Volumetric::inertiaTensorTranslate(n->getData<DemData>().inertia.asDiagonal(),v,-1.*x);
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
	woo::Volumetric::computePrincipalAxes(volume,Sg,Ig,pos,ori,inertia);
	equivRad=(inertia.array()/volume).sqrt().mean(); // mean of radii of gyration
}

std::tuple<vector<shared_ptr<Node>>,vector<shared_ptr<Particle>>> RawShapeClump::makeParticles(const shared_ptr<Material>& mat, const Vector3r& clumpPos, const Quaternionr& clumpOri, int mask, Real scale){
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
			DemData::setOriMassInertia(n); // now the particle's in, recompute
			nNodes++;
		}
	}
	// one particle: do not clump
	if(shsh.size()==1 && shsh[0]->nodes.size()==1){
		const auto& n(shsh[0]->nodes[0]);
		if(!isnan(clumpPos.maxCoeff())){ n->pos=clumpPos; n->ori=clumpOri*n->ori; }
		return std::make_tuple(vector<shared_ptr<Node>>({n}),par);
	}
	if(shsh.size()==1) LOG_WARN("RawShapeClump.makeParticle: clumping node of a single multi-nodal particle.");

	if(!clumped) throw std::runtime_error("Creating non-clumped shapes is not yet supported.");
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
	return std::make_tuple(vector<shared_ptr<Node>>({cn}),par);
}


void ShapePack::recomputeAll(){
	#ifdef WOO_OPENMP
		#pragma omp parallel for schedule(guided)
	#endif
	for(size_t i=0; i<raws.size(); i++){
		const auto& r(raws[i]);
		if(r->isOk() && r->div==div) continue;
		r->recompute(div);
	}
};


void ShapePack::sort(const int& ax, const Real& trimVol){
	recomputeAll();
	std::sort(raws.begin(),raws.end(),[&ax](const shared_ptr<ShapeClump>& a, const shared_ptr<ShapeClump>& b)->bool{ return a->pos[ax]<b->pos[ax]; });
	if(!(trimVol>0)) return;
	Real vol=0;
	for(size_t i=0; i<raws.size(); i++){
		vol+=raws[i]->volume;
		if(vol>trimVol){
			// discard remaining particles above this level
			raws.resize(i+1);
			return;
		}
	}
};

Real ShapePack::solidVolume(){
	recomputeAll();
	Real ret=0;
	for(const auto& r: raws) ret+=r->volume;
	return ret;
}

bool ShapePack::shapeSupported(const shared_ptr<Shape>& sh) const {
	return sh->isA<Sphere>() || sh->isA<Ellipsoid>()
	#ifndef WOO_NOCAPSULE
		|| sh->isA<Capsule>()
	#endif
	;
}

void ShapePack::fromDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, int mask, bool skipUnsupported){
	/* TODO: contiguous node groups should be exported as one (non-clump) shape */
	raws.clear();
	for(const auto& n: dem->nodes){
		assert(n->hasData<DemData>());
		const auto& dyn(n->getData<DemData>());
		if(!dyn.isA<ClumpData>()){
			for(const Particle* p: dyn.parRef){
				if(mask!=0 && (mask&p->mask)==0) continue;
				if(skipUnsupported && !shapeSupported(p->shape)) continue;
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
					if(skipUnsupported && !shapeSupported(p->shape)) continue;
					shsh.push_back(p->shape);
				}
			}
			if(!shsh.empty()) this->add(shsh); // if sphere-only, saves as SphereClumpGeom
		}
	}
	if(scene->isPeriodic){
		if(scene->cell->hasShear()) throw std::runtime_error("ShapePack.fromDem: Cell with shear not supported by ShapePack.");
		cellSize=scene->cell->getHSize().diagonal();
	} else {
		cellSize=Vector3r::Zero();
	}	
}

void ShapePack::toDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, const shared_ptr<Material>& mat, int mask, Real color){
	if(cellSize!=Vector3r::Zero()){
		scene->isPeriodic=true;
		scene->cell->setBox(cellSize);
	} else {
		scene->isPeriodic=false;
	}
	for(const auto& rr: raws){
		Real _color=(isnan(color)?Mathr::UnitRandom():color);
		vector<shared_ptr<Node>> nodes;
		vector<shared_ptr<Particle>> pp;
		std::tie(nodes,pp)=rr->makeParticles(/*mat*/mat,/*clumpPos: force natural position*/Vector3r(NaN,NaN,NaN),/*ori: ignored*/Quaternionr::Identity(),/*mask*/mask,/*scale*/1.);
		for(const auto& p: pp){
			p->shape->color=_color;
			dem->particles->pyAppend(p);
		}
		for(const auto& n: nodes){
			dem->pyNodesAppend(n);
		}
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

void ShapePack::add(const vector<shared_ptr<Shape>>& shsh, bool clumped){
	auto rsc=make_shared<RawShapeClump>();
	rsc->clumped=clumped;
	for(const auto& sh: shsh) rsc->rawShapes.push_back(make_shared<RawShape>(sh));
	if(clumped && rsc->isSphereOnly()) raws.push_back(rsc->asSphereClumpGeom());
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
				f<<i<<(raws[i]->clumped?" ":"u ")<<rs->className<<" "<<rs->center[0]<<" "<<rs->center[1]<<" "<<rs->center[2]<<" "<<rs->radius;
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
		boost::tokenizer<boost::char_separator<char>> toks(line,boost::char_separator<char>(" \t"));
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
		const string& idStr(tokens[0]);
		int id; bool clumped;
		if(boost::algorithm::ends_with(idStr,"u")){ id=lexical_cast<int>(idStr.substr(0,idStr.size()-1)); clumped=false; }
		else{ id=lexical_cast<int>(idStr); clumped=true; }
		if(id!=lastId && id!=(int)raws.size()){
			LOG_WARN(in<<":"<<lineNo<<": shape numbers not contiguous.");
		}
		// creating a new particle
		if(id!=lastId){
			raws.push_back(make_shared<RawShapeClump>());
			lastId=id;
			raws.back()->clumped=clumped;
		} else {
			if(raws.back()->clumped!=clumped) throw std::invalid_argument(in+":"+to_string(lineNo)+": shape #"+to_string(lastId)+" clumping not consistent (some id number have 'u'=unclumped appended, some don't).");
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



void ShapePack::filter(const shared_ptr<Predicate>& predicate, int recenter){
	if(recenter>=0){
		if((!!recenter && !movable) || (!recenter && movable)) throw std::runtime_error("ShapePack.filtered: recenter argument is ignored, but does not match ShapePack.movable.");
	}
	if(movable) throw std::runtime_error("ShapePack.filter: not implemented for movable ShapePack's yet (missing bbox computation).");
	recomputeAll();
	// FIXME: warnings with predicates being larger and such à la SpherePack
	vector<shared_ptr<ShapeClump>> raws2;
	for(const auto& r: raws){
		// FIXME: this will work exactly for spheres, but not so for clumps and others where equivRad is not at all the bounding radius; this will need some infrastructure changes to work properly
		if((*predicate)(r->pos,r->equivRad)) raws2.push_back(r);
	}
	// replace
	raws=raws2;
}

shared_ptr<ShapePack> ShapePack::filtered(const shared_ptr<Predicate>& predicate, int recenter) {
	if(recenter>=0){
		if((!!recenter && !movable) || (!recenter && movable)) throw std::runtime_error("ShapePack.filtered: recenter argument is ignored, but does not match ShapePack.movable.");
	}
	auto ret=make_shared<ShapePack>();
	if(movable) throw std::runtime_error("ShapePack.filter: not implemented for movable ShapePack's yet (missing bbox computation).");
	// copy everything needed
	ret->div=div;
	ret->cellSize=cellSize;
	ret->movable=movable;
	ret->userData=userData;
	recomputeAll();
	for(const auto& r: raws){
		// FIXME: this will work exactly for spheres, but not so for clumps and others where equivRad is not at all the bounding radius; this will need some infrastructure changes to work properly
		if((*predicate)(r->pos,r->equivRad)) ret->raws.push_back(r);
	}
	return ret;
}

void ShapePack::translate(const Vector3r& offset){
	for(auto& r: raws) r->translate(offset);
}

void ShapePack::canonicalize(){
	if(cellSize==Vector3r::Zero()) throw std::runtime_error("ShapePack.canonicalize: only meaningful on periodic packings");
	recomputeAll();
	for(auto &r: raws){
		Vector3r off=Vector3r::Zero();
		for(int ax:{0,1,2}){
			if(cellSize[ax]==0.) continue;
			Real newPos=SpherePack::cellWrapRel(r->pos[ax],0,cellSize[ax]);
			off[ax]=newPos-r->pos[ax];
		}
		r->translate(off);
	}
}

void ShapePack::cellRepeat(const Vector3i& count){
	if(cellSize==Vector3r::Zero()){ throw std::runtime_error("cellRepeat cannot be used on non-periodic packing."); }
	if(count[0]<=0 || count[1]<=0 || count[2]<=0){ throw std::invalid_argument("Repeat count components must be positive."); }
	size_t origSize=raws.size();
	raws.reserve(origSize*count.prod());
	for(int i=0; i<count[0]; i++){
		for(int j=0; j<count[1]; j++){
			for(int k=0; k<count[2]; k++){
				if((i==0) && (j==0) && (k==0)) continue; // original cell
				Vector3r off(cellSize[0]*i,cellSize[1]*j,cellSize[2]*k);
				for(size_t l=0; l<origSize; l++){
					auto copy=raws[l]->copy(); copy->translate(off);
					raws.push_back(copy);
				}
			}
		}
	}
	cellSize=Vector3r(cellSize[0]*count[0],cellSize[1]*count[1],cellSize[2]*count[2]);
}
