#ifdef WOO_OPENGL
#include<woo/pkg/dem/Gl1_DemField.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/InfCylinder.hpp>
#include<woo/pkg/dem/Wall.hpp>
//#include<woo/pkg/dem/Tracer.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/base/CompUtils.hpp>

#ifdef WOO_QT4
	#include<QCoreApplication>
	// http://stackoverflow.com/questions/12201823/ipython-and-qt4-c-qcoreapplicationprocessevents-blocks
	#define PROCESS_GUI_EVENTS_SOMETIMES // { static int _i=0; if(guiEvery>0 && _i++>guiEvery){ _i=0; /*QCoreApplication::processEvents();*/ QCoreApplication::sendPostedEvents(); } }
#else
	#define PROCESS_GUI_EVENTS_SOMETIMES
#endif

WOO_PLUGIN(gl,(Gl1_DemField));
CREATE_LOGGER(Gl1_DemField);

bool Gl1_DemField::doPostLoad;
unsigned int Gl1_DemField::mask;
bool Gl1_DemField::wire;
bool Gl1_DemField::bound;
bool Gl1_DemField::shape;
bool Gl1_DemField::nodes;
//bool Gl1_DemField::trace;
//bool Gl1_DemField::_hadTrace;
int Gl1_DemField::cNode;
bool Gl1_DemField::cPhys;
int Gl1_DemField::colorBy;
int Gl1_DemField::prevColorBy;
bool Gl1_DemField::colorSpheresOnly;
Vector3r Gl1_DemField::nonSphereColor;
shared_ptr<ScalarRange> Gl1_DemField::colorRange;
vector<shared_ptr<ScalarRange>> Gl1_DemField::colorRanges;
int Gl1_DemField::glyph;
int Gl1_DemField::vecAxis;
int Gl1_DemField::prevGlyph;
Real Gl1_DemField::glyphRelSz;
shared_ptr<ScalarRange> Gl1_DemField::glyphRange;
vector<shared_ptr<ScalarRange>> Gl1_DemField::glyphRanges;
bool Gl1_DemField::updateRefPos;
int Gl1_DemField::guiEvery;

/*
	Replace, remove or add a range from the scene;
	prev is previous range
	curr is desired range
*/
void Gl1_DemField::setSceneRange(Scene* scene, const shared_ptr<ScalarRange>& prev, const shared_ptr<ScalarRange>& curr){
	if(prev.get()==curr.get()) return; // no change need, do nothing
	int ix=-1;
	if(prev){
		for(size_t i=0; i<scene->ranges.size(); i++){
			if(scene->ranges[i].get()==prev.get()){ ix=i; break; }
		}
	}
	if(curr){
		if(ix>=0) scene->ranges[ix]=curr;
		else scene->ranges.push_back(curr);
	} else {
		if(ix>=0) scene->ranges.erase(scene->ranges.begin()+ix);
	}
}

void Gl1_DemField::initAllRanges(){
	for(int i=0; i<COLOR_SENTINEL; i++){
		auto r=make_shared<ScalarRange>();
		colorRanges.push_back(r);
		switch(i){
			case COLOR_SHAPE:	 		 r->label="Shape.color"; break;
			case COLOR_RADIUS:       r->label="radius"; break;
			case COLOR_VEL:          r->label="vel"; break;
			case COLOR_ANGVEL:       r->label="angVel"; break;
			case COLOR_MASS:         r->label="mass"; break;
			case COLOR_DISPLACEMENT: r->label="displacement"; break;
			case COLOR_ROTATION:     r->label="rotation"; break;
			case COLOR_REFPOS: r->label="ref. pos"; break;
			case COLOR_MAT_ID:       r->label="material id"; break;
		};
	}
	colorRange=colorRanges[colorBy];

	for(int i=0; i<GLYPH_SENTINEL; i++){
		if(i==GLYPH_NONE || i==GLYPH_KEEP){
			glyphRanges.push_back(shared_ptr<ScalarRange>());
			continue;
		}
		auto r=make_shared<ScalarRange>();
		glyphRanges.push_back(r);
		switch(i){
			case GLYPH_FORCE: r->label="force"; break;
			case GLYPH_VEL:   r->label="velocity"; break;
		}
	}
	glyphRange=glyphRanges[glyph];
}

