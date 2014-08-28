#include<woo/pkg/fem/Membrane.hpp>

WOO_PLUGIN(fem,(Membrane)(In2_Membrane_ElastMat));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_Membrane__CLASS_BASE_DOC_ATTRS_CTOR_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_In2_Membrane_ElastMat__CLASS_BASE_DOC_ATTRS);

// this should go to some shared header
typedef Eigen::Matrix<Real,9,1> Vector9r;

CREATE_LOGGER(Membrane);

#define MEMBRANE_CONDENSE_DKT




void Membrane::stepUpdate(){
	if(!hasRefConf()) setRefConf();
	updateNode();
	computeNodalDisplacements();
}

void Membrane::setRefConf(){
	if(!node) node=make_shared<Node>();
	// set initial node position and orientation
	node->pos=this->getCentroid();
	// for orientation, there is a few choices which one to pick
	enum {INI_CS_SIMPLE=0,INI_CS_NODE0_AT_X};
	const int ini_cs=INI_CS_NODE0_AT_X;
	//const int ini_cs=INI_CS_SIMPLE;
	switch(ini_cs){
		case INI_CS_SIMPLE:
			node->ori.setFromTwoVectors(Vector3r::UnitZ(),this->getNormal());
			break;
		case INI_CS_NODE0_AT_X:{
			Matrix3r T;
			T.col(0)=(nodes[0]->pos-node->pos).normalized(); // QQQ: row->col
			T.col(2)=this->getNormal();
			T.col(1)=T.col(2).cross(T.col(0));
			assert(T.col(0).dot(T.col(2))<1e-12);
			node->ori=Quaternionr(T);
			break;
		}
	};
	// reference nodal positions
	for(int i:{0,1,2}){
		// Vector3r nl=node->ori.conjugate()*(nodes[i]->pos-node->pos); // QQQ
		Vector3r nl=node->glob2loc(nodes[i]->pos);
		assert(nl[2]<1e-6*(max(abs(nl[0]),abs(nl[1])))); // z-coord should be zero
		refPos.segment<2>(2*i)=nl.head<2>();
	}
	// reference nodal rotations
	refRot.resize(3);
	for(int i:{0,1,2}){
		// facet node orientation minus vertex node orientation, in local frame (read backwards)
		/// QQQ: refRot[i]=node->ori*nodes[i]->ori.conjugate();
		refRot[i]=nodes[i]->ori.conjugate()*node->ori;
		//LOG_WARN("refRot["<<i<<"]="<<AngleAxisr(refRot[i]).angle()<<"/"<<AngleAxisr(refRot[i]).axis().transpose());
	};
	// set displacements to zero
	uXy=phiXy=Vector6r::Zero();
	#ifdef MEMBRANE_DEBUG_ROT
		drill=Vector3r::Zero();
		currRot.resize(3);
	#endif
	// delete stiffness matrices to force their re-creating
	KKcst.resize(0,0); 
	KKdkt.resize(0,0);
};

