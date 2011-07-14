#ifdef YADE_OPENGL

#include<yade/pkg/gl/NodeGlRep.hpp>
#include<yade/lib/base/CompUtils.hpp>
#include<yade/lib/opengl/OpenGLWrapper.hpp>
#include<yade/lib/opengl/GLUtils.hpp>
#include<yade/lib/base/CompUtils.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg/gl/Renderer.hpp>

#include<GL/gle.h>

YADE_PLUGIN(gl,(ScalarGlRep)(VectorGlRep)(TensorGlRep));

void ScalarGlRep::render(const shared_ptr<Node>& node, GLViewInfo* viewInfo){
	Vector3r color=(range?range->color(val):CompUtils::scalarOnColorScale(val,0,1));
	Vector3r pos=node->pos+(node->hasData<GlData>()?node->getData<GlData>().dGlPos:Vector3r::Zero());
	switch(how){
		case 0: {
			GLUtils::GLDrawText((boost::format("%03g")%val).str(),pos,color); // ,/*center*/true,/*font*/NULL,/*bgColor*/Vector3r::Zero());
			break;
		}
		case 1: {
			glColor3v(color);
			glPointSize((int)(100*relSz));
			glBegin(GL_POINTS);
				glVertex3v(pos);
			glEnd();
			break;
		}
		case 2: {
			glColor3v(color);
			glPushMatrix();
				glTranslatev(pos);
				glutSolidSphere(relSz*viewInfo->sceneRadius,6,12);
			glPopMatrix();
		}
	};
};

void VectorGlRep::render(const shared_ptr<Node>& node, GLViewInfo* viewInfo){
	Real valNorm=val.norm();
	Vector3r color=(range?range->color(valNorm):CompUtils::scalarOnColorScale(valNorm,0,1));
	Real mxNorm=(range?range->mnmx[1]:1);
	Real len=relSz*viewInfo->sceneRadius;
	if(!isnan(scaleExp)) len*=pow(valNorm/mxNorm,scaleExp);
	Vector3r pos=node->pos+(node->hasData<GlData>()?node->getData<GlData>().dGlPos:Vector3r::Zero());
	glColor3v(color);
	GLUtils::GLDrawArrow(pos,pos+len*(val/valNorm),color);
}

void TensorGlRep::postLoad(TensorGlRep&){
	// symmetrize the tensor
	Matrix3r sym=.5*(val+val.transpose()), skewM=.5*(val-val.transpose());
	skew=Vector3r(skewM(1,2),skewM(0,2),skewM(0,1));

	Eigen::SelfAdjointEigenSolver<Matrix3r> eig(sym);
	// multiply rotation matrix (princ) rows by diagonal entries 
	eigVec=eig.eigenvectors();
	eigVal=eig.eigenvalues();
};

void TensorGlRep::render(const shared_ptr<Node>& node, GLViewInfo* viewInfo){

	const int circDiv=20;
	static gleDouble circ[circDiv+2][3]={};

	if(circ[0][0]==0){
		gleSetNumSides(10);
		Real step=2*Mathr::PI/circDiv;
		for(int i=-1; i<circDiv+1; i++){
			circ[i+1][0]=cos(i*step);
			circ[i+1][1]=sin(i*step);
			circ[i+1][2]=0;
		}
	}

	if(range && !skewRange) skewRange=range;

	Vector3r pos=node->pos+(node->hasData<GlData>()?node->getData<GlData>().dGlPos:Vector3r::Zero());

	for(int i=0;i<3;i++){
		Vector3r color=(range?range->color(eigVal[i]):CompUtils::scalarOnColorScale(eigVal[i],-1,1));
		Real mxNorm=(range?range->maxAbs(eigVal[i]):1);
		Real len=relSz*viewInfo->sceneRadius;
		len*=isnan(scaleExp)?abs(eigVal[i]/mxNorm):pow(abs(eigVal[i])/mxNorm,scaleExp);
		glColor3v(color);
		Vector3r dx(len*eigVec.col(i));
		// arrows which go towards each other for negative eigenvalues, and away from each other for positive ones
		GLUtils::GLDrawArrow(pos+(eigVal[i]>0?Vector3r::Zero():dx),pos+(eigVal[i]>0?dx:Vector3r::Zero()),color);
		GLUtils::GLDrawArrow(pos-(eigVal[i]>0?Vector3r::Zero():dx),pos-(eigVal[i]>0?dx:Vector3r::Zero()),color);

		// draw circular arrow to show skew components, in the half-height
		// compute it in the xy plane, transform coords instead
		Real maxSkew=(skewRange?skewRange->maxAbs(skew[i]):1);
		// if(abs(skew[i])<.05*maxSkew) continue;

		int nPts=int((abs(skew[i])/maxSkew)*circDiv*.5/* show max skew as .5*2π rad */-.5/*round evenly, but exclude one segment for arrow head*/);
		if(nPts>circDiv-2) nPts=circDiv-2;
		if(nPts<=0) continue;
		Real torRad1=(skewRelSz>0?skewRelSz:relSz)*viewInfo->sceneRadius*(abs(skew[i])/maxSkew);
		Real torDist=0*.3*torRad1; // draw both arcs in-plane
		Real torRad2=torRad1*.1;
		glColor3v(skewRange?skewRange->color(skew[i]):CompUtils::scalarOnColorScale(skew[i],-1,1));
		for(int j=0; j<2; j++){
			glPushMatrix();
				Eigen::Affine3d T=Eigen::Affine3d::Identity();
				T.translate(pos+(j==0?1:-1)*eigVec.col(i)*torDist).rotate(Quaternionr().setFromTwoVectors(Vector3r::UnitZ(),eigVec.col(i)*(skew[i]>0?-1:1))).scale(torRad1);
				if(j==1) T.rotate(AngleAxisr(Mathr::PI,Vector3r::UnitZ()));
				//GLUtils::setLocalCoords(pos+(j==0?1:-1)*eigVec.col(i)*torDist,
				glMultMatrixd(T.data());
				// since we scaled coords to transform unit circle coords to our radius, we will need to scale dimensions now by 1/torRad1
				glePolyCylinder(nPts+2,circ,/* use current color*/NULL,torRad2*(1/torRad1));
				gleDouble headRad[]={2*torRad2*(1/torRad1),2*torRad2*(1/torRad1),0,0,0};
				glePolyCone(4,(gleDouble(*)[3])&(circ[nPts-1]),/*use current color*/NULL,headRad);
			glPopMatrix();
		}
	}
}



#endif