void Gl1_DemField::postLoad2(){
	colorRange=colorRanges[colorBy];
	glyphRange=glyphRanges[glyph];
	bool noPrev=(_lastScene!=scene); // prevColorBy, prevGlyph related to other scene, they don't count therefore
	bool noColor=(!shape || colorBy==COLOR_SHAPE);
	_lastScene=scene;
	setSceneRange(scene,((noPrev||prevColorBy<0)?shared_ptr<ScalarRange>():colorRanges[prevColorBy]),noColor?shared_ptr<ScalarRange>():colorRanges[colorBy]);
	setSceneRange(scene,((noPrev||prevGlyph<0)  ?shared_ptr<ScalarRange>():glyphRanges[prevGlyph]),glyphRanges[glyph]);
	prevColorBy=noColor?-1:colorBy;
	prevGlyph=glyph;
}

void Gl1_DemField::doBound(){
	Renderer::boundDispatcher.scene=scene; Renderer::boundDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->particles->manipMutex);
	FOREACH(const shared_ptr<Particle>& b, *dem->particles){
		// PROCESS_GUI_EVENTS_SOMETIMES; // rendering bounds is usually fast
		if(!b->shape || !b->shape->bound) continue;
		if(mask!=0 && !(b->mask&mask)) continue;
		glPushMatrix(); Renderer::boundDispatcher(b->shape->bound); glPopMatrix();
	}
}

// this function is called for both rendering as well as
// in the selection mode

// nice reading on OpenGL selection
// http://glprogramming.com/red/chapter13.html

void Gl1_DemField::doShape(){
	Renderer::shapeDispatcher.scene=scene; Renderer::shapeDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->particles->manipMutex);

	// instead of const shared_ptr&, get proper shared_ptr;
	// Less efficient in terms of performance, since memory has to be written (not measured, though),
	// but it is still better than crashes if the body gets deleted meanwile.
	FOREACH(shared_ptr<Particle> p, *dem->particles){
		PROCESS_GUI_EVENTS_SOMETIMES;

		if(!p->shape || p->shape->nodes.empty()) continue;
		if(mask!=0 && !(p->mask&mask)) continue;
		const shared_ptr<Shape>& sh=p->shape;

		// sets highlighted color, if the particle is selected
		// last optional arg can be used to provide additional highlight conditions (unused for now)
		const shared_ptr<Node>& n0=p->shape->nodes[0];
		Renderer::glScopedName name(p,n0);
		// bool highlight=(p->id==selId || (p->clumpId>=0 && p->clumpId==selId) || p->shape->highlight);

		FOREACH(const shared_ptr<Node>& n,p->shape->nodes) Renderer::setNodeGlData(n,updateRefPos);

		if(!sh->getVisible()) continue;


		Vector3r parColor;
		bool isSphere;
		if(colorSpheresOnly || COLOR_RADIUS) isSphere=dynamic_pointer_cast<Sphere>(p->shape);
		if(!isSphere && colorSpheresOnly) parColor=nonSphereColor;
		else{
			auto vecNormXyz=[&](const Vector3r& v)->Real{ if(vecAxis<0||vecAxis>2) return v.norm(); return v[vecAxis]; };
			switch(colorBy){
				case COLOR_RADIUS: parColor=isSphere?colorRange->color(p->shape->cast<Sphere>().radius):nonSphereColor; break;
				case COLOR_VEL: parColor=colorRange->color(vecNormXyz(n0->getData<DemData>().vel)); break;
				case COLOR_MASS: parColor=colorRange->color(n0->getData<DemData>().mass); break;
				case COLOR_DISPLACEMENT: parColor=colorRange->color(vecNormXyz(n0->pos-n0->getData<GlData>().refPos)); break;
				case COLOR_ROTATION: {
					AngleAxisr aa(n0->ori.conjugate()*n0->getData<GlData>().refOri);
					parColor=colorRange->color((vecAxis<0||vecAxis>2)?aa.angle():(aa.angle()*aa.axis())[vecAxis]);
					break;
				}
				case COLOR_REFPOS: parColor=colorRange->color(vecNormXyz(n0->getData<GlData>().refPos)); break;
				case COLOR_MAT_ID: parColor=colorRange->color(p->material->id); break;
				case COLOR_SHAPE: parColor=colorRange->color(p->shape->getBaseColor()); break;
				default: parColor=Vector3r(NaN,NaN,NaN);
			}
		}

		glPushMatrix();
			glColor3v(parColor);
			Renderer::shapeDispatcher(p->shape,/*shift*/Vector3r::Zero(),wire||sh->getWire(),*viewInfo);
		glPopMatrix();


		if(name.highlighted){
			const Vector3r& pos=sh->nodes[0]->pos;
			if(!sh->bound || wire || sh->getWire()) GLUtils::GLDrawInt(p->id,pos);
			else {
				// move the label towards the camera by the bounding box so that it is not hidden inside the body
				const Vector3r& mn=sh->bound->min; const Vector3r& mx=sh->bound->max;
				Vector3r ext(Renderer::viewDirection[0]>0?pos[0]-mn[0]:pos[0]-mx[0],Renderer::viewDirection[1]>0?pos[1]-mn[1]:pos[1]-mx[1],Renderer::viewDirection[2]>0?pos[2]-mn[2]:pos[2]-mx[2]); // signed extents towards the camera
				Vector3r dr=-1.01*(Renderer::viewDirection.dot(ext)*Renderer::viewDirection);
				GLUtils::GLDrawInt(p->id,pos+dr,Vector3r::Ones());
			}
		}
		// if the body goes over the cell margin, draw it in positions where the bbox overlaps with the cell in wire
		// precondition: pos is inside the cell.
		if(sh->bound && scene->isPeriodic && Renderer::ghosts){
			const Vector3r& cellSize(scene->cell->getSize());
			//Vector3r pos=scene->cell->unshearPt(.5*(sh->bound->min+sh->bound->max)); // middle of bbox, remove the shear component
			Vector3r pos=.5*(sh->bound->min+sh->bound->max); // middle of bbox, in sheared coords already
			pos-=scene->cell->intrShiftPos(sh->nodes[0]->getData<GlData>().dCellDist); // wrap the same as node[0] was wrapped
			// traverse all periodic cells around the body, to see if any of them touches
			Vector3r halfSize=.5*(sh->bound->max-sh->bound->min);
			Vector3r pmin,pmax;
			Vector3i i;
			// try all positions around, to see if any of them sticks outside the cell boundary
			for(i[0]=-1; i[0]<=1; i[0]++) for(i[1]=-1;i[1]<=1; i[1]++) for(i[2]=-1; i[2]<=1; i[2]++){
				if(i[0]==0 && i[1]==0 && i[2]==0) continue; // middle; already rendered above
				Vector3r pos2=pos+Vector3r(cellSize[0]*i[0],cellSize[1]*i[1],cellSize[2]*i[2]); // shift, but without shear!
				pmin=pos2-halfSize; pmax=pos2+halfSize;
				if(pmin[0]<=cellSize[0] && pmax[0]>=0 &&
					pmin[1]<=cellSize[1] && pmax[1]>=0 &&
					pmin[2]<=cellSize[2] && pmax[2]>=0) {
					Vector3r pt=scene->cell->shearPt(pos2);
					// if(pointClipped(pt)) continue;
					glPushMatrix();
						Renderer::shapeDispatcher(p->shape,/*shift*/pt-pos,/*wire*/true,*viewInfo);
					glPopMatrix();
				}
			}
		}
	}
}

