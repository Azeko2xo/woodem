#include<woo/pkg/fem/Tetra.hpp>
#include<Eigen/Eigenvalues>
#include<woo/lib/base/Volumetric.hpp>

#ifdef WOO_OPENGL
	#include<woo/lib/opengl/OpenGLWrapper.hpp>
	#include<woo/lib/opengl/GLUtils.hpp>
	#include<woo/pkg/gl/Renderer.hpp>
	#include<woo/pkg/gl/GlData.hpp>
	#include<woo/lib/base/CompUtils.hpp>
#endif


WOO_PLUGIN(fem,(Tetra)(Tet4)(Bo1_Tetra_Aabb)(In2_Tet4_ElastMat));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_fem_Tetra__CLASS_BASE_DOC_ATTRS_CTOR_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_fem_Tet4__CLASS_BASE_DOC_ATTRS_CTOR_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_In2_Tet4_ElastMat__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC(woo_fem_Bo1_Tetra_Aabb__CLASS_BASE_DOC);

#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_Tetra)(Gl1_Tet4));
#endif

WOO_IMPL_LOGGER(Tetra);

void Tetra::selfTest(const shared_ptr<Particle>& p){
	if(!numNodesOk()) throw std::runtime_error("Tetra #"+to_string(p->id)+": numNodesOk() failed (has "+to_string(nodes.size())+" nodes)");
	for(int i:{0,1,2,3}) if((nodes[i]->pos-nodes[(i+1)%4]->pos).squaredNorm()==0) throw std::runtime_error("Tetra #"+to_string(p->id)+": nodes "+to_string(i)+" and "+to_string((i+1)%4)+" are coincident.");
}

Vector3r Tetra::getCentroid() const {
	return (1/4.)*(nodes[0]->pos+nodes[1]->pos+nodes[2]->pos+nodes[3]->pos);
}

Real Tetra::getVolume() const {
	Eigen::Matrix<Real,4,4> Jt; // transposed jacobian
	Jt<<1,nodes[0]->pos.transpose(),1,nodes[1]->pos.transpose(),1,nodes[2]->pos.transpose(),1,nodes[3]->pos.transpose();
	return (1/6.)*Jt.transpose().determinant();
};

void Tetra::canonicalizeVertexOrder() {
	if(!numNodesOk()) return;
	const Vector3r& v1(nodes[0]->pos); const Vector3r& v2(nodes[1]->pos); const Vector3r& v3(nodes[2]->pos); const Vector3r& v4(nodes[3]->pos);
	if(((v2-v1).cross(v3-v2)).dot(v4-v1)<0){
		std::swap(nodes[2],nodes[3]);
	}
}

void Tetra::lumpMassInertia(const shared_ptr<Node>& n, Real density, Real& mass, Matrix3r& I, bool& rotateOk){
	checkNodesHaveDemData();
	rotateOk=true;
	auto N=std::find_if(nodes.begin(),nodes.end(),[&n](const shared_ptr<Node>& n2){ return n.get()==n2.get(); });
	if(N==nodes.end()) return;
	size_t ix=N-nodes.begin();
	// mid-points in local coordinates
	Vector3r vv[3]; for(int i:{1,2,3}) vv[i-1]=.5*n->glob2loc(nodes[(ix+i)%4]->pos);
	// tetrahedron centroid in local coords; 2 since vv are mid-points, muliply by 2 to get vertices
	Vector3r C=n->glob2loc(getCentroid());
	// near and far sub-tetra vertices
	Vector3r vvN[]={Vector3r::Zero(),vv[0],vv[1],vv[2]};
	Vector3r vvF[]={vv[0],vv[1],vv[2],C};
	Real vN, vF; // near and far volums
	I+=density*woo::Volumetric::tetraInertia_cov(vvN,vN,/*fixSign*/true);
	I+=density*woo::Volumetric::tetraInertia_cov(vvF,vF,/*fixSign*/true);
	mass+=density*vN;
	mass+=density*vF;
}

