# encoding: utf-8
# 2008 © Václav Šmilauer <eudoxos@arcig.cz> 
"""
Module containing utility functions for plotting inside woo. Most functionality is exposed through :ref:`woo.core.Plot`, however
"""

## all exported names
__all__=['live','liveInterval','autozoom','legendAlpha','scientific','scatterMarkerKw']


	

try:
	import Image
except:
	raise ImportError("PIL (python-imaging package) must be installed to use woo.plot")



import matplotlib,os,time,math,itertools

# running in batch
#
# If GtkAgg is the default, X must be working, which is not the case
# with batches (DISPLAY is unset in such case) and importing pylab fails then.
#
# Agg does not require the GUI part and works without any DISPLAY active
# just fine.
#
# see http://www.mail-archive.com/woo-dev@lists.launchpad.net/msg04320.html 
# and https://lists.launchpad.net/woo-users/msg03289.html
#
import woo.runtime
if not woo.runtime.hasDisplay: matplotlib.use('Agg')
else:
	matplotlib.use('Qt4Agg')
	# matplotlib.use('GTKAgg')
	# there is probably no reason to support TkAgg anymore, but keep this for the future perhaps...
	if 0:
		# multi-threaded support for Tk; safe to import even if Tk will not be used
		# live plots are dysfunctional anyway (dye to the qt4 gui?) so this can be safely removed later
		try: import mtTkinter as Tkinter
		except ImportError:
			print 'mtTkinter not imported, but TkAgg backend for matplotlib is used; live plots will be broken, don\'t complain'
		matplotlib.use('TkAgg')
from minieigen import *

matplotlib.rc('axes',grid=True) # put grid in all figures
import pylab

# simulation-specific bits moved to woo.core.Plot
# so that they are saved and reloaded with Scene automatically
#data,imgData,plots,labels,xylabels,legendLoc,axesWd=[getattr(woo.master.scene.plot,a) for a in  ('data','imgData','plots','labels','xylabels','legendLoc','axesWd')]

def checkObsolete():
	import warnings
	obsolete=('data','imgData','plots','labels','xylabels','legendLoc','axesWd')
	for o in obsolete:
		if o in globals(): warnings.warn("woo.plot.%s is obsolete, use Scene.plot.%s instead!"%o)

#
# those remain module-global objects
#
live=True if woo.runtime.hasDisplay else False
"Enable/disable live plot updating. Disabled without display (useless)."
liveInterval=.5
"Interval for the live plot updating, in seconds."
autozoom=True
"Enable/disable automatic plot rezooming after data update."
legendAlpha=.6
'Transparency of legend frames in plots'
scientific=True if hasattr(pylab,'ticklabel_format') else False  ## safe default for older matplotlib versions
"Use scientific notation for axes ticks."
current=-1
"Point that is being tracked with a scatter point. -1 is for the last point, set to *nan* to disable."
afterCurrentAlpha=.2
"Color alpha value for part of lines after :ref:`woo.plot.current`, between 0 (invisible) to 1 (full color)"
scatterMarkerKw=dict(verts=[(0.,0.),(-30.,10.),(-25,0),(-30.,-10.)],marker=None)
"Parameters for the current position marker"

componentSeparator='_'
componentSuffixes={Vector2:{0:'x',1:'y'},Vector3:{0:'x',1:'y',2:'z'},Vector2i:{0:'x',1:'y'},Vector3i:{0:'x',1:'y',2:'z'},Vector6:{0:'xx',1:'yy',2:'zz',3:'yz',4:'zx',5:'xy'},Matrix3:{(0,0):'xx',(1,1):'yy',(2,2):'zz',(0,1):'xy',(1,0):'yx',(0,2):'xz',(2,0):'zx',(1,2):'yz',(2,1):'zy'}}
# if a type with entry in componentSuffixes is given in addData, columns for individual components are synthesized using indices and suffixes given for each type, e.g. foo=Vector3r(1,2,3) will result in columns foox=1,fooy=2,fooz=3



def Scene_plot_reset(P):
	"Reset all plot-related variables (data, plots, labels)"
	P.data,P.plots,P.imgData={},{},{}
	pylab.close('all')

def Scene_plot_resetData(P):
	"Reset all plot data; keep plots and labels intact."
	P.data={}

def Scene_plot_splitData(P):
	"Make all plots discontinuous at this point (adds nan's to all data fields)"
	P.addData({})

def Scene_plot_reverseData(P):
	"""Reverse woo.core.Plot.data order.
	
	Useful for tension-compression test, where the initial (zero) state is loaded and, to make data continuous, last part must *end* in the zero state.
	"""
	for k in P.data: P.data[k].reverse()

def addDataColumns(data,dd):
	'''Add new columns with NaN data, without adding anything to other columns. Does nothing for columns that already exist'''
	numSamples=len(data[data.keys()[0]]) if len(data)>0 else 0
	for d in dd:
		if d in data.keys(): continue
		data[d]=[nan for i in range(numSamples)]

