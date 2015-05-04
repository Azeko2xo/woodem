.. _concrete-contact-model:

======================
Concrete contact model
======================

.. note:: This model was originally published in in the thesis :cite:`Smilauer2010b` under the name Concrete Particle Model (CPM) and ported from Yade to Woo by the author. This page was converted from the thesis source.

Contact law is formulated on the level of individual contacts between 2 particles. However, since desired results are macroscopic and not observed until larger simulations are performed, motivations for introducing various contact-level features of the model might not be obvious until after reading :ref:`concrete-model-calibration`.
   
-------------------------------
Prior discrete concrete models
-------------------------------
Before describing the CPM model, we would like to give a brief overview of some of the main particle-based, DEM and non-DEM, concrete models.

Lattice models
   :cite:`Vervuurt1997` (pg. 14-16) gives an overview of older lattice models used for concrete. Later lattice models of concrete aim usually at meso-scale modeling, where aggregates and mortar are distinguished by differing beam parameters. Beam configuration can be regular (leading to failure anisotropy), random :cite:`vanMier2003`, or generated to match observed statistical distribution :cite:`Leite2004`.

   :cite:`vanMier2003` uses brittle beam behavior, simply removing broken beams from simulation; :cite:`Leite2004` uses tensile softening to avoid dependence of global softening (which is observed even with brittle behavior of beams) on beam size. Neither of these models focuses on capturing compression behavior.

   :cite:`Cusatis2003` presents a rather sophisticated model, in which properties of lattice beams are computed from the geometry of Voronoï cells of each node. Granulometry is supposed to have substantial influence on confinement behavior; as it is not fully considered by the model, the beam-transversal stress history influences shear rigidity instead. Compression behavior is captured fairly well.

Rigid body -- spring models
   This family of models is, to our knowledge, used infrequently for concrete. :cite:`Nagai2002` presents a mesoscopic (distinguishing aggregates and mortar) model, though only formulated in 2D.

