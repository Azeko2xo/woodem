#ifdef WOO_OPENGL
#include<woo/pkg/dem/Gl1_DemField.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/base/CompUtils.hpp>

WOO_PLUGIN(gl,(Gl1_DemField));
CREATE_LOGGER(Gl1_DemField);

bool Gl1_DemField::doPostLoad;
unsigned int Gl1_DemField::mask;
bool Gl1_DemField::wire;
bool Gl1_DemField::bound;
bool Gl1_DemField::shape;
bool Gl1_DemField::nodes;
int Gl1_DemField::cNodes;
Vector2i Gl1_DemField::cNodes_range;
bool Gl1_DemField::cPhys;
bool Gl1_DemField::potWire;
int Gl1_DemField::colorBy;
shared_ptr<ScalarRange> Gl1_DemField::colorRange;
int Gl1_DemField::glyph;
Real Gl1_DemField::glyphRelSz;
shared_ptr<ScalarRange> Gl1_DemField::glyphRange;
bool Gl1_DemField::updateRefPos;

void Gl1_DemField::postLoad(Gl1_DemField&){
	_lastScene=scene;
	if(!colorRange) colorRange=make_shared<ScalarRange>();
	if(!glyphRange) glyphRange=make_shared<ScalarRange>();
	// find color range by label, remove it if necessary
	int ci=-1, gi=-1;
	for(size_t i=0; i<scene->ranges.size(); i++){
		if(scene->ranges[i].get()==colorRange.get()) ci=(int)i;
		if(scene->ranges[i].get()==glyphRange.get()) gi=(int)i;
	}
	//LOG_WARN("ci="<<ci<<", gi="<<gi);
	// remove range from scene if there is one and should not be
	if(colorBy==COLOR_NONE && ci>=0){ 
		scene->ranges.erase(scene->ranges.begin()+ci);
		if(gi>ci) gi--; // moved down
	}
	// with GLYPH_KEEP, keep the range there as well
	if((/*glyph==GLYPH_KEEP ||*/ glyph==GLYPH_NONE) && gi>=0){
		scene->ranges.erase(scene->ranges.begin()+gi);
	}
	// add range if there is none and should be
	if(colorBy!=COLOR_NONE && ci<0){ 
		//LOG_WARN("Adding colorBy ScalarRange "<<colorRange<<" to scene @ "<<scene);
		scene->ranges.push_back(colorRange); 
	}
	if(glyph!=GLYPH_KEEP && glyph!=GLYPH_NONE && gi<0){
		//LOG_WARN("Adding glyph ScalarRange "<<glyphRange.get()<<" to scene @ "<<scene);
		scene->ranges.push_back(glyphRange);
	}
	glyphRange->reset();
	colorRange->reset();
	switch(colorBy){
		case COLOR_NONE: colorRange->label="[none]";  break;
		case COLOR_RADIUS: colorRange->label="radius"; break;
		case COLOR_VEL: colorRange->label="|vel|"; break;
		case COLOR_DISPLACEMENT: colorRange->label="displacement"; break;
		case COLOR_ROTATION: colorRange->label="rotation"; break;
		default: LOG_ERROR("Unknown value Gl1_DemField.colorBy="<<colorBy<<" (ignored)??");
	}
	switch(glyph){
		case GLYPH_KEEP: break; // do nothing
		case GLYPH_NONE: glyphRange->label="[none]"; break;
		case GLYPH_FORCE: glyphRange->label="force"; break;
		case GLYPH_VEL: glyphRange->label="velocity"; break;
		default: LOG_ERROR("Unknown value Gl1_DemField.glyph="<<glyph<<" (ignored)??");
	};
}

void Gl1_DemField::doBound(){
	rrr->boundDispatcher.scene=scene; rrr->boundDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->particles->manipMutex);
	FOREACH(const shared_ptr<Particle>& b, *dem->particles){
		if(!b->shape || !b->shape->bound) continue;
		if(mask!=0 && !(b->mask&mask)) continue;
		glPushMatrix(); rrr->boundDispatcher(b->shape->bound); glPopMatrix();
	}
}

