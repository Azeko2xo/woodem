.. _user-manual-flow-analysis:

***************
Flow analysis
***************

The :obj:`woo.dem.FlowAnalysis` engine is used to periodically collect particle data on a given :obj:`~woo.dem.FlowAnalysis.box`, using uniform grid with `trilinear interpolation <http://en.wikipedia.org/wiki/Trilinear_interpolation>`__ to save values. The :obj:`~woo.dem.FLowAnalysis.dLim` parameter serves to define fraction limits, as fractions are observed separately.

This :obj:`basic example <woo.pre.horse.FallingHorse>` shows flow rate (computed after the simulation) rendered as volume cloud in Paraview, along with particles and meshes being visible; in addition, `StreamTracer <http://paraview.org/OnlineHelpCurrent/StreamTracer.html>`__ is used to integrate the flow field and render it as lines.

.. youtube:: LB3T6sBdwz0


Each grid cell is cube-shaped with math:`V=h\times h\times h`; the data collected in each grid point are

* flow vector computed from :math:`\vec{v}m/(Vt)` (momentum per volume and time, unit-wise giving :math:`\mathrm{kg(m/s)/(m^3s)=(kg/m^2)/s}`);
* kinetic energy density :math:`E_k/(Vt)`;
* hit count :math:`1/(Vt)`;

where each value is multiplied by its weight :math:`w` respective to the grid interpolation point:

#. determine indices :math:`(i,j,k)` defining the grid cell :math:`\{i,i+1\}\times\{j,j+1\}\times\{k,k+1\}` in which the point in question is contained. 
#. compute cell-normalized coordinates :math:`\hat{x}=\frac{x-x_i}{h}`, :math:`\hat{y}=\frac{y-y_j}{h}`, :math:`\hat{z}=\frac{z-z_k}{h}`.
#. There are 8 cell corners:

   ====================== ===================== ==========================================
   grid indices           normalized coordinate point weight :math:`w_i`
   ---------------------- --------------------- ------------------------------------------
   :math:`(i,j,k)`        :math:`(0,0,0)`       :math:`(1-\hat{x})(1-\hat{y})(1-\hat{z})`
   :math:`(i+1,j,k)`      :math:`(1,0,0)`       :math:`\hat{x}(1-\hat{y})(1-\hat{z})`
   :math:`(i+1,j+1,k)`    :math:`(1,1,0)`       :math:`\hat{x}\hat{y}(1-\hat{z})`
   :math:`(i,j+1,k)`      :math:`(0,1,0)`       :math:`(1-\hat{x})\hat{y}(1-\hat{z})`
   :math:`(i,j,k+1)`      :math:`(0,0,1)`       :math:`(1-\hat{x})(1-\hat{y})\hat{z}`
   :math:`(i+1,j,k+1)`    :math:`(1,0,1)`       :math:`\hat{x}(1-\hat{y})\hat{z}`
   :math:`(i+1,j+1,k+1)`  :math:`(1,1,1)`       :math:`\hat{x}\hat{y}\hat{z}`
   :math:`(i,j+1,k+1)`    :math:`(0,1,1)`       :math:`(1-\hat{x})\hat{y}\hat{z}`
   ====================== ===================== ==========================================

   which are assigned values using their respective weight :math:`w_i`. It can be verified that the sum of all :math:`w_i` is one, i.e. that the inrpolation is a partition of unity.

Fraction separation
====================

Using the :obj:`~woo.dem.FlowAnalysis.dLim`, fractions to be analyzed separately can be defined; we name those fields :math:`A` and :math:`B` in the following. Fraction behavior can be inspected visually, but analysis functions harness the computer power to show relevant data only (see the documentation of :obj:`woo.dem.FlowAnalysis` for the reference). Some of the available functions are:

#. Cross-product (called ``cross`` in the VTK files), showing rotation of one vector field with respect to another, computed simply as :math:`A\times B`.
#. Flow difference (``diff``), showing :math:`\delta_{\alpha}=A-\alpha B`, where :math:`\alpha=\bar{A}{\bar{B}}` (:math:`\bar{\bullet}` denotes global average norm value) making the fraction flow directly comparable.
#. Signed flow differences for both fractions (``diffA``, ``diffB``) defined as

   .. math::
      :nowrap:

      \begin{align*}
         \delta_{\alpha}^A&=\begin{cases}\delta_{\alpha} & \hbox{if }\delta_{\alpha}\cdot(A+B)>0 \\ \vec{0} & \hbox{otherwise}\end{cases} \\
         \delta_{\alpha}^B&=\begin{cases} \vec{0} & \hbox{if }\delta_{\alpha}\cdot(A+B)>0 \\ \delta_{\alpha} & \hbox{otherwise}\end{cases} \\
       \end{align*}


In the following example, we are examining segregation in a chute. We export fractions for Paraview separately and apply the Glyph filter to both of them. Disable the :guilabel:`Mask points` option, adjust the :guilabel:`Set Scale Factor` if necessary. Set :menuselection:`Coloring --> Solid Color` and then :menuselection:`Coloring --> Edit` to set color for each fraction differently:

.. image:: fig/flow-paraview-vector-field.*

In our example, we obtain the following two flow fields, separately for small (green) and big (red) fractions.

.. image:: fig/flow-two-fields.*
	:width: 100%

In the middle of this image, the big (red) fraction is going more towards the right while the small (green) fraction sinks -- this show that segregation is taking place in this region. Segregation can be visualized by computing `vector product <http://en.wikipedia.org/wiki/Cross_product>`__ of both fractions; the vectors now indicate rotation of the big (red) fraction flow with respect to the small (green) fraction flow; following the right-hand rule, if you align the right thumb with the arrow, the fingers will show how is the big (red) fraction deviating from the small (green) one (the stream lines were added for visual clarity):

.. image:: fig/flow-cross.*
	:width: 100%

This can be visualized along with the flow fields (the region we were analyzing is now at the bottom of the image, and another strong segregation region is on the top):

.. image:: fig/flow-cross-bigger.*
	:width: 100%

We visualize signed flow difference fields (again using different solid color for each of them); the red field shows were there is prevalent flow of the big fraction (without corresponding flow of the small fraction) and vice versa. The image therefore reveals that the deposition of particles at the bottom is irregular, since the small fraction falls down first (on the left).

.. image:: fig/flow-signed-diff.*
	:width: 100%


