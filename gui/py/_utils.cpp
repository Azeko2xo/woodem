#include<yade/extra/Shop.hpp>
#include<boost/python.hpp>
#include<yade/extra/boost_python_len.hpp>
#include<yade/core/MetaBody.hpp>
#include<yade/core/Omega.hpp>
#include<yade/pkg-common/Sphere.hpp>
#include<yade/pkg-dem/SpheresContactGeometry.hpp>
#include<yade/pkg-dem/DemXDofGeom.hpp>
#include<yade/pkg-dem/SimpleViscoelasticBodyParameters.hpp>
#include<yade/pkg-common/NormalShearInteractions.hpp>
#include<cmath>

#include<numpy/ndarrayobject.h>



using namespace boost::python;

#ifdef LOG4CXX
	log4cxx::LoggerPtr logger=log4cxx::Logger::getLogger("yade.utils");
#endif

python::tuple vec2tuple(const Vector3r& v){return boost::python::make_tuple(v[0],v[1],v[2]);}
Vector3r tuple2vec(const python::tuple& t){return Vector3r(extract<double>(t[0])(),extract<double>(t[1])(),extract<double>(t[2])());}
bool isInBB(Vector3r p, Vector3r bbMin, Vector3r bbMax){return p[0]>bbMin[0] && p[0]<bbMax[0] && p[1]>bbMin[1] && p[1]<bbMax[1] && p[2]>bbMin[2] && p[2]<bbMax[2];}

/* \todo implement groupMask */
python::tuple aabbExtrema(Real cutoff=0.0, bool centers=false){
	if(cutoff<0. || cutoff>1.) throw invalid_argument("Cutoff must be >=0 and <=1.");
	Real inf=std::numeric_limits<Real>::infinity();
	Vector3r minimum(inf,inf,inf),maximum(-inf,-inf,-inf);
	FOREACH(const shared_ptr<Body>& b, *Omega::instance().getRootBody()->bodies){
		shared_ptr<Sphere> s=dynamic_pointer_cast<Sphere>(b->geometricalModel); if(!s) continue;
		Vector3r rrr(s->radius,s->radius,s->radius);
		minimum=componentMinVector(minimum,b->physicalParameters->se3.position-(centers?Vector3r::ZERO:rrr));
		maximum=componentMaxVector(maximum,b->physicalParameters->se3.position+(centers?Vector3r::ZERO:rrr));
	}
	Vector3r dim=maximum-minimum;
	return python::make_tuple(vec2tuple(minimum+.5*cutoff*dim),vec2tuple(maximum-.5*cutoff*dim));
}
BOOST_PYTHON_FUNCTION_OVERLOADS(aabbExtrema_overloads,aabbExtrema,0,2);

python::tuple negPosExtremeIds(int axis, Real distFactor=1.1){
	python::tuple extrema=aabbExtrema();
	Real minCoord=extract<double>(extrema[0][axis])(),maxCoord=extract<double>(extrema[1][axis])();
	python::list minIds,maxIds;
	FOREACH(const shared_ptr<Body>& b, *Omega::instance().getRootBody()->bodies){
		shared_ptr<Sphere> s=dynamic_pointer_cast<Sphere>(b->geometricalModel); if(!s) continue;
		if(b->physicalParameters->se3.position[axis]-s->radius*distFactor<=minCoord) minIds.append(b->getId());
		if(b->physicalParameters->se3.position[axis]+s->radius*distFactor>=maxCoord) maxIds.append(b->getId());
	}
	return python::make_tuple(minIds,maxIds);
}
BOOST_PYTHON_FUNCTION_OVERLOADS(negPosExtremeIds_overloads,negPosExtremeIds,1,2);

python::tuple coordsAndDisplacements(int axis,python::tuple AABB=python::tuple()){
	Vector3r bbMin(Vector3r::ZERO), bbMax(Vector3r::ZERO); bool useBB=python::len(AABB)>0;
	if(useBB){bbMin=tuple2vec(extract<python::tuple>(AABB[0])());bbMax=tuple2vec(extract<python::tuple>(AABB[1])());}
	python::list retCoord,retDispl;
	FOREACH(const shared_ptr<Body>&b, *Omega::instance().getRootBody()->bodies){
		if(useBB && !isInBB(b->physicalParameters->se3.position,bbMin,bbMax)) continue;
		retCoord.append(b->physicalParameters->se3.position[axis]);
		retDispl.append(b->physicalParameters->se3.position[axis]-b->physicalParameters->refSe3.position[axis]);
	}
	return python::make_tuple(retCoord,retDispl);
}
BOOST_PYTHON_FUNCTION_OVERLOADS(coordsAndDisplacements_overloads,coordsAndDisplacements,1,2);

