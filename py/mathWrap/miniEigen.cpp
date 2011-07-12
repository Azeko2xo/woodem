// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#include<boost/python.hpp>
#include<boost/lexical_cast.hpp>
#include<boost/algorithm/string/trim.hpp>
#include<string>
#include<stdexcept>
#include<sstream>
#include<iostream>
#include<iomanip>
#include<vector>

#include<yade/lib/base/Math.hpp>
#include<yade/lib/pyutil/doc_opts.hpp>
#include<yade/lib/pyutil/except.hpp>

using boost::lexical_cast;
namespace py=boost::python;

#define IDX_CHECK(i,MAX){ if(i<0 || i>=MAX) { yade::IndexError("Index out of range 0.." + boost::lexical_cast<std::string>(MAX-1)); } }
#define IDX2_CHECKED_TUPLE_INTS(tuple,max2,arr2) {int l=py::len(tuple); if(l!=2) { yade::IndexError("Index must be integer or a 2-tuple"); } for(int _i=0; _i<2; _i++) { py::extract<int> val(tuple[_i]); if(!val.check()) yade::ValueError("Unable to convert "+boost::lexical_cast<std::string>(_i)+"-th index to int."); int v=val(); IDX_CHECK(v,max2[_i]); arr2[_i]=v; }  }

void VectorXr_set_item(VectorXr & self, int idx, Real value){ IDX_CHECK(idx,self.size()); self[idx]=value; }
void Vector6r_set_item(Vector6r & self, int idx, Real value){ IDX_CHECK(idx,6); self[idx]=value; }
void Vector6i_set_item(Vector6i & self, int idx, int  value){ IDX_CHECK(idx,6); self[idx]=value; }
void Vector3r_set_item(Vector3r & self, int idx, Real value){ IDX_CHECK(idx,3); self[idx]=value; }
void Vector3i_set_item(Vector3i & self, int idx, int  value){ IDX_CHECK(idx,3); self[idx]=value; }
void Vector2r_set_item(Vector2r & self, int idx, Real value){ IDX_CHECK(idx,2); self[idx]=value; }
void Vector2i_set_item(Vector2i & self, int idx, int  value){ IDX_CHECK(idx,2); self[idx]=value; }

void Quaternionr_set_item(Quaternionr & self, int idx, Real value){ IDX_CHECK(idx,4);  if(idx==0) self.x()=value; else if(idx==1) self.y()=value; else if(idx==2) self.z()=value; else if(idx==3) self.w()=value; }

void Matrix3r_set_item(Matrix3r & self, py::tuple _idx, Real value){ int idx[2]; int mx[2]={3,3}; IDX2_CHECKED_TUPLE_INTS(_idx,mx,idx); self(idx[0],idx[1])=value; }
void Matrix3r_set_row (Matrix3r & self, int idx, const Vector3r& row){ IDX_CHECK(idx,3); self.row(idx)=row; }
void Matrix6r_set_item(Matrix6r & self, py::tuple _idx, Real value){ int idx[2]; int mx[2]={6,6}; IDX2_CHECKED_TUPLE_INTS(_idx,mx,idx); self(idx[0],idx[1])=value; }
void Matrix6r_set_row (Matrix6r & self, int idx, const Vector6r& row){ IDX_CHECK(idx,6); self.row(idx)=row; }
void MatrixXr_set_item(MatrixXr & self, py::tuple _idx, Real value){ int idx[2]; long mx[2]={self.rows(),self.cols()}; IDX2_CHECKED_TUPLE_INTS(_idx,mx,idx); self(idx[0],idx[1])=value; }
void MatrixXr_set_row (MatrixXr & self, int idx, const VectorXr& row){ IDX_CHECK(idx,self.rows()); self.row(idx)=row; }



Real VectorXr_get_item(const VectorXr & self, int idx){ IDX_CHECK(idx,self.size()); return self[idx]; }
Real Vector6r_get_item(const Vector6r & self, int idx){ IDX_CHECK(idx,6); return self[idx]; }
int  Vector6i_get_item(const Vector6r & self, int idx){ IDX_CHECK(idx,6); return self[idx]; }
Real Vector3r_get_item(const Vector3r & self, int idx){ IDX_CHECK(idx,3); return self[idx]; }
int  Vector3i_get_item(const Vector3i & self, int idx){ IDX_CHECK(idx,3); return self[idx]; }
Real Vector2r_get_item(const Vector2r & self, int idx){ IDX_CHECK(idx,2); return self[idx]; }
int  Vector2i_get_item(const Vector2i & self, int idx){ IDX_CHECK(idx,2); return self[idx]; }

template<typename MatrixType>
MatrixType Matrix_pruned(const MatrixType& obj, typename MatrixType::Scalar absTol=1e-6){ MatrixType ret(MatrixType::Zero()); for(int i=0;i<obj.rows();i++){ for(int j=0;j<obj.cols();j++){ if(std::abs(obj(i,j))>absTol  && obj(i,j)!=-0) ret(i,j)=obj(i,j); } } return ret; }

template<typename MatrixType>
typename MatrixType::Scalar Matrix_maxAbsCoeff(const MatrixType& obj){ return Eigen::Array<typename MatrixType::Scalar,MatrixType::RowsAtCompileTime,MatrixType::ColsAtCompileTime>(obj).abs().maxCoeff(); }


Real Quaternionr_get_item(const Quaternionr & self, int idx){ IDX_CHECK(idx,4); if(idx==0) return self.x(); if(idx==1) return self.y(); if(idx==2) return self.z(); return self.w(); }
Real     Matrix3r_get_item(Matrix3r & self, py::tuple _idx){ int idx[2]; int mx[2]={3,3}; IDX2_CHECKED_TUPLE_INTS(_idx,mx,idx); return self(idx[0],idx[1]); }
Vector3r Matrix3r_get_row (Matrix3r & self, int idx){ IDX_CHECK(idx,3); return self.row(idx); }
Real     Matrix6r_get_item(Matrix6r & self, py::tuple _idx){ int idx[2]; int mx[2]={6,6}; IDX2_CHECKED_TUPLE_INTS(_idx,mx,idx); return self(idx[0],idx[1]); }
Vector6r Matrix6r_get_row (Matrix6r & self, int idx){ IDX_CHECK(idx,6); return self.row(idx); }
Real     MatrixXr_get_item(MatrixXr & self, py::tuple _idx){ int idx[2]; long mx[2]={self.rows(),self.cols()}; IDX2_CHECKED_TUPLE_INTS(_idx,mx,idx); return self(idx[0],idx[1]); }
Vector6r MatrixXr_get_row (MatrixXr & self, int idx){ IDX_CHECK(idx,self.rows()); return self.row(idx); }

std::string Vector6r_str(const Vector6r & self){ return std::string("Vector6(")+boost::lexical_cast<std::string>(self[0])+","+boost::lexical_cast<std::string>(self[1])+","+boost::lexical_cast<std::string>(self[2])+", "+boost::lexical_cast<std::string>(self[3])+","+boost::lexical_cast<std::string>(self[4])+","+boost::lexical_cast<std::string>(self[5])+")";}
std::string VectorXr_str(const VectorXr & self){ std::ostringstream oss; oss<<"VectorX(["; for(int i=0; i<self.size(); i++) oss<<(i==0?"":((i%3)?",":", "))<<boost::lexical_cast<std::string>(self[i]); oss<<"])"; return oss.str(); }
std::string Vector6i_str(const Vector6i & self){ return std::string("Vector6i(")+boost::lexical_cast<std::string>(self[0])+","+boost::lexical_cast<std::string>(self[1])+","+boost::lexical_cast<std::string>(self[2])+", "+boost::lexical_cast<std::string>(self[3])+","+boost::lexical_cast<std::string>(self[4])+","+boost::lexical_cast<std::string>(self[5])+")";}
std::string Vector3r_str(const Vector3r & self){ return std::string("Vector3(")+boost::lexical_cast<std::string>(self[0])+","+boost::lexical_cast<std::string>(self[1])+","+boost::lexical_cast<std::string>(self[2])+")";}
std::string Vector3i_str(const Vector3i & self){ return std::string("Vector3i(")+boost::lexical_cast<std::string>(self[0])+","+boost::lexical_cast<std::string>(self[1])+","+boost::lexical_cast<std::string>(self[2])+")";}
std::string Vector2r_str(const Vector2r & self){ return std::string("Vector2(")+boost::lexical_cast<std::string>(self[0])+","+boost::lexical_cast<std::string>(self[1])+")";}
std::string Vector2i_str(const Vector2i & self){ return std::string("Vector2i(")+boost::lexical_cast<std::string>(self[0])+","+boost::lexical_cast<std::string>(self[1])+")";}
std::string Quaternionr_str(const Quaternionr & self){ AngleAxisr aa(self); return std::string("Quaternion((")+boost::lexical_cast<std::string>(aa.axis()[0])+","+boost::lexical_cast<std::string>(aa.axis()[1])+","+boost::lexical_cast<std::string>(aa.axis()[2])+"),"+boost::lexical_cast<std::string>(aa.angle())+")";}
std::string Matrix3r_str(const Matrix3r & self){ std::ostringstream oss; oss<<"Matrix3("; for(int i=0; i<3; i++) for(int j=0; j<3; j++) oss<<self(i,j)<<((i==2 && j==2)?")":",")<<((i<2 && j==2)?" ":""); return oss.str(); }
//std::string Matrix6r_str(const Matrix6r & self){ std::ostringstream oss; oss<<"Matrix6(\n"; for(int i=0; i<6; i++) for(int j=0; j<6; j++) oss<<((j==0)?"\t":"")<<self(i,j)<<((i==5 && j==5)?")":",")<<((i<5 && j==5)?" ":""); return oss.str(); }
std::string Matrix6r_str(const Matrix6r & self){ std::ostringstream oss; oss<<"Matrix6(\n"; for(int i=0; i<6; i++){ oss<<"\t("; for(int j=0; j<6; j++) oss<<std::setw(7)<<self(i,j)<<(j==2?", ":(j==5?"),\n":",")); } oss<<")"; return oss.str(); }
std::string MatrixXr_str(const MatrixXr & self){ std::ostringstream oss; oss<<"MatrixX(\n"; for(int i=0; i<self.rows(); i++){ oss<<"\t("; for(int j=0; j<self.cols(); j++) oss<<std::setw(7)<<self(i,j)<<((((j+1)%3)==0 && j!=self.cols()-1)?", ":(j==(self.cols()-1)?"),\n":",")); } oss<<")"; return oss.str(); }

