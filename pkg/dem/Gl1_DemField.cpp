#ifdef WOO_OPENGL
#include<woo/pkg/dem/Gl1_DemField.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/InfCylinder.hpp>
#include<woo/pkg/dem/Wall.hpp>
#include<woo/pkg/dem/Facet.hpp>
#include<woo/pkg/dem/Funcs.hpp>
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
int Gl1_DemField::shape;
bool Gl1_DemField::shape2;
bool Gl1_DemField::nodes;
bool Gl1_DemField::deadNodes;
bool Gl1_DemField::fluct;
bool Gl1_DemField::periodic;
//bool Gl1_DemField::trace;
//bool Gl1_DemField::_hadTrace;
int Gl1_DemField::cNode;
bool Gl1_DemField::cPhys;
int Gl1_DemField::colorBy;
int Gl1_DemField::colorBy2;
Vector3r Gl1_DemField::solidColor;
shared_ptr<ScalarRange> Gl1_DemField::colorRange;
shared_ptr<ScalarRange> Gl1_DemField::colorRange2;
vector<shared_ptr<ScalarRange>> Gl1_DemField::colorRanges;
int Gl1_DemField::glyph;
int Gl1_DemField::vecAxis;
Real Gl1_DemField::glyphRelSz;
shared_ptr<ScalarRange> Gl1_DemField::glyphRange;
vector<shared_ptr<ScalarRange>> Gl1_DemField::glyphRanges;
bool Gl1_DemField::updateRefPos;
int Gl1_DemField::guiEvery;

/*
Remove all ScalarRanges in *ours*, which are not in *used*, from the Scene.
Add all from *used* to the Scene.
*/
void Gl1_DemField::setOurSceneRanges(Scene* scene, const vector<shared_ptr<ScalarRange>>& ours, const list<shared_ptr<ScalarRange>>& used){
	list<size_t> removeIx;
	// get indices of ranges that should be removed
	for(size_t i=0; i<scene->ranges.size(); i++){
		if(
			// range is in *ours*
			std::find_if(ours.begin(),ours.end(),[&](const shared_ptr<ScalarRange>& r){return r.get()==scene->ranges[i].get(); })!=ours.end()
			// but not in *used*
			&& std::find_if(used.begin(),used.end(),[&](const shared_ptr<ScalarRange>& r){ return r.get()==scene->ranges[i].get();})==used.end()
		){
			//cerr<<"Will remove range "<<i<<"/"<<scene->ranges.size()<<" @ "<<scene->ranges[i].get()<<endl;
			//cerr<<"   Is at index "<<std::find_if(ours.begin(),ours.end(),[&](const shared_ptr<ScalarRange>& r){return r.get()==scene->ranges[i].get(); })-ours.begin()<<endl;
			removeIx.push_back(i);
		}
	}
	// remove ranges to be removed; iterate backwards, from higher indices
	for(int i=removeIx.size()-1; i>=0; i--){
		//cerr<<"Remove range "<<i<<"/"<<scene->ranges.size()<<endl;
		scene->ranges.erase(scene->ranges.begin()+i);
	}
	// get ranges to be added to the scene, and add them in-place
	for(const auto& r: used){
		// range not in scene, add it
		if(std::find_if(scene->ranges.begin(),scene->ranges.end(),[&](const shared_ptr<ScalarRange>& s){ return s.get()==r.get(); })==scene->ranges.end()){
			if(r) scene->ranges.push_back(r);
		}
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
			case COLOR_REFPOS:       r->label="ref. pos"; break;
			case COLOR_MAT_ID:       r->label="material id"; break;
			case COLOR_MATSTATE:     r->label="matState"; break;
			case COLOR_SIG_N: 	    r->label="normal stress"; break;
			case COLOR_SIG_T:    	 r->label="shear stress"; break;
		};
	}
	colorRange=colorRanges[colorBy];
	colorRange2=colorRanges[colorBy2];

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
	colorRange2=colorRanges[colorBy2];
	glyphRange=glyphRanges[glyph];
	//bool noColor=;
	//bool noColor2=(!shape2 || colorBy2==COLOR_SHAPE || colorBy2==COLOR_SOLID || colorBy2==COLOR_INVISIBLE);
	list<shared_ptr<ScalarRange>> usedColorRanges;
	if(shape!=SHAPE_NONE && colorBy!=COLOR_SHAPE && colorBy!=COLOR_SOLID && colorBy!=COLOR_INVISIBLE) usedColorRanges.push_back(colorRange);
	if(shape2 && colorBy2!=COLOR_SHAPE && colorBy2!=COLOR_SOLID && colorBy2!=COLOR_INVISIBLE) usedColorRanges.push_back(colorRange2);

	setOurSceneRanges(scene,colorRanges,usedColorRanges);
	if(glyph!=GLYPH_NONE) setOurSceneRanges(scene,glyphRanges,{glyphRange});
	else setOurSceneRanges(scene,glyphRanges,{});
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

