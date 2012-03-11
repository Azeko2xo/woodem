#include<yade/pkg/dem/Funcs.hpp>
#include<yade/pkg/dem/L6Geom.hpp>
#include<yade/pkg/dem/G3Geom.hpp>
#include<yade/pkg/dem/FrictMat.hpp>


void DemFuncs::stressStiffness(/*results*/ Matrix3r& stress, Matrix6r& K, /* inputs*/ const Scene* scene, const DemField* dem, bool skipMultinodal, Real volume){
	const int kron[3][3]={{1,0,0},{0,1,0},{0,0,1}}; // Kronecker delta

	stress=Matrix3r::Zero();
	K=Matrix6r::Zero();

	FOREACH(const shared_ptr<Contact>& C, dem->contacts){
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
	FOREACH(const shared_ptr<Contact>& C, dem->contacts){
		sumF+=C->phys->force.norm(); nb++;
	}
	sumF/=nb;
	return (useMaxForce?maxF:meanF)/(sumF);
}