// this function is called for both rendering as well as
// in the selection mode

// nice reading on OpenGL selection
// http://glprogramming.com/red/chapter13.html

void Gl1_DemField::doShape(){
	rrr->shapeDispatcher.scene=scene; rrr->shapeDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->particles->manipMutex);

	// instead of const shared_ptr&, get proper shared_ptr;
	// Less efficient in terms of performance, since memory has to be written (not measured, though),
	// but it is still better than crashes if the body gets deleted meanwile.
	FOREACH(shared_ptr<Particle> p, *dem->particles){
		if(!p->shape || p->shape->nodes.empty()) continue;
		if(mask!=0 && !(p->mask&mask)) continue;
		const shared_ptr<Shape>& sh=p->shape;

		//if(!bodyDisp[p->getId()].isDisplayed) continue;
		//Vector3r pos=bodyDisp[p->getId()].pos;
		//Quaternionr ori=bodyDisp[p->getId()].ori;
		//Vector3r pos=sh->nodes[0].pos;
		//Quaternionr ori=sh->nodes[0].ori;
		//if(!p->shape || !((p->getGroupMask()&mask) || p->getGroupMask()==0)) continue;

		// int selId=(dynamic_pointer_cast<Particle>(selObj)?static_pointer_cast<Particle>(selObj)->id:-1);

		// sets highlighted color, if the particle is selected
		// last optional arg can be used to provide additional highlight conditions (unused for now)
		const shared_ptr<Node>& n0=p->shape->nodes[0];
		Renderer::glScopedName name(p,n0);
		// bool highlight=(p->id==selId || (p->clumpId>=0 && p->clumpId==selId) || p->shape->highlight);

		FOREACH(const shared_ptr<Node>& n,p->shape->nodes) rrr->setNodeGlData(n,updateRefPos);

		switch(colorBy){
			case COLOR_RADIUS: p->shape->color=dynamic_pointer_cast<Sphere>(p->shape)?colorRange->norm(p->shape->cast<Sphere>().radius):0.; break;
			case COLOR_VEL: p->shape->color=colorRange->norm(n0->getData<DemData>().vel.norm()); break;
			case COLOR_DISPLACEMENT: p->shape->color=colorRange->norm((n0->pos-n0->getData<GlData>().refPos).norm()); break;
			case COLOR_ROTATION: p->shape->color=colorRange->norm(AngleAxisr(n0->ori.conjugate()*n0->getData<GlData>().refOri).angle()); break;
			case COLOR_NONE: ;
			default: ;
		}

		for(const auto& n: p->shape->nodes){
			// prepare rep types
			// vector values
			if(glyph==GLYPH_VEL || glyph==GLYPH_FORCE){
				if(!n->rep || !dynamic_pointer_cast<VectorGlRep>(n->rep)) n->rep=make_shared<VectorGlRep>();
				auto& vec=n->rep->cast<VectorGlRep>();
				vec.range=glyphRange;
				vec.relSz=glyphRelSz;
			}
			switch(glyph){
				case GLYPH_KEEP: break; // keep whatever is there
				case GLYPH_NONE: n->rep.reset(); break; // no rep
				case GLYPH_VEL: n->rep->cast<VectorGlRep>().val=n->getData<DemData>().vel; break;
				case GLYPH_FORCE: n->rep->cast<VectorGlRep>().val=n->getData<DemData>().force; break;
				default: ;
			}
		}

		glPushMatrix();
			rrr->shapeDispatcher(p->shape,/*shift*/Vector3r::Zero(),wire||sh->getWire(),*viewInfo);
		glPopMatrix();

		if(!sh->getVisible()) continue;

		if(name.highlighted){
			const Vector3r& pos=sh->nodes[0]->pos;
			if(!sh->bound || wire || sh->getWire()) GLUtils::GLDrawInt(p->id,pos);
			else {
				// move the label towards the camera by the bounding box so that it is not hidden inside the body
				const Vector3r& mn=sh->bound->min; const Vector3r& mx=sh->bound->max;
				Vector3r ext(rrr->viewDirection[0]>0?pos[0]-mn[0]:pos[0]-mx[0],rrr->viewDirection[1]>0?pos[1]-mn[1]:pos[1]-mx[1],rrr->viewDirection[2]>0?pos[2]-mn[2]:pos[2]-mx[2]); // signed extents towards the camera
				Vector3r dr=-1.01*(rrr->viewDirection.dot(ext)*rrr->viewDirection);
				GLUtils::GLDrawInt(p->id,pos+dr,Vector3r::Ones());
			}
		}
		// if the body goes over the cell margin, draw it in positions where the bbox overlaps with the cell in wire
		// precondition: pos is inside the cell.
		if(sh->bound && scene->isPeriodic && rrr->ghosts){
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
						rrr->shapeDispatcher(p->shape,/*shift*/pt-pos,/*wire*/true,*viewInfo);
					glPopMatrix();
				}
			}
		}
	}
}

