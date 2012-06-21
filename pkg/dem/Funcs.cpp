#include<woo/pkg/dem/Funcs.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/G3Geom.hpp>
#include<woo/pkg/dem/FrictMat.hpp>

#ifdef YADE_OPENGL
	#include<woo/pkg/gl/Renderer.hpp>
#endif

#include <boost/range/adaptor/filtered.hpp>

CREATE_LOGGER(DemFuncs);

std::tuple</*stress*/Matrix3r,/*stiffness*/Matrix6r> DemFuncs::stressStiffness(const Scene* scene, const DemField* dem, bool skipMultinodal, Real volume){
	const int kron[3][3]={{1,0,0},{0,1,0},{0,0,1}}; // Kronecker delta

	Matrix3r stress=Matrix3r::Zero();
	Matrix6r K=Matrix6r::Zero();

	FOREACH(const shared_ptr<Contact>& C, *dem->contacts){
		FrictPhys* phys=YADE_CAST<FrictPhys*>(C->phys.get());
		if(C->pA->shape->nodes.size()!=1 || C->pB->shape->nodes.size()!=1){
			if(skipMultinodal) continue;
			else yade::ValueError("Particle "+lexical_cast<string>(C->pA->shape->nodes.size()!=1? C->pA->id : C->pB->id)+" has more than one node; to skip contacts with such particles, say skipMultinodal=True");
		}
		// use current distance here
		const Real d0=(C->pB->shape->nodes[0]->pos-C->pA->shape->nodes[0]->pos+(scene->isPeriodic?scene->cell->intrShiftPos(C->cellDist):Vector3r::Zero())).norm();
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
		else yade::ValueError("Positive volume value must be given for aperiodic simulations.");
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
	#ifdef YADE_OPENGL
		// to avoid crashes if 3renderer must resize the node's data array and reallocates it while other thread accesses those data
		n->setData<GlData>(make_shared<GlData>());
	#endif
	auto& dyn=n->getData<DemData>();
	dyn.parCount=1;
	dyn.mass=(4/3.)*Mathr::PI*pow(radius,3)*m->density;
	dyn.inertia=Vector3r::Ones()*(2./5.)*dyn.mass*pow(radius,2);

	auto par=make_shared<Particle>();
	par->shape=sphere;
	par->material=m;
	return par;
};

vector<Vector2r> DemFuncs::boxPsd(const Scene* scene, const DemField* dem, const AlignedBox3r& box, bool mass, int num, int mask, Vector2r rRange){
	bool haveBox=!isnan(box.min()[0]) && !isnan(box.max()[0]);
	return psd(
		*dem->particles|boost::adaptors::filtered([&](const shared_ptr<Particle>&p){ return p && p->shape && p->shape->nodes.size()==1 && (mask?(p->mask&mask):true) && (bool)(dynamic_pointer_cast<yade::Sphere>(p->shape)) && (haveBox?box.contains(p->shape->nodes[0]->pos):true); }),
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
			if(!p || !p->shape || !dynamic_pointer_cast<yade::Sphere>(p->shape)) continue;
			Real r=p->shape->cast<Sphere>().radius;
			if(r<rRange[0]) rRange[0]=r;
			if(r>rRange[1]) rRange[1]=r;
		}
		if(isinf(rRange[0])) throw std::runtime_error("DemFuncs::boxPsd: no spherical particles?");
	}
	vector<Vector2r> ret(num,Vector2r::Zero());
	size_t nPar=0;
	for(const shared_ptr<Particle>& p: pp){
		if(!p || !p->shape || !dynamic_pointer_cast<yade::Sphere>(p->shape)) continue;
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