int Vector6r_len(){return 6;}
int Vector6i_len(){return 6;}
int Vector3r_len(){return 3;}
int Vector3i_len(){return 3;}
int Vector2r_len(){return 2;}
int Vector2i_len(){return 2;}
int Quaternionr_len(){return 4;}
int Matrix3r_len(){return 3;} // rows
int Matrix6r_len(){return 6;} // rows

static int MatrixXr_len(const MatrixXr& self){return self.rows();} // rows
static int VectorXr_len(const VectorXr& self){return self.size();}

// pickling support
struct Matrix3r_pickle: py::pickle_suite{static py::tuple getinitargs(const Matrix3r& x){ return py::make_tuple(x(0,0),x(0,1),x(0,2),x(1,0),x(1,1),x(1,2),x(2,0),x(2,1),x(2,2));} };
struct Matrix6r_pickle: py::pickle_suite{static py::tuple getinitargs(const Matrix6r& x){ return py::make_tuple(x.row(0),x.row(1),x.row(2),x.row(3),x.row(4),x.row(5));} };
struct Quaternionr_pickle: py::pickle_suite{static py::tuple getinitargs(const Quaternionr& x){ return py::make_tuple(x.w(),x.x(),x.y(),x.z());} };
struct Vector6r_pickle: py::pickle_suite{static py::tuple getinitargs(const Vector6r& x){ return py::make_tuple(x[0],x[1],x[2],x[3],x[4],x[5]);} };
struct Vector6i_pickle: py::pickle_suite{static py::tuple getinitargs(const Vector6i& x){ return py::make_tuple(x[0],x[1],x[2],x[3],x[4],x[5]);} };
struct Vector3r_pickle: py::pickle_suite{static py::tuple getinitargs(const Vector3r& x){ return py::make_tuple(x[0],x[1],x[2]);} };
struct Vector3i_pickle: py::pickle_suite{static py::tuple getinitargs(const Vector3i& x){ return py::make_tuple(x[0],x[1],x[2]);} };
struct Vector2r_pickle: py::pickle_suite{static py::tuple getinitargs(const Vector2r& x){ return py::make_tuple(x[0],x[1]);} };
struct Vector2i_pickle: py::pickle_suite{static py::tuple getinitargs(const Vector2i& x){ return py::make_tuple(x[0],x[1]);} };
struct VectorXr_pickle: py::pickle_suite{static py::tuple getinitargs(const VectorXr& x){ return py::make_tuple(py::list(x)); } };
struct MatrixXr_pickle: py::pickle_suite{static py::tuple getinitargs(const MatrixXr& x){ std::vector<VectorXr> vv; for(int i=0; i<x.rows(); i++) vv[i]=x.row(i); return py::tuple(x); } };


/* template to define custom converter from sequence/list or approriate length and type, to eigen's Vector
   - length is stored in VT::RowsAtCompileTime
	- type is VT::Scalar
*/
template<class VT>
struct custom_VectorAnyAny_from_sequence{
	custom_VectorAnyAny_from_sequence(){ py::converter::registry::push_back(&convertible,&construct,py::type_id<VT>()); }
	static void* convertible(PyObject* obj_ptr){ if(!PySequence_Check(obj_ptr) || (VT::RowsAtCompileTime!=Eigen::Dynamic && (PySequence_Size(obj_ptr)!=VT::RowsAtCompileTime))) return 0; return obj_ptr; }
	static void construct(PyObject* obj_ptr, py::converter::rvalue_from_python_stage1_data* data){
		void* storage=((py::converter::rvalue_from_python_storage<VT>*)(data))->storage.bytes;
		new (storage) VT; size_t len;
		if(VT::RowsAtCompileTime!=Eigen::Dynamic){ len=VT::RowsAtCompileTime; }
		else{ len=PySequence_Size(obj_ptr); ((VT*)storage)->resize(len); }
		for(size_t i=0; i<len; i++) (*((VT*)storage))[i]=py::extract<typename VT::Scalar>(PySequence_GetItem(obj_ptr,i));
		data->convertible=storage;
	}
};

static Matrix3r* Matrix3r_fromElements(Real m00, Real m01, Real m02, Real m10, Real m11, Real m12, Real m20, Real m21, Real m22){ Matrix3r* m(new Matrix3r); (*m)<<m00,m01,m02,m10,m11,m12,m20,m21,m22; return m; }
static Matrix3r* Matrix3r_fromRows(const Vector3r& l0, const Vector3r& l1, const Vector3r& l2, bool cols=false){ Matrix3r* m(new Matrix3r); if(cols){m->col(0)=l0; m->col(1)=l1; m->col(2)=l2; } else {m->row(0)=l0; m->row(1)=l1; m->row(2)=l2;} return m; }
static Matrix3r* Matrix3r_fromDiagonal(const Vector3r& d){ Matrix3r* m(new Matrix3r); *m=d.asDiagonal(); return m; }
static MatrixXr* MatrixXr_fromDiagonal(const VectorXr& d){ MatrixXr* m(new MatrixXr); *m=d.asDiagonal(); return m;}
static MatrixXr* MatrixXr_fromRowSeq(const std::vector<VectorXr>& rr,bool setCols){
	int rows=rr.size(),cols=rr.size()>0?rr[0].size():0;
	for(int i=1; i<rows; i++) if(rr[i].size()!=cols) throw std::invalid_argument(("Matrix6r: all rows must have the same length."));
	MatrixXr* m;
	m=setCols?new MatrixXr(cols,rows):new MatrixXr(rows,cols);
	for(int i=0; i<rows; i++){ if(setCols) m->col(i)=rr[i]; else m->row(i)=rr[i]; }
	return m;
};
static MatrixXr* MatrixXr_fromRows(const VectorXr& r0, const VectorXr& r1, const VectorXr& r2, const VectorXr& r3, const VectorXr& r4, const VectorXr& r5, const VectorXr& r6, const VectorXr& r7, const VectorXr& r8, const VectorXr& r9, bool setCols){
	/* check vector dimensions */ VectorXr rr[]={r0,r1,r2,r3,r4,r5,r6,r7,r8,r9};
	int cols=-1, rows=-1;
	for(int i=0; i<10; i++){
		if(rows<0 && rr[i].size()==0) rows=i;
		if(rows>=0 && rr[i].size()>0) throw std::invalid_argument("Matrix6r: non-empty rows not allowed after first empty row, which marks end of the matrix.");
	}
	cols=(rows>0?rr[0].size():0);
	for(int i=1; i<rows; i++) if(rr[i].size()!=cols) throw std::invalid_argument(("Matrix6r: all non-empty rows must have the same length (0th row has "+lexical_cast<std::string>(rr[0].size())+" items, "+lexical_cast<std::string>(i)+"th row has "+lexical_cast<std::string>(rr[i].size())+" items)").c_str());
	MatrixXr* m;
	m=setCols?new MatrixXr(cols,rows):new MatrixXr(rows,cols);
	for(int i=0; i<rows; i++){ if(setCols) m->col(i)=rr[i]; else m->row(i)=rr[i]; }
	return m;
}
static Matrix6r* Matrix6r_fromBlocks(const Matrix3r& ul, const Matrix3r& ur, const Matrix3r& ll, const Matrix3r& lr){ Matrix6r* m(new Matrix6r); (*m)<<ul,ur,ll,lr; return m; }
static Matrix6r* Matrix6r_fromRows(const Vector6r& l0, const Vector6r& l1, const Vector6r& l2, const Vector6r& l3, const Vector6r& l4, const Vector6r& l5, bool cols=false){ Matrix6r* m(new Matrix6r); if(cols){ m->col(0)=l0; m->col(1)=l1; m->col(2)=l2; m->col(3)=l3; m->col(4)=l4; m->col(5)=l5; } else { m->row(0)=l0; m->row(1)=l1; m->row(2)=l2; m->row(3)=l3; m->row(4)=l4; m->row(5)=l5; } return m; }
static Matrix6r* Matrix6r_fromDiagonal(const Vector6r& d){ Matrix6r* m(new Matrix6r); *m=d.asDiagonal(); return m; }

