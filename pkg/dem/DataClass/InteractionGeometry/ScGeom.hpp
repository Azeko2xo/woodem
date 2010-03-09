// © 2004 Olivier Galizzi <olivier.galizzi@imag.fr>
// © 2008 Václav Šmilauer <eudoxos@arcig.cz>

#pragma once

#include<yade/core/InteractionGeometry.hpp>
#include<yade/core/State.hpp>
#include<yade/lib-base/Math.hpp>
#include<yade/pkg-dem/DemXDofGeom.hpp>
/*! Class representing geometry of two spheres in contact.
 *
 * The code under SCG_SHEAR is experimental and is used only if ElasticContactLaw::useShear is explicitly true
 */

#define SCG_SHEAR

class ScGeom: public GenericSpheresContact {
	public:
		// inherited from GenericSpheresContact: Vector3r& normal; 
		Real penetrationDepth;
		Real &radius1, &radius2;

		#ifdef SCG_SHEAR
			//! update shear on this contact given dynamic parameters of bodies. Should be called from constitutive law, exactly once per iteration. Returns shear increment in this update, which is already added to shear.
			Vector3r updateShear(const State* rbp1, const State* rbp2, Real dt, bool avoidGranularRatcheting=true);
		#endif
		
		virtual ~ScGeom();

		void updateShearForce(Vector3r& shearForce, Real ks, const Vector3r& prevNormal, const State* rbp1, const State* rbp2, Real dt, bool avoidGranularRatcheting=true);

	YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(ScGeom,GenericSpheresContact,"Class representing :yref:`geometry<InteractionGeometry>` of two :yref:`spheres<Sphere>` in contact. The contact has 3 DOFs (normal and 2×shear) and uses incremental algorithm for updating shear. (For shear formulated in total displacements and rotations, see :yref:`Dem3DofGeom` and related classes).\n\nWe use symbols :math:`\\vec{x}`, :math:`\\vec{v}`, :math:`\\vec{\\omega}` respectively for position, linear and angular velocities (all in global coordinates) and :math:`r` for particles radii; subscripted with 1 or 2 to distinguish 2 spheres in contact. Then we compute unit contact normal\n\n.. math::\n\n\t\\vec{n}=\\frac{\\vec{x}_2-\\vec{x}_1}{||\\vec{x}_2-\\vec{x}_1||}\n\nRelative velocity of spheres is then\n\n.. math::\n\n\t\\vec{v}_{12}=(\\vec{v}_2+\\vec{\\omega}_2\\times(-r_2\\vec{n}))-(\\vec{v}_1+\\vec{\\omega}_1\\times(r_1\\vec{n}))\n\nand its shear component\n\n.. math::\n\n\t\\Delta\\vec{v}_{12}^s=\\vec{v}_{12}-(\\vec{n}\\cdot\\vec{v}_{12})\\vec{n}.\n\nTangential displacement increment over last step then reads\n\n.. math::\n\n\t\\vec{x}_{12}^s=\\Delta t \\vec{v}_{12}^s.",
		((Vector3r,contactPoint,Vector3r::ZERO,"Reference point of the contact. |ycomp|"))
		#ifdef SCG_SHEAR
			((Vector3r,shear,Vector3r::ZERO,"Total value of the current shear. Update the value using ScGeom::updateShear. |ycomp|"))
			((Vector3r,prevNormal,Vector3r::ZERO,"Normal of the contact in the previous step. |ycomp|"))
		#endif
		,
		/* extra initializers */ ((radius1,GenericSpheresContact::refR1)) ((radius2,GenericSpheresContact::refR2)),
		/* ctor */ createIndex();,
		/* py */
	);
	REGISTER_CLASS_INDEX(ScGeom,GenericSpheresContact);
};
REGISTER_SERIALIZABLE(ScGeom);

