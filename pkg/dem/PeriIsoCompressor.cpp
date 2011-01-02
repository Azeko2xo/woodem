
// 2009 © Václav Šmilauer <eudoxos@arcig.cz>

#include<yade/pkg/dem/PeriIsoCompressor.hpp>
#include<yade/pkg/dem/Shop.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg/common/NormShearPhys.hpp>
#include<yade/pkg/dem/DemXDofGeom.hpp>
#include<yade/lib/pyutil/gil.hpp>

using namespace std;

YADE_PLUGIN((PeriIsoCompressor)(PeriTriaxController)(Peri3dController))

CREATE_LOGGER(PeriIsoCompressor);
void PeriIsoCompressor::action(){
	if(!scene->isPeriodic){ LOG_FATAL("Being used on non-periodic simulation!"); throw; }
	if(state>=stresses.size()) return;
	// initialize values
	if(charLen<=0){
		Bound* bv=Body::byId(0,scene)->bound.get();
		if(!bv){ LOG_FATAL("No charLen defined and body #0 has no bound"); throw; }
		const Vector3r sz=bv->max-bv->min;
		charLen=(sz[0]+sz[1]+sz[2])/3.;
		LOG_INFO("No charLen defined, taking avg bbox size of body #0 = "<<charLen);
	}
	if(maxSpan<=0){
		FOREACH(const shared_ptr<Body>& b, *scene->bodies){
			if(!b || !b->bound) continue;
			for(int i=0; i<3; i++) maxSpan=max(maxSpan,b->bound->max[i]-b->bound->min[i]);
		}

	}
	if(maxDisplPerStep<0) maxDisplPerStep=1e-2*charLen; // this should be tuned somehow…
	const long& step=scene->iter;
	Vector3r cellSize=scene->cell->getSize(); //unused: Real cellVolume=cellSize[0]*cellSize[1]*cellSize[2];
	Vector3r cellArea=Vector3r(cellSize[1]*cellSize[2],cellSize[0]*cellSize[2],cellSize[0]*cellSize[1]);
	Real minSize=min(cellSize[0],min(cellSize[1],cellSize[2])), maxSize=max(cellSize[0],max(cellSize[1],cellSize[2]));
	if(minSize<2.1*maxSpan){ throw runtime_error("Minimum cell size is smaller than 2.1*span_of_the_biggest_body! (periodic collider requirement)"); }
	if(((step%globalUpdateInt)==0) || avgStiffness<0 || sigma[0]<0 || sigma[1]<0 || sigma[2]<0){
		Vector3r sumForces=Shop::totalForceInVolume(avgStiffness,scene);
		sigma=-Vector3r(sumForces[0]/cellArea[0],sumForces[1]/cellArea[1],sumForces[2]/cellArea[2]);
		LOG_TRACE("Updated sigma="<<sigma<<", avgStiffness="<<avgStiffness);
	}
	Real sigmaGoal=stresses[state]; assert(sigmaGoal<0);
	// expansion of cell in this step (absolute length)
	Vector3r cellGrow(Vector3r::Zero());
	// is the stress condition satisfied in all directions?
	bool allStressesOK=true;
	if(keepProportions){ // the same algo as below, but operating on quantitites averaged over all dimensions
		Real sigAvg=(sigma[0]+sigma[1]+sigma[2])/3., avgArea=(cellArea[0]+cellArea[1]+cellArea[2])/3., avgSize=(cellSize[0]+cellSize[1]+cellSize[2])/3.;
		Real avgGrow=1e-4*(sigmaGoal-sigAvg)*avgArea/(avgStiffness>0?avgStiffness:1);
		Real maxToAvg=maxSize/avgSize;
		if(abs(maxToAvg*avgGrow)>maxDisplPerStep) avgGrow=Mathr::Sign(avgGrow)*maxDisplPerStep/maxToAvg;
		Real okGrow=-(minSize-2.1*maxSpan)/maxToAvg;
		if(avgGrow<okGrow) throw runtime_error("Unable to shring cell due to maximum body size (although required by stress condition). Increase particle rigidity, increase total sample dimensions, or decrease goal stress.");
		// avgGrow=max(avgGrow,-(minSize-2.1*maxSpan)/maxToAvg);
		if(avgStiffness>0) { sigma+=(avgGrow*avgStiffness)*Vector3r::Ones(); sigAvg+=avgGrow*avgStiffness; }
		if(abs((sigAvg-sigmaGoal)/sigmaGoal)>5e-3) allStressesOK=false;
		cellGrow=(avgGrow/avgSize)*cellSize;
	}
	else{ // handle each dimension separately
		for(int axis=0; axis<3; axis++){
			// Δσ=ΔεE=(Δl/l)×(l×K/A) ↔ Δl=Δσ×A/K
			// FIXME: either NormShearPhys::{kn,ks} is computed wrong or we have dimensionality problem here
			// FIXME: that is why the fixup 1e-4 is needed here
			// FIXME: or perhaps maxDisplaPerStep=1e-2*charLen is too big??
			cellGrow[axis]=1e-4*(sigmaGoal-sigma[axis])*cellArea[axis]/(avgStiffness>0?avgStiffness:1);  // FIXME: wrong dimensions? See PeriTriaxController
			if(abs(cellGrow[axis])>maxDisplPerStep) cellGrow[axis]=Mathr::Sign(cellGrow[axis])*maxDisplPerStep;
			cellGrow[axis]=max(cellGrow[axis],-(cellSize[axis]-2.1*maxSpan));
			// crude way of predicting sigma, for steps when it is not computed from intrs
			if(avgStiffness>0) sigma[axis]+=cellGrow[axis]*avgStiffness; // FIXME: dimensions
			if(abs((sigma[axis]-sigmaGoal)/sigmaGoal)>5e-3) allStressesOK=false;
		}
	}
	TRVAR4(cellGrow,sigma,sigmaGoal,avgStiffness);
	assert(scene->dt>0);
	for(int axis=0; axis<3; axis++){ scene->cell->velGrad(axis,axis)=cellGrow[axis]/(scene->dt*scene->cell->refSize[axis]); }
	// scene->cell->refSize+=cellGrow;

	// handle state transitions
	if(allStressesOK){
		if((step%globalUpdateInt)==0) currUnbalanced=Shop::unbalancedForce(/*useMaxForce=*/false,scene);
		if(currUnbalanced<maxUnbalanced){
			state+=1;
			// sigmaGoal reached and packing stable
			if(state==stresses.size()){ // no next stress to go for
				LOG_INFO("Finished");
				if(!doneHook.empty()){ LOG_DEBUG("Running doneHook: "<<doneHook); pyRunString(doneHook); }
			} else { LOG_INFO("Loaded to "<<sigmaGoal<<" done, going to "<<stresses[state]<<" now"); }
		} else {
			if((step%globalUpdateInt)==0) LOG_DEBUG("Stress="<<sigma<<", goal="<<sigmaGoal<<", unbalanced="<<currUnbalanced);
		}
	}
}