static Vector6r* Vector6r_fromElements(Real v0, Real v1, Real v2, Real v3, Real v4, Real v5){ Vector6r* v(new Vector6r); (*v)<<v0,v1,v2,v3,v4,v5; return v; }
static VectorXr* VectorXr_fromList(const std::vector<Real>& ii){ VectorXr* v(new VectorXr(ii.size())); for(size_t i=0; i<ii.size(); i++) (*v)[i]=ii[i]; return v; }
static Vector6i* Vector6i_fromElements(int v0, int v1, int v2, int v3, int v4, int v5){ Vector6i* v(new Vector6i); (*v)<<v0,v1,v2,v3,v4,v5; return v; }
static Vector3r Matrix3r_diagonal(const Matrix3r& m){ return Vector3r(m.diagonal()); }
static Vector6r Matrix6r_diagonal(const Matrix6r& m){ return Vector6r(m.diagonal()); }
static VectorXr MatrixXr_diagonal(const MatrixXr& m){ return VectorXr(m.diagonal()); }
static Vector3r Matrix3r_row(const Matrix3r& m, int ix){ IDX_CHECK(ix,3); return Vector3r(m.row(ix)); }
static Vector3r Matrix3r_col(const Matrix3r& m, int ix){ IDX_CHECK(ix,3); return Vector3r(m.col(ix)); }
static Vector6r Matrix6r_row(const Matrix6r& m, int ix){ IDX_CHECK(ix,6); return Vector6r(m.row(ix)); }
static Vector6r Matrix6r_col(const Matrix6r& m, int ix){ IDX_CHECK(ix,6); return Vector6r(m.col(ix)); }
static VectorXr MatrixXr_row(const MatrixXr& m, int ix){ IDX_CHECK(ix,m.rows()); return VectorXr(m.row(ix)); }
static VectorXr MatrixXr_col(const MatrixXr& m, int ix){ IDX_CHECK(ix,m.cols()); return VectorXr(m.col(ix)); }
static Matrix3r Matrix6r_ul(const Matrix6r& m){ return Matrix3r(m.block<3,3>(0,0)); }
static Matrix3r Matrix6r_ur(const Matrix6r& m){ return Matrix3r(m.block<3,3>(0,3)); }
static Matrix3r Matrix6r_ll(const Matrix6r& m){ return Matrix3r(m.block<3,3>(3,0)); }
static Matrix3r Matrix6r_lr(const Matrix6r& m){ return Matrix3r(m.block<3,3>(3,3)); }

static Vector6r Matrix3r_toVoigt(const Matrix3r& m, bool strain=false){ return tensor_toVoigt(m,strain); }
static Matrix3r Vector6r_toSymmTensor(const Vector6r& v, bool strain=false){ return voigt_toSymmTensor(v,strain); }

static void MatrixXr_resize(Matrix3r& m, int rows, int cols){ m.resize(rows,cols); }
static void VectorXr_resize(VectorXr& v, int n){ v.resize(n); }

static MatrixXr MatrixXr_pseudoInverse_wrapper(const MatrixXr& m){ MatrixXr ret; bool res=MatrixXr_pseudoInverse(m,ret); if(!res) throw std::invalid_argument("MatrixXr_pseudoInverse failed (incompatible matrix dimensions?)"); return ret; }

static Quaternionr Quaternionr_setFromTwoVectors(Quaternionr& q, const Vector3r& u, const Vector3r& v){ return q.setFromTwoVectors(u,v); }
static Quaternionr Quaternionr_random(Quaternionr& self){
	// thanks to http://planning.cs.uiuc.edu/node198.html
	Real u1=Mathr::UnitRandom(), u2=Mathr::UnitRandom(), u3=Mathr::UnitRandom();
	self=Quaternionr(sqrt(1-u1)*sin(2*Mathr::PI*u2),sqrt(1-u1)*cos(2*Mathr::PI*u2),sqrt(u1)*sin(2*Mathr::PI*u3),sqrt(u1)*cos(2*Mathr::PI*u3))
	; /* self.normalize(); */ return self;
}
static Vector3r Quaternionr_Rotate(Quaternionr& q, const Vector3r& u){ return q*u; }
// swizzles for Vector3r
static Vector2r Vector3r_xy(const Vector3r& v){ return Vector2r(v[0],v[1]); }
static Vector2r Vector3r_yx(const Vector3r& v){ return Vector2r(v[1],v[0]); }
static Vector2r Vector3r_xz(const Vector3r& v){ return Vector2r(v[0],v[2]); }
static Vector2r Vector3r_zx(const Vector3r& v){ return Vector2r(v[2],v[0]); }
static Vector2r Vector3r_yz(const Vector3r& v){ return Vector2r(v[1],v[2]); }
static Vector2r Vector3r_zy(const Vector3r& v){ return Vector2r(v[2],v[1]); }
// supposed to return raw pointer (or auto_ptr), boost::python takes care of the lifetime management
static Quaternionr* Quaternionr_fromAxisAngle(const Vector3r& axis, const Real angle){ return new Quaternionr(AngleAxisr(angle,axis)); }
static Quaternionr* Quaternionr_fromAngleAxis(const Real angle, const Vector3r& axis){ return new Quaternionr(AngleAxisr(angle,axis)); }
static py::tuple Quaternionr_toAxisAngle(const Quaternionr& self){ AngleAxisr aa(self); return py::make_tuple(aa.axis(),aa.angle());}
static py::tuple Quaternionr_toAngleAxis(const Quaternionr& self){ AngleAxisr aa(self); return py::make_tuple(aa.angle(),aa.axis());}

static Real Vector3r_dot(const Vector3r& self, const Vector3r& v){ return self.dot(v); }
static Real Vector3i_dot(const Vector3i& self, const Vector3i& v){ return self.dot(v); }
static Real Vector2r_dot(const Vector2r& self, const Vector2r& v){ return self.dot(v); }
static Real Vector2i_dot(const Vector2i& self, const Vector2i& v){ return self.dot(v); }
static Vector3r Vector3r_cross(const Vector3r& self, const Vector3r& v){ return self.cross(v); }
static Vector3i Vector3i_cross(const Vector3i& self, const Vector3i& v){ return self.cross(v); }
static Vector3r Vector6r_head(const Vector6r& self){ return self.start<3>(); }
static Vector3r Vector6r_tail(const Vector6r& self){ return self.end<3>(); }
static Vector3i Vector6i_head(const Vector6i& self){ return self.start<3>(); }
static Vector3i Vector6i_tail(const Vector6i& self){ return self.end<3>(); }
static bool Quaternionr__eq__(const Quaternionr& q1, const Quaternionr& q2){ return q1==q2; }
static bool Quaternionr__neq__(const Quaternionr& q1, const Quaternionr& q2){ return q1!=q2; }
#include<Eigen/SVD>
static py::tuple Matrix3r_polarDecomposition(const Matrix3r& self){ Matrix3r unitary,positive; Matrix_computeUnitaryPositive(self,&unitary,&positive); return py::make_tuple(unitary,positive); }
static py::tuple Matrix3r_symmEigen(const Matrix3r& self){ Eigen::SelfAdjointEigenSolver<Matrix3r> a(self); return py::make_tuple(a.eigenvectors(),a.eigenvalues()); } 

static Vector3r Vector3r_Unit(int ax){ IDX_CHECK(ax,3); Vector3r ret=Vector3r::Zero(); ret[ax]=1; return ret; }
static Vector3i Vector3i_Unit(int ax){ IDX_CHECK(ax,3); Vector3i ret=Vector3i::Zero(); ret[ax]=1; return ret; }
static Vector2r Vector2r_Unit(int ax){ IDX_CHECK(ax,2); Vector2r ret=Vector2r::Zero(); ret[ax]=1; return ret; }
static Vector2i Vector2i_Unit(int ax){ IDX_CHECK(ax,2); Vector2i ret=Vector2i::Zero(); ret[ax]=1; return ret; }


static Matrix3r Vector3r_asDiagonal(const Vector3r& self){ return self.asDiagonal(); }
static Matrix6r Vector6r_asDiagonal(const Vector6r& self){ return self.asDiagonal(); }
static MatrixXr VectorXr_asDiagonal(const VectorXr& self){ return self.asDiagonal(); }


#undef IDX_CHECK


#define EIG_WRAP_METH0_ROWS_COLS(klass,meth) static klass klass##_##meth(int rows, int cols){ return klass::meth(rows,cols); }
#define EIG_WRAP_METH0_SIZE(klass,meth) static klass klass##_##meth(int size){ return klass::meth(size); }
EIG_WRAP_METH0_ROWS_COLS(MatrixXr,Zero)
EIG_WRAP_METH0_ROWS_COLS(MatrixXr,Ones)
EIG_WRAP_METH0_ROWS_COLS(MatrixXr,Identity)
EIG_WRAP_METH0_SIZE(VectorXr,Zero)
EIG_WRAP_METH0_SIZE(VectorXr,Ones)