void setRefSe3(){
	FOREACH(const shared_ptr<Body>& b, *Omega::instance().getRootBody()->bodies){
		b->physicalParameters->refSe3.position=b->physicalParameters->se3.position;
		b->physicalParameters->refSe3.orientation=b->physicalParameters->se3.orientation;
	}
}

Real PWaveTimeStep(){return Shop::PWaveTimeStep();};

Real elasticEnergyInAABB(python::tuple AABB){
	Vector3r bbMin=tuple2vec(extract<python::tuple>(AABB[0])()), bbMax=tuple2vec(extract<python::tuple>(AABB[1])());
	shared_ptr<MetaBody> rb=Omega::instance().getRootBody();
	Real E=0;
	FOREACH(const shared_ptr<Interaction>&i, *rb->transientInteractions){
		if(!i->interactionPhysics) continue;
		shared_ptr<NormalShearInteraction> bc=dynamic_pointer_cast<NormalShearInteraction>(i->interactionPhysics); if(!bc) continue;
		shared_ptr<Dem3DofGeom> geom=dynamic_pointer_cast<Dem3DofGeom>(i->interactionGeometry); if(!bc){LOG_ERROR("NormalShearInteraction contact doesn't have SpheresContactGeomety associated?!"); continue;}
		const shared_ptr<Body>& b1=Body::byId(i->getId1(),rb), b2=Body::byId(i->getId2(),rb);
		bool isIn1=isInBB(b1->physicalParameters->se3.position,bbMin,bbMax), isIn2=isInBB(b2->physicalParameters->se3.position,bbMin,bbMax);
		if(!isIn1 && !isIn2) continue;
		LOG_DEBUG("Interaction #"<<i->getId1()<<"--#"<<i->getId2());
		Real weight=1.;
		if((!isIn1 && isIn2) || (isIn1 && !isIn2)){
			//shared_ptr<Body> bIn=isIn1?b1:b2, bOut=isIn2?b2:b1;
			Vector3r vIn=(isIn1?b1:b2)->physicalParameters->se3.position, vOut=(isIn2?b1:b2)->physicalParameters->se3.position;
			#define _WEIGHT_COMPONENT(axis) if(vOut[axis]<bbMin[axis]) weight=min(weight,abs((vOut[axis]-bbMin[axis])/(vOut[axis]-vIn[axis]))); else if(vOut[axis]>bbMax[axis]) weight=min(weight,abs((vOut[axis]-bbMax[axis])/(vOut[axis]-vIn[axis])));
			_WEIGHT_COMPONENT(0); _WEIGHT_COMPONENT(1); _WEIGHT_COMPONENT(2);
			assert(weight>=0 && weight<=1);
			//cerr<<"Interaction crosses AABB boundary, weight is "<<weight<<endl;
			//LOG_DEBUG("Interaction crosses AABB boundary, weight is "<<weight);
		} else {assert(isIn1 && isIn2); /* cerr<<"Interaction inside, weight is "<<weight<<endl;*/ /*LOG_DEBUG("Interaction inside, weight is "<<weight);*/}
		E+=geom->refLength*weight*(.5*bc->kn*pow(geom->strainN(),2)+.5*bc->ks*pow(geom->strainT().Length(),2));
	}
	return E;
}

/* return histogram ([bin1Min,bin2Min,…],[value1,value2,…]) from projections of interactions
 * to the plane perpendicular to axis∈[0,1,2]; the number of bins can be specified and they cover
 * the range (0,π), since interactions are bidirecional, hence periodically the same on (π,2π).
 *
 * Only contacts using SpheresContactGeometry are considered.
 * Both bodies must be in the mask (except if mask is 0, when all bodies are considered)
 * If the projection is shorter than minProjLen, it is skipped.
 *
 * If both bodies are _outside_ the aabb (if specified), the interaction is skipped.
 *
 */
