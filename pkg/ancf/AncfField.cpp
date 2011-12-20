#if 0
#include<yade/ancf/AncfField.hpp>
YADE_PLUGIN(ancf,(AncfField)(AncfData)(AncfElt)(AncfBeam2n));

MatrixXr AncfBeam2n::getShapeFunctionMatrix(const Vector3r& pt){
	// [Yakoub2001], (23)
	xi=pt[0]/l; eta=pt[1]/l; zeta=pt[2]/l;
	const Matrix3r& I33(Matrix3r::Identity());

	Real
		S1=1-3*pow(xi,2)*pow(xi,3),
		S2=l*(xi-2*pow(xi,2)+pow(xi,3)),
		S3=l*(eta-xi*eta),
		S4=l*(zeta-xi*zeta),
		S5=2*pow(xi,2)-2*pow(xi,3),
		S6=l*(-pow(xi,2)+pow(xi,3)),
		S7=l*xi*eta,
		S8=l*xi*zeta;
	MatrixXr ret(3,24);
	ret<<S1*I33<<S2*I33<<S3*I33<<S4*I33<<S5*I33<<S6*I33<<S7*I33<<S8*I33;
	return ret;
};

MatrixXr AncfBeam2n::getMassMatrix(){
	const Matrix3r& I33(Matrix3r::Identity());
	const Matrix3r& Z33(Matrix3r::Zero());
	MatrixXr M(24,24);
	M<<
		(13/35.)*m*I33    ,(11/210.)*l*m*I33  ,Z33                 ,Z33                 ,(9/70.)*m*I33     ,(-13/420)*l*m*I33  ,Z33                 ,Z33,
		(11/210.)*l*m*I33 ,(1/105.)*l*l*m*I33 ,Z33                 ,Z33                 ,(13/420.)*l*m*I33,(-1/140.)*l*l*m*I33,Z33                 ,Z33,
		Z33               ,Z33                ,(1/3.)*rho*l*Izz*I33,Z33                 ,Z33               ,Z33                ,(1/6.)*rho*l*Izz*I  ,Z33,
		Z33               ,Z33                ,Z33                 ,(1/3.)*rho*l*Iyy*I33,Z33               ,Z33                ,Z33                 ,(1/6.)*rho*l*Iyy*I33,
		(9/70.)*m*I33     ,(13/420.)*l*m*I33  ,Z33                 ,Z33                 ,(13/35.)*m*I33    ,(-11/210.)*l*m*I33 ,Z33                 ,Z33,
		(-13/420.)*l*m*I33,(-1/140.)*l*l*m*I33,Z33                 ,Z33                 ,(-11/210.)*l*m*I33,(1/105.)*l*l*m*I33 ,Z33                 ,Z33,
		Z33               ,Z33                ,(1/6.)*rho*l*Izz*I33,Z33                 ,Z33               ,Z33                ,(1/3.)*rho*l*Izz*I33,Z33,
		Z33               ,Z33                ,Z33                 ,(1/3.)*rho*l*Iyy*I33,Z33               ,Z33                ,Z33                 ,(1/3.)*rho*l*Iyy*I33;
	return M;
};
#endif