void PeriTriaxController::strainStressStiffUpdate(){
	// update strain first
	//"Natural" strain, correct for large deformations, only used for comparison with goals
	for (int i=0;i<3;i++) strain[i]=log(scene->cell->trsf(i,i));
	//stress tensor and stiffness

	//Compute volume of the deformed cell
	// NOTE : needs refSize, could be generalized to arbitrary initial shapes using trsf*refHsize
	// → initial cell size is always box, and will be. The cell repeats  periodically, initial shape doesn't (shouldn't, at least) dinfluence interactions at all/
	// → this is one more place where Hsize would make things shorter : volume=Hsize.Determinant; The full code relies on the fact that initial Hsize is a box. You didn't modify the equation btw, result is the same as in r1936, which was (with spaces...)
	//trsf*Matrix3r ( scene->cell->refSize[0],0,0, 0,scene->cell->refSize[1],0,0,0,scene->cell->refSize[2] ) ).Determinant()
	//remark : the volume of a parallelepiped u1,u2,u3 is always det(u1,u2,u3)
	Real volume=scene->cell->trsf.determinant()*scene->cell->refSize[0]*scene->cell->refSize[1]*scene->cell->refSize[2];

	//Compute sum(fi*lj) and stiffness
	stressTensor = Matrix3r::Zero();
	Vector3r sumStiff(Vector3r::Zero());
	int n=0;
	// NOTE : This sort of loops on interactions could be removed if we had callbacks in e.g. constitutive laws
	// → very likely performance hit; do you have some concrete design in mind?
	// → a vector with functors so we can law->functs->pushback(myThing), and access to the fundamental members (forces, stiffness, normal, etc.). Implementing the second part is less clear in my mind. Inheriting from law::funct(force&, stiffness&, ...)?
	FOREACH(const shared_ptr<Interaction>&I, *scene->interactions){
		if ( !I->isReal() ) continue;
		NormShearPhys* nsi=YADE_CAST<NormShearPhys*> ( I->phys.get() );
		GenericSpheresContact* gsc=YADE_CAST<GenericSpheresContact*> ( I->geom.get() );
		//Contact force
		Vector3r f= ( reversedForces?-1.:1. ) * ( nsi->normalForce+nsi->shearForce );
		//branch vector, FIXME : the first definition generalizes to non-spherical bodies but needs wrapped coords.

		Vector3r branch=(-Body::byId(I->getId1())->state->pos + Body::byId(I->getId2())->state->pos + scene->cell->Hsize*I->cellDist.cast<Real>());
// 		Vector3r branch= gsc->normal* ( gsc->refR1+gsc->refR2 );
		#if 0
			// remove this block later
			// tensorial product f*branch (hand-write the tensor product to prevent matrix instanciation inside the loop by makeTensorProduct)
			//stressTensor(0,0)+=f[0]*branch[0]; stressTensor(1,0)+=f[1]*branch[0]; stressTensor(2,0)+=f[2]*branch[0];
			//stressTensor(0,1)+=f[0]*branch[1]; stressTensor(1,1)+=f[1]*branch[1]; stressTensor(2,1)+=f[2]*branch[1];
			//stressTensor(0,2)+=f[0]*branch[2]; stressTensor(1,2)+=f[1]*branch[2]; stressTensor(2,2)+=f[2]*branch[2];
		#endif
		stressTensor+=f*branch.transpose();
		if( !dynCell )
		{
			for ( int i=0; i<3; i++ ) sumStiff[i]+=abs ( gsc->normal[i] ) *nsi->kn+ ( 1-abs ( gsc->normal[i] ) ) *nsi->ks;
			n++;
		}
	}
	// Compute stressTensor=sum(fi*lj)/Volume (Love equation)
	stressTensor /= volume;
	for(int axis=0; axis<3; axis++) stress[axis]=stressTensor(axis,axis);
	LOG_DEBUG ( "stressTensor : "<<endl
				<<stressTensor(0,0)<<" "<<stressTensor(0,1)<<" "<<stressTensor(0,2)<<endl
				<<stressTensor(1,0)<<" "<<stressTensor(1,1)<<" "<<stressTensor(1,2)<<endl
				<<stressTensor(2,0)<<" "<<stressTensor(2,1)<<" "<<stressTensor(2,2)<<endl
				<<"unbalanced = "<<Shop::unbalancedForce ( /*useMaxForce=*/false,scene ) );

	if (n>0) stiff=(1./n)*sumStiff;
	else stiff=Vector3r::Zero();
}


