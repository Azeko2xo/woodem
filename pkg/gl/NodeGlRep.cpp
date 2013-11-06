#ifdef WOO_OPENGL

#include<woo/pkg/gl/NodeGlRep.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<woo/core/Scene.hpp>
#include<woo/pkg/gl/Renderer.hpp>

#include<GL/gle.h>

WOO_PLUGIN(gl,(LabelGlRep)(ScalarGlRep)(VectorGlRep)(TensorGlRep)(ActReactGlRep)(CylGlRep));

void LabelGlRep::render(const shared_ptr<Node>& node, const GLViewInfo* viewInfo){
	Vector3r pos=node->pos+(node->hasData<GlData>()?node->getData<GlData>().dGlPos:Vector3r::Zero());
	GLUtils::GLDrawText(text,pos,color);
};


void ScalarGlRep::render(const shared_ptr<Node>& node, const GLViewInfo* viewInfo){
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

void VectorGlRep::render(const shared_ptr<Node>& node, const GLViewInfo* viewInfo){
	Real valNorm=val.norm();
	Vector3r color=(range?range->color(valNorm):CompUtils::scalarOnColorScale(valNorm,0,1));
	Real mxNorm=(range?range->mnmx[1]:1);
	Real len=relSz*viewInfo->sceneRadius;
	if(!isnan(scaleExp)) len*=pow(min(1.,valNorm/mxNorm),scaleExp);
	Vector3r pos=node->pos+(node->hasData<GlData>()?node->getData<GlData>().dGlPos:Vector3r::Zero());
	glColor3v(color);
	GLUtils::GLDrawArrow(pos,pos+len*(val/valNorm),color);
}


void ActReactGlRep::render(const shared_ptr<Node>& node, const GLViewInfo* viewInfo){
	Real len0=relSz*viewInfo->sceneRadius;
	Vector3r offset=relOff*viewInfo->sceneRadius*(node->ori.conjugate()*Vector3r::UnitX());
	Vector3r pos=node->pos;
	if(viewInfo->scene->isPeriodic) pos=viewInfo->scene->cell->canonicalizePt(pos);
	if(range && !shearRange) shearRange=range;
	// separate normal component
	if(comp==0 || comp==2){
		Vector3r c=range?range->color(val[0]):CompUtils::scalarOnColorScale(val[0],0,1);
		Real len=len0*abs(val[0])/(range?range->maxAbs(val[0]):1);
		//arr.push_back(Vector3r(len,0,0)*sgn(val[0]));
		renderDoubleArrow(pos,node->ori.conjugate()*Vector3r(sgn(val[0])*len,0,0),/*posStart*/val[0]>0,offset,c);
	}
	// separate shear component
	if(comp==1 || comp==2){
		Real shear=Vector2r(val[1],val[2]).norm();
		Vector3r c=shearRange?shearRange->color(shear):CompUtils::scalarOnColorScale(shear,0,1);
		Real len=len0*shear/(shearRange?shearRange->maxAbs(shear):1);
		renderDoubleArrow(pos,node->ori.conjugate()*Vector3r(0,val[1]/shear*len,val[2]/shear*len),/*posStart*/true,offset,c);
	}
	if(comp==3){
		Real fNorm=val.norm();
		Vector3r c=range?range->color(fNorm):CompUtils::scalarOnColorScale(fNorm,0,1);
		Real len=len0*fNorm/(range?range->maxAbs(fNorm):1);
		renderDoubleArrow(pos,node->ori.conjugate()*(len*val/fNorm),/*posStart*/val[0]>0,offset,c);
	}
}

void ActReactGlRep::renderDoubleArrow(const Vector3r& pos, const Vector3r& arr, bool posStart, const Vector3r& offset, const Vector3r& color){
	if(posStart){ GLUtils::GLDrawArrow(pos+offset,pos+offset+arr,color); GLUtils::GLDrawArrow(pos-offset,pos-offset-arr,color); }
	else        { GLUtils::GLDrawArrow(pos+offset-arr,pos+offset,color); GLUtils::GLDrawArrow(pos-offset+arr,pos-offset,color); }
}

void TensorGlRep::postLoad(TensorGlRep&,void*){
	// symmetrize the tensor
	Matrix3r sym=.5*(val+val.transpose());

	Eigen::SelfAdjointEigenSolver<Matrix3r> eig(sym);
	// multiply rotation matrix (princ) rows by diagonal entries 
	eigVec=eig.eigenvectors();
	eigVal=eig.eigenvalues();
	// non-symmetric tensor in principal coordinates, extract the asymmetric part
	Matrix3r valP=eigVec*val*eigVec.transpose();
	Matrix3r skewM=.5*(valP-valP.transpose());
	skew=2*Vector3r(skewM(1,2),skewM(0,2),skewM(0,1));
};

void TensorGlRep::render(const shared_ptr<Node>& node, const GLViewInfo* viewInfo){

	const int circDiv=20;
	static gleDouble circ[circDiv+2][3]={};

	if(circ[0][0]==0){
		gleSetNumSides(10);
		Real step=2*M_PI/circDiv;
		for(int i=-1; i<circDiv+1; i++){
			circ[i+1][0]=cos(i*step);
			circ[i+1][1]=sin(i*step);
			circ[i+1][2]=0;
		}
	}

	if(range && !skewRange) skewRange=range;

	Vector3r pos=node->pos+(node->hasData<GlData>()?node->getData<GlData>().dGlPos:Vector3r::Zero());

	for(int i:{0,1,2}){
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
		for(int j:{0,1,2}){
			glPushMatrix();
				Eigen::Affine3d T=Eigen::Affine3d::Identity();
				T.translate(pos+(j==0?1:-1)*eigVec.col(i)*torDist).rotate(Quaternionr().setFromTwoVectors(Vector3r::UnitZ(),eigVec.col(i)*(skew[i]>0?-1:1))).scale(torRad1);
				if(j==1) T.rotate(AngleAxisr(M_PI,Vector3r::UnitZ()));
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


void CylGlRep::render(const shared_ptr<Node>& node, const GLViewInfo* viewInfo){
	Real radius=viewInfo->sceneRadius*relSz*(isnan(rad)?1:(rangeRad?rangeRad->norm(rad):1));
	Real ccol=isnan(col)?0:col;
	Vector3r color=(rangeCol?rangeCol->color(ccol):CompUtils::scalarOnColorScale(ccol,0,1));
	Vector3r A=(node->pos+node->ori.conjugate()*Vector3r(xx[0],0,0)), B=(node->pos+node->ori.conjugate()*Vector3r(xx[1],0,0));
	// cerr<<"A="<<A.transpose()<<", B="<<B.transpose()<<", r="<<radius<<endl;
	GLUtils::Cylinder(A,B,radius,color,/*wire*/false,/*caps*/false,/*rad2*/-1,/*slices*/10,2);
}


#endif
