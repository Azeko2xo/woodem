#include<woo/pkg/dem/Facet.hpp>
#include<woo/lib/base/CompUtils.hpp>
WOO_PLUGIN(dem,(Facet)(Bo1_Facet_Aabb));
#ifdef WOO_OPENGL
WOO_PLUGIN(gl,(Gl1_Facet));
#endif

Vector3r Facet::getNormal() const {
	assert(numNodesOk());
	return ((nodes[1]->pos-nodes[0]->pos).cross(nodes[2]->pos-nodes[0]->pos)).normalized();
}

Real Facet::getArea() const {
	assert(numNodesOk());
	const Vector3r& A=nodes[0]->pos;
	const Vector3r& B=nodes[1]->pos;
	const Vector3r& C=nodes[2]->pos;
	return .5*((B-A).cross(C-A)).norm();
}

std::tuple<Vector3r,Vector3r,Vector3r> Facet::getOuterVectors() const {
	assert(numNodesOk());
	// is not normalized
	Vector3r nn=(nodes[1]->pos-nodes[0]->pos).cross(nodes[2]->pos-nodes[0]->pos);
	return std::make_tuple((nodes[1]->pos-nodes[0]->pos).cross(nn),(nodes[2]->pos-nodes[1]->pos).cross(nn),(nodes[0]->pos-nodes[2]->pos).cross(nn));
}

vector<Vector3r> Facet::outerEdgeNormals() const{
	auto o=getOuterVectors();
	return {std::get<0>(o).normalized(),std::get<1>(o).normalized(),std::get<2>(o).normalized()};
}


std::tuple<Vector3r,Vector3r> Facet::interpolatePtLinAngVel(const Vector3r& x) const {
	assert(numNodesOk());
	Vector3r a=CompUtils::facetBarycentrics(x,nodes[0]->pos,nodes[1]->pos,nodes[2]->pos);
	Vector3r vv[3]={nodes[0]->getData<DemData>().vel,nodes[1]->getData<DemData>().vel,nodes[2]->getData<DemData>().vel};
	Vector3r linVel=a[0]*vv[0]+a[1]*vv[1]+a[2]*vv[2];
	Vector3r angVel=(nodes[0]->pos-x).cross(vv[0])+(nodes[1]->pos-x).cross(vv[1])+(nodes[2]->pos-x).cross(vv[2]);
	return std::make_tuple(linVel,angVel);
}


void Bo1_Facet_Aabb::go(const shared_ptr<Shape>& sh){
	Facet& f=sh->cast<Facet>();
	if(!f.bound){ f.bound=make_shared<Aabb>(); }
	Aabb& aabb=f.bound->cast<Aabb>();
	const Vector3r halfThickVec=Vector3r::Constant(f.halfThick);
	if(!scene->isPeriodic){
		aabb.min=f.nodes[0]->pos-halfThickVec;
		aabb.max=f.nodes[0]->pos+halfThickVec;
		for(int i:{1,2}){
			aabb.min=aabb.min.array().min((f.nodes[i]->pos-halfThickVec).array()).matrix();
			aabb.max=aabb.max.array().max((f.nodes[i]->pos+halfThickVec).array()).matrix();
		}
	} else {
		// periodic cell: unshear everything
		aabb.min=scene->cell->unshearPt(f.nodes[0]->pos)-halfThickVec;
		aabb.max=scene->cell->unshearPt(f.nodes[0]->pos)+halfThickVec;
		for(int i:{1,2}){
			Vector3r v=scene->cell->unshearPt(f.nodes[i]->pos);
			aabb.min=aabb.min.array().min((v-halfThickVec).array()).matrix();
			aabb.max=aabb.max.array().max((v+halfThickVec).array()).matrix();
		}
	}
}


#ifdef WOO_OPENGL
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/base/CompUtils.hpp>

