#ifdef WOO_OPENGL
#include<woo/pkg/dem/Gl1_DemField.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/base/CompUtils.hpp>

WOO_PLUGIN(gl,(Gl1_DemField));

unsigned int Gl1_DemField::mask;
bool Gl1_DemField::wire;
bool Gl1_DemField::bound;
bool Gl1_DemField::shape;
bool Gl1_DemField::nodes;
int Gl1_DemField::cNodes;
Vector2i Gl1_DemField::cNodes_range;
bool Gl1_DemField::cPhys;
bool Gl1_DemField::potWire;

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
	FOREACH(shared_ptr<Particle> b, *dem->particles){
		if(!b->shape || b->shape->nodes.empty()) continue;
		if(mask!=0 && !(b->mask&mask)) continue;
		const shared_ptr<Shape>& sh=b->shape;
		if(!sh->getVisible()) continue;

		//if(!bodyDisp[b->getId()].isDisplayed) continue;
		//Vector3r pos=bodyDisp[b->getId()].pos;
		//Quaternionr ori=bodyDisp[b->getId()].ori;
		//Vector3r pos=sh->nodes[0].pos;
		//Quaternionr ori=sh->nodes[0].ori;
		//if(!b->shape || !((b->getGroupMask()&mask) || b->getGroupMask()==0)) continue;

		// int selId=(dynamic_pointer_cast<Particle>(selObj)?static_pointer_cast<Particle>(selObj)->id:-1);

		// sets highlighted color, if the particle is selected
		// last optional arg can be used to provide additional highlight conditions (unused for now)
		Renderer::glScopedName name(b,b->shape->nodes[0]);
		// bool highlight=(b->id==selId || (b->clumpId>=0 && b->clumpId==selId) || b->shape->highlight);

		FOREACH(const shared_ptr<Node>& n,b->shape->nodes) rrr->setNodeGlData(n);

		glPushMatrix();
			rrr->shapeDispatcher(b->shape,/*shift*/Vector3r::Zero(),wire||sh->getWire(),*viewInfo);
		glPopMatrix();

		if(name.highlighted){
			const Vector3r& p=sh->nodes[0]->pos;
			if(!sh->bound || wire || sh->getWire()) GLUtils::GLDrawInt(b->id,p);
			else {
				// move the label towards the camera by the bounding box so that it is not hidden inside the body
				const Vector3r& mn=sh->bound->min; const Vector3r& mx=sh->bound->max;
				Vector3r ext(rrr->viewDirection[0]>0?p[0]-mn[0]:p[0]-mx[0],rrr->viewDirection[1]>0?p[1]-mn[1]:p[1]-mx[1],rrr->viewDirection[2]>0?p[2]-mn[2]:p[2]-mx[2]); // signed extents towards the camera
				Vector3r dr=-1.01*(rrr->viewDirection.dot(ext)*rrr->viewDirection);
				GLUtils::GLDrawInt(b->id,p+dr,Vector3r::Ones());
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
						rrr->shapeDispatcher(b->shape,/*shift*/pt-pos,/*wire*/true,*viewInfo);
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
		rrr->setNodeGlData(node);
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
			rrr->setNodeGlData(node);
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

	if(shape) doShape();
	if(bound) doBound();
	doNodes();
	doContactNodes();
	if(cPhys) doCPhys();
};

#endif


