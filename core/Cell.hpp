// 2009 © Václav Šmilauer <eudoxos@arcig.cz>

/*! Periodic cell parameters and routines. Usually instantiated as Scene::cell.

The Cell has current box configuration represented by the parallelepiped's base vectors (*hSize*). Lengths of the base vectors can be accessed via *size*.

* Matrix3r *trsf* is "deformation gradient tensor" F (http://en.wikipedia.org/wiki/Finite_strain_theory) 
* Matrix3r *gradV* is "velocity gradient tensor" (http://www.cs.otago.ac.nz/postgrads/alexis/FluidMech/node7.html)

The transformation is split between "normal" part and "rotation/shear" part for contact detection algorithms. The shearPt, unshearPt, getShearTrsf etc functions refer to both shear and rotation. This decomposition is frame-dependant and does not correspond to the rotation/stretch decomposition of mechanics (with stretch further decomposed into isotropic and deviatoric). Therefore, using the shearPt family in equations of mechanics is not recommended. Similarly, attributes assuming the existence of a "reference" state are considered deprecated (refSize, hSize0). It is better to not use them since there is no guarantee that the so-called "reference state" will be maintained consistently in the future.

*/

#pragma once

#include<woo/lib/object/Object.hpp>
#include<woo/lib/base/Math.hpp>

struct Scene;

struct Cell: public Object{
	public:
	//! Get/set sizes of cell base vectors
	const Vector3r& getSize() const { return _size; }
	void setSize(const Vector3r& s){for (int k=0;k<3;k++) hSize.col(k)*=s[k]/hSize.col(k).norm(); refHSize=hSize;  postLoad(*this);}
	//! Return copy of the current size (used only by the python wrapper)
	Vector3r getSize_copy() const { return _size; }
	//! return vector of consines of skew angle in yz, xz, xy planes between respective transformed base vectors
	const Vector3r& getCos() const {return _cos;}
	//! transformation matrix applying pure shear&rotation (scaling removed)
	const Matrix3r& getShearTrsf() const { return _shearTrsf; }
	//! inverse of getShearTrsfMatrix().
	const Matrix3r& getUnshearTrsf() const {return _unshearTrsf;}
	
	/*! return pointer to column-major OpenGL 4x4 matrix applying pure shear. This matrix is suitable as argument for glMultMatrixd.

	Note: the order of OpenGL transoformations matters; for instance, if you draw sheared wire box of size *size*,
	centered at *center*, the order is:

		1. translation: glTranslatev(center);
		3. shearing: glMultMatrixd(scene->cell->getGlShearTrsfMatrix());
		2. scaling: glScalev(size);
		4. draw: glutWireCube(1);
	
	See also http://www.songho.ca/opengl/gl_transform.html#matrix .
	*/
	const double* getGlShearTrsfMatrix() const { return _glShearTrsfMatrix; }
	//! Whether any shear (non-diagonal) component of the strain matrix is nonzero.
	bool hasShear() const {return _hasShear; }

	// caches; private
	private:
		Matrix3r _invTrsf;
		Matrix3r _trsfInc;
		Vector3r _size, _cos;
		Vector3r _refSize;
		bool _hasShear;
		Matrix3r _shearTrsf, _unshearTrsf;
		double _glShearTrsfMatrix[16];
		void fillGlShearTrsfMatrix(double m[16]);
	public:

	DECLARE_LOGGER;

	// at the end of step, bring gradV(t-dt/2) to the value gradV(t+dt/2) so that e.g. kinetic energies are right
	void setNextGradV();
	//! "integrate" gradV, update cached values used by public getter.
	void integrateAndUpdate(Real dt);
	/*! Return point inside periodic cell, even if shear is applied */
	// this method will be deprecated
	Vector3r canonicalizePt(const Vector3r& pt) const { return shearPt(wrapPt(unshearPt(pt))); }