void Tetra::asRaw(Vector3r& center, Real& radius, vector<shared_ptr<Node>>&nn, vector<Real>& raw) const {
	// this should be circumscribed sphere, but that is too complicated to compute
	// so just use centroid and the largest distance to vertex
	center=getCentroid();
	radius=0; for(int i:{0,1,2,3}) radius=max(radius,(nodes[i]->pos-center).norm());
	for(int i:{0,1,2,3}) Shape::asRaw_helper_coordsFromNode(nn,raw,3*i,/*nodeNum*/i);
	// for(int i:{0,1,2,3}) for(int ax:{0,1,2}) raw[3*i+ax]=nodes[i]->pos[ax];
}

void Tetra::setFromRaw(const Vector3r& center, const Real& radius, vector<shared_ptr<Node>>& nn, const vector<Real>& raw){
	Shape::setFromRaw_helper_checkRaw_makeNodes(raw,12);
	// for(int i:{0,1,2,3}) for(int ax:{0,1,2}) nodes[i]->pos[ax]=raw[3*i+ax]+center[ax];
	for(int i:{0,1,2,4}) nodes[i]=Shape::setFromRaw_helper_nodeFromCoords(nn,raw,3*i);
}


void Bo1_Tetra_Aabb::go(const shared_ptr<Shape>& sh){
	Tetra& t=sh->cast<Tetra>();
	if(!t.bound){ t.bound=make_shared<Aabb>(); /* ignore node rotation*/ sh->bound->cast<Aabb>().maxRot=-1; }
	Aabb& aabb=t.bound->cast<Aabb>();
	const Vector3r padVec=Vector3r::Zero(); // not yet used
	if(!scene->isPeriodic){
		aabb.min=t.nodes[0]->pos-padVec;
		aabb.max=t.nodes[0]->pos+padVec;
		for(int i:{1,2,3}){
			aabb.min=aabb.min.array().min((t.nodes[i]->pos-padVec).array()).matrix();
			aabb.max=aabb.max.array().max((t.nodes[i]->pos+padVec).array()).matrix();
		}
	} else {
		// periodic cell: unshear everything
		aabb.min=scene->cell->unshearPt(t.nodes[0]->pos)-padVec;
		aabb.max=scene->cell->unshearPt(t.nodes[0]->pos)+padVec;
		for(int i:{1,2,3}){
			Vector3r v=scene->cell->unshearPt(t.nodes[i]->pos);
			aabb.min=aabb.min.array().min((v-padVec).array()).matrix();
			aabb.max=aabb.max.array().max((v+padVec).array()).matrix();
		}
	}
}



void Tet4::stepUpdate(){
	if(!hasRefConf()) setRefConf();
	computeCorotatedFrame();
	computeNodalDisplacements();
}

void Tet4::setRefConf(){
	if(!node) node=make_shared<Node>();
	// set initial node position and orientation
	node->pos=this->getCentroid();
	// keep it simple for now, use identity orientation
	node->ori=Quaternionr::Identity();
	refPos.resize(3,4);
	for(int i:{0,1,2,3}){
		refPos.col(i)=node->glob2loc(nodes[i]->pos);
	}
	// set displacements to zero
	uXyz=VectorXr::Zero(12);
}


void Tet4::computeNodalDisplacements(){
	for(int i:{0,1,2,3}){
		uXyz.segment<3>(3*i)=node->glob2loc(nodes[i]->pos)-refPos.col(i);
	}
}

