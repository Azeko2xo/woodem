#include<woo/core/Field.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<woo/core/Scene.hpp>

#ifdef WOO_OPENGL
void ScalarRange::reset(){
	mnmx=Vector2r(std::numeric_limits<Real>::infinity(),-std::numeric_limits<Real>::infinity());
	setAutoAdjust(true);
}

void ScalarRange::adjust(const Real& v){
	if(isSymmetric()){ if(abs(v)>max(abs(mnmx[0]),abs(mnmx[1])) || !isOk()){ mnmx[0]=-abs(v); mnmx[1]=abs(v); } }
	else {if(v<mnmx[0]) mnmx[0]=v; if(v>mnmx[1]) mnmx[1]=v; }
}

Real ScalarRange::norm(Real v){
	if(isAutoAdjust()) adjust(v);
	return CompUtils::clamped((v-mnmx[0])/(mnmx[1]-mnmx[0]),0,1);
};

Vector3r ScalarRange::color(Real v){
	if(isAutoAdjust()) adjust(v);
	return CompUtils::scalarOnColorScale(v,mnmx[0],mnmx[1],cmap,isReversed());
}

#endif


AlignedBox3r Field::renderingBbox() const {
	AlignedBox3r b;
	for(const shared_ptr<Node>& n: nodes){
		if(!n) continue;
		b.extend(n->pos);
	}
	return b;
}

shared_ptr<Scene> Field::py_getScene(){
	if(!scene) return shared_ptr<Scene>();
	else return static_pointer_cast<Scene>(scene->shared_from_this());
}



// Node::dataIndexMax=Node::ST_LAST;