def Scene_autoPlotData(S,**kw):
	"""Add data by evaluating contents of :ref:`Plot.plots`. Expressions rasing exceptions will be handled gracefully, but warning is printed for each.
	
	>>> from woo import plot; from woo.dem import *; from woo.core import *
	>>> from pprint import pprint
	>>> S=Scene(fields=[DemField(gravity=(0,0,-10))])
	>>> S.plot.plots={'S.step':('S.time',None,'numParticles=len(S.dem.par)')}
	>>> S.autoPlotData()
	>>> pprint(S.plot.data)
	{'S.step': [0], 'S.time': [0.0], 'numParticles': [0]}

	Note that each item in :ref:`Plot.plots` can be

	* an expression to be evaluated (using the ``eval`` builtin);
	* ``name=expression`` string, where ``name`` will appear as label in plots, and expression will be evaluated each time;
	* a dictionary-like object -- current keys are labels of plots and current values are added to :ref:`woo.plot.data`. The contents of the dictionary can change over time, in which case new lines will be created as necessary.

	A simple simulation with plot can be written in the following way; note how the energy plot is specified.

	>>> from woo import plot, utils
	>>> S=Scene(fields=[DemField(gravity=(0,0,-10))])
	>>> S.plot.plots={'i=S.step':(S.energy,None,'total energy=S.energy.total()')}
	>>> # we create a simple simulation with one ball falling down
	>>> S.dem.par.append(utils.sphere((0,0,0),1,mat=utils.defaultMaterial()))
	0
	>>> S.dem.collectNodes() 
	1
	>>> S.dt=utils.pWaveDt(S)
	>>> S.engines=[ForceResetter(),Leapfrog(kinSplit=True,damping=.4),
	...    # get data required by plots at every step
	...    PyRunner(1,'S.autoPlotData()')
	... ]
	>>> S.trackEnergy=True
	>>> S.run(2,True)
	>>> pprint(S.plot.data)   #doctest: +ELLIPSIS
	{'grav': [0.0, -25.13...],
	 'i': [0, 1],
	 'kinRot': [0.0, 0.0],
	 'kinTrans': [1.8849..., 16.964...],
	 'nonviscDamp': [nan, 10.053...],
	 'total energy': [1.884..., 1.884...]}

	.. plot::

		from woo import *
		from woo.dem import *
		from woo.core import *
		from woo import plot,utils
		S=Scene(fields=[DemField(gravity=(0,0,-10)])
		S.dem.par.append(utils.sphere((0,0,0),1));
		S.dt=utils.pWaveDt(S)
		S.engines=[Leapfrog(damping=.4,kinSplit=True,reset=True),PyRunner(S.autoPlotData()')]
		S.plot.plots={'i=S.step':(S.energy,None,'total energy=S.energy.total()')}
		S.trackEnergy=True
		S.run(500,True)
		import pylab; pylab.grid(True)
		S.plot.legendLoc=('lower left','upper right')
		plot.plot(noShow=True)

	"""
	def colDictUpdate(col,dic,kw):
		'update *dic* with the value from col, which is a "expr" or "name=expr" string; all exceptions from  ``eval`` are caught and warning is printed without adding any data.'
		name,expr=col.split('=',1) if '=' in col else (col,col)
		try:
			val=eval(expr,kw)
			dic.update({name:val})
		except:
			import traceback 
			traceback.print_exc()
			print 'WARN: ignoring exception raised while evaluating auto-column `'+expr+"'%s."%('' if name==expr else ' ('+name+')')
			
	cols={}
	data,imgData,plots=S.plot.data,S.plot.imgData,S.plot.plots
	kw.update(S=S)
	for p in plots:
		pp=plots[p]
		colDictUpdate(p.strip(),cols,kw)
		for y in tuplifyYAxis(plots[p]):
			# imgplot specifier
			if y==None: continue
			yy=addPointTypeSpecifier(y,noSplit=True)[0]
			# dict-like object
			if hasattr(yy,'keys'): cols.update(dict(yy))
			# callable returning list sequence of expressions to evaluate
			#elif callable(yy):
			#	for yyy in yy(): colDictUpdate(yyy,cols)
			# plain value
			else: colDictUpdate(yy,cols,kw)
	S.plot.addData(cols)


def Scene_plot_addData(P,*d_in,**kw):
	"""Add data from arguments name1=value1,name2=value2 to woo.plot.data.
	(the old {'name1':value1,'name2':value2} is deprecated, but still supported)

	New data will be padded with nan's, unspecified data will be nan (nan's don't appear in graphs).
	This way, equal length of all data is assured so that they can be plotted one against any other.

	>>> S=woo.master.scene
	>>> from pprint import pprint
	>>> S.plot.resetData()
	>>> S.plot.addData(a=1)
	>>> S.plot.addData(b=2)
	>>> S.plot.addData(a=3,b=4)
	>>> pprint(S.plot.data)
	{'a': [1, nan, 3], 'b': [nan, 2, 4]}

	Some sequence types can be given to addData; they will be saved in synthesized columns for individual components.

	>>> S.plot.resetData()
	>>> S.plot.addData(c=Vector3(5,6,7),d=Matrix3(8,9,10, 11,12,13, 14,15,16))
	>>> pprint(S.plot.data)
 	{'c_x': [5.0],
	 'c_y': [6.0],
	 'c_z': [7.0],
	 'd_xx': [8.0],
	 'd_xy': [9.0],
	 'd_xz': [10.0],
	 'd_yx': [11.0],
	 'd_yy': [12.0],
	 'd_yz': [13.0],
	 'd_zx': [14.0],
	 'd_zy': [15.0],
	 'd_zz': [16.0]}

	"""
	data,imgData=P.data,P.imgData
	import numpy
	if len(data)>0: numSamples=len(data[data.keys()[0]])
	else: numSamples=0
	# align with imgData, if there is more of them than data
	if len(imgData)>0 and numSamples==0: numSamples=max(numSamples,len(imgData[imgData.keys()[0]]))
	d=(d_in[0] if len(d_in)>0 else {})
	d.update(**kw)
	# handle types composed of multiple values (vectors, matrices)
	dNames=d.keys()[:] # make copy, since dict cannot change size if iterated over directly
	for name in dNames:
		if type(d[name]) in componentSuffixes:
			val=d[name]
			suffixes=componentSuffixes[type(d[name])]
			for ix in suffixes: d[name+componentSeparator+suffixes[ix]]=d[name][ix]
			del d[name]
		elif hasattr(d[name],'__len__'):
			raise ValueError('plot.addData given unhandled sequence type (is a '+type(d[name]).__name__+', must be number or '+'/'.join([k.__name__ for k in componentSuffixes])+')')
	for name in d:
		if not name in data.keys(): data[name]=[]
	for name in data:
		data[name]+=(numSamples-len(data[name]))*[nan]
		data[name].append(d[name] if name in d else nan)
	#print [(k,len(data[k])) for k in data.keys()]
	#numpy.array([nan for i in range(numSamples)])
	#numpy.append(data[name],[d[name]],1)

