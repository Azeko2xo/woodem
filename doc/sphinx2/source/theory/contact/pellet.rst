.. _pellet_contact_model:

==============================
Pellet contact model
==============================

Pellet contact model is custom-developed model with high plastic dissipation in the normal sense, accumulating plastic dissipation immediately when loaded. Adhesion force is initially zero, though it grows with normal plastic deformation, i.e. is dependent on the history of the contact loading.

Normal plastic dissipation is controlled by a single dimensionless parameter :math:`\alpha` (:obj:`normPlastCoeff <woo.dem.PelletPhys.normPlastCoeff>`, averaged from :obj:`woo.dem.PelletMat.normPlastCoeff`) which determines the measure of deviation from the linear response.  Adhesion is controlled by the :math:`k_a` parameter (:obj:`ka <woo.dem.PelletPhys.ka>`, computed from the ratio :obj:`woo.dem.PelletMat.kaDivKn`). History-independent adhesion can be added by fake confinement :math:`\sigma_c` (:obj:`sigConfine <woo.dem.Law2_L6Geom_PelletPhys_Pellet.sigConfine>`), or which negative values lead to constant adhesion.

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

If :math:`\sigma_c` (:obj:`sigConfine <woo.dem.Law2_L6Geom_PelletPhys_Pellet.sigConfine>`) is non-zero, :math:`F_n` is updated as :math:`F_n \leftarrow F_n-A\sigma_c` (:math:`A` is the :obj:`contact area <woo.dem.L6Geom.contA>`).

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






