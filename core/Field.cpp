#include<yade/core/Field.hpp>
#include<yade/lib/base/CompUtils.hpp>

void ScalarRange::reset(){
	mnmx=Vector2r(std::numeric_limits<Real>::infinity(),-std::numeric_limits<Real>::infinity());
	autoAdjust=true;
}
Real ScalarRange::norm(Real v){
	if(autoAdjust){if(v<mnmx[0]) mnmx[0]=v; if(v>mnmx[1]) mnmx[1]=v;}
	return (v-mnmx[0])/(mnmx[1]-mnmx[0]);
};

Vector3r ScalarRange::color(Real v){
	if(autoAdjust){if(v<mnmx[0]) mnmx[0]=v; if(v>mnmx[1]) mnmx[1]=v;}
	return CompUtils::scalarOnColorScale(v,mnmx[0],mnmx[1]);
}



// Node::dataIndexMax=Node::ST_LAST;