#define EIG_WRAP_METH1(klass,meth) static klass klass##_##meth(const klass& self){ return self.meth(); }
#define EIG_WRAP_METH0(klass,meth) static klass klass##_##meth(){ return klass().meth(); }
EIG_WRAP_METH1(Matrix3r,transpose);
EIG_WRAP_METH1(Matrix3r,inverse);
EIG_WRAP_METH1(Matrix6r,transpose);
EIG_WRAP_METH1(Matrix6r,inverse);
EIG_WRAP_METH1(MatrixXr,transpose);
EIG_WRAP_METH1(MatrixXr,inverse);

EIG_WRAP_METH0(Matrix3r,Zero);
EIG_WRAP_METH0(Matrix3r,Identity);
EIG_WRAP_METH0(Matrix3r,Ones);
EIG_WRAP_METH0(Matrix6r,Zero);
EIG_WRAP_METH0(Matrix6r,Identity);
EIG_WRAP_METH0(Matrix6r,Ones);
EIG_WRAP_METH0(Vector6r,Zero); EIG_WRAP_METH0(Vector6r,Ones);
EIG_WRAP_METH0(Vector6i,Zero); EIG_WRAP_METH0(Vector6i,Ones);
EIG_WRAP_METH0(Vector3r,Zero); EIG_WRAP_METH0(Vector3r,UnitX); EIG_WRAP_METH0(Vector3r,UnitY); EIG_WRAP_METH0(Vector3r,UnitZ); EIG_WRAP_METH0(Vector3r,Ones);
EIG_WRAP_METH0(Vector3i,Zero); EIG_WRAP_METH0(Vector3i,UnitX); EIG_WRAP_METH0(Vector3i,UnitY); EIG_WRAP_METH0(Vector3i,UnitZ); EIG_WRAP_METH0(Vector3i,Ones);
EIG_WRAP_METH0(Vector2r,Zero); EIG_WRAP_METH0(Vector2r,UnitX); EIG_WRAP_METH0(Vector2r,UnitY); EIG_WRAP_METH0(Vector2r,Ones);
EIG_WRAP_METH0(Vector2i,Zero); EIG_WRAP_METH0(Vector2i,UnitX); EIG_WRAP_METH0(Vector2i,UnitY); EIG_WRAP_METH0(Vector2i,Ones);
EIG_WRAP_METH0(Quaternionr,Identity);

#define EIG_OP1(klass,op,sym) decltype((sym klass()).eval()) klass##op(const klass& self){ return (sym self).eval();}
#define EIG_OP2(klass,op,sym,klass2) decltype((klass() sym klass2()).eval()) klass##op##klass2(const klass& self, const klass2& other){ return (self sym other).eval(); }
#define EIG_OP2_INPLACE(klass,op,sym,klass2) klass klass##op##klass2(klass& self, const klass2& other){ self sym other; return self; }


EIG_OP1(Matrix3r,__neg__,-)
EIG_OP2(Matrix3r,__add__,+,Matrix3r) EIG_OP2_INPLACE(Matrix3r,__iadd__,+=,Matrix3r)
EIG_OP2(Matrix3r,__sub__,-,Matrix3r) EIG_OP2_INPLACE(Matrix3r,__isub__,-=,Matrix3r)
EIG_OP2(Matrix3r,__mul__,*,Real) EIG_OP2(Matrix3r,__rmul__,*,Real) EIG_OP2_INPLACE(Matrix3r,__imul__,*=,Real)
EIG_OP2(Matrix3r,__mul__,*,int) EIG_OP2(Matrix3r,__rmul__,*,int) EIG_OP2_INPLACE(Matrix3r,__imul__,*=,int)
EIG_OP2(Matrix3r,__mul__,*,Vector3r) EIG_OP2(Matrix3r,__rmul__,*,Vector3r)
EIG_OP2(Matrix3r,__mul__,*,Matrix3r) EIG_OP2_INPLACE(Matrix3r,__imul__,*=,Matrix3r)
EIG_OP2(Matrix3r,__div__,/,Real) EIG_OP2_INPLACE(Matrix3r,__idiv__,/=,Real)
EIG_OP2(Matrix3r,__div__,/,int) EIG_OP2_INPLACE(Matrix3r,__idiv__,/=,int)

EIG_OP1(Matrix6r,__neg__,-)
EIG_OP2(Matrix6r,__add__,+,Matrix6r) EIG_OP2_INPLACE(Matrix6r,__iadd__,+=,Matrix6r)
EIG_OP2(Matrix6r,__sub__,-,Matrix6r) EIG_OP2_INPLACE(Matrix6r,__isub__,-=,Matrix6r)
EIG_OP2(Matrix6r,__mul__,*,Real) EIG_OP2(Matrix6r,__rmul__,*,Real) EIG_OP2_INPLACE(Matrix6r,__imul__,*=,Real)
EIG_OP2(Matrix6r,__mul__,*,int) EIG_OP2(Matrix6r,__rmul__,*,int) EIG_OP2_INPLACE(Matrix6r,__imul__,*=,int)
EIG_OP2(Matrix6r,__mul__,*,Vector6r) EIG_OP2(Matrix6r,__rmul__,*,Vector6r)
EIG_OP2(Matrix6r,__mul__,*,Matrix6r) EIG_OP2_INPLACE(Matrix6r,__imul__,*=,Matrix6r)
EIG_OP2(Matrix6r,__div__,/,Real) EIG_OP2_INPLACE(Matrix6r,__idiv__,/=,Real)
EIG_OP2(Matrix6r,__div__,/,int) EIG_OP2_INPLACE(Matrix6r,__idiv__,/=,int)

EIG_OP1(MatrixXr,__neg__,-)
EIG_OP2(MatrixXr,__add__,+,MatrixXr) EIG_OP2_INPLACE(MatrixXr,__iadd__,+=,MatrixXr)
EIG_OP2(MatrixXr,__sub__,-,MatrixXr) EIG_OP2_INPLACE(MatrixXr,__isub__,-=,MatrixXr)
EIG_OP2(MatrixXr,__mul__,*,Real) EIG_OP2(MatrixXr,__rmul__,*,Real) EIG_OP2_INPLACE(MatrixXr,__imul__,*=,Real)
EIG_OP2(MatrixXr,__mul__,*,int) EIG_OP2(MatrixXr,__rmul__,*,int) EIG_OP2_INPLACE(MatrixXr,__imul__,*=,int)
EIG_OP2(MatrixXr,__mul__,*,VectorXr) EIG_OP2(MatrixXr,__rmul__,*,VectorXr)
EIG_OP2(MatrixXr,__mul__,*,MatrixXr) EIG_OP2_INPLACE(MatrixXr,__imul__,*=,MatrixXr)
EIG_OP2(MatrixXr,__div__,/,Real) EIG_OP2_INPLACE(MatrixXr,__idiv__,/=,Real)
EIG_OP2(MatrixXr,__div__,/,int) EIG_OP2_INPLACE(MatrixXr,__idiv__,/=,int)

EIG_OP1(VectorXr,__neg__,-);
EIG_OP2(VectorXr,__add__,+,VectorXr); EIG_OP2_INPLACE(VectorXr,__iadd__,+=,VectorXr)
EIG_OP2(VectorXr,__sub__,-,VectorXr); EIG_OP2_INPLACE(VectorXr,__isub__,-=,VectorXr)
EIG_OP2(VectorXr,__mul__,*,Real) EIG_OP2(VectorXr,__rmul__,*,Real) EIG_OP2_INPLACE(VectorXr,__imul__,*=,Real) EIG_OP2(VectorXr,__div__,/,Real) EIG_OP2_INPLACE(VectorXr,__idiv__,/=,Real)
EIG_OP2(VectorXr,__mul__,*,int) EIG_OP2(VectorXr,__rmul__,*,int) EIG_OP2_INPLACE(VectorXr,__imul__,*=,int) EIG_OP2(VectorXr,__div__,/,int) EIG_OP2_INPLACE(VectorXr,__idiv__,/=,int)


EIG_OP1(Vector6r,__neg__,-);
EIG_OP2(Vector6r,__add__,+,Vector6r); EIG_OP2_INPLACE(Vector6r,__iadd__,+=,Vector6r)
EIG_OP2(Vector6r,__sub__,-,Vector6r); EIG_OP2_INPLACE(Vector6r,__isub__,-=,Vector6r)
EIG_OP2(Vector6r,__mul__,*,Real) EIG_OP2(Vector6r,__rmul__,*,Real) EIG_OP2_INPLACE(Vector6r,__imul__,*=,Real) EIG_OP2(Vector6r,__div__,/,Real) EIG_OP2_INPLACE(Vector6r,__idiv__,/=,Real)
EIG_OP2(Vector6r,__mul__,*,int) EIG_OP2(Vector6r,__rmul__,*,int) EIG_OP2_INPLACE(Vector6r,__imul__,*=,int) EIG_OP2(Vector6r,__div__,/,int) EIG_OP2_INPLACE(Vector6r,__idiv__,/=,int)