CREATE_LOGGER(PeriTriaxController);



void PeriTriaxController::action()
{
	if (!scene->isPeriodic){ throw runtime_error("PeriTriaxController run on aperiodic simulation."); }
	const Vector3r& cellSize=scene->cell->getSize();
	//FIXME : this is wrong I think (almost sure, B.)
	Vector3r cellArea=Vector3r(cellSize[1]*cellSize[2],cellSize[0]*cellSize[2],cellSize[0]*cellSize[1]);
	// initial updates
	const Vector3r& refSize=scene->cell->refSize;
	if (maxBodySpan[0]<=0){
		FOREACH(const shared_ptr<Body>& b,*scene->bodies){
			if(!b || !b->bound) continue;
			for(int i=0; i<3; i++) maxBodySpan[i]=max(maxBodySpan[i],b->bound->max[i]-b->bound->min[i]);
		}
	}
	// check current size
	if(2.1*maxBodySpan[0]>cellSize[0] || 2.1*maxBodySpan[1]>cellSize[1] || 2.1*maxBodySpan[2]>cellSize[2]){
		LOG_DEBUG("cellSize="<<cellSize<<", maxBodySpan="<<maxBodySpan);
		throw runtime_error("Minimum cell size is smaller than 2.1*maxBodySpan (periodic collider requirement)");
	}
	bool doUpdate((scene->iter%globUpdate)==0);
	if(doUpdate || min(stiff[0],min(stiff[1],stiff[2])) <=0 || dynCell){ strainStressStiffUpdate(); }

	// set mass to be sum of masses, if not set by the user
	if(dynCell && isnan(mass)){
		mass=0; FOREACH(const shared_ptr<Body>& b, *scene->bodies){ if(b && b->state) mass+=b->state->mass; }
		LOG_INFO("Setting cell mass to "<<mass<<" automatically.");
	}

	bool allOk=true;
	// apply condition along each axis separately (stress or strain)
	assert(scene->dt>0.);
	for(int axis=0; axis<3; axis++){
 		Real& strain_rate = scene->cell->velGrad(axis,axis);//strain rate on axis
		if(stressMask & (1<<axis)){   // control stress
			if(!dynCell){
				// stiffness K=EA; σ₁=goal stress; Δσ wanted stress difference to apply
				// ΔεE=(Δl/l)(K/A) - Grow is Δε, obtained by imposing the strain rate Δε/dt
				strain_rate=1/scene->dt*(goal[axis]-stress[axis])*cellArea[axis]/(stiff[axis]>0?stiff[axis]:1.);
				LOG_TRACE(axis<<": stress="<<stress[axis]<<", goal="<<goal[axis]<<", cellGrow="<<strain_rate*scene->dt);
			} else {  //accelerate the deformation using the density of the period, includes Cundall's damping
				assert( mass>0 );//user set
				Real dampFactor = 1 - growDamping*Mathr::Sign ( strain_rate * ( goal[axis]-stress[axis] ) );
				strain_rate+=dampFactor*scene->dt* ( goal[axis]-stress[axis] ) /mass;
				//if ((scene->iter%5000)==0){cerr << axis<<": stress="<<stress[axis]<<", goal="<<goal[axis]<<", velGrad="<<strain_rate<<endl;}
				LOG_TRACE ( axis<<": stress="<<stress[axis]<<", goal="<<goal[axis]<<", velGrad="<<strain_rate );}

		} else {    // control strain, see "true strain" definition here http://en.wikipedia.org/wiki/Finite_strain_theory
			///NOTE : everything could be generalized to 9 independant components by comparing F[i,i] vs. Matrix3r goal[i,i], but it would be simpler in that case to let the user set the prescribed loading rates velGrad[i,i] when [i,i] is not stress-controlled. This "else" would disappear.
			strain_rate = (exp ( goal[axis]-strain[axis] ) -1)/scene->dt;
			LOG_TRACE ( axis<<": strain="<<strain[axis]<<", goal="<<goal[axis]<<", cellGrow="<<strain_rate*scene->dt);
		}
		// steady evolution with fluctuations; see TriaxialStressController
		if (!dynCell) strain_rate=(1-growDamping)*strain_rate+.8*prevGrow[axis];
		// limit maximum strain rate
		if (abs(strain_rate)>maxStrainRate[axis]) strain_rate = Mathr::Sign(strain_rate)*maxStrainRate[axis];
		// do not shrink below minimum cell size (periodic collider condition), although it is suboptimal WRT resulting stress

		//if ((scene->iter%5000)==0){cerr<< axis <<": velGrad="<<strain_rate<<", maxCellsize"<<-(cellSize[axis]-2.1*maxBodySpan[axis])/scene->dt<<endl;}
		strain_rate=max(strain_rate,-(cellSize[axis]-2.1*maxBodySpan[axis])/scene->dt);
		//if ((scene->iter%5000)==0){cerr <<"velGrad="<<strain_rate<<endl<<endl;}


		// crude way of predicting stress, for steps when it is not computed from intrs
		if(doUpdate) LOG_DEBUG(axis<<": cellGrow="<<strain_rate*scene->dt<<", new stress="<<stress[axis]<<", new strain="<<strain[axis]);
		// used only for temporary goal comparisons. The exact value is assigned in strainStressStiffUpdate
		strain[axis]+=strain_rate*scene->dt;
		// signal if condition not satisfied
		if(stressMask&(1<<axis)){
			Real curr=stress[axis];
			if((goal[axis]!=0 && abs((curr-goal[axis])/goal[axis])>relStressTol) || abs(curr-goal[axis])>absStressTol) allOk=false;
		}else{
			Real curr=strain[axis];
			// since strain is prescribed exactly, tolerances need just to accomodate rounding issues
			if((goal[axis]!=0 && abs((curr-goal[axis])/goal[axis])>1e-6) || abs(curr-goal[axis])>1e-6){
				allOk=false;
				if(doUpdate) LOG_DEBUG("Strain not OK; "<<abs(curr-goal[axis])<<">1e-6");
			}
		}
	}
	// update stress and strain
	if (!dynCell) for ( int axis=0; axis<3; axis++ ){
		// take in account something like poisson's effect here…
		//Real bogusPoisson=0.25; int ax1=(axis+1)%3,ax2=(axis+2)%3;
		//don't modify stress if dynCell, testing only stiff[axis]>0 would not allow switching the control mode in simulations,
		if (stiff[axis]>0) stress[axis]+=(scene->cell->velGrad(axis,axis)*scene->dt/refSize[axis])*(stiff[axis]/cellArea[axis]);
		//-bogusPoisson*(cellGrow[ax1]/refSize[ax1])*(stiff[ax1]/cellArea[ax1])-bogusPoisson*(cellGrow[ax2]/refSize[ax2])*(stiff[ax2]/cellArea[ax2]);
	}
 	for (int k=0;k<3;k++) strainRate[k]=scene->cell->velGrad(k,k);
	//Update energy input
	Real dW=(scene->cell->velGrad*stressTensor).trace()*scene->dt*scene->cell->Hsize.determinant();
	externalWork+=dW;
	if(scene->trackEnergy) scene->energy->add(-dW,"velGradWork",velGradWorkIx,/*non-incremental*/false);
	prevGrow = strainRate;

	if(allOk){
		if(doUpdate || currUnbalanced<0){
			currUnbalanced=Shop::unbalancedForce(/*useMaxForce=*/false,scene);
			LOG_DEBUG("Stress/strain="<<(stressMask&1?stress[0]:strain[0])<<","<<(stressMask&2?stress[1]:strain[1])<<","<<(stressMask&4?stress[2]:strain[2])<<", goal="<<goal<<", unbalanced="<<currUnbalanced );}
		if(currUnbalanced<maxUnbalanced){
			// LOG_INFO("Goal reached, packing stable.");
			if (!doneHook.empty()){
				LOG_DEBUG ( "Running doneHook: "<<doneHook );
				pyRunString(doneHook);}
			else { Omega::instance().pause(); }
		}
	}
}




