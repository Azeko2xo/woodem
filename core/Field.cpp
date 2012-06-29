#include<woo/core/Field.hpp>
#include<woo/lib/base/CompUtils.hpp>

void ScalarRange::reset(){
	mnmx=Vector2r(std::numeric_limits<Real>::infinity(),-std::numeric_limits<Real>::infinity());
	autoAdjust=true;
}

void ScalarRange::adjust(const Real& v){
	if(sym){ if(abs(v)>max(abs(mnmx[0]),abs(mnmx[1])) || !isOk()){ mnmx[0]=-abs(v); mnmx[1]=abs(v); } }
	else {if(v<mnmx[0]) mnmx[0]=v; if(v>mnmx[1]) mnmx[1]=v; }
}

Real ScalarRange::norm(Real v){
	if(autoAdjust) adjust(v);
	return CompUtils::clamped((v-mnmx[0])/(mnmx[1]-mnmx[0]),0,1);
};

Vector3r ScalarRange::color(Real v){
	if(autoAdjust) adjust(v);
	return CompUtils::scalarOnColorScale(v,mnmx[0],mnmx[1],cmap);
}


bool Field::renderingBbox(Vector3r& min, Vector3r& max){
	if(nodes.empty()) return false;
	Real inf=std::numeric_limits<Real>::infinity();
	min=Vector3r(inf,inf,inf); max=Vector3r(-inf,-inf,-inf);
	FOREACH(const shared_ptr<Node>& b, nodes){
		if(!b) continue;
		max=max.array().max(b->pos.array()).matrix();
		min=min.array().min(b->pos.array()).matrix();
	}
	return true;
}


// Node::dataIndexMax=Node::ST_LAST;