void Tet4::computeCorotatedFrame(){
	node->pos=this->getCentroid();
	// notation and procedure follows Mostafa, Sivaselvan: Best-fit corotated frames
	typedef Eigen::Matrix<Real,4,3> Matrix43r; 
	typedef Eigen::Matrix<Real,4,4> Matrix4r; 
	typedef Eigen::Matrix<Real,4,1> Vector4r;
	// pg 114, proc 1, 1.
	auto C=refPos.transpose(); // we store refPos transposed, adjust here; the expression does not do a copy
	assert(C.rows()==4 && C.cols()==3);
	// pg 114, proc 1, 3.
	Matrix43r D; assert(D.rows()==C.rows() && D.cols()==C.cols());
	for(int i:{0,1,2,3}) D.row(i)=(nodes[i]->pos-node->pos).transpose(); 
	// pg 126, (A.1)
	auto crossSkewSymm=[](const Vector3r& a)->Matrix3r{ return (Matrix3r()<<0,-a[2],a[1],a[2],0,-a[0],-a[1],a[0],0).finished(); };
	// pg 109, (7)
	auto betaImag=[&crossSkewSymm](const Vector3r& cn, const Vector3r& dn)->Matrix4r{
		Matrix4r dnL, cnR; 
		// pg 126, (A.4)-(A.5);
		dnL<<0,dn.transpose(),dn, crossSkewSymm(dn);
		cnR<<0,cn.transpose(),cn,-crossSkewSymm(cn);
		return dnL-cnR;
	};
	// TODO: B_CD is symmetric, perhaps use self-adjoint view for some speedup??
	// pg 109, (8b)
	Eigen::Matrix<Real,4,4> B_CD; B_CD.setZero();
	for(int i:{0,1,2,3}){
		Matrix4r betaI=betaImag(C.row(i).transpose(),D.row(i).transpose());
		B_CD+=betaI.transpose()*betaI;
	}
	Eigen::SelfAdjointEigenSolver<Matrix4r> eig(B_CD);
	int i; eig.eigenvalues().minCoeff(&i); // find the minimum eigenvalue
	Vector4r mev=eig.eigenvectors().col(i); // minimum eigenvector
	mev.normalize();
	node->ori=Quaternionr(mev[0],mev[1],mev[2],mev[3]);
};



void Tet4::ensureStiffnessMatrix(Real young, Real nu){
	if(KK.rows()==12 && KK.cols()==12) return;
	// Felippa:: The Linear Tetrahedron
	const Vector3r& p1(nodes[0]->pos); const Vector3r& p2(nodes[1]->pos); const Vector3r& p3(nodes[2]->pos); const Vector3r& p4(nodes[3]->pos);
	#define _x(i,j) (p##i.x()-p##j.x())
	#define _y(i,j) (p##i.y()-p##j.y())
	#define _z(i,j) (p##i.z()-p##j.z())
	typedef Eigen::Matrix<Real,4,1> Vector4r;
	// a,b,c from (9.14), defined in (9.11)
	Vector4r a(
		_y(4,2)*_z(3,2)-_y(3,2)*_z(4,2),
		_y(3,1)*_z(4,3)-_y(3,4)*_z(1,3),
		_y(2,4)*_z(1,4)-_y(1,4)*_z(2,4),
		_y(1,3)*_z(2,1)-_y(1,2)*_z(3,1));
	Vector4r b(
		_x(3,2)*_z(4,2)-_x(4,2)*_z(3,2),
		_x(4,3)*_z(3,1)-_x(1,3)*_z(3,4),
		_x(1,4)*_z(2,4)-_x(2,4)*_z(1,4),
		_x(2,1)*_z(1,3)-_x(3,1)*_z(1,2));
	Vector4r c(
		_x(4,2)*_y(3,2)-_x(3,2)*_y(4,2),
		_x(3,1)*_y(4,3)-_x(3,4)*_y(1,3),
		_x(2,4)*_y(1,4)-_x(1,4)*_y(2,4),
		_x(1,3)*_y(2,1)-_x(1,2)*_y(3,1)
	);
	#undef _x
	#undef _y
	#undef _z
	// (9.33)
	auto bHi=[&](int i)->Matrix3r{ return Vector3r(a[i],b[i],c[i]).asDiagonal(); };
	auto bLo=[&](int i)->Matrix3r{ return (Matrix3r()<<b[i],a[i],0, 0,c[i],b[i], c[i],0,a[i]).finished(); };
	Eigen::Matrix<Real,6,12> B; B<<
		bHi(0),bHi(1),bHi(2),bHi(3),
		bLo(0),bLo(1),bLo(2),bLo(3);
	Real V=getVolume();
	B*=1./(6*V);
	// (9.38)
	Real e=young/((1+nu)*(1-2*nu));
	Matrix6r E; E<<e*(Matrix3r()<<1-nu,nu,nu, nu,1-nu,nu, nu,nu,1-nu).finished(),Matrix3r::Zero(),Matrix3r::Zero(),Matrix3r(Vector3r::Constant(e*(.5-nu)).asDiagonal());
	// (9.40)
	KK=V*B.transpose()*E*B;
	EB=E*B; // this could be perhaps re-used from the previous expression
	#if 0
		cerr<<"E matrix:"<<endl<<E<<endl<<endl;
		cerr<<"B matrix:"<<endl<<B<<endl<<endl;
		cerr<<"K matrix:"<<endl<<KK<<endl<<endl;
	#endif
}

