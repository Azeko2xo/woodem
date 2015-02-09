#include<woo/pkg/dem/Ice.hpp>
WOO_PLUGIN(dem,(IceMat)(IcePhys)(Cp2_IceMat_IcePhys)(Law2_L6Geom_IcePhys));
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_IceMat__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_IcePhys__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cp2_IceMat_IcePhys__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Law2_L6Geom_IcePhys__CLASS_BASE_DOC_ATTRS_PY);



void Cp2_IceMat_IcePhys::go(const shared_ptr<Material>& m1, const shared_ptr<Material>& m2, const shared_ptr<Contact>& C){ throw std::runtime_error("Cp2_IceMat_IcePhys: not yet implemented."); }

bool Law2_L6Geom_IcePhys::go(const shared_ptr<CGeom>& cg, const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C){ throw std::runtime_error("Law2_L6Geom_IcePhys: not yet implemented."); }

