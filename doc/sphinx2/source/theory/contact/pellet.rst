.. _pellet-contact-model:

==============================
Pellet contact model
==============================

Pellet contact model is custom-developed model with high plastic dissipation in the normal sense, accumulating plastic dissipation immediately when loaded. Adhesion force is initially zero, though it grows with normal plastic deformation, i.e. is dependent on the history of the contact loading.

Normal plastic dissipation is controlled by a single dimensionless parameter :math:`\alpha` (:obj:`normPlastCoeff <woo.dem.PelletPhys.normPlastCoeff>`, averaged from :obj:`woo.dem.PelletMat.normPlastCoeff`) which determines the measure of deviation from the linear response.  Adhesion is controlled by the :math:`k_a` parameter (:obj:`ka <woo.dem.PelletPhys.ka>`, computed from the ratio :obj:`woo.dem.PelletMat.kaDivKn`).

.. note:: As an extension to the model, there is history-independent adhesion (confinement) and persistent plastic deformation due to rolling (thinning), which are documented below.

The normal yield force takes the form

.. math::
   :nowrap:

   \begin{align*}
   F_n^y&=-\frac{k_n d_0}{\alpha}\log\left(\alpha\frac{-u_n}{d_0}+1\right), \\
   \frac{\partial F_n^y}{\partial u_n}&=\frac{k_n}{\alpha\frac{-u_n}{d_0}+1}
   \end{align*}

where :math:`k_n` is the :obj:`tangent stiffness <woo.dem.FrictPhys.kn>` at origin (:math:`\frac{\partial F_n^y}{\partial u_n}(u_n=0)=k_n`) computed as in :ref:`linear_contact_model`, :math:`d_0` is the :obj:`initial contact length <woo.dem.L6Geom.lens>` and :math:`u_n` is the :obj:`normal displacement <woo.dem.L6Geom.uN>`. The contact accumulates plastic displacement :math:`u_n^{\mathrm{pl}}` (:obj:`uNPl <woo.dem.PelletCData.uNPl>`) which is initially zero.

The normal trial force is evaluated as

.. math:: F_n^t=k_n(u_n-u_n^{\mathrm{pl}})

and it is used to compute the final normal force :math:`F_n` in the following way:

1. If :math:`F_n^t<0` (compression), then

   1. if :math:`F_n^t\geq F_n^y` (elastic regime), :math:`F_n=F_n^t`.

   2. otherwise (plastic slip), update :math:`u_n^{\mathrm{pl}}=u_n-\frac{F_n^y}{k_n}` and set :math:`F_n=F_n^y`.
 
2. If :math:`F_n^t>0` (tension), evaluate the adhesion function

   .. math:: h(u_n)=-k_a u_n^{\mathrm{pl}}-4\frac{-k_a}{u_n^{\mathrm{pl}}}\left(u_n-\frac{u_n^{\mathrm{pl}}}{2}\right)^2

   and set :math:`F_n=\min(F_n^t,h(u_n))`.

   The :math:`h(u_n)` is constructed as polynomial such that it is zero for :math:`u_n\in\{u_n^{\mathrm{pl}},0\}` and reaches its maximum :math:`-k_a u_n^{\mathrm{pl}}` for 
   
   .. math:: h\left(u_n=\frac{u_n^{\mathrm{pl}}}{2}\right)=-k_a u_n^{\mathrm{pl}}.

   where :math:`k_a` is the adhesion modulus introduced above.

This plot shows the deformation-force diagram with two loading-unloading cycles, showing how plastic deformation and adhesion grow.