Matrix3r Tet4::getStressTensor() const {
	if(EB.size()!=72) return Matrix3r::Zero();
	Matrix3r sigL((EB*uXyz).block<3,3>(0,0));
	Matrix3r r=node->ori.toRotationMatrix();
	return r*sigL*r.transpose();
}


void Tet4::addIntraStiffnesses(const shared_ptr<Node>& n,Vector3r& ktrans, Vector3r& krot) const {
	if(!hasRefConf()) return;
	int i=-1;
	for(int j:{0,1,2,3}){ if(nodes[j].get()==n.get()){ i=j; break; }}
	if(i<0) throw std::logic_error("Tet4::addIntraStiffness:: node "+n->pyStr()+" not found within nodes of "+this->pyStr()+".");
	if(KK.size()==0) return;
	// add translational DoFs corresponding to this node
	ktrans+=node->ori*(KK.diagonal().segment<3>(3*i));
	// krot untouched
}

void In2_Tet4_ElastMat::addIntraStiffnesses(const shared_ptr<Particle>& p, const shared_ptr<Node>& n, Vector3r& ktrans, Vector3r& krot) const {
	auto& t=p->shape->cast<Tet4>();
	if(!t.hasRefConf()) t.setRefConf();
	t.ensureStiffnessMatrix(p->material->cast<ElastMat>().young,nu);
	t.addIntraStiffnesses(n,ktrans,krot);
}


void In2_Tet4_ElastMat::go(const shared_ptr<Shape>& sh, const shared_ptr<Material>& m, const shared_ptr<Particle>& particle){
	auto& t=sh->cast<Tet4>();
	if(contacts && !particle->contacts.empty()){
		throw std::runtime_error("In2_Tet4_ElastMat: contacts==True not handled yet.");
	}
	t.stepUpdate();
	t.ensureStiffnessMatrix(particle->material->cast<ElastMat>().young,nu);
	typedef Eigen::Matrix<Real,12,1> Vector12r;
	Vector12r F=t.KK*t.uXyz;
	for(int i:{0,1,2,3}){
		Vector3r f=-F.segment<3>(3*i); // negative: displacements opposite convention?!
		t.nodes[i]->getData<DemData>().addForceTorque(t.node->ori*f,Vector3r::Zero());
	}
}






#ifdef WOO_OPENGL
Vector3r Tetra::getGlVertex(int i) const{
	assert(i>=0 && i<=3);
	const auto& n=nodes[i];
	return n->pos+(n->hasData<GlData>()?n->getData<GlData>().dGlPos:Vector3r::Zero());

}
Vector3r Tetra::getGlNormal(int f) const{
	assert(f>=0 && f<4);
	Vector3r a=getGlVertex((f+1)%4), b=getGlVertex((f+2)%4), c=getGlVertex((f+3)%4);
	return ((f%2==1)?-1:1)*((b-a).cross(c-b));
	// return (getGlVertex(1)-getGlVertex(0)).cross(getGlVertex(2)-getGlVertex(0)).normalized();
}