Discrete element models
   Such of concrete are rather rare. Most activity (judging by publications) comes from teams formed around Frédéric V. Donzé. His work (using SDEC software, see https://yade-dem.org/wiki/Yade_history for details)) first targeted at 2D DEM :cite:`Camborde2000`. Later, exploiting the dynamic nature of DEM led to fast concrete dynamics in 3D :cite:`Hentz2003` :cite:`Hentz2004` and impact simulation :cite:`Shiu2008`. In order to reduce computational costs, elastic FEM for the non-damaged subdomain is used, with some tricks to avoid spurious dynamic effects on the DEM/FEM boundary :cite:`Rousseau2009`.

With both lattice and DEM models, arriving at reasonable compression behavior is non-trivial; it seems to stem from the fact that 1D elements (beams in lattice, bonds in DEM) have no notion of an overall stress state at their vicinity, but only insofar as it is manifested in the direction of the respective element. :cite:`Cusatis2003` works around this by explicitly introducing the influence of transversal stress. :cite:`Hentz2003` (sect. 5.3), on the other hand, blocks rotations of spherical elements to achieve a higher and more realistic :math:`f_c/f_t` ratio, but it is questionable whether there is physically sound reason for such an adjustment.

-------------------
Model description
-------------------

Cohesive and non-cohesive contacts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
We use the word contact to denote both cohesive (material bond) and non-cohesive (contact two spheres meeting during their motion) contacts, as they are governed by the same equations. The non-cohesive contact only differs by that it is considered as fully damaged from the very start, by setting the damage variable :math:`\omega=1`; since damage effectively prevent tensile forces in the contact, the result is that the two spheres interact only while in tension (geometrically overlapping) and disappears when they geometrically separate. On the other hand, cohesive contact, which is always created at the beginning of the simulation, is created in virgin, undamaged state.
      
Contact parameters
^^^^^^^^^^^^^^^^^^^
As already explained above, most parameters of a contact are computed as averages of particle material properties. That is also true for the CPM model, with the respective :obj:`material <woo.dem.ConcreteMat>` and :obj:`Cp2 functor <woo.dem.Cp2_ConcreteMat_ConcretePhys>` classes.
      
Those computed as averages (from values that are typically identical, since particles share the same material) are :math:`c_{T0}`, :math:`\eps_0`, :math:`\eps_s`, :math:`k_N` (from particles' moduli :math:`E`), :math:`\tau_d`, :math:`M_d`, :math:`\tau_{pl}`, :math:`M_{pl}`, :math:`\sigma_0`, :math:`\phi`. (The meaning of symbols used here is explained in the following text.)
      
On the other hand, shear modulus :math:`k_T` is computed from :math:`k_N` using the ratio in :obj:`~woo.dem.FrictPhys.ktDivKn`. :math:`\eps_f` is computed from :math:`\eps_0` by multiplying it by dimensionless :obj:`~woo.dem.ConcreteMat.relDuctility`, which is :math:`\eps_f/\eps_0`.
      
Finally, some parameters are set in the :obj:`woo.dem.Law2_L6Geom_ConcretePhys` law functor, hence the same for all interactions. Those are :math:`Y_0` and :math:`\tilde K_s`.
      
Density of the particle material is set to :math:`\rho=4800\,{\rm kgm^{-3}}` (by default), as we have to compensate for porosity of the packing, which a little above 50% for spheres of the same radius and we take continuum concrete density roughly :math:`2400\,{\rm kgm^{-3}}`.


Normal stresses
^^^^^^^^^^^^^^^^

The normal stress-strain law  is formulated within the framework of damage mechanics:

.. math::
   :label: eq-sigma-N-simple

   \sigma_N =[1-\omega H(\eps_N)] k_N\eps_N.


Here, :math:`k_N` is the normal modulus (model parameter, [Pa]), and :math:`\omega\in\langle0,1\rangle` is the damage variable. The Heaviside function :math:`H(\eps_N)` deactivates damage influence in compression, which physically corresponds to crack closure.

The damage variable :math:`\omega` is evaluated using the *damage evolution function* :math:`g` (:ref:`figure <concrete-damage-evolution-function>`):

.. math::
   :label: eq-tilde-eps
   :nowrap:

   \begin{align}
      \omega=g(\kappa)&=1-\frac{\eps_f}{\kappa}\exp\left(-\frac{\kappa-\eps_0}{\eps_f}\right)\\
      \kappa&=\max \tilde\eps\\
      \tilde\eps&=\sqrt{\langle\eps_N\rangle^2+\xi_1^2|\eps_T|^2},
   \end{align}

where :math:`\tilde\eps` is the equivalent strain responsible for damage (:math:`\langle\eps_N \rangle` signifies the positive part of :math:`\eps_N`) and :math:`\xi_1` is a dimensionless coefficient weighting the contribution of the shear strain :math:`\eps_T` to damage. However, comparative studies indicate that the optimal value is :math:`\xi_1\equiv 0`, hence equation :eq:`eq-tilde-eps` simplifies to :math:`\tilde\eps=\langle\eps_N\rangle`. :math:`\kappa` is the maximum equivalent strain over the whole history of the contact.

.. _concrete-damage-evolution-function:

.. figure:: fig/cpm-funcG.*
   :figclass: align-center
   :width: 80%

   Damage :math:`\omega` evolution function :math:`\omega=g(\kappa_D)`, where :math:`\kappa_D=\max \eps_N` (using :math:`\eps_0=0.0001`, :math:`\eps_f=30\eps_0`).



Parameter :math:`\eps_0` is the limit elastic strain, and the product :math:`K_T\eps_0` corresponds to the local tensile strength at the level of one contact (in general different from the macroscopic tensile strength). Parameter :math:`\eps_f` is related to the slope of the softening part of the normal strain-stress diagram (:ref:`figure <concrete-strain-stress-normal>`) and must be larger than :math:`\eps_0`.


.. _concrete-strain-stress-normal:

.. figure:: fig/cpm-distProx.*
   :figclass: align-center
   :width: 80%

   Strain-stress diagram in the normal direction.


Compressive plasticity
""""""""""""""""""""""
To better capture confinement effect, we introduce, as an extension to the model, hardening in plasticity in compression. Using material parameter :math:`\eps_s<0`, we define :math:`\sigma_s=k_N\eps_s`. If :math:`\sigma<\sigma_s`, then :math:`\tilde K_s k_N` is taken for tangent stiffness, :math:`\tilde K_s \in\langle0,1\rangle` and plastic strain :math:`\eps_N^{\rm pl}` is incremented accordingly. This introduces 2 new parameters :math:`\eps_s` (stress at which the hardening branch begins) and :math:`\tilde K_s` (relative modulus of the hardening branch) that must be calibrated (see :ref:`concrete-model-calibration`) and one internal variable :math:`\eps_N^{\rm pl}`.
         
Extended strain-stress diagram for multiple loading cycles is shown in :ref:`this figure <concrete-strain-stress-normal-hardening>`. Note that this feature is activated only in compression, whereas damage is activated only in tension. Therefore, there is no need to specially consider interaction between both.

.. _concrete-strain-stress-normal-hardening:

.. .. figure:: fig/cpm-cycleProxDist.*
   :figclass: align-center
   :width: 80%

This following is the strain-stress diagram in normal direction, loaded cyclically in tension and compression; it shows several features of the model: (1) damage in tension, but not damage in compression (governed by the :math:`\omega` internal variable) (2) plasticity in compression, starting at strain :math:`\eps_s`; reduced (hardening) modulus is :math:`\tilde K_s k_N`:

.. plot::

   from woo.dem import *
   from woo.core import *
   import woo
   r=1e-3
   m=ConcreteMat(young=30e9,ktDivKn=.2,sigmaT=3e6,tanPhi=.8,epsCrackOnset=1e-4,relDuctility=5.)
   S=woo.master.scene=Scene(
      fields=[DemField(par=[Sphere.make((0,0,0),r,fixed=False,wire=True,mat=m),Sphere.make((0,2*r,0),r,fixed=False,wire=True,mat=m)])],
      engines=DemField.minimalEngines(model=woo.models.ContactModelSelector(name='concrete',distFactor=1.5),lawKw=dict(epsSoft=-2*m.epsCrackOnset,relKnSoft=.5))+[LawTester(ids=(0,1),abWeight=.5,smooth=1e-4,stages=[LawTesterStage(values=((1 if i%2==0 else -1)*1e-2,0,0,0,0,0),whats='v.....',until='C.phys.epsN'+('>' if epsN>0 else '<')+'%g'%epsN) for i,epsN in enumerate([6e-4,-6e-4,12e-4,-8e-4,15e-4])],done='S.stop()',label='tester'),PyRunner(5,'C=S.dem.con[0]; S.plot.addData(i=S.step,**C.phys.dict())')],
      plot=Plot(plots={'epsN':('sigmaN',)},labels={'epsN':r'$\varepsilon_N$','sigmaN':r'$\sigma_N$'}) # ,'i':('epsN',None,'omega')}
   )
   S.run(); S.wait()
   S.plot.plot()


.. sect-cpm-visco-damage

.. _concrete-visco-damage:

Visco-damage
""""""""""""""
In order to model time-dependent phenomena, viscosity is introduced in tension by adding viscous overstress :math:`\sigma_{Nv}` to :eq:`eq-sigma-N-simple`. As we suppose it to be related to a limited rate of crack propagation, it cannot depend on total strain rate; rather, we split total strain into the elastic strain :math:`\eps_e` and the damage part :math:`\eps_d`. Since :math:`\eps_e=\sigma_N/k_N`, we have

.. math::
   :label: eq-cpm-epsd
   
   \eps_d=\eps_N-\frac{\sigma_N}{k_N}.

We then postulate the overstress in the form

.. math::
   :label: eq-cpm-sigmaNv

   \sigma_{Nv}(\dot\eps_d)=k_N\eps_0\langle\tau_d\dot\eps_{Nd}\rangle^{M_d},

where :math:`k_N\eps_0` is rate-independent tensile strength (introduced for the sake of dimensionality), :math:`\tau_d` is characteristic time for visco-damage and :math:`M_d` is a dimensionless exponent. The :math:`\langle\dots\rangle` operator denotes positive part; therefore, for :math:`\dot\eps_{Nd}\leq0`, viscous overstress vanishes. 2 new parameters :math:`\tau_d` and :math:`M_d` were introduced.
         
The normal stress equation then reads

.. math::
   :label: eq-cpm-sigmaN

   \sigma_N=\left[1-\omega H(\eps_N)\right]k_N\eps_N+\sigma_{Nv}(\dot \eps_{Nd}).

In a finite step computation, we can evaluate the damage variable :math:`\omega` in a rate-independent manner first; then, we compute the increment of :math:`\eps_d` satisfying equations :eq:`eq-cpm-epsd`--:eq:`eq-cpm-sigmaN`. As usual, we use :math:`\curr{\bullet}`, :math:`\next{\bullet}` to denote value of :math:`\bullet` at :math:`t`, :math:`t+\Dt` (at the beginning and at the end of the current timestep) respectively. We approximate the damage strain rate by the difference formula

.. math:: \dot\eps_d=\frac{\next{\eps}_d-\curr{\eps}_d}{\Delta t}=\frac{\Delta\eps_d}{\Delta t}

and substitute :eq:`eq-cpm-sigmaNv` and :eq:`eq-cpm-sigmaN` into :eq:`eq-cpm-epsd` obtaining

.. math:: \curr{\eps}_d+\Delta\eps_d&=\next{\eps}_N-(1-\omega)\next{\eps}_N-\frac{k_N\eps0}{k_N}\left\langle\frac{\tau_d\Delta\eps_d}{\Delta t}\right\rangle^{M_d},

which can be written as

.. math::
   :label: eq-cpm-viscdmg

   \Delta\eps_d+\eps_0\left\langle\frac{\tau_d\Delta\eps_d}{\Delta t}\right\rangle^{M_d}=\omega\next{\eps}_N-\curr{\eps}_d

During unloading, i.e. :math:`\Delta\eps_d\leq0`, the power term vanishes, leading to

.. math:: \Delta\eps_d&=\omega\next{\eps}_N-\curr{\eps}_d,

applicable if :math:`\omega\next{\eps}_N\leq\curr{\eps}_d`.

In the contrary case, :eq:`eq-cpm-viscdmg` must be solved, but :math:`\langle\cdots\rangle` can be replaced by :math:`(\cdots)` since the term is now known to be positive. We divide :eq:`eq-cpm-viscdmg` by its right-hand side and apply logarithm. We transform the unknown :math:`\Delta\eps_d` into a new unknown

.. math::
   :label: eq-cpm-beta

   \beta=\ln\frac{\Delta\eps_d}{\omega\next{\eps}_N-\curr{\eps}_d},

obtaining the equation

.. math::
   :label: eq-cpm-logeq

   \ln\left({\rm e}^\beta+c{\rm e}^{M_d\beta}\right)=0,

with

.. math:: c=(1-\omega)\eps_0\left(\omega\next{\eps}_N-\curr{\eps}_d\right)^{M_d-1}\left(\frac{\tau_d}{\Delta t}\right)^{M_d}.

For positive :math:`c` and :math:`M_d`, the term :math:`\ln({\rm e}^\beta+c{\rm e}^{M_d\beta})` from :eq:`eq-cpm-logeq`) is convex, increasing and positive at :math:`\beta=0`. As a consequence, the Newton method with starting point at :math:`\beta=0` always converges monotonically to a unique negative solution of :math:`\beta`. (Using pre-determined maximum number of steps and given absolute tolerance :math:`\delta`, we start with :math:`\beta=0`. Then at each step, we compute :math:`a=c\exp(N\beta)+\exp(N)` and :math:`f=\log(a)`. If :math:`|f|<\delta`, then :math:`\beta` is the desired solution. Otherwise, we update :math:`\beta\leftarrow\beta-f/((cN\exp{N\beta}+\exp{\beta})/a)` and continue with the next step.) 

From the solution :math:`\beta`, we compute

.. math:: \Delta\eps_d=\left(\omega\next{\eps}_N-\curr{\eps}_d\right){\rm e}^\beta.

Finally, the term :math:`\sigma_{Nd}(\dot\eps_d)=\sigma_{Nd}\left(\frac{\Delta\eps_d}{\Delta t}\right)` in :eq:`eq-cpm-sigmaNv` can be evaluated and plugged into :eq:`eq-cpm-sigmaN`.

The effect of viscosity on damage for one contact is shown in the :ref:`following figure <concrete-visco-dist>`; calibration of the new parameters :math:`\tau_d` and :math:`M_d` is described in :ref:`concrete-model-calibration`.

.. _concrete-visco-dist:

.. figure:: fig/cpm-viscDist.*
   :figclass: align-center
   :width: 80%

   Strain-stress curve in tension with different rates of loading; the parameters used here are :math:`\tau_d=1\,{\rm s}` and :math:`M_d=0.1`.

.. sect-cpm-isoprestress

.. _concrete-isoprestress:

Isotropic confinement
"""""""""""""""""""""
During calibration, we faced the necessity to simulate confined compression setups. Applying boundary confinement on a specimen composed of many particles is not straightforward, since thera are necessary strong local effects. We then found a way to introduce isotropic confinement at contact level, by pre-adjusting :math:`\eps_N` and post-adjusting :math:`\sigma_N`; the supposition is that the confinement value :math:`\sigma_0` is negative and does not lead to immediate damage to contacts.
         
Given a prescribed confinement value :math:`\sigma_0`, we adjust :math:`\eps_N` value *before* computing normal and shear stresses:

.. math::

   \eps_N'&=\eps_N+\begin{cases}
      \sigma_0/k_N & \hbox{if $\sigma_0>k_N\eps_s$,} \\
      \eps_s+\frac{\sigma_0-k_N\eps_s}{k_N\tilde K_s} & \hbox{otherwise,} \\
   \end{cases}


where the second case takes in account compressive plasticity. The constitutive law then uses the adjusted value :math:`\eps_N'`. At the end, computed normal stress is adjusted back to

.. math:: \sigma_N'=\sigma_N-\sigma_0

before being applied on particle in contact.


Shear stresses
^^^^^^^^^^^^^^^^
For the shear stress we use plastic constitutive law

.. math:: \vec{\sigma}_T=k_T(\vec{\eps}_T-\vec{\eps}_{Tp})

where :math:`\eps_{Tp}` is the plastic strain on the contact and :math:`k_T` is shear contact modulus computed from :math:`k_N` as the ratio :math:`k_T/k_N` is fixed (usually 0.2, see the :ref:`concrete-model-calibration`).
   
XXX    
In the DEM formulation (large strains), however, :math:`\vec{\eps}_{Tp}` is not stored and mapped contact points on element surfaces are moved instead as explained above, sect.~\ref{sect-formulation-total-shear}.

The shear stress is limited by the yield function (:ref:`figure <concrete-yield-surface-flow-rule>`)

.. math::
   :label: plasticity-function

   f(\sigma_N,\vec{\sigma}_T)&=|\vec{\sigma}_T|-r_{pl}=|\vec{\sigma}_T-(c_T-\sigma_N\tan\phi),

   c_T=c_{T0}(1-\omega)

where material parameters :math:`c_{T0}` and :math:`\phi` are initial cohesion and internal friction angle, respectively. The initial cohesion  :math:`c_{T0}` is reduced to the current cohesion :math:`c_T` using damage state variable :math:`\omega`. Note that we split the plasticity function in a part that depends on :math:`\vec{\sigma}_T` and another part which depends on already known values of :math:`\omega` and :math:`\sigma_N`; the latter is denoted :math:`r_{pl}`, radius of the plasticity surface in given :math:`\sigma_N` plane.

The plastic flow rule 

.. math:: \vec{\dot\eps}_{Tp}&=\dot\lambda\frac{\vec{\sigma}_T}{|\vec{\sigma}_T|},

:math:`\lambda` being plastic multiplier, is associated in the plane of shear stresses but not with respect to the normal stress (:ref:`figure <concrete-yield-surface-flow-rule>`). The advantage of using a non-associated flow rule is computational. At every step, :math:`\sigma_N` can be evaluated directly, followed by a direct evaluation of :math:`\vec{\sigma}_T`; stress return in shear stress plane reduces to simple radial return and does not require any iterations as :math:`f(\sigma_N,\vec{\sigma}_T)=0` is satisfied immediately.
      
In the implementation, numerical evaluation starts from current value of :math:`\vec{\eps}_T`. Trial stress :math:`\vec{\sigma}_T^t=\vec{\eps}_T k_T` is computed and compared with current plasticity surface radius :math:`r_{pl}` from :eq:`plasticity-function`. If :math:`|\vec{\sigma}_T^t|>r_{pl}`, the radial stress return is performed; since we do not store :math:`\vec{\eps}_{Tp}`, :math:`\vec{\eps}_T` is updated as well in such case:

.. math::
   :nowrap:

   \begin{align}
      \vec{\sigma}_T&=r_{pl} \normalized{\vec{\sigma_T}} \\
      \vec{\eps}_T'&=\normalized{\vec{\eps}_T}\frac{|\vec{\sigma}_T|}{|\vec{\sigma}_T^t|}
   \end{align}

If :math:`|\vec{\sigma}_T^t|\leq r_{pl}`, there is no plastic slip and we simply assign :math:`\vec{\sigma}_T=\vec{\sigma}_T^t` without :math:`\vec{\eps}_T` update.
      
.. _concrete-yield-surface-flow-rule:

.. figure:: fig/yield-surface-and-plastic-flow-rule.*
   :figclass: align-center
   :width: 80%

   Linear yield surface and plastic flow rule.

Confinement extension
"""""""""""""""""""""
As in the case of normal stress, we introduce an extension to better capture confinement effect and to prevent shear locking under extreme compression: instead of using linear plastic surface we replace it by logarithmic surface in the compression part, which has :math:`C_1` continuity with the linear surface in tension; the continuity condition avoids pathologic behavior around the switch point and also reduces number of new parameters. Instead of :eq:`plasticity-function`, we use

.. math::

   f(\sigma_N,\sigma_T)=
      \begin{cases}
         |\sigma_T|-(c_{T0}(1-\omega)-\sigma_N\tan\phi) & \hbox{if } \sigma_N\geq0 \\
         |\sigma_T|-c_{T0}\left[(1-\omega)+Y_0\tan\phi\log\left(\frac{-\sigma_N}{c_{T0} Y_0}+1\right)\right] & \hbox{if } \sigma_N<0,
      \end{cases}

which introduces a new dimensionless parameter :math:`Y_0` determining how fast the logarithm deviates from the original, linear form. The function is shown here:

.. _concrete-yield-surf-log:

.. figure:: fig/yield-surfaces.*
   :figclass: align-center
   :width: 80%

   Comparison of linear and logarithmic (in compression) yield surfaces, for both virgin and damaged material.

Visco-plasticity
"""""""""""""""""
Viscosity in shear uses similar ideas as visco-damage from sect.~\ref{sect-cpm-visco-damage}; the value of :math:`r_t` in :eq:`plasticity-function` is augmented depending on rate of plastic flow following the equation

.. math:: r_{pl}'=r_{pl}+c_{T0}\left(\tau_{pl}\vec{\dot\eps}_{Tp}\right)^{M_{pl}}=r_{pl}+c_{T0}\left(\tau_{pl}\frac{\Delta\vec{\eps}_{Tp}}{\Dt}\right)^{M_{pl}}

where :math:`\tau_{pl}` is characteristic time for visco-plasticity, :math:`M_{pl}` is a dimensionless exponent (both to be calibrated. :math:`c_{T0}` is undamaged cohesion introduced for the sake of dimensionality. 
         
Similar solution strategy as for visco-damage is used, with different substitutions

.. math::
   :nowrap:
   
   \begin{align}
      \beta&=\ln\left(\frac{|\vec{\sigma}_T^t|-r_{pl}'}{|\vec{\sigma}_T^t|-r_{pl}}\right), \\
      c&=c_{T0}\left(|\vec{\sigma}_T^t|-r_{pl}\right)^{M_{pl}-1}\left(\frac{\tau_{pl}}{k_T \Dt}\right)^{M_{pl}}.
   \end{align}

The equation to solve is then formally the same as :eq:`eq-cpm-logeq`

.. math:: \ln\left({\rm e}^\beta+c{\rm e}^{M_{pl}\beta}\right)=0.

After finding :math:`\beta` using the Newton-Raphson method, trial stress :math:`\vec{\sigma}_T^t` is scaled by the factor

.. math:: \frac{r'_{pl}}{|\vec{\sigma}_T^t|}=1-e^\beta\left(1-\frac{r_{pl}}{|\vec{\sigma}_T^t|}\right)

to obtain the final shear stress value

.. math:: \vec{\sigma}_T=\frac{r'_{pl}}{|\vec{\sigma}_T^t|}\vec{\sigma}_T^t.
         
         
Applying stresses on particles
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Resulting stresses :math:`\sigma_N` and and :math:`\vec{\sigma}_T` are computed at the current contact point :math:`\curr{\vec{C}}`. Summary force :math:`\vec{F}_{\Sigma}=A_{eq}(\sigma_N\vec{n}+\vec{\sigma}_T)` has to be applied to particle's positions :math:`\curr{\vec{C}}_1`, :math:`\curr{\vec{C}}_2`, exerting force and torque:

.. math::
   :nowrap:

   \begin{align}
      \vec{F}_1&=\vec{F}_{\Sigma}, & \vec{T}_1&=(\curr{\vec{C}}-\curr{\vec{C}}_1)\times \vec{F}_{\Sigma}, \\
      \vec{F}_2&=-\vec{F}_{\Sigma}, & \vec{T}_2&=-(\curr{\vec{C}}-\curr{\vec{C}}_1)\times \vec{F}_{\Sigma}.
   \end{align}

Forces and torques on particles from multiple interactions accumulate during one computation step.

Contact model summary
^^^^^^^^^^^^^^^^^^^^^^

The computation described above proceeds in the following order:

1. **Isotropic confinement** :math:`\sigma_0` is applied if active. This consists in adjusting normal strain to either :math:`\eps_N\leftarrow\eps_N+\sigma_0` (if :math:`\sigma_0\geq k_N\eps_s`) or :math:`\eps_N\leftarrow\eps_s+(\sigma_0-k_N\eps_s)/(k_N\tilde K_s)`.

2. **Normal stress** :math:`\sigma_N`. :math:`\tilde\eps=\langle\eps_N\rangle` is computed, then the history variable is updated :math:`\kappa\leftarrow\max(\kappa,\tilde\eps)`; :math:`\kappa` is the state variable from which the current damage :math:`\omega=g(k)=1-(\eps_f/\kappa)\exp(-(\kappa-\eps_0)/\eps_f)` is evaluated; for non-cohesive contacts, however, we set :math:`\omega=1`. For cohesive contacts with damage disabled (:obj:`ConcretePhys.neverDamage <woo.dem.COncretePhys.neverDamage>`), we set :math:`\omega=0`.
         
   The state variable :math:`\eps_N^{\rm pl}` (initially zero) holds the normal plastic strain; we use it to compute the elastic part of the current strain :math:`\eps_N^{\rm el}=\eps_N-\eps_N^{\rm pl}`.
            
   Normal stress is computed using the equation of damage mechanics :math:`\sigma_N=(1-H(\eps_N^{\rm el})\omega)k_N\eps_N^{\rm el}`.
            
   Whether compressive hardening is present is determined; :math:`\sigma_{Ns}=k_N(\eps_s+\tilde K_s(\eps_N-\eps_s))` is pre-evaluated; then if :math:`\eps_N^{\rm el}<\eps_s` and :math:`\sigma_{Ns}>\sigma_N`, normal stress and normal plastic strains are updated :math:`\eps_N^{\rm pl}\leftarrow \eps_N^{\rm pl}+(\sigma_N-\sigma_{Ns})/k_N`, :math:`\sigma_N\leftarrow\sigma_{Ns}`. If the condition is not satisfied, compressive plasticity has no effect and does not have to be specifically considered.

3. **Shear stress** :math:`\vec{\sigma}_T`. First, trial value is computed simply from :math:`\vec{\sigma}_T\leftarrow k_T\eps_T`. This value will be compared with the radius of plastic surface :math:`r_{pl}` for given, already known :math:`\sigma_N`. As there are different plasticity functions for tension and compression, we obtain :math:`r_{pl}=c_{T0}(1-\omega)-\sigma_N\tan\phi` for :math:`\sigma_N\geq0`; in compression, the logarithmic surface makes the formula more complicated, giving :math:`r_{pl}=c_{T0}\left[(1-\omega)+Y_0\tan\phi(-\sigma_N/(c_{T0}Y_0)+1)\right]`.
         
   If the trial stress is beyond the admissible range, i.e. :math:`|\vec{\sigma}_T|>r_pl`, plastic flow will take place. Since the total formulation for strain is used, we update the :math:`\vec{\eps}_T` to have the same direction, but the magnitude of :math:`|\vec{\sigma}_T|/k_T`.
            
   Without visco-plasticity (the default), a simple update :math:`\vec{\sigma}_T\leftarrow (\sigma_T/|\sigma_T|)r_{pl}` is performed during the plastic flow. If visco-plasticity is used, we update :math:`\vec{\sigma}_T\leftarrow s\vec{\sigma}_T`, :math:`s` being a scaling scalar. It is computed as :math:`s=1-e^{\beta}(1-r_{pl}/|\vec{\sigma}_T|)`, where :math:`\beta` is solved with Newton-Raphson iteration as described above, with the coefficient :math:`c=c_{T0}(\tau_{pl}/(k_T\Dt))^{M_{pl}}(|\vec{\sigma}_T|-r_{pl})^{M_{pl}-1}` and the exponent :math:`M_{pl}`.
   
4. **Viscous normal overstress** :math:`\sigma_{Nv}` is applied for cohesive contacts only. As explained above, it is effective for non-zero damage rate. When damage is not growing (i.e. the state variable :math:`\eps_d\geq\eps_N\omega`, where :math:`\eps_d` is initially zero), we simply update :math:`\eps_d\leftarrow\eps_N\omega`, and the overstress is zero
         
   Otherwise, the viscosity equation has to be solved using the coefficient :math:`c=\eps_0(1-\omega)(\tau_d/\Dt)^{M_d}(\eps_N\omega-\eps_d)^{M_d-1}` and the exponent :math:`N=M_d`; once :math:`\beta` is solved with the Newton-Raphson method as shown above, we update :math:`\eps_d\leftarrow\eps_d+(\eps_N\omega-\eps_d)e^\beta` and finally obtain :math:`\sigma_{Nv}=(\eps_N\omega-\eps_d)k_N`. Then the overstress is applied via :math:`\sigma_N\leftarrow\sigma_N+\sigma_{Nv}`.
      

The following table summarizes the nomenclature and corresponding internal Woo (and Yade, for reference) variables:

.. list-table::
   :header-rows: 1
   
  * - symbol
    - Woo
    - Yade
    - meaning
  * - :math:`A_{eq}`
    - :obj:`L6Geom.contA <woo.dem.L6Geom.contA>`
    - :obj:`CpmPhys.crossSection <yade:yade.wrapper.CpmPhys.crossSection>`
    - geometry
  * - :math:`\tan\phi`
    - :obj:`FrictPhys.tanPhi <woo.dem.FrictPhys.tanPhi>`
    - :obj:`CpmPhys.tanFrictionAngle <yade:yade.wrapper.CpmPhys.tanFrictionAngle>`
    - material
  * - :math:`\eps_0`
    - :obj:`ConcretePhys.epsCrackOnset <woo.dem.ConcretePhys.epsCrackOnset>`
    - :obj:`CpmPhys.epsCrackOnset <yade:yade.wrapper.CpmPhys.epsCrackOnset>`
    - material
  * - :math:`\eps_f`
    - :obj:`woo.dem.ConcretePhys.epsFracture <woo.dem.ConcretePhys.epsFracture>`
    - :obj:`CpmPhys.epsFracture <yade:yade.wrapper.CpmPhys.epsFracture>`
    - :obj:`~woo.dem.ConcreteMat.epsCrackOnset` multiplied by :obj:`~woo.dem.ConcreteMat.relDuctility`
  * - :math:`\eps_N`
    - :obj:`ConcretePhys.epsN <woo.dem.ConcretePhys.epsN>`
    - :obj:`CpmPhys.epsN <yade:yade.wrapper.CpmPhys.epsN>`
    - state
  * - :math:`\vec{\eps}_T`
    - :obj:`ConcretePhys.epsT <woo.dem.ConcretePhys.epsT>`
    - :obj:`CpmPhys.epsT <yade:yade.wrapper.CpmPhys.epsT>`
    - state
  * - :math:`\eps_s`
    - :obj:`Law2_L6Geom_ConcretePhys.epsSoft <woo.dem.Law2_L6Geom_ConcretePhys.epsSoft>`
    - :obj:`Law2_ScGeom_CpmPhys_Cpm.epsSoft <yade:yade.wrapper.Law2_ScGeom_CpmPhys_Cpm.epsSoft>`
    - material
  * - :math:`\vec{F}_N`
    - -- 
    - :obj:`NormShearPhys.normalForce <yade:yade.wrapper.NormShearPhys.normalForce>`
    - state
  * - :math:`\vec{F}_T`
    - -- 
    - :obj:`NormShearPhys.shearForce <yade:yade.wrapper.NormShearPhys.shearForce>`
    - state
  * - :math:`F_N\vec{n}`
    - :obj:`CPhys.force[0] <woo.dem.CPhys.force>`
    - :obj:`CpmPhys.Fn <yade:yade.wrapper.CpmPhys.Fn>`
    - state
  * - :math:`|\vec{F}_T|`
    - --
    - :obj:`CpmPhys.Fs <yade:yade.wrapper.CpmPhys.Fs>`
    - state
  * - :math:`\vec{F}_T`
    - :obj:`CPhys.force[1,2] <woo.dem.CPhys.force>`
    - --
    - state
  * - :math:`\tilde K_s`
    - :obj:`Law2_L6Geom_ConcretePhys.relKnSoft <woo.dem.Law2_L6Geom_ConcretePhys.relKnSoft>`
    - :obj:`Law2_ScGeom_CpmPhys_Cpm.relKnSoft <yade:yade.wrapper.Law2_ScGeom_CpmPhys_Cpm.relKnSoft>`
    - material
  * - :math:`k_N`
    - :obj:`FrictPhys.kn <woo.dem.FrictPhys.kn>`
    - :obj:`CpmPhys.E <yade:yade.wrapper.CpmPhys.E>`
    - material
  * - :math:`k_T`
    - :obj:`FrictPhys.kt <woo.dem.FrictPhys.kt>`
    - :obj:`CpmPhys.G <yade:yade.wrapper.CpmPhys.G>`
    - material
  * - :math:`\kappa`
    - :obj:`~woo.dem.ConcretePhjys.kappaD`
    - :obj:`CpmPhys.kappaD <yade:yade.wrapper.CpmPhys.kappaD>`
    - state
  * - :math:`M_d`
    - :obj:`ConcreteMat.dmgRateExp <woo.dem.ConcreteMat.dmgRateExp>` :obj:`ConcretePhys.dmgRateExp <woo.dem.ConcretePhys.dmgRateExp>`
    - :obj:`CpmPhys.dmgRateExp <yade:yade.wrapper.CpmPhys.dmgRateExp>`
    - material
  * - :math:`\tau_d`
    - :obj:`~woo.dem.ConcretePhys.dmgTau`
    - :obj:`CpmPhys.dmgTau <yade:yade.wrapper.CpmPhys.dmgTau>`
    - material
  * - :math:`M_{pl}`
    - :obj:`~woo.dem.ConcretePhys.plRateExp`
    - :obj:`CpmPhys.plRateExp <yade:yade.wrapper.CpmPhys.plRateExp>`
    - material
  * - :math:`\tau_{pl}`
    - :obj:`~woo.dem.ConcretePhys.plTau`
    - :obj:`CpmPhys.plTau <yade:yade.wrapper.CpmPhys.plTau>`
    - material
  * - :math:`\omega`
    - :obj:`~woo.dem.ConcretePhys.omega`
    - :obj:`CpmPhys.omega <yade:yade.wrapper.CpmPhys.omega>`
    - state
  * - :math:`\sigma_N`
    - :obj:`ConcretePhys.sigmaN <woo.dem.ConcretePhys.sigmaN>`
    - :obj:`CpmPhys.sigmaN <yade:yade.wrapper.CpmPhys.sigmaN>`
    - state
  * - :math:`\vec{\sigma}_T`
    - :obj:`ConcretePhys.sigmaT <woo.dem.ConcretePhys.sigmaT>`
    - :obj:`CpmPhys.sigmaT <yade:yade.wrapper.CpmPhys.sigmaT>`
    - state
  * - :math:`\sigma_0`
    - :obj:`~woo.dem.ConcretePhys.isoPrestress`
    - :obj:`CpmPhys.isoPrestress <yade:yade.wrapper.CpmPhys.isoPrestress>`
    - material
  * - :math:`c_{T0}`
    - :obj:`~woo.dem.ConcretePhys.coh0`
    - :obj:`CpmPhys.undamagedCohesion <yade:yade.wrapper.CpmPhys.undamagedCohesion>`
    - material
  * - :math:`Y_0`
    - :obj:`~woo.dem.Law2_L6Geom_ConcretePhys.yieldLogSpeed`
    - :obj:`Law2_ScGeom_CpmPhys_Cpm.yieldLogSpeed <yade:yade.wrapper.Law2_ScGeom_CpmPhys_Cpm.yieldLogSpeed>`
    - material


.. _concrete-model-calibration:

----------------------
Parameter calibration
----------------------

The model comprises two sets of parameters that determine elastic and inelastic behavior. In order to match simulations to experiments, a procedure to calibrate these parameters must be given. Since elastic properties are independent of inelastic ones, they are calibrated first.

**Model parameters** can be summarized as follows:

* geometry

  * :math:`r` sphere radius
  * :math:`R_I` interaction radius

* elasticity

  * :math:`k_N` normal contact stiffness
  * :math:`k_T/k_N` relative shear contact stiffness

* damage and plasticity

  * :math:`\eps_0` limit elastic strain
  * :math:`\eps_f` parameter of damage evolution function
  * :math:`C_{T0}` shear cohesion of undamaged material
  * :math:`\phi` internal friction angle

* confinement

  * :math:`Y_0` parameter for plastic surface evolution in compression
  * :math:`\eps_s` hardening strain in compression
  * :math:`\tilde K_s` relative hardening modulus in compression

* rate-dependence

  * :math:`\tau_d` characteristic time for visco-damage
  * :math:`M_d` dimensionless visco-damage exponent
  * :math:`\tau_{pl}` characteristic time for visco-plasticity
  * :math:`M_{pl}` dimensionless visco-plasticity exponent

**Macroscopic properties** should be matched to prescribed values by running simulation on sufficiently large specimen. Let us give overview of them, in the order of calibration:

* *elastic properties*, which depend on only geometry and elastic parameters (using grouping from the list above)

  * :math:`E` Young's modulus,
  * :math:`\nu` Poisson's ratio

* *inelastic properties*, depending (in addition) on damage and plasticity parameters:

  * :math:`f_t` tensile strength
  * :math:`f_c` compressive strength
  * :math:`G_f` fracture energy (conventional definition shown in :ref:`this figure <concrete-fracture-energy>`

* *confinement properties*; they appear only in high confinement situations and can be calibrated without having substantial impact on already calibrated inelastic properties. We do not describe them quantitatively; fitting simulation and experimental curves is used instead.

* *rate-dependence properties*; they appear only in high-rate situations, therefore are again calibrated after inelastic properties independently. As in the previous case, a simple fitting approach is used here.

.. _concrete-fracture-energy:

.. figure:: fig/fracture-energy.*
   :figclass: align-center
   :width: 80%
         
   Conventional definition of fracture energy of our own, which goes only to :math:`0.2f_t` on the strain-stress curve.

Simulation setup
^^^^^^^^^^^^^^^^^

In order to calibrate macroscopic properties, simulations with multiple particles have to be run. This allows to smooth away different orientation of individual contacts and gives apparent continuum-like behavior.

We were running simple strain-controlled tension/compression test on a 1:1:2 cuboid-shaped specimen of 2000 spheres. (Later, the test was being done on hyperboloid-shaped specimen, to pre-determine fracturing area, while avoiding boundary effects.) Straining is applied in the direction of the longest dimension, on boundary particles; they are identified, on the "positive" and "negative" end of the specimen, by distance from bounding box of the specimen; as result, roughly one layer of spheres is considered as support on each side. Distance between (some) two spheres on each end along the strained axis determines the reference length :math:`l_0`; specimen elongation is computed from their current distance divided by :math:`l_0` during subsequent simulation. Straining imposes displacement on support particles along strained axis, symmetrically on either end of the specimen (half on the "positive" and half on the "negative" boundary particles), while all their other degrees of freedom are kept free, including perpendicular translations, leading to simulation of frictionless supports.

Axial force :math:`F` is computed by averaging sums of forces on support particles from both supports :math:`F^+` and :math:`-F^-`. Divided by specimen cross-section :math:`A`, average stress is obtained. The cross-section area is estimated as either cross-section of the specimen's bounding box (for cuboid specimen) or as minimum of several areas :math:`A_i` of convex hull around particles intersecting perpendicular plane at different coordinates along the axis (for non-prismatic specimen):.

.. figure:: fig/uniax-specimen.*
   :figclass: align-center
   :width: 80%

   Simplified scheme of the uniaxial tension/compression setup. Strained spheres, negative and positive support, are shown in green and red respectively. Cross-section area :math:`A` is minimum of convex hull's areas :math:`A_i`.


.. todo:: Such tension/compression test can be found in the \ysrc{examples/concrete/uniax.py} script.

*Periodic boundary conditions* were not implemented in Yade until later stages of the thesis (:obj:`~woo.core.Cell`). In such case, determining deformation and cross-section area is much simpler, as it exists objectively, embodies in the periodic cell size. Computing stress is equally trivial: first, vector of sum of all forces on contacts in the cell (taking tensile forces as positive and compressive as negative) is computed, then divided by-component by perpendicular areas of the cell. This is handled by :obj:`~woo.dem.WeirdTriaxControl`.
         
Stress tensor evaluation
"""""""""""""""""""""""""
Computation of stress from reaction forces is not suitable for cases where the loading scenario is not as straight-forwardly defined as in the case of uniaxial tension/compression. For general case, an equation for stress tensor can be derived. Using the work of :cite:`Kuhl2001`, eqs. (33) and (35), we have

.. math::
   :label: eq-cpm-stress-tensor0
   :nowrap:

   \begin{align}
      \tens{\sigma}&=\frac{1}{V}\sum_{c\in V}\left[\vec{F}_{\Sigma}^c \otimes (\vec{C}_2-\vec{C}_1)\right]^{\rm sym} = \\
         &=\frac{1}{V}\sum_{c\in V}l^c\left[\tens{N}^c \vec{F}_N^c+{\tens{T}^c}^T\vec{F}_T^c\right]
   \end{align}

where :math:`V` is the considered volume containing contacts with the :math:`c` index. For each contact, there is :math:`l=|\vec{C}_2-\vec{C}_1|`, :math:`F_{\Sigma}=F_N\vec{n}+\vec{F}_T`, with all variables assuming their current value. We use 2nd-order normal projection tensor :math:`\tens{N}=\vec{n}\otimes\vec{n}` which, evaluated component-wise, gives

.. math:: \tens{N}_{ij}&=\vec{n}_i\vec{n}_j.


The 3rd-order tangential projection tensor :math:`\tens{T}^T=\tens{I}_{\rm sym}\cdot \vec{n}-\vec{n}\otimes\vec{n}\otimes\vec{n}` is written by components

.. math::
   :nowrap:

   \begin{align}
      \tens{T}_{ijk}^T&=\frac{1}{2}\left(\delta_{ik}\delta_{jl}+\delta_{il}\delta_{jk}\right)\vec{n}_l-\vec{n}_i\vec{n}_j\vec{n}_k = \\
        &=\frac{\delta_{ik}\vec{n}_j}{2}+\frac{\delta_{jk}\vec{n}_i}{2}-\vec{n}_i\vec{n}_j\vec{n}_k.
   \end{align}

Plugging these expressions into :eq:`eq-cpm-stress-tensor0` gives

.. math:: \tens{\sigma}_{ij}=\frac{1}{V}\sum_{c\in V}\vec{n}_i^c\vec{n}_j^c F_N^c+\frac{\vec{n}_j^c \vec{F}_{Ti}^c}{2}+\frac{\vec{n}_i^c \vec{F}_{Tj}^c}{2}-\vec{n}_i^c\vec{n}_j^c\underbrace{\vec{n}_k^c\vec{F}_{Tk}^c}_{=0, \mbox{ since } \vec{n}^c\perp\vec{F}_{T}^c}
            
Results from this formula were slightly lower than stress obtained from support reaction forces. It is likely due to small number of interaction in :math:`V`; we were considering an interaction inside if the contact point was inside spherical :math:`V`, which can also happen for an interaction between two spheres outside :math:`V`; some weighting function could be used to avoid :math:`V` boundary problems.
            
Boundary effect is avoided for periodic cell (:obj:`~woo.core.Cell`), where the volume :math:`V` is defined by its size and all interaction would are summed together.
            
.. This algorithm is implemented in the \yfunfun eudoxos/_eudoxos.InteractionLocator.macroAroundPt() method. 


sect-calibration-elastic-properties

Geometry and elastic parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Let us recall the parameters that influence the elastic response of the model:

* radius :math:`r`. The radius is considered to be the same for all the spheres, for the following two reasons:

  1. The time step of the computation (which is one of the main factors determining computational costs) depends on the smallest critical time step for all bodies. Small elements have a smaller critical time step, therefore they would negatively impact the performance.

  2. A direct correlation of macroscopic and contact-level properties is based on the assumption that the sphere radii are the same.

* interaction radius :math:`R_I` is the relative distance determining the "non-locality" of contact detection. For :math:`R_I=1`, only spheres that touch are considered as being in contact. In general, the condition reads
   
  .. math:: d_0&\leq R_I(r_1+r_2).

  The value of :math:`R_I` directly influences the average number of interactions per sphere (percolation). For our purposes, we recommend to use :math:`R_I=1.5`, which gives the average of ≈12 interactions per sphere for packing with porosity < 0.5.
               
  This value was determined experimentally based on the average number of interactions; it stabilizes the packing with regards to contact-level phenomena (damage) and makes the model, in a way, "non-local".

  :math:`R_I` also importantly influences the :math:`f_t/f_c` ratio, which was the original motivation for increasing its value from 1.

  :math:`R_I` only serves to create initial (cohesive) interactions in the packing; after the initial step, interactions having been established, it is reset to 1. A disadvantage is that fractured material which becomes compact again (such as dust compaction) will have a smaller elastic stiffness, since it will have a smaller number of contacts per sphere.

* :math:`k_N` and :math:`k_T` are contact moduli in the normal and shear directions introduced above.

These 4 parameters should be calibrated in such way that the given macroscopic properties :math:`E` and :math:`\nu` are matched. It can be shown by dimensional analysis that :math:`\nu` depends on the dimensionless ratio :math:`k_N/k_T` and, if :math:`R_I` is fixed, Young's modulus is proportional to :math:`k_N` (at fixed :math:`k_N/k_T`).
         
By analogy with the microplane theory, the dependence can be derived analytically (see :cite:`Kuhl2001` and also :eq:`eq-c1111-iso`) as

.. math:: \nu=\frac{k_N-k_T}{4k_N+k_T}=\frac{1-k_T/k_N}{4+k_T/k_N},

which matches quite well the results our simulations (below). :cite:`Stransky2010` reports similar numerical results, which get closer to theoretical values as :math:`R_I` grows.


.. _concrete-nu-kt-kn:

.. figure:: fig/nu-calibration.*
   :figclass: align-center
   :width: 80%

   Relationship between :math:`k_T/k_N` and :math:`\nu`.

For :math:`E`, similar equations can be derived, leading to

.. math::
   :label: eq-concrete-ekn-ratio

   \frac{E}{k_N}=\frac{\sum A_i L_i}{3V}\frac{2+3\frac{k_T}{k_N}}{4+\frac{k_T}{k_N}},

where :math:`A_i` is cross-sectional area of contact number :math:`i`, :math:`L_i` is its length and :math:`V` is the volume of space in which the spheres are placed (total volume of the given sample). The first fraction, volume ratio, is determined solely by the interaction radius :math:`R_I`; therefore, :math:`E` depends linearly on :math:`K_N`.

In our case, however, we simply run elastic simulation to determine the actual :math:`E/k_N` ratio :eq:`eq-concrete-ekn-ratio`. To obtain desired macroscopic modulus of :math:`E^*`, the value of :math:`k_N` is scaled by :math:`E^*/E`.

Measuring macroscopic elastic properties
""""""""""""""""""""""""""""""""""""""""
Measuring linear properties in dynamic simulations faces 2 sources of non-linearity:

1. Dynamic oscillations may influence results if strain rate is too high. This can be avoided by stopping loading at some point and letting kinetic energy dissipate by using numerical damping (:obj:`woo.dem.Leapfrog.damping`).

2. Early non-linear behavior might disturb the results. For avoiding damage, special flag :obj:`ConcreteMat.neverDamage <woo.dem.ConcreteMat.neverDamage>` was introduced to the material, causing it to never damage. To prevent plasticity, loading to low strains is necessary. However, due to :math:`R_I=1.5`, there will be no plastic behavior (rearranging particles under initial load, which would make the response artificially more compliant) until later loading stages.

Young's modulus
   can be evaluated in a straight-forward way from its definition :math:`\sigma_i/\eps_i`, if :math:`i\in\{x,y,z\}` is the strained axis.
            
Poisson's ratio.
   The original idea of measuring specimen dilation by tracking displacement of some boundary spheres was quickly abandoned, as it was giving highly unstable response due to local irregularities and boundary effects. Later, a simple and reliable way was found, consisting in correlation between average axial and transversal displacements.
            
   Taking :math:`w\in\{x,y,z\}`, we evaluate displacement from the initial position :math:`\Delta w(w)` for all particles. To avoid boundary effect, only sufficient number of particles inside the specimen can be considered. The slope of linear regression :math:`\widehat{\Delta w}(w)` has the meaning of average :math:`\eps_w`, shown in the :ref:`following figure <concrete-poisson-regression>`. If :math:`z` is the strained axis, Poisson's ratio is then computed as

   .. math:: \nu=\frac{-\frac{1}{2}(\eps_x+\eps_y)}{\eps_z}.

   .. _concrete-poisson-regression:

   .. figure:: fig/poisson-regression.*
      :figclass: align-center
      :width: 80%

   Displacements during uniaxial tension test, plotted against position on respective axis. The slope of the regression :math:`\widehat{\Delta x}(x)` is the average :math:`\eps_x` in the specimen. Straining was applied in the direction of the :math:`z` axis (as :math:`\eps_z>0`) in the case pictured.}

.. The algorithms described were implemented in the \yfun eudoxos.estimatePoissonYoung() function.



Damage and plasticity parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Once the elastic parameters are calibrated, inelastic parameters :math:`\eps_0`, :math:`\eps_f`, :math:`c_{T0}` and :math:`\phi` should be adjusted such that we obtain the desired macroscopic properties :math:`f_t`, :math:`f_c`, :math:`G_f`.

The calibration procedure is as follows:

1. We transform model parameters to be dimensionless and material properties to be normalized:

   * **parameters**

     * :math:`\frac{\eps_f}{\eps_0}` (relative ductility)
     * :math:`\frac{c_{T0}}{k_T\eps_0}`
     * :math:`\phi`
     * (:math:`\eps_0` is left as-is)

   * **properties**

     * :math:`\frac{f_c}{f_t}` (strength ratio)
     * :math:`\frac{k_N G_f}{f_t^2}` (characteristic length)
     * (:math:`f_t` is left as-is)

   There is one additional degree of freedom on both sides (:math:`\eps_0` and :math:`f_t`), which we will use later.

2.  Since there is one additional parameter on the material model side, we fix :math:`c_{T0}` to a known good value. It was shown that it has the least influence on macroscopic properties, hence the choice.

3.  From graphs showing the parameter/property dependence, we set :math:`\eps_f/\eps_0` to get the desired :math:`{k_N G_f}/{f_t^2}` (:ref:`lower right in the figure <concrete-calibration-nonelastic>`), since the only remaining parameter :math:`\phi` has (almost) no influence on :math:`{k_N G_f}/{f_t^2}` (:ref:`lower left in the figure <concrete-calibration-nonelastic>`).

4. We set :math:`\tan\phi` such that we obtain the desired :math:`f_c/f_t` (:ref:`upper left in the figure <concrete-calibration-nonelastic>`).

5. We use the remaining degree of freedom to scale the stress-strain diagram to get the absolute values using :ref:`radial scaling <concrete-radial-scaling>`. By dimensional analysis it can be shown that

.. math:: f_t=k_N\eps_0\Psi\left(\frac{\eps_f}{\eps_0},\frac{c_{T0}}{k_T\eps_0},\phi\right).

Since :math:`k_N` is already determined, it is only :math:`\eps_0` that will directly determine :math:`f_t`.

.. _concrete-calibration-nonelastic:

.. figure:: fig/nonelastic-correlations.*
   :figclass: align-center
   :width: 80%

   Cross-dependencies of :math:`\eps_0/\eps_f`, :math:`EG_f/f_t^2` and :math:`\tan\phi`. Since :math:`\tan\phi` has little influence on :math:`k_N G_f/f_t^2` (lower left), first :math:`\eps_f/\eps_0` can be set based on desired :math:`k_N G_f/f_t^2` (lower right), then :math:`\tan\phi` is determined so that wanted :math:`f_c/f_t` ratio is obtained (upper left).

.. _concrete-radial-scaling:

.. figure:: fig/cpm-scaling.*
   :figclass: align-center
   :width: 80%

   Radial and vertical scaling of the stress-strain diagram; vertical scaling is used during calibration and is achieved by changing the value of :math:`\eps_0`.

Confinement parameters
^^^^^^^^^^^^^^^^^^^^^^^
Calibrating three confinement-related parameters :math:`\eps_s`, :math:`\tilde K_s` and :math:`Y_0` is not algorithmic, but rather a trial-and-error process. On the other hand, typically it will be enough to calibrate the parameters for some generic confinement data, both for the lack of availability of exact measurements and for at best fuzzy matching that can be achieved. The chief reason is that the bilinear relationship for plasticity in compression is far from perfect and could be refined by using a smooth function; in our case, however, the confinement extension was only meant to mitigate high strength overestimation under confinement, not to accurately predict behavior under such conditions. Introducing more complicated functions would further increase the number of parameters, which was not desirable.

The experimental data we use come from :cite:`Caner2000` and :cite:`Grassl2006`.

.. _concrete-confined-linear-nosoft:

.. figure:: fig/cpm-confined-plain.*
   :figclass: align-center
   :width: 80%

   Confined compression, comparing experimental data and simulation without the confinement extensions of the model. Experimental results (dashed) by Caner.
   
.. from :cite:`Caner2000`.   # FIXME: put back when Sphinx fixes the bug, see https://github.com/sphinx-doc/sphinx/issues/1798

Consider confined strain-stress diagrams at the :ref:`preceding figure <concrete-confined-linear-nosoft>` exhibiting unrealistic behavior under high confinement (-400MPa). Parameters :math:`\eps_s` and :math:`\tilde K_s` will influence at which point the curve will get to the hardening branch and what will be its tangent modulus  (:ref:`figure <concrete-strain-stress-normal-hardening>`). The :math:`Y_0` parameter determines evolution of plasticity surface in compression (:ref:`figure <concrete-yield-surf-log>`). We recommend the following values of the parameters:

.. math::
   :nowrap:

   \begin{align}
      \eps_s&=-3\cdot10^{-3}, & \tilde K_s&=0.3, & Y_0&=0.1,
   \end{align}

which give curves in the :ref:`following figure <concrete-confined-log-soft>`. It was observed when running multiple simulations that results under high confinement depend greatly on the exact packing configuration, specimen shape and specimen size; therefore, the values given above should be taken with grain of salt.

.. _concrete-confined-log-soft:

.. figure:: fig/cpm-confined-extension.*
   :figclass: align-center
   :width: 80%

   Experimental data and simulation in confined compression, using confinement extensions of the model. Cf. [previous figure] for the influence of those extensions.

.. :ref:`this figure <concrete-confined-linear-nosoft>` #  FIXME: put back when Sphinx fixes the bug, see https://github.com/sphinx-doc/sphinx/issues/1798


During simulation, the confinement effect was introduced on the contact level, in the constitutive law itself, as described in :ref:`concrete-isoprestress`; the confinement is therefore isotropic and without boundary influence.

**Cross-dependencies.** Confinement properties may, to certain extent, have influence on inelastic properties. If that happens, reiterating the calibration with new confinement properties should give wanted results quickly.

Rate-dependence parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^

The visco-damage behavior in tension introduced two parameters, characteristic time :math:`\tau_d` and exponent :math:`M_d`. There is no calibration procedure developed for them, as measuring the response is experimentally challenging and the scatter of results is rather high. Instead, we determined those two parameters by a trial-and-error procedure so that the resulting curve approximately fits the experimental data cloud − we use figures from :cite:`Ragueneau2003`, which are in turn based on published experiments.

The resulting curves show :ref:`tension <concrete-visco-tension>` and :ref:`compression <concrete-visco-compression>`. Because DEM computation would be very slow (large number of steps, determined by critical timestep) for slow rates, those results were computed with the same model implemented in the `OOFEM <http://www.oofem.org>`__ framework (using a static implicit FEM model); this also served to verify that both implementations give identical results. For high loading rates, DEM results deviate, since there is inertial mass that begins to play an important role.

.. _concrete-visco-tension:

.. figure:: fig/rb-tau-calibration-plot-ft.*
   :figclass: align-center
   :width: 80%

   Experimental data and simulation results for tension.

.. _concrete-visco-compression:

.. figure:: fig/rb-tau-calibration-plot-fc.*
   :figclass: align-center
   :width: 80%

   Experimental data and simulation results for compression.

The values that we recommend to use are

.. math::
   :nowrap:
   
   \begin{align} 
      \tau_d&=1000\,{\rm s}, \\
      M_d&=0.3.
   \end{align}

Calibration of visco-plastic parameters was rather simple: we found out that it has no beneficial effect on results; therefore, visco-plasticity should be deactivated. In the implementation, this is done by setting :math:`\tau_{pl}` (:obj:`ConcretePhys.plTau <woo.dem.ConcretePhys.plTau>` to an arbitrary non-positive value).