def Scene_plot_addImgData(P,**kw):
	##S
	data,imgData=P.data,P.imgData
	for k in kw:
		if k not in imgData: imgData[k]=[]
	# align imgData with data
	if len(data.keys())>0 and len(imgData.keys())>0:
		nData,nImgData=len(data[data.keys()[0]]),len(imgData[imgData.keys()[0]])
		#if nImgData>nData-1: raise RuntimeError("imgData is already the same length as data?")
		if nImgData<nData-1: # repeat last value
			for k in imgData.keys():
				lastValue=imgData[k][-1] if len(imgData[k])>0 else None
				imgData[k]+=(nData-len(imgData[k])-1)*[lastValue]
		elif nData<nImgData:
			for k in data.keys():
				lastValue=data[k][-1] if len(data[k])>0 else nan
				data[k]+=(nImgData-nData)*[lastValue]   # add one more, because we will append to imgData below
	# add values from kw
	newLen=(len(imgData[imgData.keys()[0]]) if imgData else 0)+1 # current length plus 1
	for k in kw:
		if k in imgData and len(imgData[k])>0: imgData[k]+=(newLen-len(imgData[k])-1)*[imgData[k][-1]]+[kw[k]] # repeat last element as necessary
		else: imgData[k]=(newLen-1)*[None]+[kw[k]]  # repeat None if no previous value
	# align values which were not in kw by repeating the last value
	for k in imgData:
		if len(imgData[k])<newLen: imgData[k]+=(newLen-len(imgData[k]))*[imgData[k][-1]]
	assert(len(set([len(i) for i in imgData.values()]))<=1)  # no data or all having the same value



# not public functions
def addPointTypeSpecifier(o,noSplit=False):
	"""Add point type specifier to simple variable name; optionally take only the part before '=' from the first item."""
	if type(o) in [tuple,list]:
		if noSplit or not type(o[0])==str: return o
		else: return (o[0].split('=',1)[0],)+tuple(o[1:])
	else: return (o if (noSplit or not type(o)==str) else (o.split('=',1)[0]),'')
def tuplifyYAxis(pp):
	"""convert one variable to a 1-tuple"""
	if type(pp) in [tuple,list]: return pp
	else: return (pp,)
def xlateLabel(l,labels):
	"Return translated label; return l itself if not in the labels dict."
	if l in labels.keys(): return labels[l]
	else: return l

class LineRef:
	"""Holds reference to plot line and to original data arrays (which change during the simulation),
	and updates the actual line using those data upon request."""
	def __init__(self,line,scatter,line2,xdata,ydata,imgData=None,dataName=None):
		self.line,self.scatter,self.line2,self.xdata,self.ydata,self.imgData,self.dataName=line,scatter,line2,xdata,ydata,imgData,dataName
	def update(self):
		if isinstance(self.line,matplotlib.image.AxesImage):
			# image name
			try:
				if len(self.xdata)==0 and self.dataName: self.xdata=self.imgData[self.dataName]  # empty list reference an empty singleton, not the list we want; adjust here
				if self.xdata[current]==None: img=Image.new('RGBA',(1,1),(0,0,0,0))
				else: img=Image.open(self.xdata[current])
				self.line.set_data(img)
			except IndexError: pass
		else:
			# regular data
			import numpy
			# current==-1 avoids copy slicing data in the else part
			if current==None or current==-1 or afterCurrentAlpha==1:
				self.line.set_xdata(self.xdata); self.line.set_ydata(self.ydata)
				self.line2.set_xdata([]); self.line2.set_ydata([])
			else:
				try: # try if we can extend the first part by one so that lines are connected
					self.xdata[:current+1]; preCurrEnd=current+1
				except IndexError: preCurrEnd=current
				preCurrEnd=current+(1 if len(self.xdata)>current else 0)
				self.line.set_xdata(self.xdata[:preCurrEnd]); self.line.set_ydata(self.ydata[:preCurrEnd])
				self.line2.set_xdata(self.xdata[current:]); self.line2.set_ydata(self.ydata[current:])
			try:
				x,y=self.xdata[current],self.ydata[current]
			except IndexError: x,y=0,0
			# this could be written in a nicer way, very likely
			try:
				pt=numpy.ndarray((2,),buffer=numpy.array([float(x),float(y)]))
				if self.scatter:
					self.scatter.set_offsets(pt)
					# change rotation of the marker (possibly incorrect)
					try:
						dx,dy=self.xdata[current]-self.xdata[current-1],self.ydata[current]-self.ydata[current-1]
						# smoothing from last n values, if possible
						# FIXME: does not show arrow at all if less than window values
						#try:
						#	window=10
						#	dx,dy=[numpy.average(numpy.diff(dta[current-window:current])) for dta in self.xdata,self.ydata]
						#except IndexError: pass
						# there must be an easier way to find on-screen derivative angle, ask on the matplotlib mailing list
						axes=self.line.get_axes()
						p=axes.patch; xx,yy=p.get_verts()[:,0],p.get_verts()[:,1]; size=max(xx)-min(xx),max(yy)-min(yy)
						aspect=(size[1]/size[0])*(1./axes.get_data_ratio())
						angle=math.atan(aspect*dy/dx)
						if dx<0: angle-=math.pi
						self.scatter.set_transform(matplotlib.transforms.Affine2D().rotate(angle))
					except IndexError: pass
			except TypeError: pass # this happens at i386 with empty data, saying TypeError: buffer is too small for requested array