python::tuple interactionAnglesHistogram(int axis, int mask=0, size_t bins=20, python::tuple aabb=python::tuple(), Real minProjLen=1e-6){
	if(axis<0||axis>2) throw invalid_argument("Axis must be from {0,1,2}=x,y,z.");
	Vector3r bbMin(Vector3r::ZERO), bbMax(Vector3r::ZERO); bool useBB=python::len(aabb)>0; if(useBB){bbMin=tuple2vec(extract<python::tuple>(aabb[0])());bbMax=tuple2vec(extract<python::tuple>(aabb[1])());}
	Real binStep=Mathr::PI/bins; int axis2=(axis+1)%3, axis3=(axis+2)%3;
	vector<Real> cummProj(bins,0.);
	shared_ptr<MetaBody> rb=Omega::instance().getRootBody();
	FOREACH(const shared_ptr<Interaction>& i, *rb->transientInteractions){
		if(!i->isReal) continue;
		const shared_ptr<Body>& b1=Body::byId(i->getId1(),rb), b2=Body::byId(i->getId2(),rb);
		if(!b1->maskOk(mask) || !b2->maskOk(mask)) continue;
		if(useBB && !isInBB(b1->physicalParameters->se3.position,bbMin,bbMax) && !isInBB(b2->physicalParameters->se3.position,bbMin,bbMax)) continue;
		shared_ptr<SpheresContactGeometry> scg=dynamic_pointer_cast<SpheresContactGeometry>(i->interactionGeometry); if(!scg) continue;
		Vector3r n(scg->normal); n[axis]=0.; Real nLen=n.Length();
		if(nLen<minProjLen) continue; // this interaction is (almost) exactly parallel to our axis; skip that one
		Real theta=acos(n[axis2]/nLen)*(n[axis3]>0?1:-1); if(theta<0) theta+=Mathr::PI;
		int binNo=theta/binStep;
		cummProj[binNo]+=nLen;
	}
	python::list val,binMid;
	for(size_t i=0; i<(size_t)bins; i++){ val.append(cummProj[i]); binMid.append(i*binStep);}
	return python::make_tuple(binMid,val);
}
BOOST_PYTHON_FUNCTION_OVERLOADS(interactionAnglesHistogram_overloads,interactionAnglesHistogram,1,4);

python::tuple bodyNumInteractionsHistogram(python::tuple aabb=python::tuple()){
	Vector3r bbMin(Vector3r::ZERO), bbMax(Vector3r::ZERO); bool useBB=python::len(aabb)>0; if(useBB){bbMin=tuple2vec(extract<python::tuple>(aabb[0])());bbMax=tuple2vec(extract<python::tuple>(aabb[1])());}
	const shared_ptr<MetaBody>& rb=Omega::instance().getRootBody();
	vector<int> bodyNumInta; bodyNumInta.resize(rb->bodies->size(),-1 /* uninitialized */);
	int maxInta=0;
	FOREACH(const shared_ptr<Interaction>& i, *rb->transientInteractions){
		if(!i->isReal) continue;
		const body_id_t id1=i->getId1(), id2=i->getId2(); const shared_ptr<Body>& b1=Body::byId(id1,rb), b2=Body::byId(id2,rb);
		if(useBB && isInBB(b1->physicalParameters->se3.position,bbMin,bbMax)) bodyNumInta[id1]=bodyNumInta[id1]>0?bodyNumInta[id1]+1:1;
		if(useBB && isInBB(b2->physicalParameters->se3.position,bbMin,bbMax)) bodyNumInta[id2]=bodyNumInta[id2]>0?bodyNumInta[id2]+1:1;
		maxInta=max(max(maxInta,bodyNumInta[b1->getId()]),bodyNumInta[b2->getId()]);
	}
	vector<int> bins; bins.resize(maxInta+1);
	for(size_t id=0; id<bodyNumInta.size(); id++){ if(bodyNumInta[id]>=0) bins[bodyNumInta[id]]+=1; }
	python::list count,num;
	for(size_t n=1; n<bins.size(); n++){
		if(bins[n]==0) continue;
		num.append(n); count.append(bins[n]);
	}
	return python::make_tuple(num,count);
}
BOOST_PYTHON_FUNCTION_OVERLOADS(bodyNumInteractionsHistogram_overloads,bodyNumInteractionsHistogram,0,1);

python::tuple inscribedCircleCenter(python::list v0, python::list v1, python::list v2)
{
	return vec2tuple(Shop::inscribedCircleCenter(Vector3r(python::extract<double>(v0[0]),python::extract<double>(v0[1]),python::extract<double>(v0[2])), Vector3r(python::extract<double>(v1[0]),python::extract<double>(v1[1]),python::extract<double>(v1[2])), Vector3r(python::extract<double>(v2[0]),python::extract<double>(v2[1]),python::extract<double>(v2[2]))));
}

