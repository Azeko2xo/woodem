// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#include<yade/pkg/dem/Wall.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#include<limits>

YADE_PLUGIN(dem,(Wall)(Bo1_Wall_Aabb)(In2_Wall_ElastMat));
#ifdef YADE_OPENGL
	YADE_PLUGIN(gl,(Gl1_Wall))
#endif

void Bo1_Wall_Aabb::go(const shared_ptr<Shape>& sh){
	Wall& wall=sh->cast<Wall>();
	if(!wall.bound){ wall.bound=make_shared<Aabb>(); }
	assert(wall.numNodesOk());
	Aabb& aabb=wall.bound->cast<Aabb>();
	if(scene->isPeriodic && scene->cell->hasShear()) throw logic_error(__FILE__ "Walls not supported in sheared cell.");
	const Real& inf=std::numeric_limits<Real>::infinity();
	aabb.min=Vector3r(-inf,-inf,-inf); aabb.max=Vector3r( inf, inf, inf);
	aabb.min[wall.axis]=aabb.max[wall.axis]=sh->nodes[0]->pos[wall.axis];
}

void In2_Wall_ElastMat::go(const shared_ptr<Shape>& sh, const shared_ptr<Material>& m, const shared_ptr<Particle>& particle){
	FOREACH(const Particle::MapParticleContact::value_type& I,particle->contacts){
		const shared_ptr<Contact>& C(I.second); if(!C->isReal()) continue;
		Vector3r F,T,xc;
		boost::tie(F,T,xc)=C->getForceTorqueBranch(particle,/*nodeI*/0,scene);
		sh->nodes[0]->getData<DemData>().addForceTorque(F,/*discard any torque on wall*/Vector3r::Zero());
	}
}


#ifdef YADE_OPENGL
#include<yade/lib/opengl/OpenGLWrapper.hpp>
#include<yade/pkg/gl/Renderer.hpp>
#include<yade/lib/base/CompUtils.hpp>
	int  Gl1_Wall::div=20;
	void Gl1_Wall::go(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2, const GLViewInfo& viewInfo){
		const Wall& wall=shape->cast<Wall>();
		int ax0=wall.axis,ax1=(wall.axis+1)%3,ax2=(wall.axis+2)%3;
		glColor3v(CompUtils::mapColor(shape->getBaseColor()));
		Vector3r a1,b1,a2,b2; // beginnings (a) and endings (b) of lines in both senses (0,1)
		Real mn1=viewInfo.sceneCenter[ax1]-viewInfo.sceneRadius, mn2=viewInfo.sceneCenter[ax2]-viewInfo.sceneRadius;
		Real step=2*viewInfo.sceneRadius/div;
		//cerr<<"center "<<glinfo.sceneCenter<<", radius "<<glinfo.sceneRadius<<", mn["<<ax1<<"]="<<mn1<<", mn["<<ax2<<"]="<<mn2<<endl;
		a1[ax0]=b1[ax0]=a2[ax0]=b2[ax0]=0;
		a1[ax1]=mn1-step; a2[ax2]=mn2-step;
		b1[ax1]=mn1+step*(div+2); b2[ax2]=mn2+step*(div+2);
		glBegin(GL_LINES);
			for(int i=0; i<=div; i++){
				a1[ax2]=b1[ax2]=mn1+i*step;
				a2[ax1]=b2[ax1]=mn2+i*step;
				glVertex3v(a1); glVertex3v(b1);
				glVertex3v(a2); glVertex3v(b2);
			}
		glEnd();
	}

#endif