EIG_OP1(Vector6i,__neg__,-);
EIG_OP2(Vector6i,__add__,+,Vector6i); EIG_OP2_INPLACE(Vector6i,__iadd__,+=,Vector6i)
EIG_OP2(Vector6i,__sub__,-,Vector6i); EIG_OP2_INPLACE(Vector6i,__isub__,-=,Vector6i)
EIG_OP2(Vector6i,__mul__,*,int) EIG_OP2(Vector6i,__rmul__,*,int) EIG_OP2_INPLACE(Vector6i,__imul__,*=,int) EIG_OP2(Vector6i,__div__,/,int) EIG_OP2_INPLACE(Vector6i,__idiv__,/=,int)

EIG_OP1(Vector3r,__neg__,-);
EIG_OP2(Vector3r,__add__,+,Vector3r); EIG_OP2_INPLACE(Vector3r,__iadd__,+=,Vector3r)
EIG_OP2(Vector3r,__sub__,-,Vector3r); EIG_OP2_INPLACE(Vector3r,__isub__,-=,Vector3r)
EIG_OP2(Vector3r,__mul__,*,Real) EIG_OP2(Vector3r,__rmul__,*,Real) EIG_OP2_INPLACE(Vector3r,__imul__,*=,Real) EIG_OP2(Vector3r,__div__,/,Real) EIG_OP2_INPLACE(Vector3r,__idiv__,/=,Real)
EIG_OP2(Vector3r,__mul__,*,int) EIG_OP2(Vector3r,__rmul__,*,int) EIG_OP2_INPLACE(Vector3r,__imul__,*=,int) EIG_OP2(Vector3r,__div__,/,int) EIG_OP2_INPLACE(Vector3r,__idiv__,/=,int)

EIG_OP1(Vector3i,__neg__,-);
EIG_OP2(Vector3i,__add__,+,Vector3i); EIG_OP2_INPLACE(Vector3i,__iadd__,+=,Vector3i)
EIG_OP2(Vector3i,__sub__,-,Vector3i); EIG_OP2_INPLACE(Vector3i,__isub__,-=,Vector3i)
EIG_OP2(Vector3i,__mul__,*,int) EIG_OP2(Vector3i,__rmul__,*,int)  EIG_OP2_INPLACE(Vector3i,__imul__,*=,int)

EIG_OP1(Vector2r,__neg__,-);
EIG_OP2(Vector2r,__add__,+,Vector2r); EIG_OP2_INPLACE(Vector2r,__iadd__,+=,Vector2r)
EIG_OP2(Vector2r,__sub__,-,Vector2r); EIG_OP2_INPLACE(Vector2r,__isub__,-=,Vector2r)
EIG_OP2(Vector2r,__mul__,*,Real) EIG_OP2(Vector2r,__rmul__,*,Real) EIG_OP2_INPLACE(Vector2r,__imul__,*=,Real) EIG_OP2(Vector2r,__div__,/,Real) EIG_OP2_INPLACE(Vector2r,__idiv__,/=,Real)
EIG_OP2(Vector2r,__mul__,*,int) EIG_OP2(Vector2r,__rmul__,*,int) EIG_OP2_INPLACE(Vector2r,__imul__,*=,int) EIG_OP2(Vector2r,__div__,/,int) EIG_OP2_INPLACE(Vector2r,__idiv__,/=,int)

EIG_OP1(Vector2i,__neg__,-);
EIG_OP2(Vector2i,__add__,+,Vector2i); EIG_OP2_INPLACE(Vector2i,__iadd__,+=,Vector2i)
EIG_OP2(Vector2i,__sub__,-,Vector2i); EIG_OP2_INPLACE(Vector2i,__isub__,-=,Vector2i)
EIG_OP2(Vector2i,__mul__,*,int)  EIG_OP2_INPLACE(Vector2i,__imul__,*=,int) EIG_OP2(Vector2i,__rmul__,*,int)


