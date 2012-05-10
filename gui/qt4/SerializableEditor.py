# encoding: utf-8
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4 import QtGui
#from PyQt4 import Qwt5

from miniEigen import *
# don't import * from yade, it would be circular import
from yade.wrapper import AttrFlags, Serializable

import re,itertools
import logging
logging.trace=logging.debug
logging.basicConfig(level=logging.INFO)
#from logging import debug,info,warning,error
#from yade import *
import yade._customConverters, yade.core
import yade.qt

seqSerializableShowType=True # show type headings in serializable sequences (takes vertical space, but makes the type hyperlinked)

# BUG: cursor is moved to the beginnign of the input field even if it has focus
#
# checking for focus seems to return True always and cursor is never moved
#
# the 'True or' part effectively disables the condition (so that the cursor is moved always), but it might be fixed in the future somehow
#
# if True or w.hasFocus(): w.home(False)
#
#

def makeWrapperHref(text,className,attr=None,static=False):
	"""Create clickable HTML hyperlink to a Yade class or its attribute.
	
	:param className: name of the class to link to.
	:param attr: attribute to link to. If given, must exist directly in given *className*; if not given or empty, link to the class itself is created and *attr* is ignored.
	:return: HTML with the hyperref.
	"""
	if not static: return '<a href="%s#yade.wrapper.%s%s">%s</a>'%(yade.qt.sphinxDocWrapperPage,className,(('.'+attr) if attr else ''),text)
	else:          return '<a href="%s#ystaticattr-%s.%s">%s</a>'%(yade.qt.sphinxDocWrapperPage,className,attr,text)

def serializableHref(ser,attr=None,text=None):
	"""Return HTML href to a *ser* optionally to the attribute *attr*.
	The class hierarchy is crawled upwards to find out in which parent class is *attr* defined,
	so that the href target is a valid link. In that case, only single inheritace is assumed and
	the first class from the top defining *attr* is used.

	:param ser: object of class deriving from :yref:`Serializable`, or string; if string, *attr* must be empty.
	:param attr: name of the attribute to link to; if empty, linke to the class itself is created.
	:param text: visible text of the hyperlink; if not given, either class name or attribute name without class name (when *attr* is not given) is used.

	:returns: HTML with the hyperref.
	"""
	# klass is a class name given as string
	if isinstance(ser,str):
		if attr: raise InvalidArgument("When *ser* is a string, *attr* must be empty (only class link can be created)")
		return makeWrapperHref(text if text else ser,ser)
	# klass is a type object
	if attr:
		klass=ser.__class__
		while attr in dir(klass.__bases__[0]): klass=klass.__bases__[0]
		if not text: text=attr
	else:
		klass=ser.__class__
		if not text: text=klass.__name__
	return makeWrapperHref(text,klass.__name__,attr,static=(attr and getattr(klass,attr,None)==getattr(ser,attr)))

# HACK: extend the QLineEdit class
# set text but preserve cursor position
def QLineEdit_setTextStable(self,text):
	c=self.cursorPosition(); self.setText(text); self.setCursorPosition(c)
def QSpinBox_setValueStable(self,value):
	c=self.lineEdit().cursorPosition(); self.setValue(value); self.lineEdit().setCursorPosition(c)
QLineEdit.setTextStable=QLineEdit_setTextStable
QSpinBox.setValueStable=QSpinBox_setValueStable

class AttrEditor():
	"""Abstract base class handing some aspects common to all attribute editors.
	Holds exacly one attribute which is updated whenever it changes."""
	def __init__(self,getter=None,setter=None):
		self.getter,self.setter=getter,setter
		self.hot,self.focused=False,False
		self.widget=None
	def refresh(self): pass
	def update(self): pass
	def isHot(self,hot=True):
		"Called when the widget gets focus; mark it hot, change colors etc."
		if hot==self.hot: return
		self.hot=hot
		if hot: self.setStyleSheet('QWidget { background: red }')
		else: self.setStyleSheet('QWidget { background: none }')
	def sizeHint(self): return QSize(150,12)
	def trySetter(self,val):
		try: self.setter(val)
		except AttributeError: self.setEnabled(False)
		self.isHot(False)