void Gl1_DemField::doNodes(){
	boost::mutex::scoped_lock lock(dem->nodesMutex);

	Renderer::nodeDispatcher.scene=scene; Renderer::nodeDispatcher.updateScenePtr();
	FOREACH(shared_ptr<Node> n, dem->nodes){
		PROCESS_GUI_EVENTS_SOMETIMES;

		Renderer::setNodeGlData(n,updateRefPos);

		if(glyph!=GLYPH_KEEP){
			// prepare rep types
			// vector values
			if(glyph==GLYPH_VEL || glyph==GLYPH_FORCE){
				if(!n->rep || !dynamic_pointer_cast<VectorGlRep>(n->rep)){ n->rep=make_shared<VectorGlRep>(); }
				auto& vec=n->rep->cast<VectorGlRep>();
				vec.range=glyphRange;
				vec.relSz=glyphRelSz;
			}
			switch(glyph){
				case GLYPH_NONE: n->rep.reset(); break; // no rep
				case GLYPH_VEL: n->rep->cast<VectorGlRep>().val=n->getData<DemData>().vel; break;
				case GLYPH_FORCE: n->rep->cast<VectorGlRep>().val=n->getData<DemData>().force; break;
				case GLYPH_KEEP:
				default: ;
			}
		}

		if(!nodes && !n->rep) continue;

		Renderer::glScopedName name(n);
		if(nodes){ Renderer::renderRawNode(n); }
		if(n->rep){ n->rep->render(n,viewInfo); }
	}
}