liveTimeStamp=0 # timestamp when live update was started, so that the old thread knows to stop if that changes
nan=float('nan')

def createPlots(P,subPlots=True,scatterSize=60,wider=False):
	import logging
	data,imgData,plots,labels,xylabels,legendLoc,axesWd=P.data,P.imgData,P.plots,P.labels,P.xylabels,P.legendLoc,P.axesWd
	if P.currLineRefs:
		logging.info('Closing existing figures')
		figs=set([l.line.get_axes().get_figure() for l in P.currLineRefs]) # get all current figures
		for f in figs: pylab.close(f) # close those
	P.currLineRefs=[]
	if len(plots)==0: return # nothing to plot
	if subPlots:
		# compute number of rows and colums for plots we have
		subCols=int(round(math.sqrt(len(plots)))); subRows=int(math.ceil(len(plots)*1./subCols))
		if wider: subRows,subCols=subCols,subRows
	for nPlot,p in enumerate(plots.keys()):
		pStrip=p.strip().split('=',1)[0]
		if not subPlots: pylab.figure()
		else: pylab.subplot(subRows,subCols,nPlot)
		if plots[p]==None: # image plot
			if not pStrip in imgData.keys(): imgData[pStrip]=[]
			# fake (empty) image if no data yet
			if len(imgData[pStrip])==0 or imgData[pStrip][-1]==None: img=Image.new('RGBA',(1,1),(0,0,0,0))
			else: img=Image.open(imgData[pStrip][-1])
			img=pylab.imshow(img,origin='lower')
			P.currLineRefs.append(LineRef(line=img,scatter=None,line2=None,xdata=imgData[pStrip],ydata=None,imgData=imgData,dataName=pStrip))
			pylab.gca().set_axis_off()
			continue
		plots_p=[addPointTypeSpecifier(o) for o in tuplifyYAxis(plots[p])]
		plots_p_y1,plots_p_y2=[],[]; y1=True
		missing=set() # missing data columns
		if pStrip not in data.keys(): missing.add(pStrip.decode('utf-8','ignore'))
		for d in plots_p:
			if d[0]==None:
				y1=False; continue
			if y1: plots_p_y1.append(d)
			else: plots_p_y2.append(d)
			try:
				if d[0] not in data.keys() and not callable(d[0]) and not hasattr(d[0],'keys'): missing.add(d[0])
			except UnicodeEncodeError: pass
		if missing:
			if len(data.keys())==0 or len(data[data.keys()[0]])==0: # no data at all yet, do not add garbage NaNs
				for m in missing: data[m]=[]
			else:
				# FIXME: this should be handled properly!!!
				try:
					print 'Missing columns in plot.data, adding NaN: ',u','.join(list(missing))
				except UnicodeEncodeError: pass
				addDataColumns(data,missing)
		def createLines(pStrip,ySpecs,isY1=True,y2Exists=False):
			'''Create data lines from specifications; this code is common for y1 and y2 axes;
			it handles y-data specified as callables, which might create additional lines when updated with liveUpdate.
			'''
			# save the original specifications; they will be smuggled into the axes object
			# the live updated will run yNameFuncs to see if there are new lines to be added
			# and will add them if necessary
			yNameFuncs=set([d[0] for d in ySpecs if callable(d[0])]) | set([d[0].keys for d in ySpecs if hasattr(d[0],'keys')])
			yNames=set()
			ySpecs2=[]
			for ys in ySpecs:
				# ys[0]() must return list of strings, which are added to ySpecs2; line specifier is synthesized by tuplifyYAxis and cannot be specified by the user
				if callable(ys[0]): ySpecs2+=[(ret,ys[1]) for ret in ys[0]()]
				elif hasattr(ys[0],'keys'): ySpecs2+=[(yy,'') for yy in ys[0].keys()]
				else: ySpecs2.append(ys)
			if len(ySpecs2)==0:
				print 'woo.plot: creating fake plot, since there are no y-data yet'
				line,=pylab.plot([nan],[nan])
				line2,=pylab.plot([nan],[nan])
				P.currLineRefs.append(LineRef(line=line,scatter=None,line2=line2,xdata=[nan],ydata=[nan]))
			# set different color series for y1 and y2 so that they are recognizable
			if pylab.rcParams.has_key('axes.color_cycle'): pylab.rcParams['axes.color_cycle']='b,g,r,c,m,y,k' if not isY1 else 'm,y,k,b,g,r,c'
			for d in ySpecs2:
				yNames.add(d)
				line,=pylab.plot(data[pStrip],data[d[0]],d[1],label=xlateLabel(d[0],P.labels))
				line2,=pylab.plot([],[],d[1],color=line.get_color(),alpha=afterCurrentAlpha)
				# use (0,0) if there are no data yet
				scatterPt=[0,0] if len(data[pStrip])==0 else (data[pStrip][current],data[d[0]][current])
				# if current value is NaN, use zero instead
				scatter=pylab.scatter(scatterPt[0] if not math.isnan(scatterPt[0]) else 0,scatterPt[1] if not math.isnan(scatterPt[1]) else 0,s=scatterSize,color=line.get_color(),**scatterMarkerKw)
				P.currLineRefs.append(LineRef(line=line,scatter=scatter,line2=line2,xdata=data[pStrip],ydata=data[d[0]]))
			axes=line.get_axes()
			labelLoc=(legendLoc[0 if isY1 else 1] if y2Exists>0 else 'best')
			l=pylab.legend(loc=labelLoc)
			if l:
				l.get_frame().set_alpha(legendAlpha)
				if hasattr(l,'draggable'): l.draggable(True)
			if scientific:
				pylab.ticklabel_format(style='sci',scilimits=(0,0),axis='both')
				# fixes scientific exponent placement for y2: https://sourceforge.net/mailarchive/forum.php?thread_name=20101223174750.GD28779%40ykcyc&forum_name=matplotlib-users
				if not isY1: axes.yaxis.set_offset_position('right')
			if isY1:
				pylab.ylabel((', '.join([xlateLabel(_p[0],P.labels) for _p in ySpecs2])) if p not in xylabels or not xylabels[p][1] else xylabels[p][1])
				pylab.xlabel(xlateLabel(pStrip,P.labels) if (p not in xylabels or not xylabels[p][0]) else xylabels[p][0])
			else:
				pylab.ylabel((', '.join([xlateLabel(_p[0],P.labels) for _p in ySpecs2])) if (p not in xylabels or len(xylabels[p])<3 or not xylabels[p][2]) else xylabels[p][2])
			# if there are callable/dict ySpecs, save them inside the axes object, so that the live updater can use those
			if yNameFuncs:
				axes.wooYNames,axes.wooYFuncs,axes.wooXName,axes.wooLabelLoc=yNames,yNameFuncs,pStrip,labelLoc # prepend woo to avoid clashes
		createLines(pStrip,plots_p_y1,isY1=True,y2Exists=len(plots_p_y2)>0)
		if axesWd>0:
			pylab.axhline(linewidth=axesWd,color='k')
			pylab.axvline(linewidth=axesWd,color='k')
		# create y2 lines, if any
		if len(plots_p_y2)>0:
			pylab.twinx() # create the y2 axis
			createLines(pStrip,plots_p_y2,isY1=False,y2Exists=True)
		### scene is not directly accessible from here, do it like this:
		S=woo.master.scene
		if S.plot==P:
			if 'title' in S.tags: pylab.title(S.tags['title'])