void Membrane::ensureStiffnessMatrices(const Real& young, const Real& nu, const Real& thickness,bool bending, const Real& bendThickness){
	assert(hasRefConf());
	// do nothing if both matrices exist already
	if(KKcst.size()==36 && (!bending ||
		#ifdef MEMBRANE_CONDENSE_DKT		
			KKdkt.size()==54
		#else 
			KKdkt.size()==81
		#endif
	)) return; 
	// check thickness
	Real t=(isnan(thickness)?2*this->halfThick:thickness);
	if(/*also covers NaN*/!(t>0)) throw std::runtime_error("Membrane::ensureStiffnessMatrices: Facet thickness is not positive!");

	// plane stress stiffness matrix
	Matrix3r E;
	E<<1,nu,0, nu,1,0,0,0,(1-nu)/2;
	E*=young/(1-pow(nu,2));

	// strain-displacement matrix (CCT element)
	Real area=this->getArea();
	const Real& x1(refPos[0]); const Real& y1(refPos[1]); const Real& x2(refPos[2]); const Real& y2(refPos[3]); const Real& x3(refPos[4]); const Real& y3(refPos[5]);

	// Felippa: Introduction to FEM eq. (15.17), pg. 259
	Eigen::Matrix<Real,3,6> B;
	B<<
		(y2-y3),0      ,(y3-y1),0      ,(y1-y2),0      ,
		0      ,(x3-x2),0      ,(x1-x3),0      ,(x2-x1),
		(x3-x2),(y2-y3),(x1-x3),(y3-y1),(x2-x1),(y1-y2);
	B*=1/(2*area);
	KKcst.resize(6,6);
	KKcst=t*area*B.transpose()*E*B;

	if(!bending) return;

	// strain-displacement matrix (DKT element)

	Real dktT=(isnan(bendThickness)?t:bendThickness);
	assert(!isnan(dktT));

	// [Batoz,Bathe,Ho: 1980], Appendix A: Expansions for the DKT element
	// (using the same notation)
	Real x23=(x2-x3), x31(x3-x1), x12(x1-x2);
	Real y23=(y2-y3), y31(y3-y1), y12(y1-y2);
	Real l23_2=(pow(x23,2)+pow(y23,2)), l31_2=(pow(x31,2)+pow(y31,2)), l12_2=(pow(x12,2)+pow(y12,2));
	Real P4=-6*x23/l23_2, P5=-6*x31/l31_2, P6=-6*x12/l12_2;
	Real t4=-6*y23/l23_2, t5=-6*y31/l31_2, t6=-6*y12/l12_2;
	Real q4=3*x23*y23/l23_2, q5=3*x31*y31/l31_2, q6=3*x12*y12/l12_2;
	Real r4=3*pow(y23,2)/l23_2, r5=3*pow(y31,2)/l31_2, r6=3*pow(y12,2)/l12_2;

	// even though w1, w2, w3 are always zero due to our definition of triangle plane,
	// we need to get the corresponding nodal force, therefore cannot condense those DOFs away.

	auto Hx_xi=[&](const Real& xi, const Real& eta) -> Vector9r { Vector9r ret; ret<<
		P6*(1-2*xi)+(P5-P6)*eta,
		q6*(1-2*xi)-(q5+q6)*eta,
		-4+6*(xi+eta)+r6*(1-2*xi)-eta*(r5+r6),
		-P6*(1-2*xi)+eta*(P4+P6),
		q6*(1-2*xi)-eta*(q6-q4),
		-2+6*xi+r6*(1-2*xi)+eta*(r4-r6),
		-eta*(P5+P4),
		eta*(q4-q5),
		-eta*(r5-r4);
		return ret;
	};

	auto Hy_xi=[&](const Real& xi, const Real &eta) -> Vector9r { Vector9r ret; ret<<
		t6*(1-2*xi)+eta*(t5-t6),
		1+r6*(1-2*xi)-eta*(r5+r6),
		-q6*(1-2*xi)+eta*(q5+q6),
		-t6*(1-2*xi)+eta*(t4+t6),
		-1+r6*(1-2*xi)+eta*(r4-r6),
		-q6*(1-2*xi)-eta*(q4-q6),
		-eta*(t4+t5),
		eta*(r4-r5),
		-eta*(q4-q5);
		return ret;
	};

	auto Hx_eta=[&](const Real& xi, const Real& eta) -> Vector9r { Vector9r ret; ret<<
		-P5*(1-2*eta)-xi*(P6-P5),
		q5*(1-2*eta)-xi*(q5+q6),
		-4+6*(xi+eta)+r5*(1-2*eta)-xi*(r5+r6),
		xi*(P4+P6),
		xi*(q4-q6),
		-xi*(r6-r4),
		P5*(1-2*eta)-xi*(P4+P5),
		q5*(1-2*eta)+xi*(q4-q5),
		-2+6*eta+r5*(1-2*eta)+xi*(r4-r5);
		return ret;
	};

	auto Hy_eta=[&](const Real& xi, const Real& eta) -> Vector9r { Vector9r ret; ret<<
		-t5*(1-2*eta)-xi*(t6-t5),
		1+r5*(1-2*eta)-xi*(r5+r6),
		-q5*(1-2*eta)+xi*(q5+q6),
		xi*(t4+t6),
		xi*(r4-r6),
		-xi*(q4-q6),
		t5*(1-2*eta)-xi*(t4+t5),
		-1+r5*(1-2*eta)+xi*(r4-r5),
		-q5*(1-2*eta)-xi*(q4-q5);
		return ret;
	};

	// return B(xi,eta) [Batoz,Bathe,Ho,1980], (30)
	auto Bphi=[&](const Real& xi, const Real& eta) -> Eigen::Matrix<Real,3,9> {
		Eigen::Matrix<Real,3,9> ret;
		ret<<
			(y31*Hx_xi(xi,eta)+y12*Hx_eta(xi,eta)).transpose(),
			(-x31*Hy_xi(xi,eta)-x12*Hy_eta(xi,eta)).transpose(),
			(-x31*Hx_xi(xi,eta)-x12*Hx_eta(xi,eta)+y31*Hy_xi(xi,eta)+y12*Hy_eta(xi,eta)).transpose()
		;
		return ret/(2*area);
	};

	// bending elasticity matrix (the same as (t^3/12)*E, E being the plane stress matrix above)
	Matrix3r Db;
	Db<<1,nu,0, nu,1,0, 0,0,(1-nu)/2;
	Db*=young*pow(dktT,3)/(12*(1-pow(nu,2)));


	// assemble the matrix here
	// KKdkt0 is 9x9, then w_i dofs are condensed away, and KKdkt is only 9x6
	#ifdef MEMBRANE_CONDENSE_DKT
		MatrixXr KKdkt0(9,9); KKdkt0.setZero();
	#else
		KKdkt.setZero(9,9);
	#endif
	// gauss integration points and their weights
	Vector3r xxi(.5,.5,0), eeta(0,.5,.5);
	Real ww[]={1/3.,1/3.,1/3.};
	for(int i:{0,1,2}){
		for(int j:{0,1,2}){
			auto b=Bphi(xxi[i],eeta[j]); // evaluate B at the gauss point
			#ifdef MEMBRANE_CONDENSE_DKT
				KKdkt0
			#else
				KKdkt
			#endif
				+=(2*area*ww[j]*ww[i])*b.transpose()*Db*b;
		}
	}
	#ifdef MEMBRANE_CONDENSE_DKT
		// extract columns [_ 1 2 _ 4 5 _ 7 8]
		KKdkt.setZero(9,6);
		for(int i:{0,1,2}){
			KKdkt.col(2*i)=KKdkt0.col(3*i+1);
			KKdkt.col(2*i+1)=KKdkt0.col(3*i+2);
		}
	#endif
};