void halfCylinder(const Vector3r& A, const Vector3r& B, Real radius, const Vector3r& upVec, const Vector3r& color, bool wire, int slices=12, int stacks=1, Real connectorAngle=0.){
	if(wire) glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	if(!isnan(color[0]))	glColor3v(color);
	Real len=(B-A).norm();
	Vector3r axis=(B-A)/len;
	Matrix3r T; T.col(0)=axis; T.col(1)=upVec.cross(axis); T.col(2)=upVec;
	glPushMatrix();
		GLUtils::setLocalCoords(A,Quaternionr(T));
		slices=max(1,slices/2); stacks=max(1,stacks);
		auto makeNormCoords=[&](int sl, int st)->Vector3r{ return Vector3r((st*1./stacks),-sin(sl*1./slices*M_PI),-cos(sl*1./slices*M_PI)); };
		for(int stack=0; stack<stacks; stack++){
			glBegin(GL_QUAD_STRIP);
				for(int slice=0; slice<=slices; slice++){
					for(int dStack:{0,1}){
						Vector3r nc=makeNormCoords(slice,stack+dStack);
						if(!wire) glNormal3v(Vector3r(0,nc[1],nc[2]));
						glVertex3v(nc.cwiseProduct(Vector3r(len,radius,radius)).eval());
					}
				}
			glEnd();
		}
		// sphere wedge connector
		if(connectorAngle!=0){
			glTranslatev(Vector3r(len,0,0)); // move to the far end
			stacks=slices; // what were slices for cylinder are stacks for the sphere
			slices=stacks*(abs(connectorAngle)/M_PI); // compute slices to make the sphere round as te 
			auto makeNormCoords=[&](int sl, int st)->Vector3r{ return Vector3r(sin(sl*1./slices*connectorAngle)*sin(st*1./stacks*M_PI),-cos(-sl*1./slices*connectorAngle)*sin(st*1./stacks*M_PI),-cos(st*1./stacks*M_PI)); };
			for(int slice=0; slice<slices; slice++){
				glBegin(GL_QUAD_STRIP);
					for(int stack=0; stack<=stacks; stack++){
						for(int dSlice:{0,1}){
							Vector3r nc=makeNormCoords(slice+dSlice,stack);
							if(!wire) glNormal3v(nc);
							glVertex3v((radius*nc).eval());
						}
					}
				glEnd();
			}
		}
	glPopMatrix();
	if(wire) glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}

bool Gl1_Facet::wire;
int Gl1_Facet::slices;

void Gl1_Facet::drawEdges(const Facet& f, const Vector3r& facetNormal, bool wire){
	if(slices>=4){
		// draw half-cylinders and caps
		Vector3r edges[]={(f.nodes[1]->pos-f.nodes[0]->pos).normalized(),(f.nodes[2]->pos-f.nodes[1]->pos).normalized(),(f.nodes[0]->pos-f.nodes[2]->pos).normalized()};
		for(int i:{0,1,2}){
			const Vector3r& A(f.nodes[i]->pos), B(f.nodes[(i+1)%3]->pos);
			halfCylinder(A,B,f.halfThick,facetNormal,Vector3r(NaN,NaN,NaN),wire,slices,1,acos(edges[(i)%3].dot(edges[(i+1)%3])));
		}
	}else{
		// just the edges
		Vector3r dz=f.halfThick*facetNormal;
		if(wire){
			const Vector3r &A(f.nodes[0]->pos), &B(f.nodes[1]->pos), &C(f.nodes[2]->pos);
			Vector3r vv[]={A+dz,B+dz,C+dz,A+dz,A-dz,B-dz,B+dz,B-dz,C-dz,C+dz,C-dz,A-dz};
			glBegin(GL_LINE_LOOP);
				for(const auto& v: vv) glVertex3v(v);
			glEnd();
		} else {
			glBegin(GL_QUADS);
				for(int edge:{0,1,2}){
					const Vector3r &A(f.nodes[edge]->pos), &B(f.nodes[(edge+1)%3]->pos);
					glNormal3v((B-A).cross(facetNormal).normalized());
					Vector3r vv[]={A-dz,A+dz,B+dz,B-dz};
					for(const Vector3r& v: vv) glVertex3v(v);
				}
			glEnd();
		}
	}
}

void Gl1_Facet::go(const shared_ptr<Shape>& sh, const Vector3r& shift, bool wire2, const GLViewInfo&){   
	Facet& f=sh->cast<Facet>();

	if(wire || wire2){
		glDisable(GL_LINE_SMOOTH);
		if(f.halfThick==0){
			glBegin(GL_LINE_LOOP);
				for(int i:{0,1,2}) glVertex3v(f.nodes[i]->pos);
		   glEnd();
		} else {
			// draw noting inside, just the boundary
			drawEdges(f,f.getNormal(),true);
		}
		glEnable(GL_LINE_SMOOTH);
	} else {
		Vector3r normal=f.getNormal();
		// this makes every triangle different WRT the light direction; important for shading
		if(f.halfThick==0){
			glDisable(GL_CULL_FACE); 
			glBegin(GL_TRIANGLES);
				glNormal3v(normal);
				for(int i:{0,1,2}) glVertex3v(f.nodes[i]->pos);
			glEnd();
		} else {
			glDisable(GL_CULL_FACE); 
			glBegin(GL_TRIANGLES);
				glNormal3v(normal);
				for(int i:{0,1,2}) glVertex3v((f.nodes[i]->pos+1*normal*f.halfThick).eval());
				glNormal3v((-normal).eval());
				for(int i:{0,2,1}) glVertex3v((f.nodes[i]->pos-1*normal*f.halfThick).eval());
			glEnd();
			drawEdges(f,normal,false);
		}
	}
}

#endif /* WOO_OPENGL */

