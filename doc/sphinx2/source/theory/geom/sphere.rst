Sphere
-----------

:obj:`Spheres <woo.dem.Sphere>` are defined by their local coordinates and :obj:`radius <woo.dem.Sphere.radius>`.


Sphere-Sphere contact
^^^^^^^^^^^^^^^^^^^^^^

.. todo:: Something along the following lines (note that htis semsntics -- negative value of distFactor -- is not yet implemented): **Contact criterion for spheres:** When there is not yet contact, it will be created if :math:`u_n=|\curr{\vec{x}}_2-\curr{\vec{x}}_1|-|f_d|(r_1+r2)<0`, where :math:`f_d` is :obj:`interaction radius <Cg2_Sphere_Sphere_L6Geom.distFactor>` (sometimes also called "interaction radius"). If :math:`f_d>0`, then :math:`\vec{u}_{0x}` will be initalized to :math:`u_N`, otherwise to 0. In another words, contact will be created if spheres enlarged by :math:`|f_d|` touch, and the "equilibrium distance" (where :math:`\vec{u}_x-\vec{u}_{0x}=0`) will be set to the current distance if :math:`f_d>0` is positive, and to the geometrically-touching distance if :math:`f_d<0`. Initial contact point is :math:`\vec{c}=\vec{x}_1+\left(r_1+\frac{\vec{u}_{0x}}{2}\right)\normalized{\vec{x}_2-\vec{x}_1}`.

.. todo:: ``distFactor`` is not yet implemented as described above; some formulas mix values at different times, should be checked carefully.


