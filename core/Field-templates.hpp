#pragma once

#include<woo/core/Scene.hpp>
#include<woo/core/Field.hpp>

template<typename FieldT> bool Field_sceneHasField(const shared_ptr<Scene>& scene)
{
	FOREACH(const shared_ptr<Field>& f,scene->fields)
	if(dynamic_pointer_cast<FieldT>(f)) return true; return false;
}
template<typename FieldT> shared_ptr<FieldT>
Field_sceneGetField(const shared_ptr<Scene>& scene){
	FOREACH(const shared_ptr<Field>& f, scene->fields) if(dynamic_pointer_cast<FieldT>(f)) return static_pointer_cast<FieldT>(f);
	cerr<<"(debug) Creating new field #"<<scene->fields.size()<<" "<<typeid(FieldT).name()<<endl;
	auto f=make_shared<FieldT>();
	scene->fields.push_back(f);
	return f;
}