.. plot::

   import numpy, pylab, woo.dem
   pylab.figure(figsize=(8,6),dpi=160)
   xx=numpy.linspace(0,-1e-3,100)
   xxShort=xx[xx>-5e-4]
   uNPl=-2e-4
   alpha,kn,d0=3.,1e1,1e-3
   ka=.1*kn
   law=woo.dem.Law2_L6Geom_PelletPhys_Pellet
   def fn(uN,uNPl=0.):
      ft=max(law.yieldForce(uN=uN,d0=d0,kn=kn,alpha=alpha),(uN-uNPl)*kn)
      if ft<0: return ft
      else: return law.adhesionForce(uN,uNPl,ka=ka)
   pylab.plot(xxShort,[kn*x for x in xxShort],label='elastic')
   pylab.plot(xx,[fn(x) for x in xx],label='plastic surface')
   pylab.plot(xx,[fn(x,uNPl) for x in xx])
   pylab.plot(xx,[fn(x,2*uNPl) for x in xx])
   pylab.xlabel('normal displacement $u_n$')
   pylab.ylabel('normal force $F_n$')

   pylab.axhline(0,color='black',linewidth=2)
   pylab.axvline(0,color='black',linewidth=2)
   pylab.annotate('$-k_a u_n^{\\rm pl}$',(uNPl,law.adhesionForce(uNPl,2*uNPl,ka=ka)),xytext=(-10,10),textcoords='offset points',)
   unloadX=1.5*uNPl
   angle=52
   pylab.annotate(r'elastic unloading',(unloadX,kn*(unloadX-uNPl)),xytext=(-70,0),textcoords='offset points',rotation=angle)
   pylab.annotate(r'plastic loading',(.5*unloadX,fn(.5*unloadX)),xytext=(-70,5),textcoords='offset points',rotation=39)
   pylab.annotate(r'adhesion',(uNPl,law.adhesionForce(uNPl,2*uNPl,ka=ka)),xytext=(-80,0),textcoords='offset points',rotation=15)
   pylab.annotate(r'$-\frac{k_n d_0}{\alpha}\log\left(\alpha\frac{-u_n}{d_0}+1\right)$',(4*uNPl,fn(4*uNPl)),xytext=(10,-10),textcoords='offset points')
   pylab.annotate(r'$k_n u_n$',(uNPl,uNPl*kn),xytext=(0,-10),textcoords='offset points')
   pylab.plot(uNPl,0,'ro')
   pylab.annotate(r'$u_n^{\rm pl}$',(uNPl,0),xytext=(10,-20),textcoords='offset points')
   pylab.grid(True)


Energy balance
--------------

As this model unloads linearly, elastic potential energy is computed as with the linear model, i.e.

.. math:: E_e=\frac{1}{2}\left(\frac{F_n^2}{k_n}+\frac{|\vec{F}_t|^2}{k_t}\right).

Normal plastic dissipation, when normal sliding takes place, is computed using backwards trapezoid integration; the previous yield force is

.. math::
   :nowrap:

   \begin{align*}
      \Delta u_n^{\mathrm{pl}}&=\prev{{u_n^{\mathrm{pl}}}}-\curr{{u_n^{\mathrm{pl}}}} \\
      \prev{{F_n^y}}&\approx \curr{{F_n^y}} + \frac{\partial F_n^y}{\partial u_n}(\curr{u_n})\Delta u_n^{\mathrm{pl}}
   \end{align*}

and the normal plastic dissipation

.. math:: \Delta E_{pn}=\frac{1}{2}\left|\prev{{F_n^y}}+\curr{{F_n^y}}\right| \left|\Delta u_n^{\mathrm{pl}}\right|.

Tangential plastic dissipation :math:`\Delta E_{pt}` is computed the same as for the :ref:`linear_contact_model`.

.. note:: Besides the global :obj:`energy tracker <woo.core.Scene.energy>`, the :obj:`contact law <woo.dem.Law2_L6Geom_PelletPhys_Pellet>` stores the dissipated energy also in :obj:`PelletMatState <woo.dem.PelletMatState>`, if contacting particles define it. Each particle receives the increment of :math:`\Delta E_{pn}/2` (:obj:`normPlast <woo.dem.PelletMatState.normPlast>`) and :math:`\Delta E_{pt}/2` (:obj:`shearPlast <woo.dem.PelletMatState.shearPlast>`) . This allows to localize and visualize energy dissipation with the granularity of a single particle -- by choosing the appropriate scalar with :obj:`woo.gl.Gl1_DemField.matStateIx`.

   .. figure:: fig/pellet-shield-dissipation.png
      :align: center
      :width: 100%

      Spatial distribution of plastic dissipation in the simulation of particle stream impacting a shield.