def liveUpdate(P,timestamp):
	global liveTimeStamp
	liveTimeStamp=timestamp
	import sys
	while True:
		if not live or liveTimeStamp!=timestamp:
			return
		figs,axes,linesData=set(),set(),set()
		data=P.data
		for l in P.currLineRefs:
			l.update()
			figs.add(l.line.get_figure())
			axes.add(l.line.get_axes())
			linesData.add(id(l.ydata))
		# find callables in y specifiers, create new lines if necessary
		for ax in axes:
			if not hasattr(ax,'wooYFuncs') or not ax.wooYFuncs: continue # not defined of empty
			yy=set();
			for f in ax.wooYFuncs:
				if callable(f): yy.update(f())
				elif hasattr(f,'keys'): yy.update(f.keys())
				else: raise ValueError("Internal error: ax.wooYFuncs items must be callables or dictionary-like objects and nothing else.")
			#print 'callables y names:',yy
			news=yy-ax.wooYNames
			if not news: continue
			for new in news:
				ax.wooYNames.add(new)
				if new in data.keys() and id(data[new]) in linesData: continue # do not add when reloaded and the old lines are already there
				print 'woo.plot: creating new line for',new
				if not new in data.keys(): data[new]=len(data[ax.wooXName])*[nan] # create data entry if necessary
				#print 'data',len(data[ax.wooXName]),len(data[new]),data[ax.wooXName],data[new]
				line,=ax.plot(data[ax.wooXName],data[new],label=xlateLabel(new,P.labels)) # no line specifier
				line2,=ax.plot([],[],color=line.get_color(),alpha=afterCurrentAlpha)
				scatterPt=(0 if len(data[ax.wooXName])==0 or math.isnan(data[ax.wooXName][current]) else data[ax.wooXName][current]),(0 if len(data[new])==0 or math.isnan(data[new][current]) else data[new][current])
				scatter=ax.scatter(scatterPt[0],scatterPt[1],s=60,color=line.get_color(),**scatterMarkerKw)
				P.currLineRefs.append(LineRef(line=line,scatter=scatter,line2=line2,xdata=data[ax.wooXName],ydata=data[new]))
				ax.set_ylabel(ax.get_ylabel()+(', ' if ax.get_ylabel() else '')+xlateLabel(new,P.labels))
			# it is possible that the legend has not yet been created
			l=ax.legend(loc=ax.wooLabelLoc)
			if l:
				l.get_frame().set_alpha(legendAlpha)
				if hasattr(l,'draggable'): l.draggable(True)
		if autozoom:
			for ax in axes:
				try:
					ax.relim() # recompute axes limits
					ax.autoscale_view()
				except RuntimeError: pass # happens if data are being updated and have not the same dimension at the very moment
		for fig in figs:
			#sys.stderr.write('*')
			try:
				fig.canvas.draw()
			except RuntimeError: pass # happens here too
		#sys.stderr.write('(')
		time.sleep(liveInterval)
		#sys.stderr.write(')')
	