void Membrane::addIntraStiffnesses(const shared_ptr<Node>& n,Vector3r& ktrans, Vector3r& krot) const {
	if(!hasRefConf()) return;
	int i=-1;
	for(int j=0; j<3; j++){ if(nodes[j].get()==n.get()){ i=j; break; }}
	if(i<0) throw std::logic_error("Membrane::addIntraStiffness:: node "+n->pyStr()+" not found within nodes of "+this->pyStr()+".");
	if(KKcst.size()==0) return;
	bool dkt=(KKdkt.size()>0);
	// local translational stiffness diagonal
	Vector3r ktl(KKcst(2*i,2*i),KKcst(2*i+1,2*i+1),dkt?abs(KKdkt(3*i)):0);
	ktrans+=node->ori*ktl;
	// local rotational stiffness, if needed
	if(dkt){
		#ifdef MEMBRANE_CONDENSE_DKT
			Vector3r krl(abs(KKdkt(3*i,2*i)),abs(KKdkt(3*i+1,2*i+1)),0);
		#else
			Vector3r krl(abs(KKdkt(3*i,3*i)),abs(KKdkt(3*i+1,3*i+1)),0);
		#endif
		krot+=node->ori*krl;
	}
}




void Membrane::updateNode(){
	assert(hasRefConf());
	node->pos=this->getCentroid();
	// temporary orientation, just to be planar
	// see http://www.colorado.edu/engineering/cas/courses.d/NFEM.d/NFEM.AppC.d/NFEM.AppC.pdf
	Quaternionr ori0; ori0.setFromTwoVectors(Vector3r::UnitZ(),this->getNormal());
	Vector6r nxy0;
	for(int i:{0,1,2}){
		Vector3r xy0=ori0.conjugate()*(nodes[i]->pos-node->pos);
		#ifdef MEMBRANE_DEBUG_ROT
			if(xy0[2]>1e-5*(max(abs(xy0[0]),abs(xy0[1])))){
				LOG_ERROR("z-coordinate is not zero for node "<<i<<": ori0="<<AngleAxisr(ori0).axis()<<" !"<<AngleAxisr(ori0).angle()<<", xy0="<<xy0<<", position in global-oriented centroid-origin CS "<<(nodes[i]->pos-node->pos).transpose());
			}
		#else
			assert(xy0[2]<1e-5*(max(abs(xy0[0]),abs(xy0[1])))); // z-coord should be zero
		#endif
		nxy0.segment<2>(2*i)=xy0.head<2>();
	}
	// compute the best fit (C.8 of the paper)
	// split the fraction into numerator (y) and denominator (x) to get the correct quadrant
	Real tanTheta3_y=refPos[0]*nxy0[1]+refPos[2]*nxy0[3]+refPos[4]*nxy0[5]-refPos[1]*nxy0[0]-refPos[3]*nxy0[2]-refPos[5]*nxy0[4];
	Real tanTheta3_x=refPos.dot(nxy0);
	// rotation to be planar, plus rotation around plane normal to the element CR frame (read backwards)
	node->ori=ori0*AngleAxisr(atan2(tanTheta3_y,tanTheta3_x),Vector3r::UnitZ());
};