Confinement
-----------

History-independent adhesion can be added by fake confinement :math:`\sigma_c` (:obj:`confSigma <woo.dem.Law2_L6Geom_PelletPhys_Pellet.confSigma>`) of which negative values lead to constant adhesion.

If :math:`\sigma_c` (:obj:`confSigma <woo.dem.Law2_L6Geom_PelletPhys_Pellet.confSigma>`) is non-zero, :math:`F_n` is updated as

.. math:: F_n \leftarrow F_n-A\sigma_c

where :math:`A` is the :obj:`contact area <woo.dem.L6Geom.contA>`.

To account for the effect of :math:`\sigma_c` depending on particle size, :math:`r_{\mathrm{ref}}` and :math:`\beta_c` (:obj:`confRefRad <woo.dem.Law2_L6Geom_PelletPhys_Pellet.confRefRad>` and :obj:`confExp <woo.dem.Law2_L6Geom_PelletPhys_Pellet.confExp>`) can be specified, in which case :math:`F_n` is updated as 

.. math:: F_n \leftarrow F_n-A\sigma_c\underbrace{\left(\frac{A}{\pi r_{\mathrm{ref}}^2}\right)^{\beta_c}}_{\dagger};

this leads to the confining stress being scaled by :math:`\dagger` -- increased/decreased for bigger/smaller particles with :math:`\beta_c>0`, or vice versa, decreased/increased for bigger/smaller particles with :math:`\beta_c<0`.


Thinning
---------

Larger-scale deformation of spherical pellets is modeled using an ad-hoc algorithm for reducing particle radius during plastic deformation along with rolling (:obj:`mass <woo.dem.DemData.mass>` and :obj:`inertia <woo.dem.DemData.inertia>` are not changed, as if the :obj:`density <woo.dem.Material.density>` were accordingly increased). When the contact is undergoing plastic deformation (i.e. :math:`F_n^t<F_n^y`; notice that :math:`F_n^y<0` and :math:`F_n^t<0` in compression), then the particle radius is updated. 

This process is controlled by three parameters:

#. :math:`\theta_t` (:obj:`thinRate <woo.dem.Law2_L6Geom_PelletPhys_Pellet.thinRate>`) controlling the amount of radius decrease per unit normal plastic deformation (:math:`\Delta u_N^{\mathrm{pl}}`) and unit rolling angle (:math:`\omega_r\Delta t`, where :math:`\omega_r` is the tangential part (:math:`y` and :math:`z`) of :obj:`relative angular velocity <woo.dem.L6Geom.angVel>` :math:`\vec{\omega}_r`); the unit of :math:`\theta_t` is therefore :math:`\mathrm{1/rad}` (i.e. dimensionless).

#. :math:`r_{\min}^{\mathrm{rel}}` (:obj:`thinRelRMin <woo.dem.Law2_L6Geom_PelletPhys_Pellet.thinRelRMin>`), dimensionless, relative minimum particle radius (the original paticle radius is noted :math:`r_0`).

#. :math:`\gamma_t` (:obj:`thinExp <woo.dem.Law2_L6Geom_PelletPhys_Pellet.thinExp>`), exponent for decreasing the thinning rate as the minimum radius is being approached. The valid range is :math:`\gamma_t\in\langle 0,\infty)`, otherwise, the reduction is not done (equivalent to :math:`\gamma_t=0`). Larger values of :math:`\gamma_t` make approaching the radius floor :math:`r_{\min}^{\mathrm{rel}}` harder.