Vector3r Gl1_DemField::getNodeVel(const shared_ptr<Node>& n) const{
	if(!scene->isPeriodic || !fluct) return n->getData<DemData>().vel;
	return scene->cell->pprevFluctVel(n->pos,n->getData<DemData>().vel,scene->dt);
}

Vector3r Gl1_DemField::getNodeAngVel(const shared_ptr<Node>& n) const{
	if(!scene->isPeriodic || !fluct) return n->getData<DemData>().angVel;
	return scene->cell->pprevFluctAngVel(n->getData<DemData>().angVel);
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

		const shared_ptr<Shape>& sh=p->shape;

		// sets highlighted color, if the particle is selected
		// last optional arg can be used to provide additional highlight conditions (unused for now)
		const shared_ptr<Node>& n0=p->shape->nodes[0];
		Renderer::glScopedName name(p,n0);
		// bool highlight=(p->id==selId || (p->clumpId>=0 && p->clumpId==selId) || p->shape->highlight);

		// if any of the particle's nodes is clipped, don't display it at all
		bool clipped=false;
		FOREACH(const shared_ptr<Node>& n,p->shape->nodes){
			Renderer::setNodeGlData(n,updateRefPos);
			if(n->getData<GlData>().isClipped()) clipped=true;
		}

		if(!sh->getVisible() || clipped) continue;

		bool useColor2=false;
		bool isSphere=dynamic_pointer_cast<Sphere>(p->shape);

		switch(shape){
			case SHAPE_NONE: useColor2=true; break;
			case SHAPE_ALL: useColor2=false; break;
			case SHAPE_SPHERES: useColor2=!isSphere; break;
			case SHAPE_NONSPHERES: useColor2=isSphere; break;
			case SHAPE_MASK: useColor2=(mask!=0 && !(p->mask & mask)); break;
			default: useColor2=true; break;  // invalid value, filter not matching
		}
		// additional conditions under which colorBy2 is used
		if(false
			|| (!isSphere && colorBy==COLOR_RADIUS )
			|| (colorBy==COLOR_MATSTATE && !p->matState)
			|| (!isSphere && (colorBy==COLOR_SIG_N || colorBy==COLOR_SIG_T))
		) useColor2=true;

		if(!shape2 && useColor2) continue; // skip particle

		Vector3r parColor;
		
		// choose colorRange for colorBy or colorBy2
		const shared_ptr<ScalarRange>& CR(!useColor2?colorRange:colorRanges[colorBy2]);
		auto vecNormXyz=[&](const Vector3r& v)->Real{ if(useColor2 || vecAxis<0||vecAxis>2) return v.norm(); return v[vecAxis]; };
		int cBy=(!useColor2?colorBy:colorBy2);
		switch(cBy){
			case COLOR_RADIUS: parColor=(isSphere?CR->color(p->shape->cast<Sphere>().radius):solidColor); break;
			case COLOR_VEL: parColor=CR->color(vecNormXyz(getNodeVel(n0))); break;
			case COLOR_ANGVEL: parColor=CR->color(vecNormXyz(getNodeAngVel(n0))); break;
			case COLOR_MASS: parColor=CR->color(n0->getData<DemData>().mass); break;
			case COLOR_DISPLACEMENT: parColor=CR->color(vecNormXyz(n0->pos-n0->getData<GlData>().refPos)); break;
			case COLOR_ROTATION: {
				AngleAxisr aa(n0->ori.conjugate()*n0->getData<GlData>().refOri);
				parColor=CR->color((vecAxis<0||vecAxis>2)?aa.angle():(aa.angle()*aa.axis())[vecAxis]);
				break;
			}
			case COLOR_REFPOS: parColor=CR->color(vecNormXyz(n0->getData<GlData>().refPos)); break;
			case COLOR_MAT_ID: parColor=CR->color(p->material->id); break;
			case COLOR_MATSTATE:	parColor=(p->matState?CR->color(p->matState->getColorScalar()):solidColor); break;
			case COLOR_SHAPE: parColor=CR->color(p->shape->getBaseColor()); break;
			case COLOR_SOLID: parColor=solidColor; break;
			case COLOR_SIG_N:
			case COLOR_SIG_T: {
				Vector3r sigN, sigT;
				bool sigOk=DemFuncs::particleStress(p,sigN,sigT);
				if(sigOk) parColor=CR->color(vecNormXyz(cBy==COLOR_SIG_N?sigN:sigT));
				else parColor=solidColor;
				break;
			}
			case COLOR_INVISIBLE: continue; // don't show this particle at all
			default: parColor=Vector3r(NaN,NaN,NaN);
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

void Gl1_DemField::doNodes(const vector<shared_ptr<Node>>& nodeContainer){
	boost::mutex::scoped_lock lock(dem->nodesMutex);

	Renderer::nodeDispatcher.scene=scene; Renderer::nodeDispatcher.updateScenePtr();
	FOREACH(shared_ptr<Node> n, nodeContainer){
		PROCESS_GUI_EVENTS_SOMETIMES;

		Renderer::setNodeGlData(n,updateRefPos);
		if(n->getData<GlData>().isClipped()) continue;

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
				case GLYPH_VEL: n->rep->cast<VectorGlRep>().val=getNodeVel(n); break;
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
		const Particle *pA=C->leakPA(), *pB=C->leakPB();
		if(C->isReal()){
			shared_ptr<CGeom> geom=C->geom;
			if(!geom) continue;
			shared_ptr<Node> node=geom->node;
			Renderer::setNodeGlData(node,updateRefPos);
			Renderer::glScopedName name(C,node);
			if((cNode & CNODE_GLREP) && node->rep){ node->rep->render(node,viewInfo); }
			if(cNode & CNODE_LINE){
				assert(pA->shape && pB->shape);
				assert(pA->shape->nodes.size()>0); assert(pB->shape->nodes.size()>0);
				Vector3r x[3]={node->pos,pA->shape->avgNodePos(),pB->shape->avgNodePos()};
				if(scene->isPeriodic){
					Vector3i cellDist;
					x[0]=scene->cell->canonicalizePt(x[0],cellDist);
					x[1]+=scene->cell->intrShiftPos(-cellDist);
					x[2]+=scene->cell->intrShiftPos(-cellDist+C->cellDist);
				}
				Vector3r color=CompUtils::mapColor(C->color);
				if(dynamic_pointer_cast<Sphere>(pA->shape)) GLUtils::GLDrawLine(x[0],x[1],color);
				GLUtils::GLDrawLine(x[0],x[2],color);
			}
			if(cNode & CNODE_NODE) Renderer::renderRawNode(node);
		} else {
			if(!(cNode & CNODE_POTLINE)) continue;
			assert(pA->shape && pB->shape);
			assert(pA->shape->nodes.size()>0); assert(pB->shape->nodes.size()>0);
			Vector3r A;
			Vector3r B=pB->shape->avgNodePos();
			if(dynamic_pointer_cast<Sphere>(pA->shape)) A=pA->shape->nodes[0]->pos;
			else if(dynamic_pointer_cast<Wall>(pA->shape)){ A=pA->shape->nodes[0]->pos; int ax=pA->shape->cast<Wall>().axis; A[(ax+1)%3]=B[(ax+1)%3]; A[(ax+2)%3]=B[(ax+2)%3]; }
			else if(dynamic_pointer_cast<InfCylinder>(pA->shape)){ A=pA->shape->nodes[0]->pos; int ax=pA->shape->cast<InfCylinder>().axis; A[ax]=B[ax]; }
			else if(dynamic_pointer_cast<Facet>(pA->shape)){ A=pA->shape->cast<Facet>().getNearestPt(B); }
			else A=pA->shape->avgNodePos();
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
		//assert(C->leakPA()->shape && C->leakPB()->shape);
		//assert(C->leakPA()->shape->nodes.size()>0); assert(C->leakPB()->shape->nodes.size()>0);
		if(!geom || !phys) continue;
		// glScopedName name(C,geom->node);
		Renderer::cPhysDispatcher(phys,C,*viewInfo);
	}
}





void Gl1_DemField::go(const shared_ptr<Field>& demField, GLViewInfo* _viewInfo){
	dem=static_pointer_cast<DemField>(demField);
	viewInfo=_viewInfo;

	if(doPostLoad || _lastScene!=scene) postLoad2();
	doPostLoad=false; _lastScene=scene;
	periodic=scene->isPeriodic;
	if(updateRefPos && periodic) scene->cell->refHSize=scene->cell->hSize;

	if(shape!=SHAPE_NONE || shape2) doShape();
	if(bound) doBound();
	doNodes(dem->nodes);
	if(deadNodes && !dem->deadNodes.empty()) doNodes(dem->deadNodes);
	doContactNodes();
	if(cPhys) doCPhys();
	updateRefPos=false;
};

#endif


