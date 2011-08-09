#pragma once

#include<yade/core/Scene.hpp>
#include<yade/core/Field.hpp>
#include<yade/core/Omega.hpp>

template<typename FieldT> bool Field_sceneHasField()
{
	FOREACH(const shared_ptr<Field>& f,Omega::instance().getScene()->fields)
	if(dynamic_pointer_cast<FieldT>(f)) return true; return false;
}
template<typename FieldT> shared_ptr<FieldT>
Field_sceneGetField(){
	const shared_ptr<Scene>& scene=Omega::instance().getScene();
	FOREACH(const shared_ptr<Field>& f, scene->fields) if(dynamic_pointer_cast<FieldT>(f)) return static_pointer_cast<FieldT>(f);
	cerr<<"(debug) Creating new field #"<<scene->fields.size()<<" "<<typeid(FieldT).name()<<endl;
	auto f=make_shared<FieldT>();
	scene->fields.push_back(f);
	return f;
}