def savePlotSequence(P,fileBase,stride=1,imgRatio=(5,7),title=None,titleFrames=20,lastFrames=30):
	'''Save sequence of plots, each plot corresponding to one line in history. It is especially meant to be used for :ref:`woo.utils.makeVideo`.

	:param stride: only consider every stride-th line of history (default creates one frame per each line)
	:param title: Create title frame, where lines of title are separated with newlines (``\\n``) and optional subtitle is separated from title by double newline. 
	:param int titleFrames: Create this number of frames with title (by repeating its filename), determines how long the title will stand in the movie.
	:param int lastFrames: Repeat the last frame this number of times, so that the movie does not end abruptly.
	:return: List of filenames with consecutive frames.
	'''
	data,imgData,plots=P.data,P.imgData,P.plots
	createPlots(P,subPlots=True,scatterSize=60,wider=True)
	sqrtFigs=math.sqrt(len(plots))
	pylab.gcf().set_size_inches(8*sqrtFigs,5*sqrtFigs) # better readable
	pylab.subplots_adjust(left=.05,right=.95,bottom=.05,top=.95) # make it more compact
	if len(plots)==1 and plots[plots.keys()[0]]==None: # only pure snapshot is there
		pylab.gcf().set_size_inches(5,5)
		pylab.subplots_adjust(left=0,right=1,bottom=0,top=1)
	#if not data.keys(): raise ValueError("plot.data is empty.")
	pltLen=max(len(data[data.keys()[0]]) if data else 0,len(imgData[imgData.keys()[0]]) if imgData else 0)
	if pltLen==0: raise ValueError("Both plot.data and plot.imgData are empty.")
	global current
	ret=[]
	print 'Saving %d plot frames, it can take a while...'%(pltLen)
	for i,n in enumerate(range(0,pltLen,stride)):
		current=n
		for l in P.currLineRefs: l.update()
		out=fileBase+'-%03d.png'%i
		pylab.gcf().savefig(out)
		ret.append(out)
		sys.stderr.write('[%d]')
	if len(ret)==0: raise RuntimeError("No images created?!")
	if title:
		titleImgName=fileBase+'-title.png'
		createTitleFrame(titleImgName,Image.open(ret[-1]).size,title)
		ret=titleFrames*[titleImgName]+ret
	if lastFrames>1: ret+=(lastFrames-1)*[ret[-1]]
	return ret

def createTitleFrame(out,size,title):
	'create figure with title and save to file; a figure object must be opened to get the right size'
	pylab.clf(); fig=pylab.gcf()
	#insize=fig.get_size_inches(); size=insize[1]*fig.get_dpi(),insize[0]*fig.get_dpi()  # this gives wrong dimensions...
	#fig.set_facecolor('blue'); fig.patch.set_color('blue'); fig.patch.set_facecolor('blue'); fig.patch.set_alpha(None)
	title,subtitle=title.split('\n\n')
	lines=[(t,True) for t in title.split('\n')]+([(t,False) for t in subtitle.split('\n')] if subtitle else [])
	nLines=len(lines); fontSizes=size[1]/10.,size[1]/16.
	import matplotlib.mathtext
	def writeLine(text,vertPos,fontsize):
		rgba,depth=matplotlib.mathtext.MathTextParser('Bitmap').to_rgba(text,fontsize=fontsize,dpi=fig.get_dpi(),color='blue')
		textsize=rgba.shape[1],rgba.shape[0]
		if textsize[0]>size[0]:
			rgba,depth=matplotlib.mathtext.MathTextParser('Bitmap').to_rgba(text,fontsize=fontsize*size[0]/textsize[0],dpi=fig.get_dpi(),color='blue')
			textsize=rgba.shape[1],rgba.shape[0]
		fig.figimage(rgba.astype(float)/255.,xo=(size[0]-textsize[0])/2.,yo=vertPos-depth)
	ht=size[1]; y0=ht-2*fontSizes[0]; yStep=(ht-2.5*fontSizes[0])/len(lines)
	for i,(l,isTitle) in enumerate(lines):
		writeLine(l,y0-i*yStep,fontSizes[0 if isTitle else 1])
	fig.savefig(out)
	


def Scene_plot_plot(P,noShow=False,subPlots=True):
	"""Do the actual plot, which is either shown on screen (and nothing is returned: if *noShow* is ``False``) or, if *noShow* is ``True``, returned as matplotlib's Figure object or list of them.
	
	You can use 
	
		>>> import woo,woo.core,os
		>>> S=woo.core.Scene()
		>>> S.plot.plots={'foo':('bar',)}
		>>> S.plot.addData(foo=1,bar=2)
		>>> somePdf=woo.master.tmpFilename()+'.pdf'
		>>> S.plot.plot(noShow=True).savefig(somePdf)
		>>> os.path.exists(somePdf)
		True
		
	to save the figure to file automatically.

	.. note:: For backwards compatibility reasons, *noShow* option will return list of figures for multiple figures but a single figure (rather than list with 1 element) if there is only 1 figure.
	"""
	createPlots(P,subPlots=subPlots)
	figs=set([l.line.get_axes().get_figure() for l in P.currLineRefs])
	if not hasattr(list(figs)[0],'show') and not noShow:
		import warnings
		warnings.warn('plot.plot not showing figure (matplotlib using headless backend?)')
		noShow=True
	if not noShow:
		if not woo.runtime.hasDisplay: return # would error out with some backends, such as Agg used in batches
		if 1:
			if live:
				import threading
				t=threading.Thread(target=liveUpdate,args=(P,time.time()))
				t.daemon=True
				t.start()
			# pylab.show() # this blocks for some reason; call show on figures directly
			for f in figs:
				f.show()
				# should have fixed https://bugs.launchpad.net/woo/+bug/606220, but does not work apparently
				if 0:
					import matplotlib.backend_bases
					if 'CloseEvent' in dir(matplotlib.backend_bases):
						def closeFigureCallback(event):
							ff=event.canvas.figure
							# remove closed axes from our update list
							P.currLineRefs=[l for l in P.currLineRefs if l.line.get_axes().get_figure()!=ff] 
						f.canvas.mpl_connect('close_event',closeFigureCallback)
	else:
		figs=list(set([l.line.get_axes().get_figure() for l in P.currLineRefs]))
		if len(figs)==1: return figs[0]
		else: return figs

