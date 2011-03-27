#include<yade/core/GlobalEngine.hpp>
#include<Eigen/Sparse>

class Interaction;

typedef Eigen::Matrix<Real,12,12> Matrix12r;
typedef Eigen::SparseMatrix<Real/*,Eigen::RowMajor*/> SparseXr;
typedef Eigen::SparseVector<Real> SparseVectorXr;
typedef Eigen::Matrix<Real,Eigen::Dynamic,1> VectorXr;


struct StaticSolver: public GlobalEngine{
	// bitmask of dof attributes; not all combinations are meaningful
	enum {
		DOF_FIXED=1,  // support node, fixed in space
		DOF_USED=4, // unless this flag is set, the node will be eliminated from the global matrix
	};


	SparseXr /*uncondensed global matrix*/ KK0, /* condensed global matrix*/ KK;

	void setupDofs();
	Matrix12r contactK(const shared_ptr<Interaction>& I);
	SparseXr assembleK();
	SparseXr condenseK(SparseXr&);
	VectorXr condensedRhs(int n);
	VectorXr solveU(const SparseXr& K, const VectorXr& f);
	void applyU(const VectorXr& U, Real alpha);

	void action();


	struct Dof{
		Dof(): flags(0), ix(-1), value(0.) {};
		int flags; // bitmask of DOF_FREE, DOF_FIXED, DOF_USED
		int ix; // line number in the global matrix
		Real value; // DOF_FREE: force value;
		void setUsed(bool used){ if(used) flags|=DOF_USED; else flags&=~(DOF_USED); }
		bool isUsed() const { return flags&DOF_USED; }
	};
	vector<Dof> dofs;
	DECLARE_LOGGER;
	YADE_CLASS_BASE_DOC_ATTRS(StaticSolver,GlobalEngine,"Find static equilibrium of the packing.",
	);
};
REGISTER_SERIALIZABLE(StaticSolver);