class AttrEditor_Bool(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.checkBox=QCheckBox(self)
		lay=QVBoxLayout(self); lay.setSpacing(0); lay.setMargin(0); lay.addStretch(1); lay.addWidget(self.checkBox); lay.addStretch(1)
		self.checkBox.clicked.connect(self.update)
	def refresh(self): self.checkBox.setChecked(self.getter())
	def update(self): self.trySetter(self.checkBox.isChecked())

class AttrEditor_Int(AttrEditor,QSpinBox):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QSpinBox.__init__(self,parent)
		self.setRange(int(-1e9),int(1e9)); self.setSingleStep(1);
		self.valueChanged.connect(self.update)
	def refresh(self):
		self.setValueStable(self.getter())
	def update(self):  self.trySetter(self.value())

class AttrEditor_Str(AttrEditor,QLineEdit):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QLineEdit.__init__(self,parent)
		self.textEdited.connect(self.isHot)
		self.selectionChanged.connect(self.isHot)
		self.editingFinished.connect(self.update)
	def refresh(self): self.setTextStable(self.getter())
	def update(self):  self.trySetter(str(self.text()))

class AttrEditor_Float(AttrEditor,QLineEdit):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QLineEdit.__init__(self,parent)
		self.textEdited.connect(self.isHot)
		self.selectionChanged.connect(self.isHot)
		self.editingFinished.connect(self.update)
	def refresh(self):
		self.setTextStable(str(self.getter()))
		#if True or not self.hasFocus(): self.home(False)
	def update(self):
		try: self.trySetter(float(self.text()))
		except ValueError: self.refresh()

class AttrEditor_Quaternion(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.grid=QHBoxLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		for i in range(4):
			if i==3:
				f=QFrame(self); f.setFrameShape(QFrame.VLine); f.setFrameShadow(QFrame.Sunken); f.setFixedWidth(4) # add vertical divider (axis | angle)
				self.grid.addWidget(f)
			w=QLineEdit('')
			self.grid.addWidget(w);
			w.textEdited.connect(self.isHot)
			w.selectionChanged.connect(self.isHot)
			w.editingFinished.connect(self.update)
	def refresh(self):
		val=self.getter(); axis,angle=val.toAxisAngle()
		for i in (0,1,2,4):
			w=self.grid.itemAt(i).widget(); w.setTextStable(str(axis[i] if i<3 else angle));
			#if True or not w.hasFocus(): w.home(False)
	def update(self):
		try:
			x=[float((self.grid.itemAt(i).widget().text())) for i in (0,1,2,4)]
		except ValueError: self.refresh()
		q=Quaternion(Vector3(x[0],x[1],x[2]),x[3]); q.normalize() # from axis-angle
		self.trySetter(q) 
	def setFocus(self): self.grid.itemAt(0).widget().setFocus()


class AttrEditor_IntRange(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter): 
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.grid=QGridLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		curr,(mn,mx)=getter()
		self.slider,self.spin=QSlider(self),QSpinBox(self)
		self.grid.addWidget(self.spin,0,0)
		self.grid.addWidget(self.slider,0,1,1,2) # rowSpan=1,columnSpan=2
		self.slider.setMinimum(mn); self.slider.setMaximum(mx)
		self.spin.setMinimum(mn); self.spin.setMaximum(mx)
		self.slider.setOrientation(Qt.Horizontal)
		self.spin.valueChanged.connect(self.updateFromSpin)
		self.slider.sliderMoved.connect(self.sliderMoved)
		self.slider.sliderReleased.connect(self.updateFromSlider)
	def refresh(self):
		curr,(mn,mx)=self.getter()
		c=self.spin.lineEdit().cursorPosition()
		self.spin.setValueStable(curr); self.spin.setMinimum(mn); self.spin.setMaximum(mx)
		self.spin.lineEdit().setCursorPosition(c)
		self.slider.setValue(curr)
	def updateFromSpin(self):
		self.slider.setValue(self.spin.value())
		self.trySetter(self.slider.value())
	def updateFromSlider(self):
		self.spin.setValue(self.slider.value())
		self.trySetter(self.slider.value())
	def sliderMoved(self,val):
		self.isHot(True)
		self.spin.setValue(val) # self.slider.value())
	def setFocus(self): self.slider.setFocus()


class AttrEditor_FloatRange(AttrEditor,QFrame):
	sliDiv=500
	def __init__(self,parent,getter,setter): 
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		curr,(mn,mx)=getter()
		self.slider,self.edit=QSlider(self),QLineEdit(str(curr))
		self.grid=QHBoxLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		self.grid.addWidget(self.edit); self.grid.addWidget(self.slider)
		self.grid.setStretchFactor(self.edit,1); self.grid.setStretchFactor(self.slider,2)
		self.slider.setMinimum(0); self.slider.setMaximum(self.sliDiv)
		self.slider.setOrientation(Qt.Horizontal)
		self.edit.textEdited.connect(self.isHot)
		self.edit.selectionChanged.connect(self.isHot)
		self.edit.editingFinished.connect(self.updateFromText)
		self.slider.sliderMoved.connect(self.sliderMoved) #
		self.slider.sliderReleased.connect(self.sliderReleased) #
	def refresh(self):
		curr,(mn,mx)=self.getter()
		self.edit.setTextStable('%g'%curr)
		self.slider.setValue(int(self.sliDiv*((1.*curr-mn)/(1.*mx-mn))))
	def updateFromText(self):
		curr,(mn,mx)=self.getter()
		try:
			value=float(self.edit.text())
			self.slider.setValue(int(self.sliDiv*((1.*value-mn)/(1.*mx-mn))))
			self.trySetter(value)
		except ValueError: self.refresh()
	def sliderPosToNum(self,sliderValue):
		curr,(mn,mx)=self.getter()
		return mn+sliderValue*(1./self.sliDiv)*(mx-mn)
	def sliderMoved(self,newSliderPos):
		self.isHot(True)
		self.edit.setText('%g'%self.sliderPosToNum(newSliderPos))
	def sliderReleased(self):
		value=self.sliderPosToNum(self.slider.value())
		self.edit.setText('%g'%value)
		self.trySetter(value)
	def setFocus(self): self.slider.setFocus()


class AttrEditor_Se3(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.grid=QGridLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		for row,col in itertools.product(range(2),range(5)): # one additional column for vertical line in quaternion
			if (row,col)==(0,3): continue
			if (row,col)==(0,4): self.grid.addWidget(QLabel(u'←<i>pos</i> ↙<i>ori</i>',self),row,col); continue
			if (row,col)==(1,3):
				f=QFrame(self); f.setFrameShape(QFrame.VLine); f.setFrameShadow(QFrame.Sunken); f.setFixedWidth(4); self.grid.addWidget(f,row,col); continue
			w=QLineEdit('')
			self.grid.addWidget(w,row,col);
			w.textEdited.connect(self.isHot)
			w.selectionChanged.connect(self.isHot)
			w.editingFinished.connect(self.update)
	def refresh(self):
		pos,ori=self.getter(); axis,angle=ori.toAxisAngle()
		for i in (0,1,2,4):
			w=self.grid.itemAtPosition(1,i).widget(); w.setTextStable(str(axis[i] if i<3 else angle));
			#if True or not w.hasFocus(): w.home(False)
		for i in (0,1,2):
			w=self.grid.itemAtPosition(0,i).widget(); w.setTextStable(str(pos[i]));
			#if True or not w.hasFocus(): w.home(False)
	def update(self):
		try:
			q=[float((self.grid.itemAtPosition(1,i).widget().text())) for i in (0,1,2,4)]
			v=[float((self.grid.itemAtPosition(0,i).widget().text())) for i in (0,1,2)]
		except ValueError: self.refresh()
		qq=Quaternion(Vector3(q[0],q[1],q[2]),q[3]); qq.normalize() # from axis-angle
		self.trySetter((v,qq)) 
	def setFocus(self): self.grid.itemAtPosition(0,0).widget().setFocus()

class AttrEditor_flagArray(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter,labels):
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.labels=labels
		self.dim=len(labels),max([len(l) for l in labels])
		self.grid=QGridLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		for r in range(len(self.labels)):
			for c in range(len(self.labels[r])):
				if self.labels[r][c]==None: continue
				w=QCheckBox()
				w.setText(self.labels[r][c])
				self.grid.addWidget(w,r,c)
				w.clicked.connect(self.update)
	def refresh(self):
		val=self.getter()
		#print self.getter, val
		for r in range(len(val)):
			for c in range(len(val[r])):
				if val[r][c]==None: continue
				self.grid.itemAtPosition(r,c).widget().setChecked(val[r][c])
	def update(self):
		ret=[]
		for r in range(len(self.labels)):
			ret.append([])
			for c in range(len(self.labels[r])):
				ret[r].append(None if self.labels[r][c]==None else self.grid.itemAtPosition(r,c).widget().isChecked())
		self.setter(ret)
	def setFocus(self): self.grid.itemAtPosition(0,0).widget().setFocus()

# FIXME: broken
class AttrEditor_DemData_flags(AttrEditor_flagArray):
	# getter: gets from cxx and returns as array of bools
	# setter: receives bools from widgets, sets cxx value
	def boolSetter(self,bb): # grab bools from widgets, call self.setter with bools given as string
		assert(len(bb)==1 and len(bb[0])==6)
		ss=''.join(['xyzXYZ'[i] for i in range(6) if bb[0][i]])
		self.realSetter(ss)
	def boolGetter(self):
		ss=self.realGetter() #
		return [[('xyzXYZ'[i] in ss) for i in range(6)]]
	def __init__(self,parent,getter,setter):
		AttrEditor_flagArray.__init__(self,parent,getter=self.boolGetter,setter=self.boolSetter,labels=[['x','y','z','X','Y','Z']])
		self.realSetter=setter
		self.realGetter=getter


class AttrEditor_MatrixX(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter,rows,cols,idxConverter):
		'idxConverter converts row,col tuple to either (row,col), (col) etc depending on what access is used for []'
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.rows,self.cols=rows,cols
		self.idxConverter=idxConverter
		self.setContentsMargins(0,0,0,0)
		val=self.getter()
		self.grid=QGridLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		for row,col in itertools.product(range(self.rows),range(self.cols)):
			w=QLineEdit('')
			self.grid.addWidget(w,row,col);
			w.textEdited.connect(self.isHot)
			w.selectionChanged.connect(self.isHot)
			w.editingFinished.connect(self.update)
	def refresh(self):
		val=self.getter()
		#if (hasattr(val,'cols') and self.cols!=val.cols()) or (hasattr(val,'rows') and self.rows!=val.rows()):
		#	# matrix size changed, reinitialize the widget completely
		#	for row,col in itertools.product(range(self.rows),range(self.cols)):
		#		w=self.grid.itemAtPosition(row,col).widget()
		#		self.grid.removeWidget(w)
		#		w.setParent(None)
		#	self.grid.setParent(None)
		#	self.__init__(self.parent,self.getter,self.setter,self.rows,self.cols,self.idxConverter)
		for row,col in itertools.product(range(self.rows),range(self.cols)):
			w=self.grid.itemAtPosition(row,col).widget()
			w.setTextStable(str(val[self.idxConverter(row,col)]))
			#if True or not w.hasFocus: w.home(False) # make the left-most part visible, if the text is wider than the widget
	def update(self):
		try:
			val=self.getter()
			for row,col in itertools.product(range(self.rows),range(self.cols)):
				w=self.grid.itemAtPosition(row,col).widget()
				if w.isModified(): val[self.idxConverter(row,col)]=float(w.text())
			logging.debug('setting'+str(val))
			self.trySetter(val)
		except ValueError: self.refresh()
	def setFocus(self): self.grid.itemAtPosition(0,0).widget().setFocus()

class AttrEditor_MatrixXi(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter,rows,cols,idxConverter):
		'idxConverter converts row,col tuple to either (row,col), (col) etc depending on what access is used for []'
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.rows,self.cols=rows,cols
		self.idxConverter=idxConverter
		self.setContentsMargins(0,0,0,0)
		self.grid=QGridLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		for row,col in itertools.product(range(self.rows),range(self.cols)):
			w=QSpinBox()
			w.setRange(int(-1e9),int(1e9)); w.setSingleStep(1);
			self.grid.addWidget(w,row,col);
		self.refresh() # refresh before connecting signals!
		for row,col in itertools.product(range(self.rows),range(self.cols)):
			self.grid.itemAtPosition(row,col).widget().valueChanged.connect(self.update)
	def refresh(self):
		val=self.getter()
		for row,col in itertools.product(range(self.rows),range(self.cols)):
			w=self.grid.itemAtPosition(row,col).widget().setValueStable(val[self.idxConverter(row,col)])
	def update(self):
		val=self.getter(); modified=False
		for row,col in itertools.product(range(self.rows),range(self.cols)):
			w=self.grid.itemAtPosition(row,col).widget()
			if w.value()!=val[self.idxConverter(row,col)]:
				modified=True; val[self.idxConverter(row,col)]=w.value()
		if not modified: return
		logging.debug('setting'+str(val))
		self.trySetter(val)
	def setFocus(self): self.grid.itemAtPosition(0,0).widget().setFocus()

class AttrEditor_Vector6i(AttrEditor_MatrixXi):
	def __init__(self,parent,getter,setter):
		AttrEditor_MatrixXi.__init__(self,parent,getter,setter,1,6,lambda r,c:c)
class AttrEditor_Vector3i(AttrEditor_MatrixXi):
	def __init__(self,parent,getter,setter):
		AttrEditor_MatrixXi.__init__(self,parent,getter,setter,1,3,lambda r,c:c)
class AttrEditor_Vector2i(AttrEditor_MatrixXi):
	def __init__(self,parent,getter,setter):
		AttrEditor_MatrixXi.__init__(self,parent,getter,setter,1,2,lambda r,c:c)

class AttrEditor_VectorX(AttrEditor_MatrixX):
	def __init__(self,parent,getter,setter):
		val=getter()
		AttrEditor_MatrixX.__init__(self,parent,getter,setter,1,len(val),lambda r,c:c)
class AttrEditor_Vector6(AttrEditor_MatrixX):
	def __init__(self,parent,getter,setter):
		AttrEditor_MatrixX.__init__(self,parent,getter,setter,1,6,lambda r,c:c)
class AttrEditor_Vector3(AttrEditor_MatrixX):
	def __init__(self,parent,getter,setter):
		AttrEditor_MatrixX.__init__(self,parent,getter,setter,1,3,lambda r,c:c)
class AttrEditor_Vector2(AttrEditor_MatrixX):
	def __init__(self,parent,getter,setter):
		AttrEditor_MatrixX.__init__(self,parent,getter,setter,1,2,lambda r,c:c)
class AttrEditor_Matrix3(AttrEditor_MatrixX):
	def __init__(self,parent,getter,setter):
		AttrEditor_MatrixX.__init__(self,parent,getter,setter,3,3,lambda r,c:(r,c))
class AttrEditor_MatrixXX(AttrEditor_MatrixX):
	def __init__(self,parent,getter,setter):
		val=getter()
		AttrEditor_MatrixX.__init__(self,parent,getter,setter,val.rows(),val.cols(),lambda r,c:(r,c))
class AttrEditor_AlignedBox3(AttrEditor_MatrixX):
	def __init__(self,parent,getter,setter):
		AttrEditor_MatrixX.__init__(self,parent,getter,setter,2,3,lambda r,c:(r,c))
class AttrEditor_AlignedBox2(AttrEditor_MatrixX):
	def __init__(self,parent,getter,setter):
		AttrEditor_MatrixX.__init__(self,parent,getter,setter,2,2,lambda r,c:(r,c))

class Se3FakeType: pass

_fundamentalEditorMap={bool:AttrEditor_Bool,str:AttrEditor_Str,int:AttrEditor_Int,float:AttrEditor_Float,Quaternion:AttrEditor_Quaternion,Vector2:AttrEditor_Vector2,Vector3:AttrEditor_Vector3,Vector6:AttrEditor_Vector6,Matrix3:AttrEditor_Matrix3,Vector6i:AttrEditor_Vector6i,Vector3i:AttrEditor_Vector3i,Vector2i:AttrEditor_Vector2i,MatrixX:AttrEditor_MatrixXX,VectorX:AttrEditor_VectorX,Se3FakeType:AttrEditor_Se3,AlignedBox3:AttrEditor_AlignedBox3,AlignedBox2:AttrEditor_AlignedBox2}
_fundamentalInitValues={bool:True,str:'',int:0,float:0.0,Quaternion:Quaternion.Identity,Vector3:Vector3.Zero,Matrix3:Matrix3.Zero,Vector6:Vector6.Zero,Vector6i:Vector6i.Zero,Vector3i:Vector3i.Zero,Vector2i:Vector2i.Zero,Vector2:Vector2.Zero,Se3FakeType:(Vector3.Zero,Quaternion.Identity),AlignedBox3:(Vector3.Zero,Vector3.Zero),AlignedBox2:(Vector2.Zero,Vector2.Zero),MatrixX:MatrixX(),VectorX:VectorX()}

_fundamentalSpecialEditors={
	# FIXME: re-eanble once fixed
	#id(yade.dem.DemData.flags):AttrEditor_DemData_flags,
}

_attributeGuessedTypeMap={yade._customConverters.NodeList:(yade.core.Node,), }

class SerQLabel(QLabel):
	def __init__(self,parent,label,tooltip,path,elide=False):
		QLabel.__init__(self,parent)
		self.path=path
		self.setTextToolTip(label,tooltip,elide=elide)
		self.linkActivated.connect(yade.qt.openUrl)
	def setTextToolTip(self,label,tooltip,elide=False):
		if elide: label=self.fontMetrics().elidedText(label,Qt.ElideRight,2*self.width())
		self.setText(label)
		if tooltip or self.path: self.setToolTip(('<b>'+self.path+'</b><br>' if self.path else '')+(tooltip if tooltip else ''))
		else: self.setToolTip(None)
	def mousePressEvent(self,event):
		if event.button()!=Qt.MidButton:
			event.ignore(); return
		if self.path==None: return # no path set
		# middle button clicked, paste pasteText to clipboard
		cb=QApplication.clipboard()
		cb.setText(self.path,mode=QClipboard.Clipboard)
		cb.setText(self.path,mode=QClipboard.Selection) # X11 global selection buffer
		event.accept()

class SerializableEditor(QFrame):
	"Class displaying and modifying serializable attributes of a yade object."
	import collections
	import logging
	# each attribute has one entry associated with itself
	class EntryData:
		def __init__(self,name,T,doc,groupNo,trait,containingClass):
			self.name,self.T,self.doc,self.trait,self.groupNo,self.containingClass=name,T,doc,trait,groupNo,containingClass
			self.widget=None
			self.widgets={'label':None,'value':None}
		def propertyId(self):
			try:
				return id(getattr(self.containingClass,self.name))
			except AttributeError: return None
	class EntryGroupData:
		def __init__(self,name): self.name=name
	def __init__(self,ser,parent=None,ignoredAttrs=set(),showType=False,path=None,attrVar=True):
		"Construct window, *ser* is the object we want to show."
		QtGui.QFrame.__init__(self,parent)
		self.ser=ser
		self.path=('yade.'+ser.label if (hasattr(ser,'label') and ser.label) else path)
		self.showType=showType
		self.attrVar=attrVar # show variable name; if false, docstring is used instead
		self.hot=False
		self.entries=[]
		self.entryGroups=[]
		self.ignoredAttrs=ignoredAttrs
		logging.debug('New Serializable of type %s'%ser.__class__.__name__)
		self.setWindowTitle(str(ser))
		self.mkWidgets()
		self.refreshTimer=QTimer(self)
		self.refreshTimer.timeout.connect(self.refreshEvent)
		self.refreshTimer.start(500)
	def getListTypeFromDocstring(self,attr,cxxType=None):
		"Guess type of array by scanning docstring for :yattrtype: and parsing its argument; ugly, but works."
		if not cxxType:
			doc=getattr(self.ser.__class__,attr).__doc__
			if doc==None:
				logging.error("Attribute %s has no docstring."%attr)
				return None
			m=re.search(r':yattrtype:`([^`]*)`',doc)
			if not m:
				logging.error("Attribute %s does not contain :yattrtype:`....` (docstring is '%s'"%(attr,doc))
				return None
			cxxT=m.group(1)
			logging.debug('Got type "%s" from :yattrtype:'%cxxT)
		else: cxxT=cxxType
		def vecTest(T,cxxT):
			regexp=r'^\s*(std\s*::)?\s*vector\s*<\s*(shared_ptr\s*<\s*)?\s*(std\s*::)?\s*('+T+r')(\s*>)?\s*>\s*$'
			m=re.match(regexp,cxxT)
			return m
		def vecGuess(T):
			regexp=r'^\s*(std\s*::)?\s*vector\s*<\s*(shared_ptr\s*<\s*)?\s*(std\s*::)?\s*(?P<elemT>[a-zA-Z_][a-zA-Z0-9_]+)(\s*>)?\s*>\s*$'
			m=re.match(regexp,T)
			return m
		from yade import dem
		from yade import gl
		from yade import core
		vecMap={
			'bool':bool,'int':int,'long':int,'Body::id_t':long,'size_t':long,
			'Real':float,'float':float,'double':float,
			'Vector6r':Vector6,'Vector6i':Vector6i,'Vector3i':Vector3i,'Vector2r':Vector2,'Vector2i':Vector2i,
			'Vector3r':Vector3,'Matrix3r':Matrix3,'Se3r':Se3FakeType,'Quaternionr':Quaternion,
			'string':str
		}
		for T,ret in vecMap.items():
			if vecTest(T,cxxT):
				logging.debug("Got type %s from cxx type %s"%(repr(ret),cxxT))
				return (ret,)
		#print 'No luck with ',T
		m=vecGuess(cxxT)
		if m:
			#print 'guessed literal type',m.group('elemT')
			elemT=m.group('elemT')
			for mod in gl,core,dem:
				#print dir(mod)
				if elemT in dir(mod) and type(mod.__dict__[elemT]).__name__=='class':
					#print 'found type %s.%s for %s'%(mod.__name__,elemT,cxxT)
					return (mod.__dict__[elemT],) # return tuple to signify sequence
		logging.error("Unable to guess python type from cxx type '%s'"%cxxT)
		return None
	def mkAttrEntries(self):
		if self.ser==None: return
		#try:
		#	d=self.ser.dict()
		#except TypeError:
		#	logging.error('TypeError when getting attributes of '+str(self.ser)+',skipping. ')
		#	import traceback
		#	traceback.print_exc()
		#attrs=self.ser.yattrs(); # do not sort here, since we need separators

		# crawl class hierarchy up, ask each one for attribute traits
		attrTraits=[]; k=self.ser.__class__
		while k!=yade.wrapper.Serializable:
			attrTraits=k._attrTraits+attrTraits
			k=k.__bases__[0]
		
		for trait in attrTraits:
			attr=trait.name
			val=getattr(self.ser,attr) # get the value using serattt, as it might be different from what the dictionary provides (e.g. Body.blockedDOFs)
			t=None
			isStatic=(getattr(self.ser.__class__,attr,None)==getattr(self.ser,attr))
			if isStatic: doc=self.getStaticAttrDocstring(attr,raw=True)
			else:
				try:
					doc=trait.doc
					#doc=getattr(self.ser.__class__,attr).__doc__
				except AttributeError:
					print 'No docstring for ',self.ser.__class__.__name__+'.'+attr+": using None (pure python attribute?)"
					doc=None
			if attr in self.ignoredAttrs: continue

			# group sparator, handle specially
			m=re.match('_groupSeparator_(.*)',attr)
			if m:
				self.entryGroups.append(self.EntryGroupData(name=self.getDocstring(attr)))
				continue

			if isinstance(val,list):
				t=self.getListTypeFromDocstring(attr,cxxType=trait.cxxType)
				if not t and len(val)==0: t=(val[0].__class__,) # 1-tuple is list of the contained type
				#if not t: raise RuntimeError('Unable to guess type of '+str(self.ser)+'.'+attr)
			# hack for Se3, which is returned as (Vector3,Quaternion) in python
			elif isinstance(val,tuple) and len(val)==2 and val[0].__class__==Vector3 and val[1].__class__==Quaternion: t=Se3FakeType
			else:
				t=val.__class__
				if t in _attributeGuessedTypeMap: t=_attributeGuessedTypeMap[val.__class__]

			if len(self.entryGroups)==0: self.entryGroups.append(self.EntryGroupData(name=None))
			groupNo=len(self.entryGroups)-1

			#if not match: print 'No attr match for docstring of %s.%s'%(self.ser.__class__.__name__,attr)

			#logging.debug('Attr %s is of type %s'%(attr,((t[0].__name__,) if isinstance(t,tuple) else t.__name__)))
			self.entries.append(self.EntryData(name=attr,T=t,groupNo=groupNo,doc=self.getDocstring(attr),trait=trait,containingClass=self.ser.__class__))
	def getDocstring(self,attr=None):
		"If attr is *None*, return docstring of the Serializable itself"
		try:
			doc=(getattr(self.ser.__class__,attr).__doc__ if attr else self.ser.__class__.__doc__)
		except AttributeError: doc=None
		if not doc: return ''
		doc=re.sub(':y(attrtype|default|attrflags):`[^`]*`','',doc)
		statAttr=re.compile('^.. ystaticattr::.*$',re.MULTILINE|re.DOTALL)
		doc=re.sub(statAttr,'',doc) # static classes have their proper docs at the beginning, discard static memeber docs
		# static: attribute of the type is the same object as attribute of the instance
		# in that case, get docstring from the class documentation by parsing it
		if attr and getattr(self.ser.__class__,attr)==getattr(self.ser,attr): doc=self.getStaticAttrDocstring(attr)
		doc=re.sub(':yref:`([^`]*)`','\\1',doc)
		import textwrap
		wrapper=textwrap.TextWrapper(replace_whitespace=False)
		return wrapper.fill(textwrap.dedent(doc))
	def getStaticAttrDocstring(self,attr,raw=False):
		ret=''; c=self.ser.__class__
		while hasattr(c,attr) and hasattr(c.__base__,attr): c=c.__base__
		doc=c.__doc__
		head='.. ystaticattr:: %s.%s('%(c.__name__,attr)
		start=doc.find(head)
		#print 'Found',attr,'at',start,'in docstring',doc
		if start<0: return '[no documentation found]'
		end=doc.find('.. ystaticattr:: ',start+len(head)) # start at the end of last match; returns -1 if not found
		#print 'Attribute',attr,'returning:\n',doc[start+len(head)-1:end]
		doc=doc[start+len(head):end]
		if raw: return doc
		meta=re.compile('^.*:yattrflags:`\s*[0-9]+\s*`',re.MULTILINE|re.DOTALL)
		doc=re.sub(meta,'',doc).strip()
		return doc
		

	def handleFloatRange(self,widgetKlass,getter,setter,entry):
		rangeSuffix='_range'
		rangeAttr=entry.name+rangeSuffix
		# return editor for given attribute; no-op, unless float with associated range attribute
		if entry.T==float and hasattr(self.ser,rangeAttr) and getattr(self.ser,rangeAttr).__class__==Vector2:
			# getter returns tuple value,range
			# setter needs just the value itself
			return AttrEditor_FloatRange,lambda: (getattr(self.ser,entry.name),getattr(self.ser,rangeAttr)),lambda x: setattr(self.ser,entry.name,x)
		elif entry.T==int and hasattr(self.ser,rangeAttr) and getattr(self.ser,rangeAttr).__class__==Vector2i:
			return AttrEditor_IntRange,lambda: (getattr(self.ser,entry.name),getattr(self.ser,rangeAttr)),lambda x: setattr(self.ser,entry.name,x)
		else: return widgetKlass,getter,setter
		
	def mkWidget(self,entry):
		if not entry.T: return None
		# single fundamental object
		Klass=_fundamentalEditorMap.get(entry.T,None) if entry.propertyId() not in _fundamentalSpecialEditors else _fundamentalSpecialEditors[entry.propertyId()]
		getter,setter=lambda: getattr(self.ser,entry.name), lambda x: setattr(self.ser,entry.name,x)
		Klass,getter,setter=self.handleFloatRange(Klass,getter,setter,entry)
		if Klass:
			widget=Klass(self,getter=getter,setter=setter)
			widget.setFocusPolicy(Qt.StrongFocus)
			if entry.trait.readonly: widget.setEnabled(False)
			return widget
		# sequences
		if entry.T.__class__==tuple:
			assert(len(entry.T)==1) # we don't handle tuples of other lenghts
			# sequence of serializables
			T=entry.T[0]
			if (issubclass(T,Serializable) or T==Serializable):
				widget=SeqSerializable(self,getter,setter,T,path=(self.path+'.'+entry.name if self.path else None),shrink=True)
				return widget
			if (T in _fundamentalEditorMap):
				widget=SeqFundamentalEditor(self,getter,setter,T)
				return widget
			return None
		# a serializable
		if issubclass(entry.T,Serializable) or entry.T==Serializable:
			obj=getattr(self.ser,entry.name)
			if hasattr(obj,'label') and obj.label: path=obj.label
			elif self.path: path=self.path+'.'+entry.name
			else: path=None
			widget=SerializableEditor(getattr(self.ser,entry.name),parent=self,showType=self.showType,path=(self.path+'.'+entry.name if self.path else None),attrVar=self.attrVar)
			widget.setFrameShape(QFrame.Box); widget.setFrameShadow(QFrame.Raised); widget.setLineWidth(1)
			return widget
		return None
	def serQLabelMenu(self,widget,position):
		menu=QMenu(self)
		toggleAttrVar=menu.addAction("Show "+("descriptions" if self.attrVar else "variables"))
		toggleAttrVar.triggered.connect(self.toggleAttrVar)
		menu.popup(self.mapToGlobal(position))
		#print 'menu popped up at ',widget.mapToGlobal(position),' (local',position,')'
	def getAttrLabelToolTip(self,entry):
		if self.attrVar: return serializableHref(self.ser,entry.name),entry.doc
		return entry.doc, entry.name
	def toggleAttrVar(self):
		self.attrVar=not self.attrVar
		for entry in self.entries:
			if not entry.widgets['label']: continue
			entry.widgets['label'].setTextToolTip(*self.getAttrLabelToolTip(entry),elide=not self.attrVar)

	def mkWidgets(self):
		self.mkAttrEntries()
		onlyDefaultGroups=(len(self.entryGroups)==1 and self.entryGroups[0].name==None)
		formLayouts=[]
		if self.showType: # create type label
			lab=SerQLabel(self,makeSerializableLabel(self.ser,addr=True,href=True),tooltip=self.getDocstring(),path=self.path)
			lab.setFrameShape(QFrame.Box); lab.setFrameShadow(QFrame.Sunken); lab.setLineWidth(2); lab.setAlignment(Qt.AlignHCenter); lab.linkActivated.connect(yade.qt.openUrl)
			## attach context menu to the label
			lab.setContextMenuPolicy(Qt.CustomContextMenu)
			lab.customContextMenuRequested.connect(lambda pos: self.serQLabelMenu(lab,pos))
		if onlyDefaultGroups:
			lay=QFormLayout(self)
			lay.setContentsMargins(2,2,2,2)
			lay.setVerticalSpacing(0)
			lay.setLabelAlignment(Qt.AlignRight)
			if self.showType: lay.setWidget(0,QFormLayout.SpanningRole,lab)
			formLayouts=[lay]
			self.allLayouts=formLayouts
		else:
			# make one grid, in which optionally the label and the toolbox lives
			# the toolbox has one tab for each group; each tab contains the formlayout, where will the attribute widgets live
			# this warns when called after re-layout
			lay=QGridLayout(self)
			lay.setContentsMargins(2,2,2,2)
			lay.setVerticalSpacing(0)
			if self.showType: lay.addWidget(lab)
			#lay.setVerticalSpacing(0)
			tbx=QToolBox()
			#style=QtGui.QCommonStyle()
			#rightArrow=style.standardIcon(QtGui.QStyle.SP_ArrowRight)
			for group in self.entryGroups:
				#g=QGridLayout(); g.setContentsMargins(0,0,0,0); g.setVerticalSpacing(0)
				form=QFormLayout()
				frame=QFrame()
				frame.setLayout(form)
				form.setContentsMargins(2,2,2,2)
				form.setVerticalSpacing(0)
				form.setLabelAlignment(Qt.AlignRight)
				tbx.addItem(frame,u'▶ '+(group.name if group.name else ''))
				formLayouts.append(form)
			tbx.setStyleSheet('QToolBox::tab { font: bold; }')
			lay.addWidget(tbx)
			#if attrVar:
			for f in formLayouts: f.setRowWrapPolicy(QFormLayout.WrapLongRows)
			self.allLayouts=formLayouts+[lay]
		for entry in self.entries:
			if entry.trait.noGui: continue
			entry.widget=self.mkWidget(entry)
			entry.widgets['value']=entry.widget # for code compat
			objPath=(self.path+'.'+entry.name) if self.path else None
			labelText,labelTooltip=self.getAttrLabelToolTip(entry)
			label=SerQLabel(self,labelText,tooltip=labelTooltip,path=objPath,elide=not self.attrVar)
			entry.widgets['label']=label
			if not self.attrVar:
				label.setWordWrap(True)
				label.setAlignment(Qt.AlignRight|Qt.AlignVCenter)
				#label.setSizePolicy(QSizePolicy(QSizePolicy.MinimumExpanding,QSizePolicy.Preferred));
				label.sizePolicy().setHeightForWidth(True)
				#label.sizePolicy().setHorizontalStretch(1000)
				label.setMinimumWidth(150)
				#label.setScaledContents(True)
			try:
				formLayouts[entry.groupNo].addRow(label,entry.widget if entry.widget else (QLabel('<i>None</i>' if getattr(self.ser,entry.name)==None else QLabel('<i>unhandled type</i>'))))
			except RuntimeError:
				print 'ERROR while creating widget for entry %s (%s)'%(entry.name,objPath)
				import traceback
				traceback.print_exc()
		if onlyDefaultGroups: self.setLayout(lay)
		self.refreshEvent()
	def refreshEvent(self):
		for e in self.entries:
			if e.widget and not e.widget.hot: e.widget.refresh()
	def refresh(self): pass

def makeSerializableLabel(ser,href=False,addr=True,boldHref=True,num=-1,count=-1):
	ret=u''
	if num>=0:
		if count>=0: ret+=u'%d/%d. '%(num,count)
		else: ret+=u'%d. '%num
	if href: ret+=(u' <b>' if boldHref else u' ')+serializableHref(ser)+(u'</b> ' if boldHref else u' ')
	else: ret+=ser.__class__.__name__+' '
	if hasattr(ser,'label') and ser.label: ret+=u' “'+unicode(ser.label)+u'”'
	# do not show address if there is a label already
	elif addr and ser!=None:
		import re
		ss=unicode(ser); m=re.match(u'<(.*) instance at (0x.*)>',ss)
		if m: ret+=m.group(2)
		else: logging.warning(u"Serializable converted to str ('%s') does not contain 'instance at 0x…'"%ss)
	return ret

class SeqSerializableComboBox(QFrame):
	def __init__(self,parent,getter,setter,serType,path=None,shrink=False):
		QFrame.__init__(self,parent)
		self.getter,self.setter,self.serType,self.path,self.shrink=getter,setter,serType,path,shrink
		self.layout=QVBoxLayout(self)
		topLineFrame=QFrame(self)
		topLineLayout=QHBoxLayout(topLineFrame);
		for l in self.layout, topLineLayout: l.setSpacing(0); l.setContentsMargins(0,0,0,0)
		topLineFrame.setLayout(topLineLayout)
		buttons=(self.newButton,self.killButton,self.upButton,self.downButton)=[QPushButton(label,self) for label in (u'☘',u'☠',u'↑',u'↓')]
		buttonSlots=(self.newSlot,self.killSlot,self.upSlot,self.downSlot) # same order as buttons
		for b in buttons: b.setStyleSheet('QPushButton { font-size: 15pt; }'); b.setFixedWidth(30); b.setFixedHeight(30)
		self.combo=QComboBox(self)
		self.combo.setSizeAdjustPolicy(QComboBox.AdjustToContents)
		for w in buttons[0:2]+[self.combo,]+buttons[2:4]: topLineLayout.addWidget(w)
		self.layout.addWidget(topLineFrame) # nested layout
		self.scroll=QScrollArea(self); self.scroll.setWidgetResizable(True)
		self.layout.addWidget(self.scroll)
		self.seqEdit=None # currently edited serializable
		self.setLayout(self.layout)
		self.hot=None # API compat with SerializableEditor
		self.setFrameShape(QFrame.Box); self.setFrameShadow(QFrame.Raised); self.setLineWidth(1)
		self.newDialog=None # is set when new dialog is created, and destroyed when it returns
		# signals
		for b,slot in zip(buttons,buttonSlots): b.clicked.connect(slot)
		self.combo.currentIndexChanged.connect(self.comboIndexSlot)
		self.refreshEvent()
		# periodic refresh
		self.refreshTimer=QTimer(self)
		self.refreshTimer.timeout.connect(self.refreshEvent)
		self.refreshTimer.start(1000) # 1s should be enough
		#print 'SeqSerializable path is',self.path
	def comboIndexSlot(self,ix): # different seq item selected
		currSeq=self.getter();
		if len(currSeq)==0: ix=-1
		logging.debug('%s comboIndexSlot len=%d, ix=%d'%(self.serType.__name__,len(currSeq),ix))
		self.downButton.setEnabled(ix<len(currSeq)-1)
		self.upButton.setEnabled(ix>0)
		self.combo.setEnabled(ix>=0)
		if ix>=0:
			ser=currSeq[ix]
			self.seqEdit=SerializableEditor(ser,parent=self,showType=seqSerializableShowType,path=(self.path+'['+str(ix)+']') if self.path else None)
			self.scroll.setWidget(self.seqEdit)
			if self.shrink:
				self.sizeHint=lambda: QSize(100,1000)
				self.scroll.sizeHint=lambda: QSize(100,1000)
				self.sizePolicy().setVerticalPolicy(QSizePolicy.Expanding)
				self.scroll.sizePolicy().setVerticalPolicy(QSizePolicy.Expanding)
				self.setMinimumHeight(min(300,self.seqEdit.height()+self.combo.height()+10))
				self.setMaximumHeight(100000)
				self.scroll.setMaximumHeight(100000)
		else:
			self.scroll.setWidget(QFrame())
			if self.shrink:
				self.setMaximumHeight(self.combo.height()+10);
				self.scroll.setMaximumHeight(0)
	def serLabel(self,ser,i=-1):
		return ('' if i<0 else str(i)+'. ')+str(ser)[1:-1].replace('instance at ','')
	def refreshEvent(self,forceIx=-1):
		currSeq=self.getter()
		comboEnabled=self.combo.isEnabled()
		if comboEnabled and len(currSeq)==0: self.comboIndexSlot(-1) # force refresh, otherwise would not happen from the initially empty state
		ix,cnt=self.combo.currentIndex(),self.combo.count()
		# serializable currently being edited (which can be absent) or the one of which index is forced
		ser=(self.seqEdit.ser if self.seqEdit else None) if forceIx<0 else currSeq[forceIx] 
		if comboEnabled and len(currSeq)==cnt and (ix<0 or ser==currSeq[ix]): return
		if not comboEnabled and len(currSeq)==0: return
		logging.debug(self.serType.__name__+' rebuilding list from scratch')
		self.combo.clear()
		if len(currSeq)>0:
			prevIx=-1
			for i,s in enumerate(currSeq):
				self.combo.addItem(makeSerializableLabel(s,num=i,count=len(currSeq),addr=False))
				if s==ser: prevIx=i
			if forceIx>=0: newIx=forceIx # force the index (used from newSlot to make the new element active)
			elif prevIx>=0: newIx=prevIx # if found what was active before, use it
			elif ix>=0: newIx=ix         # otherwise use the previous index (e.g. after deletion)
			else: newIx=0                  # fallback to 0
			logging.debug('%s setting index %d'%(self.serType.__name__,newIx))
			self.combo.setCurrentIndex(newIx)
		else:
			logging.debug('%s EMPTY, setting index 0'%(self.serType.__name__))
			self.combo.setCurrentIndex(-1)
		self.killButton.setEnabled(len(currSeq)>0)
	def newSlot(self):
		print 'newSlot called'
		if self.newDialog:
			raise RuntimeError("newSlot called, but there is already a dialogue?")
		self.newDialog=NewSerializableDialog(self,self.serType.__name__)
		self.newDialog.show()
		self.newDialog.accepted.connect(self.newInsertSlot)
		self.newDialog.rejected.connect(self.newCancelledSlot)
		#raise RuntimeError("newSlot does not work due to dialogs getting closed immediately")
		if 0: # old code which does not work due to exec_ returning immediately (used to work?!)
			if not dialog.exec_():
				print 'NewSerializableDialog cancelled'
				return # cancelled
			ser=dialog.result()
			ix=self.combo.currentIndex()
			currSeq=self.getter(); currSeq.insert(ix,ser); self.setter(currSeq)
			logging.debug('%s new item created at index %d'%(self.serType.__name__,ix))
			self.refreshEvent(forceIx=ix)
	def newCancelledSlot(self):
		self.newDialog=None # this must be tracked properly if cancelled
	def newInsertSlot(self):
		ser=self.newDialog.result()
		self.newDialog=None
		ix=self.combo.currentIndex()
		currSeq=self.getter(); currSeq.insert(ix,ser); self.setter(currSeq)
		logging.debug('%s new item created at index %d'%(self.serType.__name__,ix))
		self.refreshEvent(forceIx=ix)
	def killSlot(self):
		ix=self.combo.currentIndex()
		currSeq=self.getter(); del currSeq[ix]; self.setter(currSeq)
		self.refreshEvent()
	def upSlot(self):
		i=self.combo.currentIndex()
		assert(i>0)
		currSeq=self.getter();
		prev,curr=currSeq[i-1:i+1]; currSeq[i-1],currSeq[i]=curr,prev; self.setter(currSeq)
		self.refreshEvent(forceIx=i-1)
	def downSlot(self):
		i=self.combo.currentIndex()
		currSeq=self.getter(); assert(i<len(currSeq)-1);
		curr,nxt=currSeq[i:i+2]; currSeq[i],currSeq[i+1]=nxt,curr; self.setter(currSeq)
		self.refreshEvent(forceIx=i+1)
	def refresh(self): pass # API compat with SerializableEditor

SeqSerializable=SeqSerializableComboBox


class NewFundamentalDialog(QDialog):
	def __init__(self,parent,attrName,typeObj,typeStr):
		QDialog.__init__(self,parent)
		self.setWindowTitle('%s (type %s)'%(attrName,typeStr))
		self.layout=QVBoxLayout(self)
		self.scroll=QScrollArea(self)
		self.scroll.setWidgetResizable(True)
		self.buttons=QDialogButtonBox(QDialogButtonBox.Ok|QDialogButtonBox.Cancel);
		self.buttons.accepted.connect(self.accept)
		self.buttons.rejected.connect(self.reject)
		self.layout.addWidget(self.scroll)
		self.layout.addWidget(self.buttons)
		self.setWindowModality(Qt.WindowModal)
		class FakeObjClass: pass
		self.fakeObj=FakeObjClass()
		self.attrName=attrName
		Klass=_fundamentalEditorMap.get(typeObj,None)
		initValue=_fundamentalInitValues.get(typeObj,typeObj())
		setattr(self.fakeObj,attrName,initValue)
		if Klass:
			self.widget=Klass(None,self.fakeObj,attrName)
			self.scroll.setWidget(self.widget)
			self.scroll.show()
			self.widget.refresh()
		else: raise RuntimeError("Unable to construct new dialog for type %s"%(typeStr))
	def result(self):
		self.widget.update()
		return getattr(self.fakeObj,self.attrName)

class NewSerializableDialog(QDialog):
	def __init__(self,parent,baseClassName,includeBase=True):
		import yade.system
		QDialog.__init__(self,parent)
		self.setWindowTitle('Create new object of type %s'%baseClassName)
		self.layout=QVBoxLayout(self)
		self.combo=QComboBox(self)
		childs=list(yade.system.childClasses(baseClassName,includeBase=False)); childs.sort()
		if includeBase:
			self.combo.addItem(baseClassName)
			self.combo.insertSeparator(1000)
		self.combo.addItems(childs)
		self.combo.currentIndexChanged.connect(self.comboSlot)
		self.scroll=QScrollArea(self)
		self.scroll.setWidgetResizable(True)
		self.buttons=QDialogButtonBox(QDialogButtonBox.Ok|QDialogButtonBox.Cancel);
		self.buttons.accepted.connect(self.accept)
		self.buttons.rejected.connect(self.reject)
		self.layout.addWidget(self.combo)
		self.layout.addWidget(self.scroll)
		self.layout.addWidget(self.buttons)
		self.ser=None
		self.combo.setCurrentIndex(0); self.comboSlot(0)
		self.setWindowModality(Qt.WindowModal)
	def comboSlot(self,index):
		item=str(self.combo.itemText(index))
		from yade import core,dem,gl,qt
		self.ser=eval(item+'()',dict(sum([m.__dict__.items() for m in core,dem,gl,qt],[])))
		self.scroll.setWidget(SerializableEditor(self.ser,self.scroll,showType=True))
		self.scroll.show()
	def result(self): return self.ser
	def sizeHint(self): return QSize(180,400)

class SeqFundamentalEditor(QFrame):
	def __init__(self,parent,getter,setter,itemType):
		QFrame.__init__(self,parent)
		self.getter,self.setter,self.itemType=getter,setter,itemType
		self.layout=QVBoxLayout()
		topLineFrame=QFrame(self); topLineLayout=QHBoxLayout(topLineFrame)
		self.form=QFormLayout()
		self.form.setContentsMargins(0,0,0,0)
		self.form.setVerticalSpacing(0)
		self.form.setLabelAlignment(Qt.AlignLeft)
		self.formFrame=QFrame(self); self.formFrame.setLayout(self.form)
		self.layout.addWidget(self.formFrame)
		self.setLayout(self.layout)
		# SerializableEditor API compat
		self.hot=False
		self.rebuild()
		# periodic refresh
		self.refreshTimer=QTimer(self)
		self.refreshTimer.timeout.connect(self.refreshEvent)
		self.refreshTimer.start(1000) # 1s should be enough
	def contextMenuEvent(self, event):
		index=self.localPositionToIndex(event.pos())
		seq=self.getter()
		if len(seq)==0: index=-1
		field=self.form.itemAt(index,QFormLayout.LabelRole).widget() if index>=0 else None
		menu=QMenu(self)
		actNew,actKill,actUp,actDown=[menu.addAction(name) for name in (u'☘ New',u'☠ Remove',u'↑ Up',u'↓ Down')]
		if index<0: [a.setEnabled(False) for a in actKill,actUp,actDown]
		if index==len(seq)-1: actDown.setEnabled(False)
		if index==0: actUp.setEnabled(False)
		# disable until we figure out how to cancel when the no item in the menu is chosen
		#if field: field.setStyleSheet('QWidget { background: green }')
		if 1:
			menu.popup(self.mapToGlobal(event.pos()))
			actNew.triggered.connect(lambda: self.newSlot(index))
			actKill.triggered.connect(lambda: self.killSlot(index))
			actUp.triggered.connect(lambda: self.upSlot(index))
			actDown.triggered.connect(lambda: self.downSlot(index))
			# this does not work...?!
			#menu.destroyed.connect(lambda: field.setStyleSheet('QWidget { background : none }'))
		else: # this is the old code which returns immediately; don't use it anymore
			act=menu.exec_(self.mapToGlobal(event.pos()))
			if field: field.setStyleSheet('QWidget { background: none }')
			if not act:
				return
			if act==actNew: self.newSlot(aindex)
			elif act==actKill: self.killSlot(index)
			elif act==actUp: self.upSlot(index)
			elif act==actDown: self.downSlot(index)
	def localPositionToIndex(self,pos):
		gp=self.mapToGlobal(pos)
		for row in range(self.form.count()/2):
			w,i=self.form.itemAt(row,QFormLayout.FieldRole),self.form.itemAt(row,QFormLayout.LabelRole)
			for wi in w.widget(),i.widget():
				x0,y0,x1,y1=wi.geometry().getCoords(); globG=QRect(self.mapToGlobal(QPoint(x0,y0)),self.mapToGlobal(QPoint(x1,y1)))
				if globG.contains(gp):
					return row
		return -1
	def newSlot(self,i):
		seq=self.getter();
		seq.insert(i,_fundamentalInitValues.get(self.itemType,self.itemType()))
		self.setter(seq)
		self.rebuild()
	def killSlot(self,i):
		seq=self.getter(); assert(i<len(seq)); del seq[i]; self.setter(seq)
		self.refreshEvent()
	def upSlot(self,i):
		seq=self.getter(); assert(i<len(seq));
		prev,curr=seq[i-1:i+1];
		seq[i-1],seq[i]=curr,prev;
		self.setter(seq)
		self.refreshEvent(forceIx=i-1)
	def downSlot(self,i):
		seq=self.getter(); assert(i<len(seq)-1);
		curr,nxt=seq[i:i+2]; seq[i],seq[i+1]=nxt,curr; self.setter(seq)
		self.refreshEvent(forceIx=i+1)
	def rebuild(self):
		currSeq=self.getter()
		# clear everything
		rows=self.form.count()/2
		for row in range(rows):
			logging.trace('counts',self.form.rowCount(),self.form.count())
			for wi in self.form.itemAt(row,QFormLayout.FieldRole),self.form.itemAt(row,QFormLayout.LabelRole):
				self.form.removeItem(wi)
				logging.trace('deleting widget',wi.widget())
				widget=wi.widget(); widget.hide(); del widget # for some reason, deleting does not make the thing disappear visually; hiding does, however
			logging.trace('counts after ',self.form.rowCount(),self.form.count())
		logging.debug('cleared')
		# add everything
		Klass=_fundamentalEditorMap.get(self.itemType,None)
		if not Klass:
			errMsg=QTextEdit(self)
			errMsg.setReadOnly(True); errMsg.setText("Sorry, editing sequences of %s's is not (yet?) implemented."%(self.itemType.__name__))
			self.form.insertRow(0,'<b>Error</b>',errMsg)
			return
		class ItemGetter():
			def __init__(self,getter,index): self.getter,self.index=getter,index
			def __call__(self): return self.getter()[self.index]
		class ItemSetter():
			def __init__(self,getter,setter,index): self.getter,self.setter,self.index=getter,setter,index
			def __call__(self,val): seq=self.getter(); seq[self.index]=val; self.setter(seq)
		for i,item in enumerate(currSeq):
			widget=Klass(self,ItemGetter(self.getter,i),ItemSetter(self.getter,self.setter,i)) #proxy,'value')
			self.form.insertRow(i,'%d. '%i,widget)
			logging.debug('added item %d %s'%(i,str(widget)))
		if len(currSeq)==0: self.form.insertRow(0,'<i>empty</i>',QLabel('<i>(right-click for menu)</i>'))
		logging.debug('rebuilt, will refresh now')
		self.refreshEvent(dontRebuild=True) # avoid infinite recursion it the length would change meanwhile
	def refreshEvent(self,dontRebuild=False,forceIx=-1):
		currSeq=self.getter()
		if len(currSeq)!=self.form.count()/2: #rowCount():
			if dontRebuild: return # length changed behind our back, just pretend nothing happened and update next time instead
			self.rebuild()
			currSeq=self.getter()
		for i in range(len(currSeq)):
			item=self.form.itemAt(i,QFormLayout.FieldRole)
			logging.trace('got item #%d %s'%(i,str(item.widget())))
			widget=item.widget()
			if not widget.hot:
				widget.refresh()
			if forceIx>=0 and forceIx==i: widget.setFocus()
	def refresh(self): pass # SerializableEditor API