def Scene_plot_saveDataTxt(P,fileName,vars=None):
	"""Save plot data into a (optionally compressed) text file. The first line contains a comment (starting with ``#``) giving variable name for each of the columns. This format is suitable for being loaded for further processing (outside woo) with ``numpy.genfromtxt`` function, which recognizes those variable names (creating numpy array with named entries) and handles decompression transparently.

	>>> import woo, woo.core
	>>> from pprint import pprint
	>>> S=woo.core.Scene()
	>>> S.plot.addData(a=1,b=11,c=21,d=31)  # add some data here
	>>> S.plot.addData(a=2,b=12,c=22,d=32)
	>>> pprint(S.plot.data)
	{'a': [1, 2], 'b': [11, 12], 'c': [21, 22], 'd': [31, 32]}
	>>> txt=woo.master.tmpFilename()+'.txt.bz2'
	>>> S.plot.saveDataTxt(txt,vars=('a','b','c'))
	>>> import numpy
	>>> d=numpy.genfromtxt(txt,dtype=None,names=True)
	>>> d['a']
	array([1, 2])
	>>> d['b']
	array([11, 12])

	:param fileName: file to save data to; if it ends with ``.bz2`` / ``.gz``, the file will be compressed using bzip2 / gzip. 
	:param vars: Sequence (tuple/list/set) of variable names to be saved. If ``None`` (default), all variables in :ref:`woo.plot.plot` are saved.
	"""
	import bz2,gzip
	data=P.data
	if not vars:
		vars=data.keys(); vars.sort()
	if fileName.endswith('.bz2'): f=bz2.BZ2File(fileName,'wb')
	elif fileName.endswith('.gz'): f=gzip.GzipFile(fileName,'wb')
	else: f=open(fileName,'wb')
	f.write("# "+"\t".join(vars)+"\n")
	for i in range(len(data[vars[0]])):
		f.write("\t".join([str(data[var][i]) for var in vars])+"\n")
	f.close()


def savePylab(baseName,timestamp=False,title=None):
	'''This function is not finished, do not use it.'''
	import time
	if len(data.keys())==0: raise RuntimeError("No data for plotting were saved.")
	if timestamp: baseName+=_mkTimestamp()
	baseNameNoPath=baseName.split('/')[-1]
	saveDataTxt(fileName=baseName+'.data.bz2')
	if len(plots)==0: raise RuntimeError("No plots to save, only data saved.")
	py=file(baseName+'.py','w')
	py.write('#!/usr/bin/env python\n# encoding: utf-8\n# created '+time.asctime()+' ('+time.strftime('%Y%m%d_%H:%M')+')\n#\nimport pylab, numpy\n')
	py.write("data=numpy.genfromtxt('%s.data.bz2',dtype=None,names=True)\n"%baseName)
	subCols=int(round(math.sqrt(len(plots)))); subRows=int(math.ceil(len(plots)*1./subCols))
	for nPlot,p in enumerate(plots.keys()):
		pStrip=p.strip().split('=',1)[0]
		if plots[p]==None: continue # image plots, which is not exported
		if len(plots)==1: py.write('pylab.figure()\n')
		else: py.write('pylab.subplot(%d,%d,%d)\n'%(subRows,subCols,nPlots))

def _mkTimestamp():
	import time
	return time.strftime('_%Y%m%d_%H:%M')