.. plot::

	import numpy, pylab, woo.dem
	r0,r1=0.7,1.0
	rr=numpy.linspace(r0,r1,150)
	expFunc=lambda r,gamma: ((r-r0)/(r1-r0))**gamma
	kw=dict(linewidth=3,alpha=.5)
	for gamma in (0,.2,1,2,10): pylab.plot(rr,[expFunc(r,gamma) for r in rr],label=r'$\gamma_t=%g$'%gamma,**kw)
	l=pylab.legend(loc='best')
	l.get_frame().set_alpha(.6)
	pylab.ylim(-.05,1.05)
	pylab.xlim(.9*r0,1.1*r1)
	pylab.xlabel('radius')
	pylab.ylabel(r'reduction of thinning rate $\theta_t$')
	for r in (r0,r1): pylab.axvline(r,color='black',linewidth=2,alpha=.4)
	pylab.grid(True)
	pylab.suptitle('Reduction of $\\theta_t$ based on $r$ (with $r_0=1$, $r_{\min}^{\mathrm{rel}}=0.7$)')

When thinning is active, the radius is updated as follows:

.. math::
	:nowrap:

	\begin{align}
		r_{\min} &= r_{\min}^{\mathrm{rel}} r_0, \tag{*} \\
		\Delta u_N^{\mathrm{pl}}&=\curr{(u_N^{\mathrm{pl}})}-\prev{(u_N^{\mathrm{pl}})}, \\
		(\Delta r)_0 &=\theta_t \Delta u_N^{\mathrm{pl}} \omega_r \Delta t, \tag{**} \\
		\Delta r&=\begin{cases}(\Delta_r)_0 & \gamma_t<0 \\ (\Delta_r)_0\left(\frac{r-r_{\min}}{r_0-r_{\min}}\right)^{\gamma_t} & \mbox{otherwise} \end{cases}, \\
		r & \rightarrow \min(r-\Delta r,r_{\min}).
	\end{align}

.. note:: Unlike :math:`u_n^{\mathrm{pl}}` which is stored per-contact (:obj:`uNPl <woo.dem.PelletCData.uNPl>`) and is zero-initialized for every new contact, the change of :obj:`radius <woo.dem.Sphere.radius>` is *permanent*. It is possible to recover the original radius in :obj:`woo.dem.BoxDeleter` by setting the :obj:`recoverRadius <woo.dem.BoxDeleter.recoverRadius>` flag, which re-computes the radius from mass and density.

.. _pellet-contact-model-thinning-radius-dependence:

Radius-dependence
'''''''''''''''''
The :math:`r_{\min}^{\mathrm{rel}}` and :math:`\theta_t` in the previous can be made effectively size-dependent by setting the :math:`r_{\rm thinRefRad}` (:obj:`thinRefRad <woo.dem.Law2_L6Geom_PelletPhys_Pellet.thinRefRad>`) to a positive value. In that case, two additional exponents :obj:`thinMinExp <woo.dem.Law2_L6Geom_PelletPhys_Pellet.thinMinExp>` and :obj:`thinRateExp <woo.dem.Law2_L6Geom_PelletPhys_Pellet.thinRateExp>` are used to multiply the equations :math:`(*)` and :math:`(**)` by exponential scaling factors. Note that the dependency is on the original radius :math:`r_0`, **not** the curretn radius :math:`r`:

.. math::
	:nowrap:

	\begin{align*}
		r_{\min} &= \underbrace{\left(\frac{r_0}{r_{\rm thinRefRad}}\right)^{\rm thinMinExp} r_{\min}^{\mathrm{rel}} }  r_0, \tag{*} \\
		(\Delta r)_0 &=\underbrace{\left(\frac{r_0}{r_{\rm thinRefRad}}\right)^{\rm thinRateExp} \theta_t} \Delta u_N^{\mathrm{pl}} \omega_r \Delta t. \tag{**} \\
	\end{align*}


