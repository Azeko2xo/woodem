#include<woo/pkg/dem/FlexFacet.hpp>

WOO_PLUGIN(dem,(FlexFacet));

void FlexFacet::pyUpdate(){
	if(!hasRefConf()) setRefConf();
	updateNode();
	computeNodalDisplacements();
}

void FlexFacet::setRefConf(){
	if(!node) node=make_shared<Node>();
	// set initial node position and orientation
	// for now make it just arbitrary (instead of requiring e.g. y0=0)
	node->ori=Quaternionr::FromTwoVectors(this->getNormal(),Vector3r::UnitZ());
	node->pos=this->getCentroid();
	// reference nodal positions
	for(int i:{0,1,2}){
		Vector3r nl=node->ori*nodes[i]->pos;
		assert(nl[2]<1e-6*(max(abs(nl[0]),abs(nl[1])))); // z-coord should be zero
		refPos.segment<2>(2*i)=nl.head<2>();
	}
	// reference nodal rotations
	refRot.resize(3);
	for(int i:{0,1,2}){
		// facet node orientation minus vertex node orientation
		refRot[i]=nodes[i]->ori.conjugate()*node->ori;
	};
	// set displacements to zero
	uXy=phiXy=Vector6r::Zero();
};

void FlexFacet::updateNode(){
	assert(hasRefConf());
	node->pos=this->getCentroid();
	// temporary orientation, just to be planar
	// see http://www.colorado.edu/engineering/cas/courses.d/NFEM.d/NFEM.AppC.d/NFEM.AppC.pdf
	Quaternionr ori0=Quaternionr::FromTwoVectors(this->getNormal(),Vector3r::UnitZ());
	Vector6r nxy0;
	for(int i:{0,1,2}){
		Vector3r xy0=ori0*(nodes[i]->pos-node->pos);
		assert(xy0[2]<1e-6*(max(abs(xy0[0]),abs(xy0[1])))); // z-coord should be zero
		nxy0.segment<2>(2*i)=xy0.head<2>();
	}
	// compute the best fit (C.8 of the paper)
	Real tanTheta3=(refPos[0]*nxy0[1]+refPos[2]*nxy0[3]+refPos[4]*nxy0[5]-refPos[1]*nxy0[0]-refPos[3]*nxy0[2]-refPos[5]*nxy0[4])/(refPos.dot(nxy0));
	// rotation to be planar, plus rotation around plane normal to the element CR frame
	node->ori=AngleAxisr(atan(tanTheta3),Vector3r::UnitZ())*ori0;
};

void FlexFacet::computeNodalDisplacements(){
	assert(hasRefConf());
	// supposes node is updated already
	for(int i:{0,1,2}){
		Vector3r xy=node->ori*(nodes[i]->pos-node->pos);
		assert(xy[2]<1e-6*(max(abs(xy[0]),abs(xy[1]))));
		// displacements
		uXy.segment<2>(2*i)=xy.head<2>()-refPos.segment<2>(2*i);
		// rotations
		AngleAxisr aa(refRot[i].conjugate()*(nodes[i]->ori.conjugate()*node->ori));
		Vector3r rot=node->ori*Vector3r(aa.angle()*aa.axis()); // rotation vector in local coords
		phiXy.segment<2>(2*i)=rot.head<2>(); // drilling rotation discarded
	}
};

#ifdef WOO_OPENGL

#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/opengl/GLUtils.hpp>

WOO_PLUGIN(gl,(Gl1_FlexFacet));


bool Gl1_FlexFacet::node;
bool Gl1_FlexFacet::refConf;
Vector3r Gl1_FlexFacet::refColor;
Real Gl1_FlexFacet::uScale;
int Gl1_FlexFacet::uWd;
bool Gl1_FlexFacet::uSplit;
Real Gl1_FlexFacet::relPhi;
int Gl1_FlexFacet::phiWd;
bool Gl1_FlexFacet::phiSplit;
shared_ptr<ScalarRange> Gl1_FlexFacet::uRange;
shared_ptr<ScalarRange> Gl1_FlexFacet::phiRange;

void Gl1_FlexFacet::go(const shared_ptr<Shape>& sh, const Vector3r& shift, bool wire2, const GLViewInfo& viewInfo){
	Gl1_Facet::go(sh,shift,/*always with wire*/true,viewInfo);
	FlexFacet& ff=sh->cast<FlexFacet>();
	if(!ff.hasRefConf()) return;
	if(node) Renderer::renderRawNode(ff.node);
	// draw everything in in local coords now
	glPushMatrix();
		glTranslatev(ff.node->pos);
		glMultMatrixd(Eigen::Affine3d(Matrix3r(ff.node->ori).transpose()).data());
		Vector3r refPt[3];
		for(int i:{0,1,2}) refPt[i]=Vector3r(ff.refPos[2*i],ff.refPos[2*i+1],0);

		if(refConf){
			glColor3v(refColor);
			glBegin(GL_LINE_LOOP);
				for(int i:{0,1,2}) glVertex3v(refPt[i]);
			glEnd();
		}
		if(uScale!=0){
			glLineWidth(uWd);
			for(int i:{0,1,2})
				if(!uSplit) GLUtils::GLDrawLine(refPt[i],refPt[i]+uScale*Vector3r(ff.uXy[2*i],ff.uXy[2*i+1],0),uRange->color(ff.uXy.segment<2>(2*i).norm()));
				else{
					GLUtils::GLDrawLine(refPt[i],refPt[i]+uScale*Vector3r(ff.uXy[2*i],0,0),uRange->color(ff.uXy[2*i]));
					GLUtils::GLDrawLine(refPt[i],refPt[i]+uScale*Vector3r(0,ff.uXy[2*i+1],0),uRange->color(ff.uXy[2*i+1]));
				}
		}
		if(relPhi!=0){
			glLineWidth(phiWd);
			for(int i:{0,1,2}){
				if(!phiSplit) GLUtils::GLDrawLine(refPt[i],refPt[i]+relPhi*viewInfo.sceneRadius*Vector3r(ff.phiXy[2*i],ff.phiXy[2*i+1],0),phiRange->color(ff.phiXy.segment<2>(2*i).norm()));
				else{
					GLUtils::GLDrawLine(refPt[i],refPt[i]+relPhi*viewInfo.sceneRadius*Vector3r(ff.phiXy[2*i],0,0),phiRange->color(ff.phiXy[2*i]));
					GLUtils::GLDrawLine(refPt[i],refPt[i]+relPhi*viewInfo.sceneRadius*Vector3r(0,ff.phiXy[2*i+1],0),phiRange->color(ff.phiXy[2*i+1]));
				}
			}
		}
	glPopMatrix();
}
#endif