void Membrane::computeNodalDisplacements(){
	assert(hasRefConf());
	// supposes node is updated already
	for(int i:{0,1,2}){
		Vector3r xy=node->glob2loc(nodes[i]->pos);
		// relative tolerance of 1e-6 was too little, in some cases?!
		#ifdef MEMBRANE_DEBUG_ROT
			if(xy[2]>1e-5*(max(abs(xy[0]),abs(xy[1])))){
				LOG_ERROR("local z-coordinate is not zero for node "<<i<<": node->ori="<<AngleAxisr(node->ori).axis()<<" !"<<AngleAxisr(node->ori).angle()<<", xy="<<xy<<", position in global-oriented centroid-origin CS "<<(nodes[i]->pos-node->pos).transpose());
			}
		#else
			assert(xy[2]<1e-5*(max(abs(xy[0]),abs(xy[1]))));
		#endif
		// displacements
		uXy.segment<2>(2*i)=xy.head<2>()-refPos.segment<2>(2*i);
		// rotations
		AngleAxisr aa(refRot[i].conjugate()*(nodes[i]->ori.conjugate()*node->ori));
		/* aa sometimes gives angle close to 2π for rotations which are actually small and negative;
			I posted that question at http://forum.kde.org/viewtopic.php?f=74&t=110854 .
			Such a result is fixed by conditionally subtracting 2π:
		*/
		if(aa.angle()>M_PI) aa.angle()-=2*M_PI;
		Vector3r rot=Vector3r(aa.angle()*aa.axis()); // rotation vector in local coords
		// if(aa.angle()>3)
		if(rot.head<2>().squaredNorm()>3*3) LOG_WARN("Membrane's in-plane rotation in a node is > 3 radians, expect unstability!");
		phiXy.segment<2>(2*i)=rot.head<2>(); // drilling rotation discarded
		#ifdef MEMBRANE_DEBUG_ROT
			AngleAxisr rr(refRot[i]);
			AngleAxisr cr(nodes[i]->ori.conjugate()*node->ori);
			LOG_TRACE("node "<<i<<"\n   refRot : "<<rr.axis()<<" !"<<rr.angle()<<"\n   currRot: "<<cr.axis()<<" !"<<cr.angle()<<"\n   diffRot: "<<aa.axis()<<" !"<<aa.angle());
			drill[i]=rot[2];
			currRot[i]=nodes[i]->ori.conjugate()*node->ori;
		#endif
	}
};

CREATE_LOGGER(In2_Membrane_ElastMat);


void In2_Membrane_ElastMat::addIntraStiffnesses(const shared_ptr<Particle>& p, const shared_ptr<Node>& n, Vector3r& ktrans, Vector3r& krot) const {
	auto& ff=p->shape->cast<Membrane>();
	if(!ff.hasRefConf()) ff.setRefConf();
	ff.ensureStiffnessMatrices(p->material->cast<ElastMat>().young,nu,thickness,/*bending*/bending,bendThickness);
	ff.addIntraStiffnesses(n,ktrans,krot);
}