void Gl1_DemField::doNodes(){
	boost::mutex::scoped_lock lock(dem->nodesMutex);

	rrr->nodeDispatcher.scene=scene; rrr->nodeDispatcher.updateScenePtr();
	FOREACH(shared_ptr<Node> node, dem->nodes){
		rrr->setNodeGlData(node,updateRefPos);
		if(!nodes && !node->rep) continue;
		Renderer::glScopedName name(node);
		if(nodes){ rrr->renderRawNode(node); }
		if(node->rep){ node->rep->render(node,viewInfo); }
	}
	// contact nodes
}


void Gl1_DemField::doContactNodes(){
	rrr->nodeDispatcher.scene=scene; rrr->nodeDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->contacts->manipMutex);
	for(size_t i=0; i<dem->contacts->size(); i++){
		const shared_ptr<Contact>& C((*dem->contacts)[i]);
		if(C->isReal()){
			shared_ptr<CGeom> geom=C->geom;
			if(!geom) continue;
			shared_ptr<Node> node=geom->node;
			rrr->setNodeGlData(node,updateRefPos);
			if(!(cNodes>=0) && !node->rep) continue;
			Renderer::glScopedName name(C,node);
			if(cNodes>0){ // cNodes>0: show something else than just the GlRep
				if(cNodes & 1){ // connect node by lines with particle's positions
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
					GLUtils::GLDrawLine(x[0],x[1],color);
					GLUtils::GLDrawLine(x[0],x[2],color);
				}
				if(cNodes & 2) rrr->renderRawNode(node);
			}
			if(node->rep){ node->rep->render(node,viewInfo); }
		} else {
			if(!potWire) continue;
			assert(C->pA->shape && C->pB->shape);
			assert(C->pA->shape->nodes.size()>0); assert(C->pB->shape->nodes.size()>0);
			Vector3r x[2]={C->pA->shape->avgNodePos(),C->pB->shape->avgNodePos()};
			if(scene->isPeriodic){ x[1]+=scene->cell->intrShiftPos(C->cellDist); }
			GLUtils::GLDrawLine(x[0],x[1],.5*CompUtils::mapColor(C->color));
		}
	}
}


void Gl1_DemField::doCPhys(){
	rrr->cPhysDispatcher.scene=scene; rrr->cPhysDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->contacts->manipMutex);
	FOREACH(const shared_ptr<Contact>& C, *dem->contacts){
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
		rrr->cPhysDispatcher(phys,C,*viewInfo);
	}
}





void Gl1_DemField::go(const shared_ptr<Field>& demField, GLViewInfo* _viewInfo){
	rrr=_viewInfo->renderer;
	dem=static_pointer_cast<DemField>(demField);
	viewInfo=_viewInfo;

	if(doPostLoad || _lastScene!=scene) postLoad(*this);
	doPostLoad=false;

	if(updateRefPos && (colorBy==COLOR_DISPLACEMENT || colorBy==COLOR_ROTATION)) colorRange->reset();


	if(shape) doShape();
	if(bound) doBound();
	doNodes();
	doContactNodes();
	if(cPhys) doCPhys();
	updateRefPos=false;
};

#endif


