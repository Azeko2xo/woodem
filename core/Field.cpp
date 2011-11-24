#include<yade/core/Field.hpp>
#include<yade/lib/base/CompUtils.hpp>

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
	return (v-mnmx[0])/(mnmx[1]-mnmx[0]);
};

Vector3r ScalarRange::color(Real v){
	if(autoAdjust) adjust(v);
	return CompUtils::scalarOnColorScale(v,mnmx[0],mnmx[1],cmap);
}



// Node::dataIndexMax=Node::ST_LAST;