CREATE_LOGGER(Peri3dController);
void Peri3dController::action(){
	if(!scene->isPeriodic){ LOG_FATAL("Being used on non-periodic simulation!"); throw; }
	const Real& dt=scene->dt;
	assert(dt>0);

	/* "Constructor" (if (step==0) )
			ps is the vector of indices, where stress is prescribed
			pe is the vector of indices, where strain is prescribed
		 	example: goal = 0b000110 : ps=(1,2,0,0,0,0), pe=(0,3,4,5,0,0)
			lenPs (lenPe) is the meaningful length of ps (pe) (the zeros at the end of ps and pe has no meaning),
				i.e. the number of indices with prescribed stress (strain)
	*/
	bool stressBasedSimulation=false; // true when all stresses are prescribed or if all prescribed strains equal zero
	if (progress==0) {
		lenPs=0; lenPe=0;
		ps=Vector6i::Zero();
		pe=Vector6i::Zero();
		stressGoal = Vector6r::Zero();
		strainGoal = Vector6r::Zero();
		for (int i=0; i<6; i++){
			if (stressMask&(1<<i)){ // if stress is prescribed at direction i, add this direction to ps and increase lenPs by one
				ps(lenPs++)=i;
				stressGoal(i) = goal(i);
			} else{ // if strain is prescribed at direction i, add this direction to pe and increase lenPe by one
				pe(lenPe++)=i;
				strainGoal(i) = goal(i);
			}
		}

		// variables used in evaluation of ideal stress and ideal strain for each part defined by ##Path
		paths[0]=&xxPath; paths[1]=&yyPath; paths[2]=&zzPath; paths[3]=&yzPath; paths[4]=&zxPath; paths[5]=&xyPath; // pointers to the Paths
		pathSizes[0]=xxPath.size(); pathSizes[1]=yyPath.size(); pathSizes[2]=zzPath.size();
		pathSizes[3]=yzPath.size(); pathSizes[4]=zxPath.size(); pathSizes[5]=xyPath.size();
		for (int i=0; i<6; i++) {pathsCounter[i] = 0;} // inidicator in which part of the path we are

		// path[0] is a pointer to xxPath
		// path[0]->operator[](j) is j-th Vector2r in path[0]
		// PATH_OP_OP(0,j,k) = path[0]->operator[](j).operator(k) is k-th element of j-th Vector2r of xxPath
		#define PATH_OP_OP(pi,op1i,op2i) paths[pi]->operator[](op1i).operator()(op2i)

		for (int i=0; i<6; i++) {
			for (int j=1; j<pathSizes[i]; j++) {
				// check if the user defined time axis is monothonically increasing
				{ if ( PATH_OP_OP(i,j-1,0) >= PATH_OP_OP(i,j,0) ) {
					throw runtime_error("Peri3dCoontroller.##Path: Time in ##Path must be monothonically increasing");
				}}
			}
			for (int j=0; j<pathSizes[i]; j++) {
				// convert relative progress values of ##Path to absolute values
				PATH_OP_OP(i,j,0) *= 1./PATH_OP_OP(i,pathSizes[i]-1,0);
				// convert relative stress/strain values of ##Path to absolute stress strain values
				if (abs(PATH_OP_OP(i,pathSizes[i]-1,1)) >= 1e-9) { // the final value is not 0 (otherwise always absolute values are considered)
					PATH_OP_OP(i,j,1) *= goal(i)/PATH_OP_OP(i,pathSizes[i]-1,1);
				}
			}
		}

		// set weather the simulation is "stress based" (all stress components prescribed or all prescribed strains equal zero)
		if (lenPe == 0) { stressBasedSimulation = true; }
		else {
			stressBasedSimulation = true;
			for (int i=0; i<lenPe; i++) { stressBasedSimulation = stressBasedSimulation && PATH_OP_OP(pe(i),1,1)<1e9; }
		}
	}

	// increase the pathCounter by one if we cross to the next part of path
	for (int i=0; i<6; i++) {
		if (progress >= PATH_OP_OP(i,pathsCounter[i],0)) { pathsCounter[i]++; }
	}

	/* values of prescribed stress (strain) rate in respect to prescribed path.
	   The strain indices where stress is prescribed will be overwritten by predictor */
	for (int i=0; i<lenPe; i++){
		int j = pe(i);
		if (pathSizes[j] == 1) { // path has only one part (only final values are prescribed)
			strainRate(j) = strainGoal(j)/(nSteps*dt); // ideal strain rate in respect of dSteps and dValue
		}
		else if (pathsCounter[j] == 0) { // path has more parts, but we are still at the first one
			const Real& dProgress = PATH_OP_OP(j,0,0); // progress difference of respective part of the path
			const Real& dValue = PATH_OP_OP(j,0,1); // strain difference at the respective part of the path
			strainRate(j) = dValue/(dProgress*nSteps*dt); // ideal strain rate in respect of dSteps and dValue
		}
		else if (progress < 1.) {
			const Real dProgress = PATH_OP_OP(j,pathsCounter[j],0) - PATH_OP_OP(j,pathsCounter[j]-1,0); // progress difference of respective part of the path
			const Real dValue = PATH_OP_OP(j,pathsCounter[j],1) - PATH_OP_OP(j,pathsCounter[j]-1,1); // strain difference at the respective part of the path
			strainRate(j) = dValue/(dProgress*nSteps*dt); // ideal strain rate in respect of dSteps and dValue
		}
		else { strainRate(j) = 0; }
	}
	for (int i=0; i<lenPs; i++){
		int j = ps(i);
		if (pathSizes[j] == 1) { // path has only one part (only final values are prescribed)
			stressRate(j) = stressGoal(j)/(nSteps*dt); // ideal stress rate in respect of dSteps and dValue
		}
		else if (pathsCounter[j] == 0) { // path has more parts, but we are still at the first one
			const Real& dProgress = PATH_OP_OP(j,0,0); // progress difference of respective part of the path
			const Real& dValue = PATH_OP_OP(j,0,1); // stress difference at the respective part of the path
			stressRate(j) = dValue/(dProgress*nSteps*dt); // ideal stress rate in respect of dSteps and dValue
		}
		else if (progress < 1.) {
			const Real dProgress = PATH_OP_OP(j,pathsCounter[j],0) - PATH_OP_OP(j,pathsCounter[j]-1,0); //  progress difference of respective part of the path
			const Real dValue = PATH_OP_OP(j,pathsCounter[j],1) - PATH_OP_OP(j,pathsCounter[j]-1,1); // stress difference at the respective part of the path
			stressRate(j) = dValue/(dProgress*nSteps*dt); // ideal stress rate in respect of dSteps and dValue
		}
		else { stressRate(j) = 0; }
	}

	// Update - update values from previous step to current step
	stressOld = stress; // stresssOld = stress at previous step
	sigma = Shop::stressTensorOfPeriodicCell(/*smallStrains=*/true); // current stress tensor
	stress = tensor_toVoigt(sigma); // current stress vector
	stressIdeal += stressRate*dt; // stress that would be obtained if the predictor would be perfect
	strain += strainRate*dt; // current strain vector
	epsilon = voigt_toSymmTensor(strain,/*strain=*/true); // current strain tensor

	/* StrainPredictor
			extremely primitive predictor, but roboust enough and works fine :-) could be replaced by some more rigorous in future.
			In the direction with prescribed strain rate this prescribed strain rate is applied.
			In direction with prescribed stress: from values of stress and strain in previous two steps the value of strain rate
			is predicted so as the stress in the next step would be as close as possible to the ideal stress value,
			see the documentation for more info
	*/
	if (lenPs > 0){ // if at least 1 stress component is prescribed (otherwise prescribed strain is applied in all 6 directions
		if (progress == 0 && stressBasedSimulation) { // the very first step, use compliance estimation (compliance matrix for elastic isotropic material)
			Real complianceEstimation[6][6] = {
				{1/youngEstimation, -poissonEstimation/youngEstimation, -poissonEstimation/youngEstimation, 0,0,0},
				{-poissonEstimation/youngEstimation, 1/youngEstimation, -poissonEstimation/youngEstimation, 0,0,0},
				{-poissonEstimation/youngEstimation, -poissonEstimation/youngEstimation, 1/youngEstimation, 0,0,0},
				{0,0,0, (1+poissonEstimation)/youngEstimation,0,0},
				{0,0,0, 0,(1+poissonEstimation)/youngEstimation,0},
				{0,0,0, 0,0,(1+poissonEstimation)/youngEstimation}};
			for (int i=0; i<lenPs; i++) {
				strainRate(ps(i)) = 0;
				for (int j=0; j<lenPs; j++) { strainRate(ps(i)) += complianceEstimation[ps(i)][ps(j)]*stressRate[ps(j)]; }
				for (int j=0; j<lenPe; j++) { strainRate(ps(i)) += complianceEstimation[ps(i)][ps(j)]*stressRate[ps(j)]; }
			}
			//for (int i=0; i<lenPs; i++) { strainRate(ps(i)) -= maxStrainRate; }
		}
		else { // actual predictor
			Real sr=strainRate.cwise().abs().maxCoeff();
			for (int i=0; i<lenPs; i++) {
				int j=ps(i);
				// linear extrapolation of stress error (difference between actual and ideal stress)
				Real linPred = 2*(stress(j)-stressIdeal(j)) - (stressOld(j)-(stressIdeal(j)-stressRate(j)*dt));
				// correction of strain in respect to the extrapolated stress error
				if (linPred>0){strainRate(j) -= sr*mod;}
				else {strainRate(j) += sr*mod;}
			}
		}
	}

	// correction coefficient ix strainRate.maxabs() > maxStrainRate
	Real srCorr = (strainRate.cwise().abs().maxCoeff() > maxStrainRate)? (maxStrainRate/strainRate.cwise().abs().maxCoeff()):1.;
	strainRate *= srCorr;

	// Actual action (see the documentation for more info)
	const Matrix3r& trsf=scene->cell->trsf;
	// compute rotational and nonrotational (strain in local coordinates) part of trsf
	Matrix_computeUnitaryPositive(trsf,&rot,&nonrot);

	// prescribed velocity gradient (strain tensor rate) in global coordinates
	epsilonRate = voigt_toSymmTensor(strainRate,/*strain=*/true);
	/* transformation of prescribed strain rate (computed by predictor) into local cell coordinates,
	   multiplying by time to obtain strain increment and adding it to nonrot (current strain in local coordinates)*/
	nonrot += rot.transpose()*(epsilonRate*dt)*rot;
	Matrix3r& velGrad=scene->cell->velGrad;
	// compute new trsf as rot*nonrot, substract actual trsf (= trsf increment), divide by dt (=trsf rate = velGrad
	//trsf = rot*nonrot;
	//velGrad = (rot*nonrot - trsf)/dt;
	velGrad = ((rot*nonrot)*trsf.inverse()- Matrix3r::Identity()) / dt ;
	progress += srCorr/nSteps;

	if (progress >= 1. || strain.cwise().abs().maxCoeff() > maxStrain) {
		if(doneHook.empty()){ LOG_INFO("No doneHook set, dying."); dead=true; }
		else{ LOG_INFO("Running doneHook: "<<doneHook);	pyRunString(doneHook);}
	}
}

