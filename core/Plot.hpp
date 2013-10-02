#pragma once
#include<woo/lib/object/Object.hpp>

namespace woo{
	struct Plot: public Object{
		WOO_CLASS_BASE_DOC_ATTRS(Plot,Object,"Storage for plots updated during simulation.",
			// since Scene.plot is noGui, we don't have to specify it here for attributes
			// (there are no handlers for dicts etc in the gui code)
			((py::dict,data,,,"Global dictionary containing all data values, common for all plots, in the form {'name':[value,...],...}. Data should be added using plot.addData function. All [value,...] columns have the same length, they are padded with NaN if unspecified."))
			((py::dict,imgData,,,"Dictionary containing lists of strings, which have the meaning of images corresponding to respective :obj:`woo.plot.data` rows. See :obj:`woo.plot.plots` on how to plot images."))
			((py::dict,plots,,,"dictionary x-name -> (yspec,...), where yspec is either y-name or (y-name,'line-specification'). If ``(yspec,...)`` is ``None``, then the plot has meaning of image, which will be taken from respective field of :obj:`woo.plot.imgData`."))
			((py::dict,labels,,,"Dictionary converting names in data to human-readable names (TeX names, for instance); if a variable is not specified, it is left untranslated."))
			((py::dict,xylabels,,,"Dictionary of 2-tuples specifying (xlabel,ylabel) for respective plots; if either of them is None, the default auto-generated title is used."))
			((py::tuple,legendLoc,py::make_tuple("upper left","upper right"),,"Location of the y1 and y2 legends on the plot, if y2 is active."))
			((Real,axesWd,1,,"Linewidth (in points) to make *x* and *y* axes better visible; not activated if non-positive."))
			((py::object,currLineRefs,,AttrTrait<Attr::noSave>().noGui(),"References to axes which are being shown. Internal use only."))
			((string,annotateFmt," {xy[1]:.4g}",,"Format for annotations in plots; if empty, no annotation is shown; has no impact on existing plots. *xy* is 2-tuple of the current point in data space."))
		);
	};
};
WOO_REGISTER_OBJECT(woo::Plot);
