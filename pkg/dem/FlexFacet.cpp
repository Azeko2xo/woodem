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
	node->pos=this->getCentroid();
	// for orientation, there is a few choices which one to pick
	enum {INI_CS_SIMPLE=0,INI_CS_NODE0_AT_X};
	const int ini_cs=INI_CS_NODE0_AT_X;
	//const int ini_cs=INI_CS_SIMPLE;
	switch(ini_cs){
		case INI_CS_SIMPLE:
			node->ori=Quaternionr::FromTwoVectors(this->getNormal(),Vector3r::UnitZ());
			break;
		case INI_CS_NODE0_AT_X:{
			Matrix3r T;
			T.row(0)=(nodes[0]->pos-node->pos).normalized();
			T.row(2)=this->getNormal();
			T.row(1)=T.row(2).cross(T.row(0));
			assert(T.row(0).dot(T.row(2))<1e-12);
			node->ori=Quaternionr(T);
			break;
		}
	};
	// reference nodal positions
	for(int i:{0,1,2}){
		Vector3r nl=node->ori*(nodes[i]->pos-node->pos);
		assert(nl[2]<1e-6*(max(abs(nl[0]),abs(nl[1])))); // z-coord should be zero
		refPos.segment<2>(2*i)=nl.head<2>();
	}
	// reference nodal rotations
	refRot.resize(3);
	for(int i:{0,1,2}){
		// facet node orientation minus vertex node orientation, in local frame (read backwards)
		refRot[i]=quatDiffInNodeCS(nodes[i]->ori);
		//LOG_WARN("refRot["<<i<<"]="<<AngleAxisr(refRot[i]).angle()<<"/"<<AngleAxisr(refRot[i]).axis().transpose());
	};
	// set displacements to zero
	uXy=phiXy=Vector6r::Zero();
	#ifdef FLEXFACET_DEBUG_ROT
		drill=Vector3r::Zero();
		currRot.resize(3);
	#endif
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
	// split the fraction into numerator (y) and denominator (x) to get the correct quadrant
	Real tanTheta3_y=refPos[0]*nxy0[1]+refPos[2]*nxy0[3]+refPos[4]*nxy0[5]-refPos[1]*nxy0[0]-refPos[3]*nxy0[2]-refPos[5]*nxy0[4];
	Real tanTheta3_x=refPos.dot(nxy0);
	// rotation to be planar, plus rotation around plane normal to the element CR frame
	node->ori=AngleAxisr(-atan2(tanTheta3_y,tanTheta3_x),Vector3r::UnitZ())*ori0;
};

void FlexFacet::computeNodalDisplacements(){
	assert(hasRefConf());
	// supposes node is updated already
	for(int i:{0,1,2}){
		Vector3r xy=node->ori*(nodes[i]->pos-node->pos);
		//LOG_WARN("node "<<i<<": xy="<<xy);
		assert(xy[2]<1e-6*(max(abs(xy[0]),abs(xy[1]))));
		// displacements
		uXy.segment<2>(2*i)=xy.head<2>()-refPos.segment<2>(2*i);
		// rotations
		AngleAxisr aa(refRot[i]*quatDiffInNodeCS(nodes[i]->ori).conjugate());
		Vector3r rot=Vector3r(aa.angle()*aa.axis()); // rotation vector in local coords
		phiXy.segment<2>(2*i)=rot.head<2>(); // drilling rotation discarded
		#ifdef FLEXFACET_DEBUG_ROT
			drill[i]=rot[2];
			currRot[i]=quatDiffInNodeCS(nodes[i]->ori);
		#endif
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
bool Gl1_FlexFacet::arrows;
shared_ptr<ScalarRange> Gl1_FlexFacet::uRange;
shared_ptr<ScalarRange> Gl1_FlexFacet::phiRange;

void Gl1_FlexFacet::drawLocalDisplacement(const Vector2r& nodePt, const Vector2r& xy, const shared_ptr<ScalarRange>& range, bool split, char arrow, int lineWd, const Real z){
	Vector3r nodePt3(nodePt[0],nodePt[1],0);
	if(split){
		Vector3r p1=Vector3r(nodePt[0]+xy[0],nodePt[1],0), c1=range->color(xy[0]), p2=Vector3r(nodePt[0],nodePt[1]+xy[1],0), c2=range->color(xy[1]);
		if(arrow==0){
			glLineWidth(lineWd);
			GLUtils::GLDrawLine(nodePt3,p1,c1);
			GLUtils::GLDrawLine(nodePt3,p2,c2);
		} else {
			GLUtils::GLDrawArrow(nodePt3,p1,c1,/*doubled*/arrow>1);
			GLUtils::GLDrawArrow(nodePt3,p2,c2,/*doubled*/arrow>1);
		}
	} else {
		Vector3r p1=Vector3r(nodePt[0]+xy[0],nodePt[1]+xy[1],0), c1=range->color(xy.norm());
		if(arrow==0){
			glLineWidth(lineWd);
			GLUtils::GLDrawLine(nodePt3,p1,c1);
		} else {
			GLUtils::GLDrawArrow(nodePt3,p1,c1,/*doubled*/arrow>1);
		}
	}
	if(!isnan(z)){
		glLineWidth(lineWd);
		GLUtils::GLDrawLine(nodePt3,Vector3r(nodePt[0],nodePt[1],z),range->color(z));
	}
}


void Gl1_FlexFacet::go(const shared_ptr<Shape>& sh, const Vector3r& shift, bool wire2, const GLViewInfo& viewInfo){
	Gl1_Facet::go(sh,shift,/*always with wire*/true,viewInfo);
	FlexFacet& ff=sh->cast<FlexFacet>();
	if(!ff.hasRefConf()) return;
	if(node){
		Renderer::renderRawNode(ff.node);
		if(ff.node->rep) ff.node->rep->render(ff.node,&viewInfo);
		#ifdef FLEXFACET_DEBUG_ROT
			// show what FlexFacet thinks the orientation of nodes is - render those midway
			if(ff.currRot.size()==3){
				for(int i:{0,1,2}){
					shared_ptr<Node> n=make_shared<Node>();
					n->pos=ff.node->pos+.5*(ff.nodes[i]->pos-ff.node->pos);
					n->ori=ff.currRot[i].conjugate()*ff.node->ori;
					Renderer::renderRawNode(n);
				}
			}
		#endif
	}

	// draw everything in in local coords now
	glPushMatrix();
		GLUtils::setLocalCoords(ff.node->pos,ff.node->ori.conjugate());

		if(refConf){
			glColor3v(refColor);
			glBegin(GL_LINE_LOOP);
				for(int i:{0,1,2}) glVertex3v(Vector3r(ff.refPos[2*i],ff.refPos[2*i+1],0));
			glEnd();
		}
		if(uScale!=0){
			glLineWidth(uWd);
			for(int i:{0,1,2}) drawLocalDisplacement(ff.refPos.segment<2>(2*i),uScale*ff.uXy.segment<2>(2*i),uRange,uSplit,arrows?1:0,uWd);
		}
		if(relPhi!=0){
			glLineWidth(phiWd);
			for(int i:{0,1,2}) drawLocalDisplacement(ff.refPos.segment<2>(2*i),relPhi*viewInfo.sceneRadius*ff.phiXy.segment<2>(2*i),phiRange,phiSplit,arrows?2:0,phiWd
				#ifdef FLEXFACET_DEBUG_ROT
					, relPhi*viewInfo.sceneRadius*ff.drill[i] /* show the out-of-plane component */
				#endif
			);
		}
	glPopMatrix();
}
#endif