Vector3r Tetra::getGlCentroid() const{
	return (1/4.)*(getGlVertex(0)+getGlVertex(1)+getGlVertex(2)+getGlVertex(3));
}


bool Gl1_Tetra::wire;
int Gl1_Tetra::wd;
Real Gl1_Tetra::fastDrawLim;

void Gl1_Tetra::go(const shared_ptr<Shape>& sh, const Vector3r& shift, bool wire2, const GLViewInfo& viewInfo){   
	Tetra& t=sh->cast<Tetra>();

	if(Renderer::fastDraw && (t.getCentroid()-sh->nodes[0]->pos).squaredNorm()<pow(fastDrawLim*viewInfo.sceneRadius,2)) return;
	Vector3r shifts[4]={shift,shift,shift,shift};
	if(scene->isPeriodic){
		assert(t.nodes[0]->hasData<GlData>() && t.nodes[1]->hasData<GlData>() && t.nodes[2]->hasData<GlData>() && t.nodes[3]->hasData<GlData>());
		GlData* g[4]={&(t.nodes[0]->getData<GlData>()),&(t.nodes[1]->getData<GlData>()),&(t.nodes[2]->getData<GlData>()),&(t.nodes[3]->getData<GlData>())};
		// not all nodes shifted the same; in that case, check where the centroid is, and use that one
		if(!(g[0]->dCellDist==g[1]->dCellDist && g[1]->dCellDist==g[2]->dCellDist && g[2]->dCellDist==g[3]->dCellDist)){
			Vector3i dCell;
			scene->cell->canonicalizePt(t.getCentroid(),dCell);
			for(int i:{0,1,2,4}) shifts[i]+=scene->cell->intrShiftPos(g[i]->dCellDist-dCell);
		}
	}

	if(wire || wire2 || Renderer::fastDraw){
		glDisable(GL_LINE_SMOOTH);
		glLineWidth(wd);
		glBegin(GL_LINE_LOOP);
			for(int i:{0,1,2,3,0,2,1,3}) glVertex3v((t.getGlVertex(i)+shifts[i]).eval());
		glEnd();
		glEnable(GL_LINE_SMOOTH);
	} else {
		glDisable(GL_CULL_FACE); 
		glBegin(GL_TRIANGLES);
			Vector3i vv[]={Vector3i(2,1,0),Vector3i(0,1,3),Vector3i(0,3,2),Vector3i(1,2,3)};
			for(int i:{0,1,2,3}){
				glNormal3v(t.getGlNormal(i));
				const Vector3i& v(vv[i]);
				for(int j:{0,1,2}) glVertex3v((t.getGlVertex(v[j])+shifts[v[j]]).eval());
			}
		glEnd();

	}
}


bool Gl1_Tet4::node;
bool Gl1_Tet4::rep;
bool Gl1_Tet4::refConf;
Vector3r Gl1_Tet4::refColor;
int Gl1_Tet4::refWd;
int Gl1_Tet4::uWd;

void Gl1_Tet4::go(const shared_ptr<Shape>& sh, const Vector3r& shift, bool wire2, const GLViewInfo& viewInfo){
	Gl1_Tetra::go(sh,shift,wire2,viewInfo);
	if(Renderer::fastDraw) return;
	Tet4& t=sh->cast<Tet4>();
	if(!t.node) return;
	if(node || rep){
		Renderer::setNodeGlData(t.node);
		if(node) Renderer::renderRawNode(t.node);
		if(rep && t.node->rep) t.node->rep->render(t.node,&viewInfo);
	}

	if(refConf){
		glPushMatrix();
			GLUtils::setLocalCoords(t.node->pos,t.node->ori);
			glColor3v(refColor);
			glLineWidth(refWd);
			glBegin(GL_LINE_STRIP);
				for(int i:{0,1,2,3,0,2,1,3}) glVertex3v(Vector3r(t.refPos.col(i)));
			glEnd();
		glPopMatrix();
	}
};



#endif /* WOO_OPENGL */