BOOST_PYTHON_MODULE(miniEigen){
	py::scope().attr("__doc__")="Basic math functions for Yade: small matrix, vector and quaternion classes. This module internally wraps small parts of the `Eigen <http://eigen.tuxfamily.org>`_ library. Refer to its documentation for details. All classes in this module support pickling.";

	YADE_SET_DOCSTRING_OPTS;

	custom_VectorAnyAny_from_sequence<VectorXr>();
	custom_VectorAnyAny_from_sequence<Vector6r>();
	custom_VectorAnyAny_from_sequence<Vector6i>();
	custom_VectorAnyAny_from_sequence<Vector3r>();
	custom_VectorAnyAny_from_sequence<Vector3i>();
	custom_VectorAnyAny_from_sequence<Vector2r>();
	custom_VectorAnyAny_from_sequence<Vector2i>();

	py::class_<Matrix3r>("Matrix3","3x3 float matrix.\n\nSupported operations (``m`` is a Matrix3, ``f`` if a float/int, ``v`` is a Vector3): ``-m``, ``m+m``, ``m+=m``, ``m-m``, ``m-=m``, ``m*f``, ``f*m``, ``m*=f``, ``m/f``, ``m/=f``, ``m*m``, ``m*=m``, ``m*v``, ``v*m``, ``m==m``, ``m!=m``.",py::init<>())
		.def(py::init<Matrix3r const &>((py::arg("m"))))
		.def(py::init<Quaternionr const &>((py::arg("q"))))
		.def("__init__",py::make_constructor(&Matrix3r_fromElements,py::default_call_policies(),(py::arg("m00"),py::arg("m01"),py::arg("m02"),py::arg("m10"),py::arg("m11"),py::arg("m12"),py::arg("m20"),py::arg("m21"),py::arg("m22"))))
		.def("__init__",py::make_constructor(&Matrix3r_fromRows,py::default_call_policies(),(py::arg("r0"),py::arg("r1"),py::arg("r2"),py::arg("cols")=false)))
		.def("__init__",py::make_constructor(&Matrix3r_fromDiagonal,py::default_call_policies(),(py::arg("diag"))))
		.def_pickle(Matrix3r_pickle())
		//
		.def("determinant",&Matrix3r::determinant)
		.def("trace",&Matrix3r::trace)
		.def("norm",&Matrix3r::norm)
		.def("inverse",&Matrix3r_inverse)
		.def("transpose",&Matrix3r_transpose)
		.def("polarDecomposition",&Matrix3r_polarDecomposition)
		.def("symmEigen",&Matrix3r_symmEigen)
		.def("diagonal",&Matrix3r_diagonal)
		.def("row",&Matrix3r_row)
		.def("col",&Matrix3r_col)
		.def("pruned",&Matrix_pruned<Matrix3r>,py::arg("absTol")=1e-6)
		.def("maxAbsCoeff",&Matrix_maxAbsCoeff<Matrix3r>)

		//
		.def("__neg__",&Matrix3r__neg__)
		.def("__add__",&Matrix3r__add__Matrix3r).def("__iadd__",&Matrix3r__iadd__Matrix3r)
		.def("__sub__",&Matrix3r__sub__Matrix3r).def("__isub__",&Matrix3r__isub__Matrix3r)
		.def("__mul__",&Matrix3r__mul__Real).def("__rmul__",&Matrix3r__rmul__Real).def("__imul__",&Matrix3r__imul__Real)
		.def("__mul__",&Matrix3r__mul__int).def("__rmul__",&Matrix3r__rmul__int).def("__imul__",&Matrix3r__imul__int)
		.def("__mul__",&Matrix3r__mul__Vector3r).def("__rmul__",&Matrix3r__rmul__Vector3r)
		.def("__mul__",&Matrix3r__mul__Matrix3r).def("__imul__",&Matrix3r__imul__Matrix3r)
		.def("__div__",&Matrix3r__div__Real).def("__idiv__",&Matrix3r__idiv__Real)
		.def("__div__",&Matrix3r__div__int).def("__idiv__",&Matrix3r__idiv__int)
		.def(py::self == py::self)
		.def(py::self != py::self)
		//
 		.def("__len__",&::Matrix3r_len).staticmethod("__len__").def("__setitem__",&::Matrix3r_set_item).def("__getitem__",&::Matrix3r_get_item).def("__str__",&::Matrix3r_str).def("__repr__",&::Matrix3r_str)
		/* extras for matrices */
		.def("__setitem__",&::Matrix3r_set_row).def("__getitem__",&::Matrix3r_get_row)
		.add_static_property("Identity",&Matrix3r_Identity)
		.add_static_property("Zero",&Matrix3r_Zero)
		.add_static_property("Ones",&Matrix3r_Ones)
		// specials
		.def("toVoigt",&Matrix3r_toVoigt,(py::arg("strain")=false),"Convert 2nd order tensor to 6-vector (Voigt notation), symmetrizing the tensor;	if *strain* is ``True``, multiply non-diagonal compoennts by 2.")
		
	;

	py::class_<Matrix6r>("Matrix6","6x6 float matrix. Constructed from 4 3x3 sub-matrices, from 6xVector6 (rows).\n\nSupported operations (``m`` is a Matrix6, ``f`` if a float/int, ``v`` is a Vector6): ``-m``, ``m+m``, ``m+=m``, ``m-m``, ``m-=m``, ``m*f``, ``f*m``, ``m*=f``, ``m/f``, ``m/=f``, ``m*m``, ``m*=m``, ``m*v``, ``v*m``, ``m==m``, ``m!=m``.",py::init<>())
		.def(py::init<Matrix6r const &>((py::arg("m"))))
		.def("__init__",py::make_constructor(&Matrix6r_fromBlocks,py::default_call_policies(),(py::arg("ul"),py::arg("ur"),py::arg("ll"),py::arg("lr"))))
		.def("__init__",py::make_constructor(&Matrix6r_fromRows,py::default_call_policies(),(py::arg("l0"),py::arg("l1"),py::arg("l2"),py::arg("l3"),py::arg("l4"),py::arg("l5"),py::arg("cols")=false)))
		.def("__init__",py::make_constructor(&Matrix6r_fromDiagonal,py::default_call_policies(),(py::arg("diag"))))
		.def_pickle(Matrix6r_pickle())
		//
		.def("determinant",&Matrix6r::determinant)
		.def("trace",&Matrix6r::trace)
		.def("inverse",&Matrix6r_inverse)
		.def("transpose",&Matrix6r_transpose)
		//.def("polarDecomposition",&Matrix6r_polarDecomposition)
		.def("diagonal",&Matrix6r_diagonal)
		.def("row",&Matrix6r_row)
		.def("col",&Matrix6r_col)
		.def("ul",&Matrix6r_ul)
		.def("ur",&Matrix6r_ur)
		.def("ll",&Matrix6r_ll)
		.def("lr",&Matrix6r_lr)

		//
		.def("__neg__",&Matrix6r__neg__)
		.def("__add__",&Matrix6r__add__Matrix6r).def("__iadd__",&Matrix6r__iadd__Matrix6r)
		.def("__sub__",&Matrix6r__sub__Matrix6r).def("__isub__",&Matrix6r__isub__Matrix6r)
		.def("__mul__",&Matrix6r__mul__Real).def("__rmul__",&Matrix6r__rmul__Real).def("__imul__",&Matrix6r__imul__Real)
		.def("__mul__",&Matrix6r__mul__int).def("__rmul__",&Matrix6r__rmul__int).def("__imul__",&Matrix6r__imul__int)
		.def("__mul__",&Matrix6r__mul__Vector6r).def("__rmul__",&Matrix6r__rmul__Vector6r)
		.def("__mul__",&Matrix6r__mul__Matrix6r).def("__imul__",&Matrix6r__imul__Matrix6r)
		.def("__div__",&Matrix6r__div__Real).def("__idiv__",&Matrix6r__idiv__Real)
		.def("__div__",&Matrix6r__div__int).def("__idiv__",&Matrix6r__idiv__int)
		.def(py::self == py::self)
		.def(py::self != py::self)
		//
 		.def("__len__",&::Matrix6r_len).staticmethod("__len__").def("__setitem__",&::Matrix6r_set_item).def("__getitem__",&::Matrix6r_get_item).def("__str__",&::Matrix6r_str).def("__repr__",&::Matrix6r_str)
		/* extras for matrices */
		.def("__setitem__",&::Matrix6r_set_row).def("__getitem__",&::Matrix6r_get_row)
		.add_static_property("Identity",&Matrix6r_Identity)
		.add_static_property("Zero",&Matrix6r_Zero)
		.add_static_property("Ones",&Matrix6r_Ones)
	;

	py::class_<VectorXr>("VectorX","Dynamic-sized float vector.\n\nSupported operations (``f`` if a float/int, ``v`` is a VectorX): ``-v``, ``v+v``, ``v+=v``, ``v-v``, ``v-=v``, ``v*f``, ``f*v``, ``v*=f``, ``v/f``, ``v/=f``, ``v==v``, ``v!=v``.\n\nImplicit conversion from sequence (list,tuple, …) of X floats.",py::init<>())
		.def(py::init<VectorXr>((py::arg("other"))))
		.def("__init__",py::make_constructor(&VectorXr_fromList,py::default_call_policies(),(py::arg("vv"))))
		.def_pickle(VectorXr_pickle())
		// properties
		.add_static_property("Ones",&VectorXr_Ones).add_static_property("Zero",&VectorXr_Zero)
		// methods
		.def("norm",&VectorXr::norm).def("squaredNorm",&VectorXr::squaredNorm).def("normalize",&VectorXr::normalize).def("normalized",&VectorXr::normalized)
		.def("asDiagonal",&VectorXr_asDiagonal)
		.def("size",&VectorXr::size)
		.def("resize",&VectorXr_resize)
		
		// .def("head",&VectorXr_head).def("tail",&VectorXr_tail)
		// operators
		.def("__neg__",&VectorXr__neg__) // -v
		.def("__add__",&VectorXr__add__VectorXr).def("__iadd__",&VectorXr__iadd__VectorXr) // +, +=
		.def("__sub__",&VectorXr__sub__VectorXr).def("__isub__",&VectorXr__isub__VectorXr) // -, -=
		.def("__mul__",&VectorXr__mul__Real).def("__rmul__",&VectorXr__rmul__Real) // f*v, v*f
		.def("__div__",&VectorXr__div__Real).def("__idiv__",&VectorXr__idiv__Real) // v/f, v/=f
		.def("__mul__",&VectorXr__mul__int).def("__rmul__",&VectorXr__rmul__int) // f*v, v*f
		.def("__div__",&VectorXr__div__int).def("__idiv__",&VectorXr__idiv__int) // v/f, v/=f
		.def(py::self != py::self).def(py::self == py::self)
		// specials
		.def("__abs__",&VectorXr::norm)
		.def("__len__",&::VectorXr_len)
		.def("__setitem__",&::VectorXr_set_item).def("__getitem__",&::VectorXr_get_item)
		.def("__str__",&::VectorXr_str).def("__repr__",&::VectorXr_str)
	;


	py::class_<MatrixXr>("MatrixX","XxX *dynamic-sized) float matrix. Constructed from list of rows (as VectorX).\n\nSupported operations (``m`` is a MatrixX, ``f`` if a float/int, ``v`` is a VectorX): ``-m``, ``m+m``, ``m+=m``, ``m-m``, ``m-=m``, ``m*f``, ``f*m``, ``m*=f``, ``m/f``, ``m/=f``, ``m*m``, ``m*=m``, ``m*v``, ``v*m``, ``m==m``, ``m!=m``.",py::init<>())
		.def(py::init<MatrixXr const &>((py::arg("m"))))
		.def("__init__",py::make_constructor(&MatrixXr_fromRows,py::default_call_policies(),(
			py::arg("r0")=VectorXr(),
			py::arg("r1")=VectorXr(),
			py::arg("r2")=VectorXr(),
			py::arg("r3")=VectorXr(),
			py::arg("r4")=VectorXr(),
			py::arg("r5")=VectorXr(),
			py::arg("r6")=VectorXr(),
			py::arg("r7")=VectorXr(),
			py::arg("r8")=VectorXr(),
			py::arg("r9")=VectorXr(),
			py::arg("cols")=false)
		))
		.def("__init__",py::make_constructor(&MatrixXr_fromDiagonal,py::default_call_policies(),(py::arg("diag"))))
		.def("__init__",py::make_constructor(&MatrixXr_fromRowSeq,py::default_call_policies(),(py::arg("rows"),py::arg("cols")=false)))
		.def_pickle(MatrixXr_pickle())
		//
		.def("determinant",&MatrixXr::determinant)
		.def("trace",&MatrixXr::trace)
		.def("inverse",&MatrixXr_inverse)
		.def("transpose",&MatrixXr_transpose)
		.def("pinv",&MatrixXr_pseudoInverse_wrapper)
		.def("diagonal",&MatrixXr_diagonal)
		.def("row",&MatrixXr_row)
		.def("col",&MatrixXr_col)
		.def("rows",&MatrixXr::rows)
		.def("cols",&MatrixXr::cols)
		.def("resize",&MatrixXr_resize)

		//
		.def("__neg__",&MatrixXr__neg__)
		.def("__add__",&MatrixXr__add__MatrixXr).def("__iadd__",&MatrixXr__iadd__MatrixXr)
		.def("__sub__",&MatrixXr__sub__MatrixXr).def("__isub__",&MatrixXr__isub__MatrixXr)
		.def("__mul__",&MatrixXr__mul__Real).def("__rmul__",&MatrixXr__rmul__Real).def("__imul__",&MatrixXr__imul__Real)
		.def("__mul__",&MatrixXr__mul__int).def("__rmul__",&MatrixXr__rmul__int).def("__imul__",&MatrixXr__imul__int)
		.def("__mul__",&MatrixXr__mul__VectorXr).def("__rmul__",&MatrixXr__rmul__VectorXr)
		.def("__mul__",&MatrixXr__mul__MatrixXr).def("__imul__",&MatrixXr__imul__MatrixXr)
		.def("__div__",&MatrixXr__div__Real).def("__idiv__",&MatrixXr__idiv__Real)
		.def("__div__",&MatrixXr__div__int).def("__idiv__",&MatrixXr__idiv__int)
		.def(py::self == py::self)
		.def(py::self != py::self)
		//
 		.def("__len__",&::MatrixXr_len).def("__setitem__",&::MatrixXr_set_item).def("__getitem__",&::MatrixXr_get_item).def("__str__",&::MatrixXr_str).def("__repr__",&::MatrixXr_str)
		/* extras for matrices */
		.def("__setitem__",&::MatrixXr_set_row).def("__getitem__",&::MatrixXr_get_row)
		.add_static_property("Identity",&MatrixXr_Identity)
		.add_static_property("Zero",&MatrixXr_Zero)
		.add_static_property("Ones",&MatrixXr_Ones)
	;


	py::class_<Quaternionr>("Quaternion","Quaternion representing rotation.\n\nSupported operations (``q`` is a Quaternion, ``v`` is a Vector3): ``q*q`` (rotation composition), ``q*=q``, ``q*v`` (rotating ``v`` by ``q``), ``q==q``, ``q!=q``.",py::init<>())
		.def("__init__",py::make_constructor(&Quaternionr_fromAxisAngle,py::default_call_policies(),(py::arg("axis"),py::arg("angle"))))
		.def("__init__",py::make_constructor(&Quaternionr_fromAngleAxis,py::default_call_policies(),(py::arg("angle"),py::arg("axis"))))
		.def(py::init<Real,Real,Real,Real>((py::arg("w"),py::arg("x"),py::arg("y"),py::arg("z")),"Initialize from coefficients.\n\n.. note:: The order of coefficients is *w*, *x*, *y*, *z*. The [] operator numbers them differently, 0…4 for *x* *y* *z* *w*!"))
		.def(py::init<Matrix3r>((py::arg("rotMatrix")))) //,"Initialize from given rotation matrix.")
		.def(py::init<Quaternionr>((py::arg("other"))))
		.def_pickle(Quaternionr_pickle())
		// properties
		.add_static_property("Identity",&Quaternionr_Identity)
		// methods
		.def("setFromTwoVectors",&Quaternionr_setFromTwoVectors,((py::arg("u"),py::arg("v"))))
		.def("conjugate",&Quaternionr::conjugate)
		.def("toAxisAngle",&Quaternionr_toAxisAngle).def("toAngleAxis",&Quaternionr_toAngleAxis)
		.def("toRotationMatrix",&Quaternionr::toRotationMatrix)
		.def("Rotate",&Quaternionr_Rotate,((py::arg("v"))))
		.def("inverse",&Quaternionr::inverse)
		.def("norm",&Quaternionr::norm)
		.def("normalize",&Quaternionr::normalize)
		.def("normalized",&Quaternionr::normalized)
		.def("random",&Quaternionr_random,"Assign random orientation to the quaternion.")
		// operators
		.def(py::self * py::self)
		.def(py::self *= py::self)
		.def(py::self * py::other<Vector3r>())
		//.def(py::self != py::self).def(py::self == py::self) // these don't work... (?)
		.def("__eq__",&Quaternionr__eq__).def("__neq__",&Quaternionr__neq__)
		// specials
		.def("__abs__",&Quaternionr::norm)
		.def("__len__",&Quaternionr_len).staticmethod("__len__")
		.def("__setitem__",&Quaternionr_set_item).def("__getitem__",&Quaternionr_get_item)
		.def("__str__",&Quaternionr_str).def("__repr__",&Quaternionr_str)
	;



	py::class_<Vector6r>("Vector6","6-dimensional float vector.\n\nSupported operations (``f`` if a float/int, ``v`` is a Vector6): ``-v``, ``v+v``, ``v+=v``, ``v-v``, ``v-=v``, ``v*f``, ``f*v``, ``v*=f``, ``v/f``, ``v/=f``, ``v==v``, ``v!=v``.\n\nImplicit conversion from sequence (list,tuple, …) of 6 floats.",py::init<>())
		.def(py::init<Vector6r>((py::arg("other"))))
		.def("__init__",py::make_constructor(&Vector6r_fromElements,py::default_call_policies(),(py::arg("v0"),py::arg("v1"),py::arg("v2"),py::arg("v3"),py::arg("v4"),py::arg("v5"))))
		.def_pickle(Vector6r_pickle())
		// properties
		.add_static_property("Ones",&Vector6r_Ones).add_static_property("Zero",&Vector6r_Zero)
		//.add_static_property("UnitX",&Vector6r_UnitX).add_static_property("UnitY",&Vector6r_UnitY).add_static_property("UnitZ",&Vector6r_UnitZ)
		// methods
		//.def("dot",&Vector6r_dot).def("cross",&Vector6r_cross)
		.def("norm",&Vector6r::norm).def("squaredNorm",&Vector6r::squaredNorm).def("normalize",&Vector6r::normalize).def("normalized",&Vector6r::normalized)
		.def("head",&Vector6r_head).def("tail",&Vector6r_tail)
		.def("asDiagonal",&Vector6r_asDiagonal)
		// operators
		.def("__neg__",&Vector6r__neg__) // -v
		.def("__add__",&Vector6r__add__Vector6r).def("__iadd__",&Vector6r__iadd__Vector6r) // +, +=
		.def("__sub__",&Vector6r__sub__Vector6r).def("__isub__",&Vector6r__isub__Vector6r) // -, -=
		.def("__mul__",&Vector6r__mul__Real).def("__rmul__",&Vector6r__rmul__Real) // f*v, v*f
		.def("__div__",&Vector6r__div__Real).def("__idiv__",&Vector6r__idiv__Real) // v/f, v/=f
		.def("__mul__",&Vector6r__mul__int).def("__rmul__",&Vector6r__rmul__int) // f*v, v*f
		.def("__div__",&Vector6r__div__int).def("__idiv__",&Vector6r__idiv__int) // v/f, v/=f
		.def(py::self != py::self).def(py::self == py::self)
		// specials
		.def("__abs__",&Vector6r::norm)
		.def("__len__",&::Vector6r_len).staticmethod("__len__")
		.def("__setitem__",&::Vector6r_set_item).def("__getitem__",&::Vector6r_get_item)
		.def("__str__",&::Vector6r_str).def("__repr__",&::Vector6r_str)
		// specials
		.def("toSymmTensor",&Vector6r_toSymmTensor,(py::args("strain")=false),"Convert Vector6 in the Voigt notation to the corresponding 2nd order symmetric tensor (as Matrix3); if *strain* is ``True``, multiply non-diagonal components by .5")
	;

	py::class_<Vector6i>("Vector6i","6-dimensional float vector.\n\nSupported operations (``f`` if a float/int, ``v`` is a Vector6): ``-v``, ``v+v``, ``v+=v``, ``v-v``, ``v-=v``, ``v*f``, ``f*v``, ``v*=f``, ``v/f``, ``v/=f``, ``v==v``, ``v!=v``.\n\nImplicit conversion from sequence (list,tuple, …) of 6 floats.",py::init<>())
		.def(py::init<Vector6i>((py::arg("other"))))
		.def("__init__",py::make_constructor(&Vector6i_fromElements,py::default_call_policies(),(py::arg("v0"),py::arg("v1"),py::arg("v2"),py::arg("v3"),py::arg("v4"),py::arg("v5"))))
		.def_pickle(Vector6i_pickle())
		// properties
		.add_static_property("Ones",&Vector6i_Ones).add_static_property("Zero",&Vector6i_Zero)
		//.add_static_property("UnitX",&Vector6i_UnitX).add_static_property("UnitY",&Vector6i_UnitY).add_static_property("UnitZ",&Vector6i_UnitZ)
		// methods
		//.def("dot",&Vector6i_dot).def("cross",&Vector6i_cross)
		.def("norm",&Vector6i::norm).def("squaredNorm",&Vector6i::squaredNorm).def("normalize",&Vector6i::normalize).def("normalized",&Vector6i::normalized)
		.def("head",&Vector6i_head).def("tail",&Vector6i_tail)
		// operators
		.def("__neg__",&Vector6i__neg__) // -v
		.def("__add__",&Vector6i__add__Vector6i).def("__iadd__",&Vector6i__iadd__Vector6i) // +, +=
		.def("__sub__",&Vector6i__sub__Vector6i).def("__isub__",&Vector6i__isub__Vector6i) // -, -=
		.def("__mul__",&Vector6i__mul__int).def("__rmul__",&Vector6i__rmul__int) // f*v, v*f
		.def("__div__",&Vector6i__div__int).def("__idiv__",&Vector6i__idiv__int) // v/f, v/=f
		.def(py::self != py::self).def(py::self == py::self)
		// specials
		.def("__abs__",&Vector6i::norm)
		.def("__len__",&::Vector6i_len).staticmethod("__len__")
		.def("__setitem__",&::Vector6i_set_item).def("__getitem__",&::Vector6i_get_item)
		.def("__str__",&::Vector6i_str).def("__repr__",&::Vector6i_str)
	;

	py::class_<Vector3r>("Vector3","3-dimensional float vector.\n\nSupported operations (``f`` if a float/int, ``v`` is a Vector3): ``-v``, ``v+v``, ``v+=v``, ``v-v``, ``v-=v``, ``v*f``, ``f*v``, ``v*=f``, ``v/f``, ``v/=f``, ``v==v``, ``v!=v``, plus operations with ``Matrix3`` and ``Quaternion``.\n\nImplicit conversion from sequence (list,tuple, …) of 3 floats.",py::init<>())
		.def(py::init<Vector3r>((py::arg("other"))))
		.def(py::init<Real,Real,Real>((py::arg("x"),py::arg("y"),py::arg("z"))))
		.def_pickle(Vector3r_pickle())
		// properties
		.add_static_property("Ones",&Vector3r_Ones).add_static_property("Zero",&Vector3r_Zero)
		.add_static_property("UnitX",&Vector3r_UnitX).add_static_property("UnitY",&Vector3r_UnitY).add_static_property("UnitZ",&Vector3r_UnitZ)
		// methods
		.def("dot",&Vector3r_dot).def("cross",&Vector3r_cross)
		.def("norm",&Vector3r::norm).def("squaredNorm",&Vector3r::squaredNorm).def("normalize",&Vector3r::normalize).def("normalized",&Vector3r::normalized)
		.def("asDiagonal",&Vector3r_asDiagonal)
		.def("Unit",&Vector3r_Unit).staticmethod("Unit")
		.def("pruned",&Matrix_pruned<Vector3r>,py::arg("absTol")=1e-6)
		// swizzles
		.def("xy",&Vector3r_xy).def("yx",&Vector3r_yx).def("xz",&Vector3r_xz).def("zx",&Vector3r_zx).def("yz",&Vector3r_yz).def("zy",&Vector3r_zy)
		// operators
		.def("__neg__",&Vector3r__neg__) // -v
		.def("__add__",&Vector3r__add__Vector3r).def("__iadd__",&Vector3r__iadd__Vector3r) // +, +=
		.def("__sub__",&Vector3r__sub__Vector3r).def("__isub__",&Vector3r__isub__Vector3r) // -, -=
		.def("__mul__",&Vector3r__mul__Real).def("__rmul__",&Vector3r__rmul__Real) // f*v, v*f
		.def("__div__",&Vector3r__div__Real).def("__idiv__",&Vector3r__idiv__Real) // v/f, v/=f
		.def("__mul__",&Vector3r__mul__int).def("__rmul__",&Vector3r__rmul__int) // f*v, v*f
		.def("__div__",&Vector3r__div__int).def("__idiv__",&Vector3r__idiv__int) // v/f, v/=f
		.def(py::self != py::self).def(py::self == py::self)
		// specials
		.def("__abs__",&Vector3r::norm)
		.def("__len__",&::Vector3r_len).staticmethod("__len__")
		.def("__setitem__",&::Vector3r_set_item).def("__getitem__",&::Vector3r_get_item)
		.def("__str__",&::Vector3r_str).def("__repr__",&::Vector3r_str)
	;	
	py::class_<Vector3i>("Vector3i","3-dimensional integer vector.\n\nSupported operations (``i`` if an int, ``v`` is a Vector3i): ``-v``, ``v+v``, ``v+=v``, ``v-v``, ``v-=v``, ``v*i``, ``i*v``, ``v*=i``, ``v==v``, ``v!=v``.\n\nImplicit conversion from sequence  (list,tuple, …) of 3 integers.",py::init<>())
		.def(py::init<Vector3i>((py::arg("other"))))
		.def(py::init<int,int,int>((py::arg("x"),py::arg("y"),py::arg("z"))))
		.def_pickle(Vector3i_pickle())
		// properties
		.add_static_property("Ones",&Vector3i_Ones).add_static_property("Zero",&Vector3i_Zero)
		.add_static_property("UnitX",&Vector3i_UnitX).add_static_property("UnitY",&Vector3i_UnitY).add_static_property("UnitZ",&Vector3i_UnitZ)
		// methods
		.def("dot",&Vector3i_dot).def("cross",&Vector3i_cross)
		.def("norm",&Vector3i::norm).def("squaredNorm",&Vector3i::squaredNorm)
		.def("Unit",&Vector3i_Unit).staticmethod("Unit")
		// operators
		.def("__neg__",&Vector3i__neg__) // -v
		.def("__add__",&Vector3i__add__Vector3i).def("__iadd__",&Vector3i__iadd__Vector3i) // +, +=
		.def("__sub__",&Vector3i__sub__Vector3i).def("__isub__",&Vector3i__isub__Vector3i) // -, -=
		.def("__mul__",&Vector3i__mul__int).def("__rmul__",&Vector3i__rmul__int) // f*v, v*f
		.def(py::self != py::self).def(py::self == py::self)
		// specials
		.def("__abs__",&Vector3i::norm)
		.def("__len__",&::Vector3i_len).staticmethod("__len__")
		.def("__setitem__",&::Vector3i_set_item).def("__getitem__",&::Vector3i_get_item)
		.def("__str__",&::Vector3i_str).def("__repr__",&::Vector3i_str)
	;	
	py::class_<Vector2r>("Vector2","3-dimensional float vector.\n\nSupported operations (``f`` if a float/int, ``v`` is a Vector3): ``-v``, ``v+v``, ``v+=v``, ``v-v``, ``v-=v``, ``v*f``, ``f*v``, ``v*=f``, ``v/f``, ``v/=f``, ``v==v``, ``v!=v``.\n\nImplicit conversion from sequence (list,tuple, …) of 2 floats.",py::init<>())
		.def(py::init<Vector2r>((py::arg("other"))))
		.def(py::init<Real,Real>((py::arg("x"),py::arg("y"))))
		.def_pickle(Vector2r_pickle())
		// properties
		.add_static_property("Ones",&Vector2r_Ones).add_static_property("Zero",&Vector2r_Zero)
		.add_static_property("UnitX",&Vector2r_UnitX).add_static_property("UnitY",&Vector2r_UnitY)
		// methods
		.def("dot",&Vector2r_dot)
		.def("norm",&Vector2r::norm).def("squaredNorm",&Vector2r::squaredNorm).def("normalize",&Vector2r::normalize).def("normalized",&Vector2r::normalize)
		.def("Unit",&Vector2r_Unit).staticmethod("Unit")
		// operators
		.def("__neg__",&Vector2r__neg__) // -v
		.def("__add__",&Vector2r__add__Vector2r).def("__iadd__",&Vector2r__iadd__Vector2r) // +, +=
		.def("__sub__",&Vector2r__sub__Vector2r).def("__isub__",&Vector2r__isub__Vector2r) // -, -=
		.def("__mul__",&Vector2r__mul__Real).def("__rmul__",&Vector2r__rmul__Real) // f*v, v*f
		.def("__div__",&Vector2r__div__Real).def("__idiv__",&Vector2r__idiv__Real) // v/f, v/=f
		.def("__mul__",&Vector2r__mul__int).def("__rmul__",&Vector2r__rmul__int) // f*v, v*f
		.def("__div__",&Vector2r__div__int).def("__idiv__",&Vector2r__idiv__int) // v/f, v/=f
		.def(py::self != py::self).def(py::self == py::self)
		// specials
		.def("__abs__",&Vector2r::norm)
		.def("__len__",&::Vector2r_len).staticmethod("__len__")
		.def("__setitem__",&::Vector2r_set_item).def("__getitem__",&::Vector2r_get_item)
		.def("__str__",&::Vector2r_str).def("__repr__",&::Vector2r_str)
	;	
	py::class_<Vector2i>("Vector2i","2-dimensional integer vector.\n\nSupported operations (``i`` if an int, ``v`` is a Vector2i): ``-v``, ``v+v``, ``v+=v``, ``v-v``, ``v-=v``, ``v*i``, ``i*v``, ``v*=i``, ``v==v``, ``v!=v``.\n\nImplicit conversion from sequence (list,tuple, …) of 2 integers.",py::init<>())
		.def(py::init<Vector2i>((py::arg("other"))))
		.def(py::init<int,int>((py::arg("x"),py::arg("y"))))
		.def_pickle(Vector2i_pickle())
		// properties
		.add_static_property("Ones",&Vector2i_Ones).add_static_property("Zero",&Vector2i_Zero)
		.add_static_property("UnitX",&Vector2i_UnitX).add_static_property("UnitY",&Vector2i_UnitY)
		// methods
		.def("dot",&Vector2i_dot)
		.def("norm",&Vector2i::norm).def("squaredNorm",&Vector2i::squaredNorm).def("normalize",&Vector2i::normalize)
		.def("Unit",&Vector2i_Unit).staticmethod("Unit")
		// operators
		.def("__neg__",&Vector2i__neg__) // -v
		.def("__add__",&Vector2i__add__Vector2i).def("__iadd__",&Vector2i__iadd__Vector2i) // +, +=
		.def("__sub__",&Vector2i__sub__Vector2i).def("__isub__",&Vector2i__isub__Vector2i) // -, -=
		.def("__mul__",&Vector2i__mul__int).def("__rmul__",&Vector2i__rmul__int) // f*v, v*f
		.def(py::self != py::self).def(py::self == py::self)
		// specials
		.def("__abs__",&Vector2i::norm)
		.def("__len__",&::Vector2i_len).staticmethod("__len__")
		.def("__setitem__",&::Vector2i_set_item).def("__getitem__",&::Vector2i_get_item)
		.def("__str__",&::Vector2i_str).def("__repr__",&::Vector2i_str)
	;	
	
};