void In2_Membrane_ElastMat::go(const shared_ptr<Shape>& sh, const shared_ptr<Material>& m, const shared_ptr<Particle>& particle, const bool skipContacts){
	auto& ff=sh->cast<Membrane>();
	if(contacts && !skipContacts && !particle->contacts.empty()){
		Vector3r normal=Vector3r::Zero(); // compiler happy
		if(bending||applyBary) normal=ff.getNormal();
		for(const auto& pC: particle->contacts){
			const shared_ptr<Contact>& C(pC.second); if(!C->isReal()) continue;
			Vector3r F,T,xc;
			Vector3r weights;
			if(bending||applyBary){
				// find barycentric coordinates of the projected contact point, and use those as weights
				// I *guess* this would be the solution for linear interpolation anyway
				const Vector3r& c=C->geom->node->pos;
				Vector3r p=c-(c-sh->nodes[0]->pos).dot(normal)*normal;
				weights=CompUtils::triangleBarycentrics(p,sh->nodes[0]->pos,sh->nodes[1]->pos,sh->nodes[2]->pos);
			} else {
				// distribute equally when bending is not considered
				weights=Vector3r::Constant(1/3.);
			}
			// TODO: this could be done more efficiently
			for(int i:{0,1,2}){
				std::tie(F,T,xc)=C->getForceTorqueBranch(particle,/*nodeI*/i,scene);
				F*=weights[i]; T*=weights[i];
				ff.nodes[i]->getData<DemData>().addForceTorque(F,xc.cross(F)+T);
			}
		}
	}
	ff.stepUpdate();
	// assemble local stiffness matrix, in case it does not exist yet
	ff.ensureStiffnessMatrices(particle->material->cast<ElastMat>().young,nu,thickness,/*bending*/bending,bendThickness);
	// compute nodal forces response here
	// ?? CST forces are applied with the - sign, DKT with the + sign; are uXy/phiXy introduced differently?
	Vector6r Fcst=-(ff.KKcst*ff.uXy).transpose();


	Vector9r Fdkt;
	if(bending){
		#ifdef MEMBRANE_CONDENSE_DKT
			assert(ff.KKdkt.size()==54);
			Fdkt=(ff.KKdkt*ff.phiXy).transpose();
		#else
			assert(ff.KKdkt.size()==81);
			Vector9r uDkt_;
			uDkt_<<0,ff.phiXy.segment<2>(0),0,ff.phiXy.segment<2>(2),0,ff.phiXy.segment<2>(4);
			Fdkt=(ff.KKdkt*uDkt_).transpose();
			#ifdef MEMBRANE_DEBUG_ROT
				ff.uDkt=uDkt_; // debugging copy, acessible from python
			#endif
		#endif
	} else {
		Fdkt=Vector9r::Zero();
	}
	LOG_TRACE("CST: "<<Fcst.transpose())
	LOG_TRACE("DKT: "<<Fdkt.transpose())
	// surface load, if any
	Real surfLoadForce=0.;
	if(!isnan(ff.surfLoad) && ff.surfLoad!=0.){ surfLoadForce=(1/3.)*ff.getArea()*ff.surfLoad; }
	// apply nodal forces
	for(int i:{0,1,2}){
		Vector3r Fl=Vector3r(Fcst[2*i],Fcst[2*i+1],Fdkt[3*i]+surfLoadForce);
		Vector3r Tl=Vector3r(Fdkt[3*i+1],Fdkt[3*i+2],0);
		ff.nodes[i]->getData<DemData>().addForceTorque(ff.node->ori*Fl,ff.node->ori*Tl);
		LOG_TRACE("  "<<i<<" F: "<<Fl.transpose()<<" \t| "<<ff.node->ori*Fl);
		LOG_TRACE("  "<<i<<" T: "<<Tl.transpose()<<" \t| "<<ff.node->ori*Tl);
	}
}

#ifdef WOO_OPENGL

#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/opengl/GLUtils.hpp>

WOO_PLUGIN(gl,(Gl1_Membrane));


bool Gl1_Membrane::node;
bool Gl1_Membrane::refConf;
Vector3r Gl1_Membrane::refColor;
int Gl1_Membrane::refWd;
Real Gl1_Membrane::uScale;
int Gl1_Membrane::uWd;
bool Gl1_Membrane::uSplit;
Real Gl1_Membrane::relPhi;
int Gl1_Membrane::phiWd;
bool Gl1_Membrane::phiSplit;
bool Gl1_Membrane::arrows;
shared_ptr<ScalarRange> Gl1_Membrane::uRange;
shared_ptr<ScalarRange> Gl1_Membrane::phiRange;