	//! Compute angular velocity vector from given spin tensor (dual pseudovector)
	static Vector3r spin2angVel(const Matrix3r& W);


	/*! Return point inside periodic cell, even if shear is applied; store cell coordinates in period. */
	Vector3r canonicalizePt(const Vector3r& pt, Vector3i& period) const { return shearPt(wrapPt(unshearPt(pt),period)); }
	/*! Apply inverse shear on point; to put it inside (unsheared) periodic cell, apply wrapPt on the returned value. */
	Vector3r unshearPt(const Vector3r& pt) const { return _unshearTrsf*pt; }
	//! Apply shear on point. 
	Vector3r shearPt(const Vector3r& pt) const { return _shearTrsf*pt; }
	/*! Wrap point to inside the periodic cell; don't compute number of periods wrapped */
	Vector3r wrapPt(const Vector3r& pt) const {
		Vector3r ret; for(int i=0;i<3;i++) ret[i]=wrapNum(pt[i],_size[i]); return ret;}
	/*! Wrap point to inside the periodic cell; period will contain by how many cells it was wrapped. */
	Vector3r wrapPt(const Vector3r& pt, Vector3i& period) const {
		Vector3r ret; for(int i=0; i<3; i++){ ret[i]=wrapNum(pt[i],_size[i],period[i]); } return ret;}
	/*! Wrap number to interval 0…sz */
	static Real wrapNum(const Real& x, const Real& sz) {
		Real norm=x/sz; return (norm-floor(norm))*sz;}
	/*! Wrap number to interval 0…sz; store how many intervals were wrapped in period */
	static Real wrapNum(const Real& x, const Real& sz, int& period) {
		Real norm=x/sz; period=(int)floor(norm); return (norm-period)*sz;}

	// relative position and velocity for interaction accross multiple cells
	Vector3r intrShiftPos(const Vector3i& cellDist) const { return hSize*cellDist.cast<Real>(); }
	Vector3r intrShiftVel(const Vector3i& cellDist) const;

	// fluctuation velocities at t-dt/2 and at t
	Vector3r pprevFluctVel(const Vector3r& pos, const Vector3r& vel, const Real& dt);
	// flutuation angular velocities at t-dt/2 and at t
	Vector3r pprevFluctAngVel(const Vector3r& angVel);


	// get/set current shape; setting resets trsf to identity
	Matrix3r getHSize() const { return hSize; }
	void setHSize(const Matrix3r& m){ hSize=refHSize=m; pprevHsize=hSize; postLoad(*this); }
	// set current transformation; has no influence on current configuration (hSize); sets display refHSize as side-effect
	Matrix3r getTrsf() const { return trsf; }
	void setTrsf(const Matrix3r& m){ trsf=m; postLoad(*this); }
	// get undeformed shape
	Matrix3r getHSize0() const { return _invTrsf*hSize; }
	Vector3r getSize0() const { Matrix3r h0=getHSize0(); return Vector3r(h0.col(0).norm(),h0.col(1).norm(),h0.col(2).norm()); }
	// set box shape of the cell
	void setBox(const Vector3r& size){ setHSize(size.asDiagonal()); trsf=Matrix3r::Identity(); postLoad(*this); }
	void setBox3(const Real& s0, const Real& s1, const Real& s2){ setBox(Vector3r(s0,s1,s2)); }
	

	// return current cell volume
	Real getVolume() const {return hSize.determinant();}
	void postLoad(Cell&){ integrateAndUpdate(0); }

	// to resolve overloads
	Vector3r canonicalizePt_py(const Vector3r& pt) const { return canonicalizePt(pt);}
	Vector3r wrapPt_py(const Vector3r& pt) const { return wrapPt(pt);}

	Matrix3r getVelGrad() const;
	void setVelGrad(const Matrix3r& v);
	Matrix3r getGradV() const;
	void setGradV(const Matrix3r& v);
	void setCurrGradV(const Matrix3r& v);