def Scene_plot_saveGnuplot(baseName,term='wxt',extension=None,timestamp=False,comment=None,title=None,varData=False):
	"""Save data added with :ref:`woo.plot.addData` into (compressed) file and create .gnuplot file that attempts to mimick plots specified with :ref:`woo.plot.plots`.

:param baseName: used for creating baseName.gnuplot (command file for gnuplot), associated ``baseName.data.bz2`` (data) and output files (if applicable) in the form ``baseName.[plot number].extension``
:param term: specify the gnuplot terminal; defaults to ``x11``, in which case gnuplot will draw persistent windows to screen and terminate; other useful terminals are ``png``, ``cairopdf`` and so on
:param extension: extension for ``baseName`` defaults to terminal name; fine for png for example; if you use ``cairopdf``, you should also say ``extension='pdf'`` however
:param bool timestamp: append numeric time to the basename
:param bool varData: whether file to plot will be declared as variable or be in-place in the plot expression
:param comment: a user comment (may be multiline) that will be embedded in the control file

:return: name of the gnuplot file created.
	"""
	data,imgData,plots,labels,xylabels=P.data,P.imgData,P.plots,P.labels,P.xylabels
	if len(data.keys())==0: raise RuntimeError("No data for plotting were saved.")
	if timestamp: baseName+=_mkTimestamp()
	baseNameNoPath=baseName.split('/')[-1]
	vars=data.keys(); vars.sort()
	saveDataTxt(fileName=baseName+'.data.bz2',vars=vars)
	fPlot=file(baseName+".gnuplot",'w')
	fPlot.write('#!/usr/bin/env gnuplot\n#\n# created '+time.asctime()+' ('+time.strftime('%Y%m%d_%H:%M')+')\n#\n')
	if comment: fPlot.write('# '+comment.replace('\n','\n# ')+'#\n')
	dataFile='"< bzcat %s.data.bz2"'%(baseNameNoPath)
	if varData:
		fPlot.write('dataFile=%s'%dataFile); dataFile='dataFile'
	if not extension: extension=term
	i=0
	for p in plots:
		pStrip=p.strip().split('=',1)[0]
		if plots[p]==None: continue  ## this plot is image plot, which is not applicable to gnuplot
		plots_p=[addPointTypeSpecifier(o) for o in tuplifyYAxis(plots[p])]
		if term in ['wxt','x11']: fPlot.write("set term %s %d persist\n"%(term,i))
		else: fPlot.write("set term %s; set output '%s.%d.%s'\n"%(term,baseNameNoPath,i,extension))
		fPlot.write("set xlabel '%s'\n"%xlateLabel(p,labels))
		fPlot.write("set grid\n")
		fPlot.write("set datafile missing 'nan'\n")
		if title: fPlot.write("set title '%s'\n"%title)
		y1=True; plots_y1,plots_y2=[],[]
		# replace callable/dict-like data specifiers by the results, it that particular data exists
		plots_p2=[]
		for pp in plots_p:
			if callable(pp[0]): plots_p2+=[(ppp,'') for ppp in pp[0]() if ppp in data.keys()]
			elif hasattr(pp[0],'keys'): plots_p2+=[(name,val) for name,val in pp[0].items() if name in data.keys()]
			else: plots_p2.append((pp[0],pp[1]))
		plots_p=plots_p2
		#plots_p=sum([([(pp,'') for pp in p[0]() if pp in data.keys()] if callable(p[0]) else [(p[0],p[1])] ) for p in plots_p],[])
		for d in plots_p:
			if d[0]==None:
				y1=False; continue
			if y1: plots_y1.append(d)
			else: plots_y2.append(d)
		fPlot.write("set ylabel '%s'\n"%(','.join([xlateLabel(_p[0],labels) for _p in plots_y1]))) 
		if len(plots_y2)>0:
			fPlot.write("set y2label '%s'\n"%(','.join([xlateLabel(_p[0],labels) for _p in plots_y2])))
			fPlot.write("set y2tics\n")
		ppp=[]
		for pp in plots_y1: ppp.append(" %s using %d:%d title '← %s(%s)' with lines"%(dataFile,vars.index(pStrip)+1,vars.index(pp[0])+1,xlateLabel(pp[0],labels),xlateLabel(pStrip,labels),))
		for pp in plots_y2: ppp.append(" %s using %d:%d title '%s(%s) →' with lines axes x1y2"%(dataFile,vars.index(pStrip)+1,vars.index(pp[0])+1,xlateLabel(pp[0],labels),xlateLabel(pStrip,labels),))
		fPlot.write("plot "+",".join(ppp)+"\n")
		i+=1
	fPlot.close()
	return baseName+'.gnuplot'


def _deprecPlotFunc(old,func,new=None,takesScene=False,*args,**kw):
	"Wrapper for deprecated functions, example below."
	import warnings
	if not new: new=old
	warnings.warn('Function plot.%s is deprecated, use %s.%s instead.'%(old,('Scene' if takesScene else 'Scene.plot'),new),stacklevel=3,category=DeprecationWarning)
	S=woo.master.scene
	if takesScene: return func(S,*args,**kw)
	else: return func(S.plot,*args,**kw)

#
# DEPRECATED functions, will be removed at some point!
#
def reset(): _deprecPlotFunc('reset',Scene_plot_reset)
def resetData(): _deprecPlotFunc('resetData',Scene_plot_resetData)
def splitData(): _deprecPlotFunc('splitData',Scene_plot_splitData)
def reverseData(): _deprecPlotFunc('reverseData',Scene_plot_reverseData)
def addAutoData(): _deprecPlotFunc('addAutoData',Scene_autoPlotData,new='autoPlotData',takesScene=True)
def addData(): _deprecPlotFunc('addData',Scene_plot_addData)
def addImgData(): _deprecPlotFunc('addImgData',Scene_plot_addImgData)
def saveGnuplot(): _deprecPlotFunc('saveGnuplot',Scene_plot_saveGnuplot)
def saveDataTxt(): _deprecPlotFunc('saveDataTxt',Scene_plot_saveDataTxt)
def plot(): _deprecPlotFunc('plot',Scene_plot_plot)

# called at startup from  from woo._monkey.plot
def defMonkeyMethods():
	import woo.core
	woo.core.Plot.reset=Scene_plot_reset
	woo.core.Plot.resetData=Scene_plot_resetData
	woo.core.Plot.splitData=Scene_plot_splitData
	woo.core.Plot.reverseData=Scene_plot_reverseData
	woo.core.Scene.autoPlotData=Scene_autoPlotData
	woo.core.Plot.addData=Scene_plot_addData
	woo.core.Plot.addImgData=Scene_plot_addImgData
	woo.core.Plot.saveGnuplot=Scene_plot_saveGnuplot
	woo.core.Plot.saveDataTxt=Scene_plot_saveDataTxt
	woo.core.Plot.plot=Scene_plot_plot

defMonkeyMethods()