python::dict getViscoelasticFromSpheresInteraction(Real m, Real tc, Real en, Real es)
{
    shared_ptr<SimpleViscoelasticBodyParameters> b = shared_ptr<SimpleViscoelasticBodyParameters>(new SimpleViscoelasticBodyParameters());
    Shop::getViscoelasticFromSpheresInteraction(m,tc,en,es,b);
	python::dict d;
	d["kn"]=b->kn;
	d["cn"]=b->cn;
	d["ks"]=b->ks;
	d["cs"]=b->cs;
    return d;
}

/*!Sum moments acting on given bodies
 *
 * @param mask is Body::groupMask that must match for a body to be taken in account.
 * @param axis is the direction of axis with respect to which the moment is calculated.
 * @param axisPt is a point on the axis.
 *
 * The computation is trivial: moment from force is is by definition r×F, where r
 * is position relative to axisPt; moment from moment is m; such moment per body is
 * projected onto axis.
 */
Real sumBexTorques(python::tuple ids, python::tuple _axis, python::tuple _axisPt){
	shared_ptr<MetaBody> rb=Omega::instance().getRootBody();
	rb->bex.sync();
	Real ret=0;
	Vector3r axis=tuple2vec(_axis), axisPt=tuple2vec(_axisPt); size_t len=python::len(ids);
	for(size_t i=0; i<len; i++){
		const Body* b=(*rb->bodies)[python::extract<int>(ids[i])].get();
		const Vector3r& m=rb->bex.getTorque(b->getId());
		const Vector3r& f=rb->bex.getForce(b->getId());
		Vector3r r=b->physicalParameters->se3.position-axisPt;
		ret+=axis.Dot(m+r.Cross(f));
	}
	return ret;
}
/* Sum forces actiong on bodies within mask.
 *
 * @param mask groupMask of matching bodies
 * @param direction direction in which forces are summed
 *
 */
Real sumBexForces(python::tuple ids, python::tuple _direction){
	shared_ptr<MetaBody> rb=Omega::instance().getRootBody();
	rb->bex.sync();
	Real ret=0;
	Vector3r direction=tuple2vec(_direction); size_t len=python::len(ids);
	for(size_t i=0; i<len; i++){
		body_id_t id=python::extract<int>(ids[i]);
		const Vector3r& f=rb->bex.getForce(id);
		ret+=direction.Dot(f);
	}
	return ret;
}

/* Tell us whether a point lies in polygon given by array of points.
 *  @param xy is the point that is being tested
 *  @param vertices is Numeric.array (or list or tuple) of vertices of the polygon.
 *         Every row of the array is x and y coordinate, numer of rows is >= 3 (triangle).
 *
 * Copying the algorithm from http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
 * is gratefully acknowledged:
 *
 * License to Use:
 * Copyright (c) 1970-2003, Wm. Randolph Franklin
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *   1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimers.
 *   2. Redistributions in binary form must reproduce the above copyright notice in the documentation and/or other materials provided with the distribution.
 *   3. The name of W. Randolph Franklin may not be used to endorse or promote products derived from this Software without specific prior written permission. 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://numpy.scipy.org/numpydoc/numpy-13.html told me how to use Numeric.array from c
 */
bool pointInsidePolygon(python::tuple xy, python::object vertices){
	Real testx=python::extract<double>(xy[0])(),testy=python::extract<double>(xy[1])();
	char** vertData; int rows, cols; PyArrayObject* vert=(PyArrayObject*)vertices.ptr();
	int result=PyArray_As2D((PyObject**)&vert /* is replaced */ ,&vertData,&rows,&cols,PyArray_DOUBLE);
	if(result!=0) throw invalid_argument("Unable to cast vertices to 2d array");
	if(cols!=2 || rows<3) throw invalid_argument("Vertices must have 2 columns (x and y) and at least 3 rows.");
	int i /*current node*/, j/*previous node*/; bool inside=false;
	for(i=0,j=rows-1; i<rows; j=i++){
		double vx_i=*(double*)(vert->data+i*vert->strides[0]), vy_i=*(double*)(vert->data+i*vert->strides[0]+vert->strides[1]), vx_j=*(double*)(vert->data+j*vert->strides[0]), vy_j=*(double*)(vert->data+j*vert->strides[0]+vert->strides[1]);
		if (((vy_i>testy)!=(vy_j>testy)) && (testx < (vx_j-vx_i) * (testy-vy_i) / (vy_j-vy_i) + vx_i) ) inside=!inside;
	}
	Py_DECREF(vert);
	return inside;
}