	// check that trsf is upper triangular, throw an exception if not
	void checkTrsfUpperTriangular();

	enum { HOMO_NONE=0, HOMO_POS=1, HOMO_VEL=2, HOMO_VEL_2ND=3, HOMO_GRADV2=4 };
	#define Cell_CLASS_DESCRIPTOR \
		/*class,base,doc*/ \
		Cell,Object,"Parameters of periodic boundary conditions. Only applies if O.isPeriodic==True.", \
		/*attrs*/ \
		((bool,trsfUpperTriangular,false,AttrTrait<Attr::readonly>(),"Require that :ref:`Cell.trsf` is upper-triangular, to conform with the requirement of voro++ for sheared periodic cells."))\
		/* overridden below to be modified by getters/setters because of intended side-effects */\
		((Matrix3r,trsf,Matrix3r::Identity(),,"[overridden]")) \
		((Matrix3r,refHSize,Matrix3r::Identity(),,"Reference cell configuration, only used with :ref:`OpenGLRenderer.dispScale`. Updated automatically when :ref:`hSize<Cell.hSize>` or :ref:`trsf<Cell.trsf>` is assigned directly; also modified by :ref:`woo.utils.setRefSe3` (called e.g. by the :gui:`Reference` button in the UI)."))\
		((Matrix3r,hSize,Matrix3r::Identity(),,"[overridden below]"))\
		((Matrix3r,pprevHsize,Matrix3r::Identity(),AttrTrait<Attr::readonly>(),"hSize at t-dt/2; used to compute velocity correction in contact with non-zero cellDist. Updated automatically."))\
		/* normal attributes */\
		((Matrix3r,gradV,Matrix3r::Zero(),,"[overridden below]"))\
		((Matrix3r,W,Matrix3r::Zero(),AttrTrait<Attr::readonly>(),"Spin tensor, computed from gradV when it is updated."))\
		((Vector3r,spinVec,Vector3r::Zero(),AttrTrait<Attr::readonly>(),"Angular velocity vector (1/2 * dual of spin tensor W), computed from W when it is updated."))\
		((Matrix3r,nextGradV,Matrix3r::Zero(),,"Value of gradV to be applied in the next step (that is, at t+dt/2). If any engine changes gradV, it should do it via this variable. The value propagates to gradV at the very end of each timestep, so if it is user-adjusted between steps, it will not become effective until after 1 steps. It should not be changed between Leapfrog and end of the step!"))\
		((int,homoDeform,HOMO_GRADV2,AttrTrait<>().triggerPostLoad().choice({{HOMO_NONE,"None"},{HOMO_POS,"position only"},{HOMO_VEL,"pos & vel, 1st order"},{HOMO_VEL_2ND,"pos & vel, 2nd order"},{HOMO_GRADV2,"all, leapfrog-consistent"}}),"Deform (:ref:`gradV<Cell.gradV>`) the cell homothetically, by adjusting positions or velocities of particles. The values have the following meaning: 0: no homothetic deformation, 1: set absolute particle positions directly (when ``gradV`` is non-zero), but without changing their velocity, 2: adjust particle velocity (only when ``gradV`` changed) with Δv_i=Δ ∇v x_i. 3: as 2, but include a 2nd order term in addition -- the derivative of 1 (convective term in the velocity update)."))\
		,\
	/*deprec*/ ((Hsize,hSize,"conform to Yade's names convention.")),\
	/*init*/ ,\
	/*ctor*/ _invTrsf=Matrix3r::Identity(); integrateAndUpdate(0),\
	/*py*/\
		/* override some attributes above*/ \
		.add_property("hSize",&Cell::getHSize,&Cell::setHSize,"Base cell vectors (columns of the matrix), updated at every step from :ref:`gradV<Cell.gradV>` (:ref:`trsf<Cell.trsf>` accumulates applied :ref:`gradV<Cell.gradV>` transformations). Setting *hSize* during a simulation is not supported by most contact laws, it is only meant to be used at iteration 0 before any interactions have been created.")\
		.add_property("size",&Cell::getSize_copy,&Cell::setSize,"Current size of the cell, i.e. lengths of the 3 cell lateral vectors contained in :ref:`Cell.hSize` columns. Updated automatically at every step. Assigning a value will change the lengths of base vectors (see :ref:`Cell.hSize`), keeping their orientations unchanged.")\
		/* useful properties*/ \
		.add_property("trsf",&Cell::getTrsf,&Cell::setTrsf,"Current transformation matrix of the cell, obtained from time integration of :ref:`Cell.gradV`.")\
		.def_readonly("size",&Cell::getSize_copy,"Current size of the cell, i.e. lengths of the 3 cell lateral vectors contained in :ref:`Cell.hSize` columns. Updated automatically at every step.")\
		.add_property("volume",&Cell::getVolume,"Current volume of the cell.")\
		/* functions */ \
		.def("setBox",&Cell::setBox,"Set :ref:`Cell` shape to be rectangular, with dimensions along axes specified by given argument. Shorthand for assigning diagonal matrix with respective entries to :ref:`hSize<Cell.hSize>`.")\
		.def("setBox",&Cell::setBox3,"Set :ref:`Cell` shape to be rectangular, with dimensions along $x$, $y$, $z$ specified by arguments. Shorthand for assigning diagonal matrix with the respective entries to :ref:`hSize<Cell.hSize>`.")\
		.add_property("velGrad",&Cell::getVelGrad,&Cell::setVelGrad)\
		.add_property("gradV",&Cell::getGradV,&Cell::setGradV,"Velocity gradient of the transformation; used in :ref:`Leapfrog`. Values of :ref:`gradV<Cell.gradV>` accumulate in :ref:`trsf<Cell.trsf>` at every step. This is the value at t-dt/2 and should _never_ be changed directly. Engines can modify nextGradV, which will be assigned between time-steps. Humans should assign Cell.nnextGradV so that correct velocity correction is applied to all particles, in which case the given value will be effective in t+2*dt.")\
		.def("setCurrGradV",&Cell::setCurrGradV)\
		/* debugging only */ \
		.def("canonicalizePt",&Cell::canonicalizePt_py,"Transform any point such that it is inside the base cell")\
		.def("unshearPt",&Cell::unshearPt,"Apply inverse shear on the point (removes skew+rot of the cell)")\
		.def("shearPt",&Cell::shearPt,"Apply shear (cell skew+rot) on the point")\
		.def("wrapPt",&Cell::wrapPt_py,"Wrap point inside the reference cell, assuming the cell has no skew+rot.")\
		.def_readonly("shearTrsf",&Cell::_shearTrsf,"Current skew+rot transformation (no resize)")\
		.def_readonly("unshearTrsf",&Cell::_unshearTrsf,"Inverse of the current skew+rot transformation (no resize)")\
		.add_property("hSize0",&Cell::getHSize0,"Value of untransformed hSize, with respect to current :ref:`trsf<Cell.trsf>` (computed as :ref:`trsf<Cell.trsf>`⁻¹ × :ref:`hSize<Cell.hSize>`.")\
		.add_property("size0",&Cell::getSize0,"norms of columns of `hSize0` (edge lengths of the untransformed configuration)") ;\
			_classObj.attr("HomoNone")=(int)Cell::HOMO_NONE; \
			_classObj.attr("HomoPos")=(int)Cell::HOMO_POS; \
			_classObj.attr("HomoVel")=(int)Cell::HOMO_VEL; \
			_classObj.attr("HomoVel2")=(int)Cell::HOMO_VEL_2ND; \
			_classObj.attr("HomoGradV2")=(int)Cell::HOMO_GRADV2;

	YAD3_CLASS_DECLARATION(Cell_CLASS_DESCRIPTOR);
};
REGISTER_SERIALIZABLE(Cell);
