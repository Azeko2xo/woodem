#include<woo/pkg/dem/FlexFacet.hpp>

WOO_PLUGIN(dem,(FlexFacet)(In2_FlexFacet_ElastMat));

void FlexFacet::stepUpdate(){
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
	KK.resize(0,0); // delete stiffness matrix so that it is recreated again
};

void FlexFacet::ensureStiffnessMatrix(const Real& young, const Real& nu, const Real& thickness){
	assert(hasRefConf());
	if(KK.size()==36) return; // 6x6 means the matrix already exists
	// plane stress stiffness matrix
	Matrix3r E;
	E<<1,nu,0, nu,1,0,0,0,(1-nu)/2;
	E*=young/(1-nu*nu);
	// strain-displacement matrix
	Real area=this->getArea();
	const Real& x1(refPos[0]); const Real& y1(refPos[1]); const Real& x2(refPos[2]); const Real& y2(refPos[3]); const Real& x3(refPos[4]); const Real& y3(refPos[5]);
	Eigen::Matrix<Real,3,6> B;
	B<<
		(y2-y3),0      ,(y3-y1),0      ,(y1-y2),0      ,
		0      ,(x3-x2),0      ,(x1-x3),0      ,(x2-x1),
		(x3-x2),(y2-y3),(x1-x3),(y3-y1),(x2-x1),(y1-y2);
	B*=(1/(2*area));
	Real t=(isnan(thickness)?2*this->halfThick:thickness);
	if(/*also covers NaN*/!(t>0)) throw std::runtime_error("FlexFacet::ensureStiffnessMatrix: Facet thickness is not positive!");
	KK.resize(6,6);
	KK=t*area*B.transpose()*E*B;
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

void In2_FlexFacet_ElastMat::go(const shared_ptr<Shape>& sh, const shared_ptr<Material>& m, const shared_ptr<Particle>& particle){
	auto& ff=sh->cast<FlexFacet>();
	if(contacts){
		FOREACH(const Particle::MapParticleContact::value_type& I,particle->contacts){
			const shared_ptr<Contact>& C(I.second); if(!C->isReal()) continue;
			Vector3r F,T,xc;
			// TODO: this should be done more efficiently
			for(int i:{0,1,2}){
				std::tie(F,T,xc)=C->getForceTorqueBranch(particle,/*nodeI*/i,scene);
				F/=3.; T/=3.;
				ff.nodes[i]->getData<DemData>().addForceTorque(F,xc.cross(F)+T);
			}
		}
	}
	ff.stepUpdate();
	// assemble local stiffness matrix, in case it does not exist yet
	ff.ensureStiffnessMatrix(particle->material->cast<ElastMat>().young,nu,thickness);
	// compute nodal forces response here
	Vector6r nodalForces=-(ff.KK*ff.uXy).transpose();
	//LOG_WARN("Nodal forces: "<<nodalForces);
	// apply nodal forces
	for(int i:{0,1,2}){
		//LOG_WARN("    "<<i<<" global: "<<(ff.node->ori.conjugate()*Vector3r(nodalForces[2*i],nodalForces[2*i+1],0)).transpose())
		ff.nodes[i]->getData<DemData>().addForce(ff.node->ori.conjugate()*Vector3r(nodalForces[2*i],nodalForces[2*i+1],0));
		//LOG_WARN("    "<<i<<" written:"<<ff.nodes[i]->getData<DemData>().force);
	}
}

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