/*
CREATE_LOGGER(Peri3dController);
void Peri3dController::update(){
	/ * strain
		polar decomposition of transformation:
			compute strain tensor from the non-rotational part of trsf, then rotate it back to global coords
	/
	Matrix3r rot,nonrot; //nonrot=skew+normal deformation
	const Matrix3r& trsf=scene->cell->trsf;
	Eigen::SVD<Matrix3r>(trsf).computeUnitaryPositive(&rot,&nonrot);
	// compute matrix logarithm (see documentation)
	Eigen::SelfAdjointEigenSolver<Matrix3r> eigDec(nonrot);
	strain=rot*eigDec.eigenvectors()*eigDec.eigenvalues().cwise().log().asDiagonal()*eigDec.eigenvectors().transpose();
	#if 0
		// small strains only
		strain=rot*(nonrot-Matrix3r::Identity());
	#endif
	LOG_TRACE("Updated strain value\n"<<strain);
	/ * stress and stiffness
	/
	K=Matrix6r::Zero();
	stress=Matrix3r::Zero();
	const Real volume=scene->cell->trsf.determinant()*scene->cell->refSize[0]*scene->cell->refSize[1]*scene->cell->refSize[2];
	int nIntr=0;
	FOREACH(const shared_ptr<Interaction>& I, *scene->interactions){
		if(!I->isReal()) continue;
		nIntr++;
		Dem3DofGeom* geom=YADE_CAST<Dem3DofGeom*>(I->geom.get());
		NormShearPhys* phys=YADE_CAST<NormShearPhys*>(I->phys.get());
		// not clear whether this should be the reference or the current distance
		// current: gives consistent results for same configuration with different initial state
		// reference: does not change stress tensor when the same forces exist on interactions with changing length
		//const Real& d0=geom->refLength;
		const Real d0=(geom->se31.position-geom->se32.position).norm();
		const Vector3r& n=geom->normal;
		const Vector3r& fT=phys->shearForce;
		const Real fN=phys->normalForce.dot(n);
		for(int i=0; i<3; i++) for(int j=0;j<3; j++){
			stress(i,j)+=d0*(fN*n[i]*n[j]+.5*(fT[i]*n[j]+fT[j]*n[i]));
		}
		const Real& kN=phys->kn; const Real& kT=phys->ks;
		// mapping between 6x6 matrix indices and tensor indices (Voigt notation)
		const int map[6][6][4]={
			{{0,0,0,0},{0,0,1,1},{0,0,2,2},{0,0,1,2},{0,0,2,0},{0,0,0,1}},
			{{8,8,8,8},{1,1,1,1},{1,1,2,2},{1,1,1,2},{1,1,2,0},{1,1,0,1}},
			{{8,8,8,8},{8,8,8,8},{2,2,2,2},{2,2,1,2},{2,2,2,0},{2,2,0,1}},
			{{8,8,8,8},{8,8,8,8},{8,8,8,8},{1,2,1,2},{1,2,2,0},{1,2,0,1}},
			{{8,8,8,8},{8,8,8,8},{8,8,8,8},{8,8,8,8},{2,0,2,0},{2,0,0,1}},
			{{8,8,8,8},{8,8,8,8},{8,8,8,8},{8,8,8,8},{8,8,8,8},{0,1,0,1}}};
		const int kron[3][3]={{1,0,0},{0,1,0},{0,0,1}}; // Kronecker delta
		for(int p=0; p<6; p++) for(int q=p;q<6;q++){
			int i=map[p][q][0], j=map[p][q][1], k=map[p][q][2], l=map[p][q][3];
			K(p,q)+=d0*d0*(kN*n[i]*n[j]*n[k]*n[l]+kT*(.25*(n[j]*n[k]*kron[i][l]+n[j]*n[l]*kron[i][k]+n[i]*n[k]*kron[j][l]+n[i]*n[l]*kron[j][k])-n[i]*n[j]*n[k]*n[l]));
		}
	}
	stress/=volume;
	K/=volume;
	for(int p=0; p<6; p++)for(int q=p+1;q<6;q++) K(q,p)=K(p,q);
	LOG_TRACE("Updated stress (from "<<nIntr<<" interactions)\n"<<stress);
	LOG_TRACE("Updated stiffness tensor\n"<<K);
}

void Peri3dController::action(){
	// TODO: only call sometimes
	update();
	typedef Eigen::Matrix<Real,Eigen::Dynamic,Eigen::Dynamic> MatrixXr;
	typedef Eigen::Matrix<Real,Eigen::Dynamic,1> VectorXr;
	const Real& dt=scene->dt;
	/ *
	sigma = K * eps

	decompose the stiffness matrix depending on what is prescribed

	| sigma_u | = | Kuu Kup | * | eps_u |
	| sigma_p |   | Kpu Kpp |   | eps_p |

	then

	eps_u=(Kuu^-1)*(sigma_u-Kup eps_p)

	(we replace all eps and sigma by their rates in the computation)
	/
	int numP=0,numU=0; // number of prescribed and unprescribed strain components
	for(int i=0;i<6;i++) if(stressMask&(1<<i))numU++;
	numP=6-numU;
	// sub-matrices of the stiffness matrix
	MatrixXr Kuu(numU,numU), Kup(numU,numP);
	// epsP, epsU, sigU are _rates_
	VectorXr epsP(numP), epsU(numU), sigU(numU);
	// conversion from Voigt indices to 3x3 tensor indices (upper-triangle part)
	const int mapI[6]={0,1,2,1,2,0};
	const int mapJ[6]={0,1,2,2,0,1};
	int jU=0,jP=0;
	for(int j=0; j<6; j++){
		// prescribed stress, i.e. un-prescribed strain
		if(stressMask&(1<<j)){
			sigU[jU]=(1/dt)*(goal(mapI[j],mapJ[j])-stress(mapI[j],mapJ[j]));
			int iU=0;
			for(int i=0;i<6;i++){ if((stressMask&(1<<i))) Kuu(iU++,jU)=K(i,j);}
			jU++;
		} else {
			epsP[jP]=(1/dt)*(j<3?1.:2.)*(goal(mapI[j],mapJ[j])-strain(mapI[j],mapJ[j])); / * multiply tensor shear by 2, see http://en.wikiversity.org/wiki/Introduction_to_Elasticity/Constitutive_relations /
			int iP=0;
			for(int i=0;i<6;i++){ if((stressMask&(1<<i))) Kup(iP++,jP)=K(i,j);}
			jP++;
		}
	}
	assert(jU==numU); assert(jP==numP);
	LOG_TRACE("Kuu=\n"<<Kuu<<"\nKup=\n"<<Kup<<"\nepsP=\n"<<epsP<<"\nsigU=\n"<<sigU);
	// if Kuu is (nearly) singular, the goal strain is inf and the corresponding velGrad component then NaN
	// in such case, sanitize it with some random value (irrelevant which one)
	// FIXME: find a better way for this than determinant (expensive for larger matrices)
	if(Kuu.rows()*Kuu.cols()>0 && Kuu.determinant()<1e-20){ Kuu+=MatrixXr(Kuu.cols(),Kuu.rows()).setIdentity(); LOG_TRACE("Kuu after sanitization\n"<<Kuu); }
	epsU=Kuu.inverse()*(sigU-Kup*epsP);
	// assemble strain rate tensor (as matrix), from prescribed values epsP and computed values epsU
	jU=0; jP=0;
	Matrix3r eps; // rate!
	for(int j=0; j<6; j++){
		if(stressMask&(1<<j)) eps(mapI[j],mapJ[j])=epsU[jU++];
		else eps(mapI[j],mapJ[j])=(j<3?1.:.5)*epsP[jP++]; / * multiply shear components back by 1/2 when converting from Voigt vector back to tensor /
	}
	eps(2,1)=eps(1,2); eps(0,2)=eps(2,0); eps(1,0)=eps(0,1);
	Matrix3r& velGrad=scene->cell->velGrad;
	// rate of (goal strain - current strain)
	velGrad=eps;
	Real mx=max(abs(velGrad.minCoeff()),abs(velGrad.maxCoeff()));
	if(mx>abs(maxStrainRate)) velGrad*=abs(maxStrainRate)/mx;
	LOG_TRACE("epsU=\n"<<epsU<<"\neps=\n"<<"\nabs(maxCoeff)="<<mx<<"\nvelGrad=\n"<<velGrad);
	// TODO: check unbalanced force and run some hook when the goal state is achieved
}
*/
