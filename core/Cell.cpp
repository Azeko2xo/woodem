#include<woo/core/Cell.hpp>
#include<woo/core/Master.hpp>

WOO_PLUGIN(core,(Cell));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_core_Cell__CLASS_BASE_DOC_ATTRS_CTOR_PY);


CREATE_LOGGER(Cell);

Matrix3r Cell::getVelGrad() const{ LOG_WARN("Cell.velGrad is a deprecated alias for Cell.gradV, update your code!."); return gradV; }
void Cell::setVelGrad(const Matrix3r& v){
	throw std::invalid_argument("Cell.velGrad was renamed to Cell.gradV. Besides, it is no longer directly settable, set Cell.nextGradV instead, to be applied one step later.");
}
Matrix3r Cell::getGradV() const { return gradV; }
void Cell::setGradV(const Matrix3r& v){
	throw std::invalid_argument("Cell.gradV is not directly settable, set Cell.nextGradV instead, to be applied one step later. [If you really know what you're doing, call Cell.setCurrGradV(...).");
}
void Cell::setCurrGradV(const Matrix3r& v){
	nextGradV=v; /* assigns gradV itself */ integrateAndUpdate(0);
}


void Cell::checkTrsfUpperTriangular(){
 if (trsf(1,0)!=0. || trsf(2,0)!=0. || trsf(2,1)!=0.) throw std::runtime_error("Cell.trsf must be upper-triagular (Cell.trsfUpperTriangular==True), but it is not! (Cell.gradV must be upper-triangular too, since its components propagate to Cell.trsf)");
}

Vector3r Cell::intrShiftVel(const Vector3i& cellDist) const {
	switch(homoDeform){
		case HOMO_VEL: case HOMO_VEL_2ND: return gradV*hSize*cellDist.cast<Real>();
		case HOMO_GRADV2: return gradV*pprevHsize*cellDist.cast<Real>();
		default: return Vector3r::Zero();
	}
}

Vector3r Cell::pprevFluctVel(const Vector3r& currPos, const Vector3r& pprevVel, const Real& dt){
	switch(homoDeform){
		case HOMO_NONE:
		case HOMO_POS:
			return pprevVel;
		case HOMO_VEL:
		case HOMO_VEL_2ND:
			return (pprevVel-gradV*currPos);
		case HOMO_GRADV2:
			return pprevVel-gradV*(currPos-dt/2*pprevVel);
		default:
			LOG_FATAL("Cell::ptPprevFlutVel_pprev: invalid value of homoDeform");
			abort();
	};
}

Vector3r Cell::pprevFluctAngVel(const Vector3r& pprevAngVel){
	switch(homoDeform){
		case HOMO_GRADV2: return pprevAngVel-spinVec;
		default: return pprevAngVel;
	}
}

/* at the end of step, bring gradV(t-dt/2) to the value gradV(t+dt/2) so that e.g. kinetic energies are right */
void Cell::setNextGradV(){
	gradV=nextGradV;
	// spin tensor
	W=.5*(gradV-gradV.transpose());
	spinVec=.5*leviCivita(W);
	// if(!isnan(nnextGradV(0,0))){ nextGradV=nnextGradV; nnextGradV<<NaN,NaN,NaN, NaN,NaN,NaN, NaN,NaN,NaN; }
}

Vector3r Cell::shearAlignedExtents(const Vector3r& perpExtent) const{
	if(!hasShear()) return perpExtent;
	Vector3r ret(perpExtent);
	const Vector3r& cos=getCos();
	for(short ax:{0,1,2}){
		short ax1=(ax+1)%3, ax2=(ax+2)%3;
		ret[ax1]+=.5*perpExtent[ax1]*(1/cos[ax]-1);
		ret[ax2]+=.5*perpExtent[ax2]*(1/cos[ax]-1);
	}
	return ret;
}


void Cell::integrateAndUpdate(Real dt){
	//incremental displacement gradient
	_trsfInc=dt*gradV;
	// total transformation; M = (Id+G).M = F.M
	trsf+=_trsfInc*trsf;
	_invTrsf=trsf.inverse();

	if(trsfUpperTriangular) checkTrsfUpperTriangular();

	// hSize contains colums with updated base vectors
	Matrix3r prevHsize=hSize; // remember old value
	if(homoDeform==HOMO_GRADV2) hSize=(Matrix3r::Identity()-gradV*dt/2.).inverse()*(Matrix3r::Identity()+gradV*dt/2.)*hSize;
	else hSize+=_trsfInc*hSize;
	pprevHsize=.5*(prevHsize+hSize); // average of previous and current value

	if(hSize.determinant()==0){ throw std::runtime_error("Cell is degenerate (zero volume)."); }
	// lengths of transformed cell vectors, skew cosines
	Matrix3r Hnorm; // normalized transformed base vectors
	for(int i=0; i<3; i++){
		Vector3r base(hSize.col(i));
		_size[i]=base.norm(); base/=_size[i]; //base is normalized now
		Hnorm(0,i)=base[0]; Hnorm(1,i)=base[1]; Hnorm(2,i)=base[2];
	};
	// skew cosines
	for(int i=0; i<3; i++){
		int i1=(i+1)%3, i2=(i+2)%3;
		// sin between axes is cos of skew
		_cos[i]=(Hnorm.col(i1).cross(Hnorm.col(i2))).squaredNorm();
	}
	// pure shear trsf: ones on diagonal
	_shearTrsf=Hnorm;
	// pure unshear transformation
	_unshearTrsf=_shearTrsf.inverse();
	// some parts can branch depending on presence/absence of shear
	_hasShear=(hSize(0,1)!=0 || hSize(0,2)!=0 || hSize(1,0)!=0 || hSize(1,2)!=0 || hSize(2,0)!=0 || hSize(2,1)!=0);
	// OpenGL shear matrix (used frequently)
	fillGlShearTrsfMatrix(_glShearTrsfMatrix);

	if(!(homoDeform==HOMO_NONE || homoDeform==HOMO_POS || homoDeform==HOMO_VEL || homoDeform==HOMO_VEL_2ND || homoDeform==HOMO_GRADV2)) throw std::invalid_argument("Cell.homoDeform must be in {0,1,2,3,4}.");
}

void Cell::fillGlShearTrsfMatrix(double m[16]){
	m[0]=_shearTrsf(0,0); m[4]=_shearTrsf(0,1); m[8]=_shearTrsf(0,2); m[12]=0;
	m[1]=_shearTrsf(1,0); m[5]=_shearTrsf(1,1); m[9]=_shearTrsf(1,2); m[13]=0;
	m[2]=_shearTrsf(2,0); m[6]=_shearTrsf(2,1); m[10]=_shearTrsf(2,2);m[14]=0;
	m[3]=0;               m[7]=0;               m[11]=0;              m[15]=1;
}


