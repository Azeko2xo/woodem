// 2008 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

#include<vector>
#include<iostream>
#include<cassert>
#include<woo/lib/base/Math.hpp>

/* Linear interpolation routine for sequential interpolation; in contrast to the version below,
   it only returns two endpoints and scalar value (timeT) in range (0,1) for interpolation between those two.
*/
template<typename T, typename timeT>
std::tuple<T,T,timeT> linearInterpolateRel(const Real t, const std::vector<timeT>& tt, const std::vector<T>& values, size_t& pos){
	assert(tt.size()==values.size());
	if(t<=tt[0]){ pos=0; return std::make_tuple(values[0],values[0],0.);}
	if(t>=*tt.rbegin()){ pos=tt.size()-2; return std::make_tuple(*values.rbegin(),*values.rbegin(),1.);}
	pos=std::min(pos,tt.size()-2);
	while((tt[pos]>t) || (tt[pos+1]<t)){
		assert(tt[pos]<tt[pos+1]);
		if(tt[pos]>t) pos--;
		else pos++;
	}
	const Real &t0=tt[pos], &t1=tt[pos+1];
	return std::make_tuple(values[pos],values[pos+1],(t-t0)/(t1-t0));
}

/* Linear interpolation function suitable only for sequential interpolation.
 *
 * Template parameter T must support: addition, subtraction, scalar multiplication.
 *
 * @param t "time" at which the interpolated variable should be evaluated.
 * @param tt "time" points at which values are given; must be increasing.
 * @param values values at "time" points specified by tt
 * @param pos holds lookup state
 *
 * @return value at "time" t; out of range: t<t0 → value(t0), t>t_last → value(t_last)
 */
template<typename T, typename timeT>
T linearInterpolate(const Real t, const std::vector<timeT>& tt, const std::vector<T>& values, size_t& pos){
	#if 0
	assert(tt.size()==values.size());
	if(t<=tt[0]){ pos=0; return values[0];}
	if(t>=*tt.rbegin()){ pos=tt.size()-2; return *values.rbegin();}
	pos=std::min(pos,tt.size()-2);
	while((tt[pos]>t) || (tt[pos+1]<t)){
		assert(tt[pos]<tt[pos+1]);
		if(tt[pos]>t) pos--;
		else pos++;
	}
	const Real &t0=tt[pos], &t1=tt[pos+1]; const T &v0=values[pos], &v1=values[pos+1];
	return v0+(v1-v0)*((t-t0)/(t1-t0));
	#endif
	T A, B; // endpoints
	timeT tRel;
	std::tie(A,B,tRel)=linearInterpolateRel(t,tt,values,pos);
	if(tRel==0.) return A;
	if(tRel==1.) return B;
	return A+(B-A)*tRel;
}

// templated to avoid linking multiple defs
template<typename Scalar>
Scalar linearInterpolate(const Scalar x, const vector<Eigen::Matrix<Scalar,2,1>>& xxyy, size_t& pos){
	if(x<=xxyy[0].x()){ pos=0; return xxyy[0].y(); }
	if(x>=xxyy.rbegin()->x()){ pos=xxyy.size()-2; return xxyy.rbegin()->y();}
	pos=std::min(pos,xxyy.size()-2);
	while((xxyy[pos].x()>x) || (xxyy[pos+1].x()<x)){
		assert(xxyy[pos].x()<xxyy[pos+1].x());
		if(xxyy[pos].x()>x) pos--;
		else pos++;
	}
	const Scalar &x0=xxyy[pos].x(), &x1=xxyy[pos+1].x(); const Real &y0=xxyy[pos].y(), &y1=xxyy[pos+1].y();
	return y0+(y1-y0)*((x-x0)/(x1-x0));
}



#if 0
	// test program
	int main(void){
		Real t,v;
		std::vector<Real> tt,vv;
		while(std::cin){
			std::cin>>t>>v;
			tt.push_back(t); vv.push_back(v);	
		}
		size_t pos;
		for(Real t=0; t<10; t+=0.1){
			std::cout<<t<<" "<<linearInterpolate<Real,Real>(t,tt,vv,pos)<<std::endl;
		}
	}
#endif 
