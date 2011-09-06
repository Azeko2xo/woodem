
#include<yade/core/Cell.hpp>

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

/* at the end of step, bring gradV(t-dt/2) to the value gradV(t+dt/2) so that e.g. kinetic energies are right */
void Cell::setNextGradV(){
	gradV=nextGradV;
	// if(!isnan(nnextGradV(0,0))){ nextGradV=nnextGradV; nnextGradV<<NaN,NaN,NaN, NaN,NaN,NaN, NaN,NaN,NaN; }
}

void Cell::integrateAndUpdate(Real dt){
	//incremental displacement gradient
	_trsfInc=dt*gradV;
	// total transformation; M = (Id+G).M = F.M
	trsf+=_trsfInc*trsf;
	_invTrsf=trsf.inverse();

	if(trsfUpperTriangular) checkTrsfUpperTriangular();
	// hSize contains colums with updated base vectors
	hSize+=_trsfInc*hSize;
	if(hSize.determinant()==0){ throw std::runtime_error("Cell is degenerate (zero volume)."); }
	// lengths of transformed cell vectors, skew cosines
	Matrix3r Hnorm; // normalized transformed base vectors
	for(int i=0; i<3; i++){
		Vector3r base(hSize.col(i));
		_size[i]=base.norm(); base/=_size[i]; //base is normalized now
		Hnorm(0,i)=base[0]; Hnorm(1,i)=base[1]; Hnorm(2,i)=base[2];};
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

	if(!(homoDeform==HOMO_NONE || homoDeform==HOMO_POS || homoDeform==HOMO_VEL || homoDeform==HOMO_VEL_2ND)) throw std::invalid_argument("Cell.homoDeform must be in {0,1,2,3}.");
}

void Cell::fillGlShearTrsfMatrix(double m[16]){
	m[0]=_shearTrsf(0,0); m[4]=_shearTrsf(0,1); m[8]=_shearTrsf(0,2); m[12]=0;
	m[1]=_shearTrsf(1,0); m[5]=_shearTrsf(1,1); m[9]=_shearTrsf(1,2); m[13]=0;
	m[2]=_shearTrsf(2,0); m[6]=_shearTrsf(2,1); m[10]=_shearTrsf(2,2);m[14]=0;
	m[3]=0;               m[7]=0;               m[11]=0;              m[15]=1;
}