void Gl1_Membrane::drawLocalDisplacement(const Vector2r& nodePt, const Vector2r& xy, const shared_ptr<ScalarRange>& range, bool split, char arrow, int lineWd, const Real z){
	Vector3r nodePt3(nodePt[0],nodePt[1],0);
	if(split){
		Vector3r p1=Vector3r(nodePt[0]+xy[0],nodePt[1],0), c1=range->color(xy[0]), p2=Vector3r(nodePt[0],nodePt[1]+xy[1],0), c2=range->color(xy[1]);
		if(arrow==0){
			glLineWidth(lineWd);
			GLUtils::GLDrawLine(nodePt3,p1,c1);
			GLUtils::GLDrawLine(nodePt3,p2,c2);
		} else {
			// this messes OpenGL coords up!??
			GLUtils::GLDrawArrow(nodePt3,p1,c1,/*doubled*/arrow>1);
			GLUtils::GLDrawArrow(nodePt3,p2,c2,/*doubled*/arrow>1);
		}
	} else {
		Vector3r p1=Vector3r(nodePt[0]+xy[0],nodePt[1]+xy[1],0), c1=range->color(xy.norm());
		if(arrow==0){
			glLineWidth(lineWd);
			GLUtils::GLDrawLine(nodePt3,p1,c1);
		} else {
			// this messes OpenGL coords up!??
			GLUtils::GLDrawArrow(nodePt3,p1,c1,/*doubled*/arrow>1);
		}
	}
	if(!isnan(z)){
		glLineWidth(lineWd);
		GLUtils::GLDrawLine(nodePt3,Vector3r(nodePt[0],nodePt[1],z),range->color(z));
	}
}


void Gl1_Membrane::go(const shared_ptr<Shape>& sh, const Vector3r& shift, bool wire2, const GLViewInfo& viewInfo){
	Gl1_Facet::go(sh,shift,/*don't force wire rendering*/ wire2,viewInfo);
	if(Renderer::fastDraw) return;
	Membrane& ff=sh->cast<Membrane>();
	if(!ff.hasRefConf()) return;
	if(node){
		Renderer::setNodeGlData(ff.node,false/*set refPos only for the first time*/);
		Renderer::renderRawNode(ff.node);
		if(ff.node->rep) ff.node->rep->render(ff.node,&viewInfo);
		#ifdef MEMBRANE_DEBUG_ROT
			// show what Membrane thinks the orientation of nodes is - render those midway
			if(ff.currRot.size()==3){
				for(int i:{0,1,2}){
					shared_ptr<Node> n=make_shared<Node>();
					n->pos=ff.node->pos+.5*(ff.nodes[i]->pos-ff.node->pos);
					n->ori=(ff.currRot[i]*ff.node->ori.conjugate()).conjugate();
					Renderer::renderRawNode(n);
				}
			}
		#endif
	}

	// draw everything in in local coords now
	glPushMatrix();
		if(!Renderer::scaleOn){
			// without displacement scaling, local orientation is easy
			GLUtils::setLocalCoords(ff.node->pos,ff.node->ori);
		} else {
			/* otherwise compute scaled orientation such that
				* +x points to the same point on the triangle (in terms of angle)
				* +z is normal to the facet in the GL space
			*/
			Real phi0=atan2(ff.refPos[1],ff.refPos[0]);
			Vector3r C=ff.getGlCentroid();
			Matrix3r rot;
			rot.row(2)=ff.getGlNormal();
			rot.row(0)=(ff.getGlVertex(0)-C).normalized();
			rot.row(1)=rot.row(2).cross(rot.row(0));
			Quaternionr ori=AngleAxisr(phi0,Vector3r::UnitZ())*Quaternionr(rot);
			GLUtils::setLocalCoords(C,ori.conjugate());
		}

		if(refConf){
			glColor3v(refColor);
			glLineWidth(refWd);
			glBegin(GL_LINE_LOOP);
				for(int i:{0,1,2}) glVertex3v(Vector3r(ff.refPos[2*i],ff.refPos[2*i+1],0));
			glEnd();
		}
		if(uScale!=0){
			glLineWidth(uWd);
			for(int i:{0,1,2}) drawLocalDisplacement(ff.refPos.segment<2>(2*i),uScale*ff.uXy.segment<2>(2*i),uRange,uSplit,arrows?1:0,uWd);
		}
		if(relPhi!=0 && ff.hasBending()){
			glLineWidth(phiWd);
			for(int i:{0,1,2}) drawLocalDisplacement(ff.refPos.segment<2>(2*i),relPhi*viewInfo.sceneRadius*ff.phiXy.segment<2>(2*i),phiRange,phiSplit,arrows?2:0,phiWd
				#ifdef MEMBRANE_DEBUG_ROT
					, relPhi*viewInfo.sceneRadius*ff.drill[i] /* show the out-of-plane component */
				#endif
			);
		}
	glPopMatrix();
}
#endif