/* Project 3d point into 2d using spiral projection along given axis;
 * the returned tuple is
 * 	
 *  ( (height relative to the spiral, distance from axis), theta )
 *
 * dH_dTheta is the inclination of the spiral (height increase per radian),
 * theta0 is the angle for zero height (by given axis).
 */
python::tuple spiralProject(python::tuple _pt, Real dH_dTheta, int axis=2, Real periodStart=std::numeric_limits<Real>::quiet_NaN(), Real theta0=0){
	int ax1=(axis+1)%3,ax2=(axis+2)%3;
	Vector3r pt=tuple2vec(_pt);
	Real r=sqrt(pow(pt[ax1],2)+pow(pt[ax2],2));
	Real theta;
	if(r>Mathr::ZERO_TOLERANCE){
		theta=acos(pt[ax1]/r);
		if(pt[ax2]<0) theta=Mathr::TWO_PI-theta;
	}
	else theta=0;
	Real hRef=dH_dTheta*(theta-theta0);
	long period;
	if(isnan(periodStart)){
		Real h=Shop::periodicWrap(pt[axis]-hRef,hRef-Mathr::PI*dH_dTheta,hRef+Mathr::PI*dH_dTheta,&period);
		return python::make_tuple(python::make_tuple(r,h),theta);
	}
	else{
		// Real hPeriodStart=(periodStart-theta0)*dH_dTheta;
		//TRVAR4(hPeriodStart,periodStart,theta0,theta);
		//Real h=Shop::periodicWrap(pt[axis]-hRef,hPeriodStart,hPeriodStart+2*Mathr::PI*dH_dTheta,&period);
		theta=Shop::periodicWrap(theta,periodStart,periodStart+2*Mathr::PI,&period);
		Real h=pt[axis]-hRef+period*2*Mathr::PI*dH_dTheta;
		//TRVAR3(pt[axis],pt[axis]-hRef,period);
		//TRVAR2(h,theta);
		return python::make_tuple(python::make_tuple(r,h),theta);
	}
}
BOOST_PYTHON_FUNCTION_OVERLOADS(spiralProject_overloads,spiralProject,2,5);

// for now, don't return anything, since we would have to include the whole yadeControl.cpp because of pyInteraction
void Shop__createExplicitInteraction(body_id_t id1, body_id_t id2){ (void) Shop::createExplicitInteraction(id1,id2);}

python::tuple Shop__scalarOnColorScale(Real scalar){ return vec2tuple(Shop::scalarOnColorScale(scalar));}

BOOST_PYTHON_FUNCTION_OVERLOADS(unbalancedForce_overloads,Shop::unbalancedForce,0,1);
Real Shop__kineticEnergy(){return Shop::kineticEnergy();}

BOOST_PYTHON_MODULE(_utils){
	// http://numpy.scipy.org/numpydoc/numpy-13.html mentions this must be done in module init, otherwise we will crash
	import_array();

	def("PWaveTimeStep",PWaveTimeStep);
	def("aabbExtrema",aabbExtrema,aabbExtrema_overloads(args("cutoff","centers")));
	def("negPosExtremeIds",negPosExtremeIds,negPosExtremeIds_overloads(args("axis","distFactor")));
	def("coordsAndDisplacements",coordsAndDisplacements,coordsAndDisplacements_overloads(args("AABB")));
	def("setRefSe3",setRefSe3);
	def("interactionAnglesHistogram",interactionAnglesHistogram,interactionAnglesHistogram_overloads(args("axis","mask","bins","aabb")));
	def("bodyNumInteractionsHistogram",bodyNumInteractionsHistogram,bodyNumInteractionsHistogram_overloads(args("aabb")));
	def("elasticEnergy",elasticEnergyInAABB);
	def("inscribedCircleCenter",inscribedCircleCenter);
	def("getViscoelasticFromSpheresInteraction",getViscoelasticFromSpheresInteraction);
	def("unbalancedForce",&Shop::unbalancedForce,unbalancedForce_overloads(args("useMaxForce")));
	def("kineticEnergy",Shop__kineticEnergy);
	def("sumBexForces",sumBexForces);
	def("sumBexTorques",sumBexTorques);
	def("createInteraction",Shop__createExplicitInteraction);
	def("spiralProject",spiralProject,spiralProject_overloads(args("axis","periodStart","theta0")));
	def("pointInsidePolygon",pointInsidePolygon);
	def("scalarOnColorScale",Shop__scalarOnColorScale);
}