void Gl1_DemField::doContactNodes(){
	if(cNode==CNODE_NONE) return;
	Renderer::nodeDispatcher.scene=scene; Renderer::nodeDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->contacts->manipMutex);
	for(size_t i=0; i<dem->contacts->size(); i++){
		PROCESS_GUI_EVENTS_SOMETIMES;
		const shared_ptr<Contact>& C((*dem->contacts)[i]);
		if(C->isReal()){
			shared_ptr<CGeom> geom=C->geom;
			if(!geom) continue;
			shared_ptr<Node> node=geom->node;
			Renderer::setNodeGlData(node,updateRefPos);
			Renderer::glScopedName name(C,node);
			if((cNode & CNODE_GLREP) && node->rep){ node->rep->render(node,viewInfo); }
			if(cNode & CNODE_LINE){
				assert(C->pA->shape && C->pB->shape);
				assert(C->pA->shape->nodes.size()>0); assert(C->pB->shape->nodes.size()>0);
				Vector3r x[3]={node->pos,C->pA->shape->avgNodePos(),C->pB->shape->avgNodePos()};
				if(scene->isPeriodic){
					Vector3i cellDist;
					x[0]=scene->cell->canonicalizePt(x[0],cellDist);
					x[1]+=scene->cell->intrShiftPos(-cellDist);
					x[2]+=scene->cell->intrShiftPos(-cellDist+C->cellDist);
				}
				Vector3r color=CompUtils::mapColor(C->color);
				if(dynamic_pointer_cast<Sphere>(C->pA->shape)) GLUtils::GLDrawLine(x[0],x[1],color);
				GLUtils::GLDrawLine(x[0],x[2],color);
			}
			if(cNode & CNODE_NODE) Renderer::renderRawNode(node);
		} else {
			if(!(cNode & CNODE_POTLINE)) continue;
			assert(C->pA->shape && C->pB->shape);
			assert(C->pA->shape->nodes.size()>0); assert(C->pB->shape->nodes.size()>0);
			Vector3r A;
			Vector3r B=C->pB->shape->avgNodePos();
			if(dynamic_pointer_cast<Sphere>(C->pA->shape)) A=C->pA->shape->nodes[0]->pos;
			else if(dynamic_pointer_cast<Wall>(C->pA->shape)){ A=C->pA->shape->nodes[0]->pos; int ax=C->pA->shape->cast<Wall>().axis; A[(ax+1)%3]=B[(ax+1)%3]; A[(ax+2)%3]=B[(ax+2)%3]; }
			else if(dynamic_pointer_cast<InfCylinder>(C->pA->shape)){ A=C->pA->shape->nodes[0]->pos; int ax=C->pA->shape->cast<InfCylinder>().axis; A[ax]=B[ax]; }
			else A=C->pA->shape->avgNodePos();
			if(scene->isPeriodic){ B+=scene->cell->intrShiftPos(C->cellDist); }
			GLUtils::GLDrawLine(A,B,.5*CompUtils::mapColor(C->color));
		}
	}
}


void Gl1_DemField::doCPhys(){
	Renderer::cPhysDispatcher.scene=scene; Renderer::cPhysDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->contacts->manipMutex);
	FOREACH(const shared_ptr<Contact>& C, *dem->contacts){
		PROCESS_GUI_EVENTS_SOMETIMES;
		#if 1
			shared_ptr<CGeom> geom(C->geom);
			shared_ptr<CPhys> phys(C->phys);
		#else
			// HACK: make fast now
			shared_ptr<CGeom>& geom(C->geom);
			shared_ptr<CPhys>& phys(C->phys);
		#endif
		//assert(C->pA->shape && C->pB->shape);
		//assert(C->pA->shape->nodes.size()>0); assert(C->pB->shape->nodes.size()>0);
		if(!geom || !phys) continue;
		// glScopedName name(C,geom->node);
		Renderer::cPhysDispatcher(phys,C,*viewInfo);
	}
}





void Gl1_DemField::go(const shared_ptr<Field>& demField, GLViewInfo* _viewInfo){
	dem=static_pointer_cast<DemField>(demField);
	viewInfo=_viewInfo;

	if(doPostLoad || _lastScene!=scene) postLoad2();
	doPostLoad=false;

	if(shape) doShape();
	if(bound) doBound();
	doNodes();
	doContactNodes();
	if(cPhys) doCPhys();
	updateRefPos=false;
};

#endif


