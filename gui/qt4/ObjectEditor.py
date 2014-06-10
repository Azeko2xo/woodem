# encoding: utf-8
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4 import QtGui
#from PyQt4 import Qwt5

from minieigen import *
# don't import * from woo, it would be circular import
from woo.core import Object

import re,itertools,math
import logging
logging.trace=logging.debug
logging.basicConfig(level=logging.INFO)
#from logging import debug,info,warning,error
#from woo import *
import woo._customConverters, woo.core, woo.utils
import woo.qt
import woo.document
import sys
import os.path, os


from ExceptionDialog import *

seqObjectShowType=True # show type headings in serializable sequences (takes vertical space, but makes the type hyperlinked)

# BUG: cursor is moved to the beginnign of the input field even if it has focus
#
# checking for focus seems to return True always and cursor is never moved
#
# the 'True or' part effectively disables the condition (so that the cursor is moved always), but it might be fixed in the future somehow
#
# if True or w.hasFocus(): w.home(False)
#
#

colormapIconSize=(50,20)

def makeColormapIcons():
	ret=[]
	for cmap in range(len(woo.master.cmaps)):
		wd,ht=colormapIconSize
		img=QtGui.QImage(wd,ht,QtGui.QImage.Format_RGB32)
		for col in range(wd):
			c=255*woo.utils.mapColor(col*1./wd,cmap=cmap)
			#c=(0,0,0)
			cc=QtGui.qRgb(int(c[0]),int(c[1]),int(c[2]))
			for row in range(ht):
				img.setPixel(col,row,cc)
		ret.append(QtGui.QIcon(QtGui.QPixmap.fromImage(img)))
	return ret

# construct colormap icons, for later use
# use this proxy function so that makeColormapIcons is not called at import time
# since that leads to crash (qt is not fully initialized yet)
_colormapIcons=[]
def getColormapIcons():
	global _colormapIcons
	if not _colormapIcons: _colormapIcons=makeColormapIcons()
	return _colormapIcons


class WidgetUpdatesDisabled():
	'Context manager/decorator for disabling updates of qt4 widgets temporarily'
	def __init__(self,widget): self.widget=widget
	def __enter__(self): self.widget.setUpdatesEnabled(False)
	def __exit__(self,eType,eValue,eTrace): self.widget.setUpdatesEnabled(True)



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
		self.multiplier=None
		self.widget=None
		self.readonly=False
	def refresh(self): pass
	def update(self): pass
	def isHot(self,hot=True):
		"Called when the widget gets focus; mark it hot, change colors etc."
		if hot==self.hot: return
		self.hot=hot
		if hot: self.setStyleSheet('QWidget { background: red }')
		else: self.setStyleSheet('QWidget { background: none }')
	def sizeHint(self): return QSize(100,12)
	def trySetter(self,val):
		if not self.readonly:
			try: self.setter(val)
			except AttributeError: self.setEnabled(False)
		self.isHot(False)
	def multiplierChanged(self,convSpec):
		raise RuntimeError("This widget has no multiplierChanged method defined.")

class AttrEditor_Bool(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.checkBox=QCheckBox(self)
		lay=QVBoxLayout(self); lay.setSpacing(0); lay.setMargin(0); lay.addStretch(1); lay.addWidget(self.checkBox); lay.addStretch(1)
		self.checkBox.clicked.connect(self.update)
	def refresh(self):
		assert(not self.multiplier)
		self.checkBox.setChecked(self.getter())
	def update(self): self.trySetter(self.checkBox.isChecked())

class AttrEditor_Int(AttrEditor,QSpinBox):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QSpinBox.__init__(self,parent)
		self.setRange(int(-1e9),int(1e9)); self.setSingleStep(1);
		self.valueChanged.connect(self.update)
	def refresh(self):
		assert(not self.multiplier)
		self.setValueStable(self.getter())
	def update(self):  self.trySetter(self.value())

class AttrEditor_Str(AttrEditor,QLineEdit):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QLineEdit.__init__(self,parent)
		self.textEdited.connect(self.isHot)
		self.selectionChanged.connect(self.isHot)
		self.editingFinished.connect(self.update)
	def refresh(self):
		assert(not self.multiplier)
		self.setTextStable(self.getter())
	def update(self):  self.trySetter(str(self.text()))

class AttrEditor_Float(AttrEditor,QLineEdit):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QLineEdit.__init__(self,parent)
		self.textEdited.connect(self.isHot)
		self.selectionChanged.connect(self.isHot)
		self.editingFinished.connect(self.update)
	def refresh(self):
		v=self.getter()
		if self.multiplier: v*=self.multiplier
		self.setTextStable(str(v))
	def update(self):
		# when everything is deleted, don't refresh because of float('') raising ValueError
		if self.hot and not self.text(): return
		try:
			v=float(self.text())
			if self.multiplier: v/=self.multiplier
			self.trySetter(v)
		except ValueError: self.refresh()
	def multiplierChanged(self,convSpec):
		if isinstance(self.multiplier,tuple): raise RuntimeError("Float cannot have multiple units.")
		if self.multiplier: self.setToolTip(u'Converting %s (× %g)'%(convSpec,self.multiplier))
		else: self.setToolTip('')
		self.refresh()

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
		assert(not self.multiplier)
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
		self.slider.setFocusPolicy(Qt.ClickFocus)
		self.spin.valueChanged.connect(self.updateFromSpin)
		self.slider.sliderMoved.connect(self.sliderMoved)
		self.slider.sliderReleased.connect(self.updateFromSlider)
	def refresh(self):
		assert(not self.multiplier)
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
		curr,(mn,mx)=self.multipliedGetter()
		self.slider,self.edit=QSlider(self),QLineEdit(str(curr))
		self.grid=QHBoxLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		self.grid.addWidget(self.edit); self.grid.addWidget(self.slider)
		self.grid.setStretchFactor(self.edit,1); self.grid.setStretchFactor(self.slider,2)
		self.slider.setMinimum(0); self.slider.setMaximum(self.sliDiv)
		self.slider.setOrientation(Qt.Horizontal)
		self.slider.setFocusPolicy(Qt.ClickFocus)
		self.edit.textEdited.connect(self.isHot)
		self.edit.selectionChanged.connect(self.isHot)
		self.edit.editingFinished.connect(self.updateFromText)
		self.slider.sliderMoved.connect(self.sliderMoved) #
		self.slider.sliderReleased.connect(self.sliderReleased) #
	def multipliedGetter(self):
		curr,(mn,mx)=self.getter()
		if self.multiplier:
			curr*=self.multiplier; mn*=self.multiplier; mx*=self.multiplier
		return curr,(mn,mx)
	def refresh(self):
		curr,(mn,mx)=self.multipliedGetter()
		self.edit.setTextStable('%g'%curr)
		if not math.isnan(curr):
			self.slider.setValue(int(self.sliDiv*((1.*curr-mn)/(1.*mx-mn))))
			self.slider.setEnabled(True)
		else: self.slider.setEnabled(False)
	def updateFromText(self):
		curr,(mn,mx)=self.multipliedGetter()
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
		if self.multiplier: value/=self.multiplier
		self.trySetter(value)
	def setFocus(self): self.slider.setFocus()
	def multiplierChanged(self,convSpec):
		if isinstance(self.multiplier,tuple): raise RuntimeError("Float range cannot have multiple units.")
		logging.debug("New multiplier is "+str(self.multiplier))
		if self.multiplier: self.setToolTip("Unit-conversion %s: factor %g"%(convSpec,self.multiplier))
		else: self.setToolTip('')
		self.refresh()
		#pass # this is OK

class AttrEditor_Choice(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		curr,choices=getter()
		self.choices=choices
		if len(choices)<1: raise RuntimeError("There are 0 choices for this attribute?")
		self.combo=QComboBox(self)
		# if choices are single items, then we choose from those values
		# otherwise the first item is code value and second is the displayed value
		self.justValues=(choices[0].__class__!=tuple)
		self.admitValues=[(c if self.justValues else c[0]) for c in choices]
		if choices[0].__class__==tuple and len(choices[0])!=2: raise ValueError("Choice must be either single items or 2-tuples of code-value,display-value")
		for c in choices:
			self.combo.addItem(str(c if self.justValues else c[1])) 
		### hack for COLORMAPS!!, only based on length of the choice list
		nCmaps=len(woo.master.cmaps)
		if len(choices) in (nCmaps,nCmaps+1): # +1 for [default] (-1)
			if len(choices)==nCmaps+1:
				for i in range(len(choices)): self.combo.setItemIcon(i,getColormapIcons()[(i-1) if i>0 else woo.master.cmap[0]])
			else:
				for i in range(len(choices)): self.combo.setItemIcon(i,getColormapIcons()[i])
			self.combo.setIconSize(QSize(colormapIconSize[0],colormapIconSize[1]))
		self.grid=QGridLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		self.grid.addWidget(self.combo,0,0)
		self.combo.activated.connect(self.update)
	def refresh(self):
		curr,choices=self.getter()
		if curr not in self.admitValues:
			raise ValueError("Choice attribute value "+str(curr)+" is not within admitted values "+str(self.admitValues))
		#	self.combo.setEnabled(False)
		#else:
		#	if not self.combo.enabled(): self.combo.setEnabled(True)
		if self.admitValues[self.combo.currentIndex()]!=curr: # update the combo
			self.combo.setCurrentIndex(self.admitValues.index(curr))
	def update(self):
		choice=self.choices[self.combo.currentIndex()]
		val=choice if self.justValues else choice[0]
		self.trySetter(val)
	def setFocus(self): self.combo.setFocus()
	def multiplierChanged(self,convSpec):
		if isinstance(self.multiplier,tuple): raise RuntimeError("Float choice cannot have multiple units.")
		else: self.setToolTip()
		for i,c in enumerate(self.choices):
			val=c if self.justValues else c[1]
			if c.__class__!=float: raise RuntimeError("Only float choices can meaningfully support unit multipliers")
			if self.multiplier: val*=self.multiplier
			self.combo.setItemText(i,str(val))
		self.refresh()

class AttrEditor_Bits(AttrEditor,QFrame):
	checkersPerRow=3
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		curr,self.bits=getter()
		if len(self.bits)<1: raise RuntimeError("There are 0 bits for this attribute?")
		self.checkers=[]
		self.value=0 # current value of checkboxes
		self.grid=QGridLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		for i,b in enumerate(self.bits):
			w=QCheckBox(self)
			w.setText(b)
			w.clicked.connect(self.update)
			self.checkers.append(w)
			self.grid.addWidget(w,i//self.checkersPerRow,i%self.checkersPerRow)
	def refresh(self):
		curr,bits=self.getter()
		if curr==self.value: return
		if curr>=2**len(self.checkers): raise RuntimeError("Value %d exceeds maximum value %d (=2^%d-1) representable by the UI bitfield"%(curr,2**len(self.checkers)-1,len(self.checkers))) 
		self.value=curr
		for bit,w in enumerate(self.checkers):
			b=curr&(1<<bit)
			if b!=w.isChecked(): w.setChecked(b)
	def update(self):
		val=0
		for bit,w in enumerate(self.checkers):
			if w.isChecked(): val|=(1<<bit)
		self.trySetter(val)
	def setFocus(self): self.checkers[0].setFocus()

class AttrEditor_RgbColor(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter):
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.grid=QGridLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		self.butt=QPushButton()
		self.butt.clicked.connect(self.dialogShow)
		self.rgbWidgets=[]
		self.grid.addWidget(self.butt,0,0)
		self.dialog=None
		for i in (0,1,2):
			w=QLineEdit('')
			self.grid.addWidget(w,0,i+1)
			w.textEdited.connect(self.isHot)
			w.selectionChanged.connect(self.isHot)
			w.editingFinished.connect(self.update)
			self.rgbWidgets.append(w)
	def to256c(self,f):
		return min(255,max(0,int(f*256))) if not math.isnan(f) else 0
	def to256(self,v): return (self.to256c(v[0]),self.to256c(v[1]),self.to256c(v[2]))
	def to256str(self,v,sep=','): return sep.join([str(self.to256c(v[0])),str(self.to256c(v[1])),str(self.to256c(v[2]))])
	def dialogShow(self):
		rgb=self.getter()
		self.dialog=QColorDialog(self)
		self.dialog.setCurrentColor(QColor(*self.to256(rgb)))
		self.dialog.setOptions(QColorDialog.NoButtons)
		self.dialog.currentColorChanged.connect(self.dialogChanged)
		self.dialog.show()
	def dialogChanged(self):
		rgba=self.dialog.currentColor().getRgb()
		self.setter(Vector3(*rgba[0:3])/256)
		self.refresh()
	def refresh(self):
		rgb=self.getter()
		for i in (0,1,2):
			self.rgbWidgets[i].setTextStable(str(rgb[i]))
		self.butt.setStyleSheet('QPushButton { background-color: rgb(%s) }'%(self.to256str(rgb)))
	def update(self):
		try:
			rgb=Vector3([float(self.rgbWidgets[i].text()) for i in (0,1,2)])
		except ValueError: self.refresh()
		self.trySetter(rgb)

class AttrEditor_FileDir(AttrEditor,QFrame):
	def __init__(self,parent,getter,setter,isDir,isExisting):
		AttrEditor.__init__(self,getter,setter)
		QFrame.__init__(self,parent)
		self.isDir,self.isExisting=isDir,isExisting
		self.grid=QGridLayout(self); self.grid.setSpacing(0); self.grid.setMargin(0)
		w=self.nameEdit=QLineEdit('')
		self.grid.addWidget(w,0,0)
		w.textEdited.connect(self.isHot)
		w.selectionChanged.connect(self.isHot)
		w.editingFinished.connect(self.update)
		b=self.butt=QPushButton()
		style=QApplication.style()
		if isDir: b.setIcon(style.standardIcon(QStyle.SP_DirIcon))
		else: b.setIcon(style.standardIcon(QStyle.SP_FileIcon))
		self.butt.clicked.connect(self.dialogShow)
		self.grid.addWidget(self.butt,0,2)
		rel=self.rel=QPushButton()
		rel.setCheckable(True)
		rel.setText('rel')
		rel.setToolTip(u'Toggle absolute/relative path.\nPaths are relative to the current directory,\nwhich is now %s.'%(unicode(os.getcwd())))
		rel.toggled.connect(self.relToggled)
		rel.setStyleSheet('padding: 0px;')
		self.grid.addWidget(rel,0,1)
		
	def dialogShow(self):
		curr=self.nameEdit.text()
		if self.isDir:	f=QFileDialog.getExistingDirectory(self,'Select directory',curr)
		elif self.isExisting: f=QFileDialog.getOpenFileName(self,'Select existing file',curr)
		else: f=QFileDialog.getSaveFileName(self,'Select file name',curr,options=QFileDialog.DontConfirmOverwrite)
		if not f: return # cancelled
		f=str(f)
		if self.rel.isChecked(): f=os.path.relpath(f)
		self.setter(f)
	def refresh(self):
		f=self.getter()
		self.nameEdit.setTextStable(f)
		self.rel.setChecked(not os.path.isabs(f))
	def update(self):
		f=str(self.nameEdit.text())
		self.trySetter(f)
		self.rel.setChecked(not os.path.isabs(f))
	def relToggled(self,isRel):
		f=str(self.nameEdit.text())
		if f=='': return # do nothing for empty path
		if isRel: self.trySetter(os.path.relpath(f))
		else: self.trySetter(os.path.abspath(f))
		
	

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
			v=val[self.idxConverter(row,col)]
			mult=(self.multiplier[col] if isinstance(self.multiplier,tuple) else self.multiplier)
			if mult!=None: v*=mult
			w.setTextStable(str(v))
			#if True or not w.hasFocus: w.home(False) # make the left-most part visible, if the text is wider than the widget
	def update(self):
		try:
			val=self.getter()
			for row,col in itertools.product(range(self.rows),range(self.cols)):
				w=self.grid.itemAtPosition(row,col).widget()
				if w.isModified():
					v=float(w.text())
					mult=(self.multiplier[col] if isinstance(self.multiplier,tuple) else self.multiplier)
					if mult: v/=mult
					val[self.idxConverter(row,col)]=v
			logging.debug('setting'+str(val))
			self.trySetter(val)
		except ValueError: self.refresh()
	def setFocus(self): self.grid.itemAtPosition(0,0).widget().setFocus()
	def multiplierChanged(self,convSpec):
		if self.multiplier: self.setToolTip(convSpec)
		else: self.setToolTip('')
		logging.debug("Multiplier changed to "+str(self.multiplier))
		self.refresh()

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

_attributeGuessedTypeMap={woo._customConverters.NodeList:(woo.core.Node,),woo._customConverters.ObjectList:(woo.core.Object,) }

def hasActiveLabel(s):
	if not hasattr(s,'label') or not  s.label: return False
	for t in s._getAllTraits():
		if t.activeLabel==True: return True
	return False

class SerQLabel(QLabel):
	def __init__(self,parent,label,tooltip,path,ser=None,elide=False):
		QLabel.__init__(self,parent)
		self.path=path
		self.ser=ser
		self.minWd=-1
		self.setTextToolTip(label,tooltip,elide=elide)
		self.linkActivated.connect(woo.qt.openUrl)
	#def sizeHint(self): return QSize(self.minWd,20)
	#def minimumSizeHint(self): return QSize(self.minWd,20)
	def setTextToolTip(self,label,tooltip,elide=False):
		# ignore UnicodeDecodeError (appears sometimes under Windows), perhaps there are non-ascii characters in docstrings somewhere?
		try:
			if elide:
				# label is the text description; elide it at some fixed width
				#self.minWd=100
				label=self.fontMetrics().elidedText(label,Qt.ElideRight,1.5*self.width())
			else:
				pass
				#self.minWd=60
			self.setText(label)
			#self.adjustSize()
			if tooltip or self.path: self.setToolTip(('<b>'+self.path+'</b><br>' if self.path else '')+(tooltip if tooltip else ''))
			else: self.setToolTip(None)
		except UnicodeDecodeError:
			self.setToolTip(None)
	def mousePressEvent(self,event):
		if event.button()!=Qt.MidButton:
			event.ignore(); return
		if self.ser and (event.modifiers() & Qt.AltModifier or event.modifiers() & Qt.ControlModifier):
			# open this object in new window
			se=ObjectEditor(self.ser,parent=None,showType=True,labelIsVar=True,showChecks=False,showUnits=False,objManip=True)
			se.show()
			return
		if self.path==None: return # no path set
		# middle button clicked, paste pasteText to clipboard
		cb=QApplication.clipboard()
		cb.setText(self.path,mode=QClipboard.Clipboard)
		cb.setText(self.path,mode=QClipboard.Selection) # X11 global selection buffer
		event.accept()

def _unicodeUnit(u): return (u if isinstance(u,unicode) else unicode(u,'utf-8'))

def makeLibraryBrowser(parentmenu,callback,types,libName='Library'):
	'''Create menu under *parentmenu* named *libName*. *types* are passed to :obj:`woo.objectlibrary.checkout`. *callback* is called with chosen (name,object) as arguments.'''
	import woo.objectlibrary
	# root=parentmenu.addMenu('libName')
	objs=woo.objectlibrary.checkout(types=types)
	menus={():parentmenu.addMenu(libName)}
	if objs:
		for key,val in objs.items():
			# create submenus
			for i in range(1,len(key)):
				if key[:i] not in menus:
					menus[key[:i]]=menus[key[:i-1]].addMenu(key[i-1])
			# create menu item
			item=menus[key[:-1]].addAction(key[-1])
			item.triggered.connect(lambda checked, name=key,obj=val: callback(name,obj))
	else: menus[()].setEnabled(False) # disable library menu if there are no objects in there




class ObjectEditor(QFrame):
	"Class displaying and modifying attributes of a woo object, or of unrelated attributes of different objects."
	import collections
	import logging
	# each attribute has one entry associated with itself

	# maximum nesting level in the UI, to avoid cycles and freezes
	maxNest=6
	# maximum length of inline lists
	maxListLen=40 


	class EntryData:
		def __init__(self,obj,name,T,doc,groupNo,trait,containingClass,editor,label):
			self.obj,self.name,self.T,self.doc,self.trait,self.groupNo,self.containingClass,self.label=obj,name,T,doc,trait,groupNo,containingClass,label
			self.widget=None
			self.visible=True
			self.hidden=False # this overrides the "visible" attribute
			self.gridAndRow=(None,-1)
			self.widgets={} #{'label':None,'value':None,'unit':None}
			self.editor=editor
		def propertyId(self):
			try:
				return id(getattr(self.containingClass,self.name))
			except AttributeError: return None
		def unitChanged(self,ix0=-1,forceBaseUnit=False):
			if not self.trait.unit or len(self.trait.altUnits[0])==0: return
			c=self.unitLayout
			if not self.trait.multiUnit:
				#print self.name,self.containingClass.__name__,self.trait.unit
				ix=c.itemAt(0).widget().currentIndex() if not forceBaseUnit else 0
				w=self.widgets['value']
				if ix==0: # base unit:
					w.multiplier=None
					w.multiplierChanged('')
				else:
					w.multiplier=self.trait.altUnits[0][ix-1][1]
					# print 20*'!',self.trait.name,self.editor
					w.multiplierChanged(u'%s × %g = %s'%(_unicodeUnit(self.trait.altUnits[0][ix-1][0]) if ix>0 else '',w.multiplier,_unicodeUnit(self.trait.unit[0])))
			else: # multiplier is a tuple applied to each column separately
				mult,msg=[],[]
				for i in range(len(self.trait.unit)):
					if self.trait.altUnits[i]: # not empty, there is a combo box
						ix=c.itemAt(i).widget().currentIndex() if not forceBaseUnit else 0
						mult.append(None if ix==0 else self.trait.altUnits[i][ix-1][1])
						if mult[-1]!=None: msg.append(u'%s × %g = %s'%(_unicodeUnit(self.trait.altUnits[i][ix-1][0]) if ix>0 else '-',mult[-1],_unicodeUnit(self.trait.unit[i])))
						else: msg.append('')
				w=self.widgets['value']
				w.multiplier=tuple(mult)
				w.multiplierChanged('<br>'.join(msg))
		def toggleChecked(self,checked):
			self.widgets['value'].setEnabled(not checked)
			self.widgets['label'].setEnabled(not checked)
		def eval_hideIf(self):
			return self.trait.hideIf and eval(self.trait.hideIf,globals(),{'self':self.editor.ser})
		def setVisible(self,visible=None):
			if visible==None: visible=self.visible
			self.visible=visible
			# if visible, the entry can be nevertheless hidden due to hideIf
			self.hidden=self.eval_hideIf()
			if not self.visible or self.hidden:
				for w in self.widgets.values():
					if w: w.hide()
			else:
				for w in self.widgets.values():
					if 'unit' in self.widgets and w==self.widgets['unit']: w.setVisible(self.editor.showUnits)
					elif 'check' in self.widgets and w==self.widgets['check']: w.setVisible(self.editor.showChecks)
					else: w.show()
			
			
	class EntryGroupData:
		def __init__(self,number,name):
			self.number=number
			self.name=name
			self.expander=None
			self.entries=[]
			style=QtGui.QCommonStyle()
			self.rightArrow=style.standardIcon(QtGui.QStyle.SP_ArrowRight)
			self.downArrow =style.standardIcon(QtGui.QStyle.SP_ArrowDown)
		def makeExpander(self,expanded):
			self.expander=QPushButton(self.name)
			self.expander.setCheckable(True)
			self.expander.setChecked(expanded)
			self.expander.setStyleSheet('text-align: left; padding: 0pt; padding-left: 6px; ')
			self.expander.setFocusPolicy(Qt.StrongFocus)
			self.expander.toggled.connect(self.toggleExpander)
			self.setExpanderIcon()
			return self.expander
		def setExpanderIcon(self): self.expander.setIcon(self.downArrow if self.expander.isChecked() else self.rightArrow)

		def toggleExpander(self):
			with WidgetUpdatesDisabled(self.expander.parentWidget()):
				self.setExpanderIcon()
				for e in self.entries:
					e.setVisible(self.expander.isChecked())


	def __init__(self,ser,parent=None,ignoredAttrs=set(),objAttrLabelList=None,showType=False,path=None,labelIsVar=True,showChecks=False,showUnits=False,objManip=True,nesting=0):
		"Construct window, *ser* is the object we want to show."
		# print path
		QtGui.QFrame.__init__(self,parent)
		self.ser=ser
		self.oneObject=ser
		self.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Expanding)
		#where do widgets go in the grid
		self.gridCols={'check':0,'label':1,'value':2,'unit':3}
		# set path or use label, if active (allows 'label' attributes which don't imply automatic python variables of that name)
		self.path=('woo.master.scene.lab.'+ser.label if ser!=None and hasActiveLabel(ser) else path)
		self.showType=showType
		self.nesting=nesting
		self.labelIsVar=labelIsVar # show variable name; if false, docstring is used instead
		self.showChecks=showChecks
		self.showUnits=showUnits
		self.objManip=objManip
		self.hot=False
		self.entries=[]
		self.entryGroups=[]
		self.ignoredAttrs=ignoredAttrs
		self.hasSer=True

		if nesting>ObjectEditor.maxNest:
			self.mkStub()
			return
		
		if objAttrLabelList:
			self.hasSer=False
			# create entries for given attributes
			self.addListObjAttrEntries(objAttrLabelList)
		else:
			# no objAttrLabelList
			if self.ser==None:
				logging.debug('New None Object')
				# show None
				lay=QGridLayout(self); lay.setContentsMargins(2,2,2,2); lay.setVerticalSpacing(0)
				lab=QLabel('<b>None</b>'); lab.setFrameShape(QFrame.Box); lab.setFrameShadow(QFrame.Sunken); lab.setLineWidth(2); lab.setAlignment(Qt.AlignHCenter);
				if self.showType: lay.addWidget(lab,0,0,1,-1)
				return # no timers, nothing will change at all
			logging.debug('New Object of type %s'%ser.__class__.__name__)
		# create entries for all attributes of this object
		if ser:
			self.setWindowTitle(str(ser))
			self.addSerAttrEntries()
		with WidgetUpdatesDisabled(self): self.mkWidgets()
		
		self.refreshTimer=QTimer(self)
		self.refreshTimer.timeout.connect(self.refreshEvent)
		self.refreshTimer.start(500)

	def getListTypeFromDocstring(self,trait):
		"Guess type of array from parsing trait.cxxType. Ugly but works."
		def vecTest(T,cxxT):
			regexp=r'^\s*(std\s*::)?\s*vector\s*<\s*(shared_ptr\s*<\s*)?\s*(std\s*::)?\s*('+T+r')(\s*>)?\s*>\s*$'
			m=re.match(regexp,cxxT)
			return m
		def vecGuess(T):
			regexp=r'^\s*(std\s*::)?\s*vector\s*<\s*(shared_ptr\s*<\s*)?\s*(std\s*::)?\s*(?P<elemT>[a-zA-Z_][a-zA-Z0-9_]+)(\s*>)?\s*>\s*$'
			m=re.match(regexp,T)
			return m
		from woo import dem
		if 'opengl' in woo.config.features: from woo import gl
		from woo import core
		vecMap={
			'bool':bool,'int':int,'long':int,'Body::id_t':long,'size_t':long,
			'Real':float,'float':float,'double':float,
			'Vector6r':Vector6,'Vector6i':Vector6i,'Vector3i':Vector3i,'Vector2r':Vector2,'Vector2i':Vector2i,
			'Vector3r':Vector3,'Matrix3r':Matrix3,'Se3r':Se3FakeType,'Quaternionr':Quaternion,
			'AlignedBox2r':AlignedBox2,'AlignedBox3r':AlignedBox3,
			'string':str
		}
		cxxT=trait.cxxType
		if not cxxT:
			logging.error("Trait for %s does not define cxxType"%(trait.name))
			return None
		for T,ret in vecMap.items():
			if vecTest(T,cxxT):
				logging.debug("Got type %s from cxx type %s"%(repr(ret),cxxT))
				return (ret,)
		#print 'No luck with ',T
		m=vecGuess(cxxT)
		if m:
			#print 'guessed literal type',m.group('elemT')
			elemT=m.group('elemT')
			klasses=[c for c in woo.system.childClasses(woo.core.Object,includeBase=True) if c.__name__==elemT]
			if len(klasses)==0: logging.warn('%s: no Python type object with name %s found (cxxType=%s)'%(wHead,elemT,trait.cxxType))
			elif len(klasses)>1: logging.warn('%s: multiple Python types with name %s found (cxxType=%s): %s'%(wHead,elemT,trait.cxxType,', '.join([c.__module__+'.'+c.__name__ for c in klasses])))
			else: return (klasses[0],) # return tuple to signify sequence
		logging.error("Unable to guess python type from cxx type '%s'"%cxxT)
		return None
	def guessInstanceTypeFromCxxType(self,obj,trait):
		'Return type object guessed from cxxType'
		wHead=obj.__class__.__module__+'.'+obj.__class__.__name__+'.'+trait.name
		m=re.match(r'^\s*(weak_ptr\s*<|shared_ptr\s*<)?([A-Za-z0-9_:]+)(\s*>)?\s*',trait.cxxType)
		if m:
			cT=m.group(2)
			logging.debug('%s: got c++ base type: %s -> %s'%(wHead,trait.cxxType,cT))
			klasses=[c for c in woo.system.childClasses(woo.core.Object,includeBase=True) if c.__name__==cT]
			if len(klasses)==0: logging.warn('%s: no Python type object with name %s found (cxxType=%s)'%(wHead,cT,trait.cxxType))
			elif len(klasses)>1: logging.warn('%s: multiple Python types with name %s found (cxxType=%s): %s'%(wHead,cT,trait.cxxType,', '.join([c.__module__+'.'+c.__name__ for c in klasses])))
			else: return klasses[0]
		logging.warn('%s: no c++ base type found for %s'%(wHead,trait.cxxType))
		logging.warn('%s: using woo.core.Object as type'%(wHead))
		return Object

	def addListObjAttrEntries(self,objAttrLabelList):
		for obj,attr,label in objAttrLabelList:
			if not isinstance(obj,woo.core.Object): logging.error('%s is not a woo.core.Object (attribtue %s requested)'%(str(obj),attr))
			ii=[i for i in obj._getAllTraitsWithClasses() if i[0].name==attr]
			if len(ii)!=1:
				if not ii: logging.error('Object %s has no attribute trait %s.'%(obj.__class__.__name__,attr))
				else: logging.error('Object %s has multiple attribute traits named %s??'%(obj.__class__.__name__,attr))
				continue
			trait,klass=ii[0]
			self.addAttrEntry(obj=obj,trait=trait,klass=klass,label=label)

	def addSerAttrEntries(self):
		for trait,klass in self.oneObject._getAllTraitsWithClasses():
			self.addAttrEntry(obj=self.ser,trait=trait,klass=klass)

	def addAttrEntry(self,obj,trait,klass,label=None):
		'Return attribute entry where *obj* is the parent object, *trait* is the attribute trait, *klass* is the class of the attribute (as python type; if 1-tuple, list of those types). *label* is what will be shown in the UI; if omitted, trait.name will be used by default.'
		if trait.hidden or trait.noGui: return
		attr=trait.name
		try:
			val=getattr(obj,attr) # get the value using serattt, as it might be different from what the dictionary provides (e.g. Body.blockedDOFs)
		except TypeError: # no conversion possible
			logging.error("To-python conversion failed for %s.%s!"%(obj.__class__.__name__,trait.name))
			return
		t=None
		doc=trait.doc
		# remove sphinx markup from docstrings
		doc=re.sub(':[a-zA-Z0-9_-]+:`([^`]*)`',r'<i>\1</i>',doc)
		doc=re.sub('``([^`]+)``',r'<tt>\1</tt>',doc)

		if attr in self.ignoredAttrs: return

		# this attribute starts a new attribute group
		if trait.startGroup: self.entryGroups.append(self.EntryGroupData(number=len(self.entryGroups),name=trait.startGroup))

		# determine entry type
		if isinstance(val,list):
			t=self.getListTypeFromDocstring(trait)
			if not t and len(val)==0: t=(val[0].__class__,) # 1-tuple is list of the contained type
			#if not t: raise RuntimeError('Unable to guess type of '+str(obj)+'.'+attr)
		elif val.__class__ in _attributeGuessedTypeMap: t=_attributeGuessedTypeMap[val.__class__]
		elif not isinstance(val,woo.core.Object) and val!=None: t=val.__class__
		else: # for Woo objects, determine base class if manipulation is allowed; if not, use current instance type (it can't be changed anyway) or Object (it the value is None)
			if self.objManip: t=self.guessInstanceTypeFromCxxType(obj,trait)
			elif val!=None: t=val.__class__
			else: t=Object

		if len(self.entryGroups)==0: self.entryGroups.append(self.EntryGroupData(number=0,name=None))
		groupNo=len(self.entryGroups)-1

		#if not match: print 'No attr match for docstring of %s.%s'%(obj.__class__.__name__,attr)
		#logging.debug('Attr %s is of type %s'%(attr,((t[0].__name__,) if isinstance(t,tuple) else t.__name__)))
		self.entries.append(self.EntryData(obj=obj,name=attr,T=t,groupNo=groupNo,doc=doc,trait=trait,containingClass=klass,editor=self,label=label))

	def getDocstring(self,attr=None):
		"If attr is *None*, return docstring of the Object itself"
		if attr==None:
			if self.oneObject.__class__.__doc__!=None: doc=self.oneObject.__class__.__doc__.decode('utf-8')
			else:
				logging.error('Class %s __doc__ is None?'%self.ser.__class__.__name__)
				return None
		else:
			ee=[e.doc for e in self.entries if e.name==attr]
			if not ee:
				logging.error('No entry for attribute named %s?'%(attr))
				doc='[no documentation found]'
			else: doc=e[0].doc
		# doc=re.sub(':[a-zA-Z0-9_-]+:`([^`]*)`',r'<i>\1</i>',doc)
		import textwrap
		wrapper=textwrap.TextWrapper(replace_whitespace=False)
		return wrapper.fill(textwrap.dedent(doc))

	def handleRanges(self,getter,setter,entry):
		rg=entry.trait.range
		# return editor for given attribute; no-op, unless float with associated range attribute
		if entry.T==float and rg and rg.__class__==Vector2:
			# getter returns tuple value,range
			# setter needs just the value itself
			return AttrEditor_FloatRange(self,lambda: (getattr(entry.obj,entry.name),rg),lambda x: setattr(entry.obj,entry.name,x))
		elif entry.T==int and rg and rg.__class__==Vector2i:
			return AttrEditor_IntRange(self,lambda: (getattr(entry.obj,entry.name),rg),lambda x: setattr(entry.obj,entry.name,x))
		# range for sequences has the special meaning of minimum and maximum lenth; handled in SeqEditor, not here
		elif isinstance(entry.T,(list,tuple)) and len(entry.T)==1: return None
		else:
			raise RuntimeError("Invalid range object for "+entry.obj.__class__.__name__+"."+entry.name+": type is "+str(entry.T)+", range is "+str(rg)+" (of type "+rg.__class__.__name__+")")
	def handleChoices(self,getter,setter,entry):
		# choice for sequences has the special meaning of descriptions of individual items; handled in SeqEditor, not here
		if isinstance(entry.T,(list,tuple)) and len(entry.T)==1: return None
		choice=entry.trait.choice
		return AttrEditor_Choice(self,lambda: (getattr(entry.obj,entry.name),choice),lambda x: setattr(entry.obj,entry.name,x))
	def handleBits(self,getter,setter,entry):
		bits=entry.trait.bits
		return AttrEditor_Bits(self,lambda: (getattr(entry.obj,entry.name),bits),lambda x: setattr(entry.obj,entry.name,x))
	def handleRgbColor(self,getter,setter,entry):
		return AttrEditor_RgbColor(self,getter,setter)
	def handleFileDir(self,getter,setter,entry):
		return AttrEditor_FileDir(self,getter,setter,isDir=entry.trait.dirname,isExisting=entry.trait.existingFilename)
		
	def mkWidget(self,entry):
		if not entry.T:
			#print 'return None for %s.%s'%(entry.obj.__class__.__name__,entry.name)
			return None
		

		# single fundamental object
		widget=None
		# default getter and setter
		getter,setter=lambda: getattr(entry.obj,entry.name), lambda x: setattr(entry.obj,entry.name,x)
		# try to find specific widget first based on traits
		# these functions may return none, indicating that it won't be handled specially
		if entry.trait.range: widget=self.handleRanges(getter,setter,entry)
		elif entry.trait.choice: widget=self.handleChoices(getter,setter,entry)
		elif entry.trait.bits: widget=self.handleBits(getter,setter,entry)
		elif entry.trait.rgbColor: widget=self.handleRgbColor(getter,setter,entry)
		elif entry.trait.filename or entry.trait.existingFilename or entry.trait.dirname: widget=self.handleFileDir(getter,setter,entry)
		# no specific widget found, try one for fundamental types
		else:
			Klass=_fundamentalEditorMap.get(entry.T,None)
			if Klass: widget=Klass(self,getter=getter,setter=setter)
		if widget:
			widget.setFocusPolicy(Qt.StrongFocus)
			if entry.trait.readonly: widget.setEnabled(False)
			return widget

		# sequences
		if entry.T.__class__==tuple:
			assert len(entry.T)==1 # we don't handle tuples of other lengths
			# sequence of serializables
			T=entry.T[0]
			if (issubclass(T,Object) or T==Object):
				# HACK!!!
				# per-instance traits for py-derived objects
				if hasattr(entry.obj,'_instanceTraits') and entry.trait.name in entry.obj._instanceTraits:
					entry.trait=entry.obj._instanceTraits[entry.trait.name]
				widget=SeqObject(self,getter,setter,T=T,trait=entry.trait,path=(self.path+'.'+entry.name if self.path else None),shrink=True,nesting=self.nesting+1)
				return widget
			if (T in _fundamentalEditorMap):
				widget=SeqFundamentalEditor(self,getter,setter,T)
				return widget
			print 'No widget for (%s,) in %s.%s'(T.__name__,entry.obj.__class__.__name__,entry.name)
			return None
		# a woo.Object
		if issubclass(entry.T,Object) or entry.T==Object:
			obj=getattr(entry.obj,entry.name)
			# should handle the case of obj==None as well
			widget=ObjectEditor(getattr(entry.obj,entry.name),parent=self,showType=self.showType,path=(self.path+'.'+entry.name if self.path else None),labelIsVar=self.labelIsVar,showChecks=self.showChecks,showUnits=self.showUnits,objManip=self.objManip,nesting=self.nesting+1)
			widget.setFrameShape(QFrame.Box); widget.setFrameShadow(QFrame.Raised); widget.setLineWidth(1)
			return widget
		print 'No widget for %s in %s.%s'%(entry.T.__name__,entry.obj.__class__.__name__,entry.name)
		return None
	def serQLabelMenu(self,widget,position):
		menu=QMenu(self)
		toggleLabelIsVar=menu.addAction('Variables')
		toggleLabelIsVar.setCheckable(True); toggleLabelIsVar.setChecked(self.labelIsVar)
		toggleLabelIsVar.triggered.connect(lambda: self.toggleLabelIsVar(None))
		toggleShowChecks=menu.addAction(u'Checks')
		toggleShowChecks.setCheckable(True); toggleShowChecks.setChecked(self.showChecks)
		toggleShowChecks.triggered.connect(lambda: self.toggleShowChecks(None))
		toggleShowUnits=menu.addAction('Units')
		toggleShowUnits.setCheckable(True); toggleShowUnits.setChecked(self.showUnits)
		toggleShowUnits.triggered.connect(lambda: self.toggleShowUnits(None))
		if self.hasSer and self.ser is not None:
			actionSave=menu.addAction(u'⛁ Save')
			actionSave.triggered.connect(lambda obj=self.ser: self.saveObject(obj))
		#if self.path is not None:
		#	actionLoad=menu.addAction(u'↥ Load')
		#	actionLoad.trigger.connect(lambda: self.loadObject)
		menu.popup(self.mapToGlobal(position))
		#print 'menu popped up at ',widget.mapToGlobal(position),' (local',position,')'
	def saveObject(self,obj):
		assert obj is not None
		f=QFileDialog.getSaveFileName(self,'Save object: use .json, .expr, .pickle, .html ...','.')
		if not f: return
		woo._monkey.io.Object_dump(obj,unicode(f),format='auto',fallbackFormat='expr')
	# XXX: deprecated chunk
	#def loadObject(self):
	#	assert self.path
	#	f=QFileDialog.getOpenFileName(self,msg,'.')
	#	if not f: return # no file selected
	#	try:
	#		if isObj: obj=entry.T.load(f) # be user friendly if garbage is being loaded
	#		else: obj=woo._monkey.io.Object_load(None,f)
	#		raise NotImplementedError('Loading objects to path is not yet implemented.')
	#		# get parent object (or parent sequence!) and use setattr/setitem to assign the object
	#		# parent=path.split('.').join('.')
	#		# eval(path)=obj
	#	except Exception as e:
	#		import traceback
	#		traceback.print_exc()
	#		showExceptionDialog(self,e)
	def getAttrLabelToolTip(self,entry):
		try:
			ini=str(entry.trait.ini) if (entry.trait.ini and not isinstance(entry.trait.ini,Object)) else ''
		except TypeError:
			# boost::python won't convert weak_ptr, catch it here
			ini=''
		toolTip=entry.containingClass.__name__+'.<b><i>'+entry.name+'</i></b><br>'+entry.doc+('<br><small>default: %s</small>'%ini)
		if self.labelIsVar: return woo.document.makeObjectHref(entry.obj,entry.name,text=entry.label),toolTip
		return entry.doc.decode('utf-8'),toolTip
	def toggleLabelIsVar(self,val=None):
		self.labelIsVar=(not self.labelIsVar if val==None else val)
		for entry in self.entries:
			entry.widgets['label'].setTextToolTip(*self.getAttrLabelToolTip(entry),elide=not self.labelIsVar)
			if entry.widget.__class__==ObjectEditor:
				entry.widget.toggleLabelIsVar(self.labelIsVar)
	def toggleShowChecks(self,val=None):
		with WidgetUpdatesDisabled(self):
			self.showChecks=(not self.showChecks if val==None else val)
			#for g in self.entryGroups: g.showChecks=self.showChecks # propagate down
			for entry in self.entries:
				if entry.visible and not entry.hidden: entry.widgets['check'].setVisible(self.showChecks)
				if not entry.trait.readonly:
					if 'value' in entry.widgets: entry.widgets['value'].setEnabled(True)
					entry.widgets['label'].setEnabled(True)
				if entry.widget.__class__==ObjectEditor:
					entry.widget.toggleShowChecks(self.showChecks)
	def toggleShowUnits(self,val=None):
		with WidgetUpdatesDisabled(self):
			self.showUnits=(not self.showUnits if val==None else val)
			#print self.showUnits
			for entry in self.entries:
				entry.setVisible(None)
				entry.unitChanged(forceBaseUnit=(not self.showUnits))
				if entry.widget.__class__==ObjectEditor:
					entry.widget.toggleShowUnits(self.showUnits)
	def objManipLabelMenu(self,entry,pos):
		'context menu for creating/deleting/loading/saving woo.core.Object from within the editor'
		menu=QMenu(self)
		isNone=(getattr(entry.obj,entry.name)==None)
		# isinstance is false for None, but None is always (?) missing woo.core.Object anyway
		isObj=isinstance(getattr(entry.obj,entry.name),woo.core.Object) or isNone
		default=menu.addAction(u'↺ Default')
		default.triggered.connect(lambda: self.doObjManip('default',entry,isNone,isObj))
		if isObj:
			if isNone:
				newDel=menu.addAction(u'☘ New')
				newDel.triggered.connect(lambda: self.doObjManip('new',entry,isNone,isObj))
			else:
				d=menu.addAction(u'☠  Delete')
				d.triggered.connect(lambda: self.doObjManip('del',entry,isNone,isObj))
				replace=menu.addAction(u'⇄ Replace')
				replace.triggered.connect(lambda: self.doObjManip('replace',entry,isNone,isObj))
		if not isNone:
			save=menu.addAction(u'⛁ Save')
			save.triggered.connect(lambda: self.doObjManip('save',entry,isNone,isObj))
		load=menu.addAction(u'↥ Load')
		load.triggered.connect(lambda: self.doObjManip('load',entry,isNone,isObj))
		lib=makeLibraryBrowser(menu,lambda name,obj: self.setFromLib(entry,name,obj),entry.T,u'⇈ Library')
		# put this into each varibles menus instead, as there is no the label at the top
		if not self.showType or not self.hasSer:
			menu.addSeparator()
			v=menu.addAction('Variables')
			v.setCheckable(True); v.setChecked(self.labelIsVar)
			v.triggered.connect(lambda: self.toggleLabelIsVar(None))
			c=menu.addAction(u'Checks')
			c.setCheckable(True); c.setChecked(self.showChecks)
			c.triggered.connect(lambda: self.toggleShowChecks(None))
			u=menu.addAction('Units')
			u.setCheckable(True); u.setChecked(self.showUnits)
			u.triggered.connect(lambda: self.toggleShowUnits(None))


		menu.popup(entry.widgets['label'].mapToGlobal(pos))
	def setFromLib(self,entry,name,obj):
		# setting library object
		if isinstance(obj,woo.core.Object): obj=obj.deepcopy() # prevent lib object modification
		setattr(entry.obj,entry.name,obj)
	def doObjManip(self,action,entry,isNone,isObj):
		'Handle menu action from objManipLabelMenu'
		# FIXME: this is an ugly hack of using woo._monkey.io.Object_{dump,load} directly!!!
		import woo, woo.core, woo._monkey.io
		#print 'Manipulating Object',entry.obj.__class__.__name__+'.'+entry.name
		if action=='del':
			assert isObj
			setattr(entry.obj,entry.name,None)
		elif action=='new' or action=='replace':
			assert isObj
			types=woo.system.childClasses(entry.T,includeBase=True)
			if(len(types)==1): setattr(entry.obj,entry.name,entry.T())
			else:
				d=NewObjectDialog(self,entry.T)
				if not d.exec_(): return # cancelled
				setattr(entry.obj,entry.name,d.result())
		elif action=='save':
			assert not isNone
			obj=getattr(entry.obj,entry.name)
			f=QFileDialog.getSaveFileName(self,'Save object: use .json, .expr, .pickle, .html ...','.')
			if not f: return
			woo._monkey.io.Object_dump(obj,unicode(f),format='auto',fallbackFormat='expr')
		elif action=='load':
			msg='Load a %s'%entry.T.__name__ if isObj else 'Load'
			f=QFileDialog.getOpenFileName(self,msg,'.')
			if not f: return # no file selected
			try:
				if isObj: obj=entry.T.load(f) # be user friendly if garbage is being loaded
				else: obj=woo._monkey.io.Object_load(None,f)
				setattr(entry.obj,entry.name,obj)
			except Exception as e:
				import traceback
				traceback.print_exc()
				showExceptionDialog(self,e)
		elif action=='default':
			if isObj:
				val=entry.trait.ini.deepcopy() if entry.trait.ini!=None else None # don't call deepcopy on none, it fails :)
			else:
				import copy
				val=copy.deepcopy(entry.trait.ini)
			setattr(entry.obj,entry.name,val)
		else: raise RuntimError('Unknown action %s for object manipulation context menu!'%action)
		self.refreshEvent()

	def mkStub(self):
		lay=QGridLayout(self)
		lay.setContentsMargins(2,2,2,2)
		lay.setVerticalSpacing(0)
		self.setLayout(lay)
		lay.addWidget(QLabel("GUI nesting > %d"%(ObjectEditor.maxNest)),1,1)


	def mkWidgets(self):
		onlyDefaultGroups=(len(self.entryGroups)==1 and self.entryGroups[0].name==None)
		if self.showType and self.hasSer: # create type label
			lab=SerQLabel(self,makeObjectLabel(self.ser,addr=True,href=True),tooltip=self.getDocstring(),path=self.path,ser=self.ser)
			lab.setFrameShape(QFrame.Box); lab.setFrameShadow(QFrame.Sunken); lab.setLineWidth(2); lab.setAlignment(Qt.AlignHCenter); lab.linkActivated.connect(woo.qt.openUrl)
			## attach context menu to the label
			lab.setContextMenuPolicy(Qt.CustomContextMenu)
			lab.customContextMenuRequested.connect(lambda pos: self.serQLabelMenu(lab,pos))
			lab.setFocusPolicy(Qt.ClickFocus)
		lay=QGridLayout(self)
		lay.setContentsMargins(2,2,2,2)
		lay.setVerticalSpacing(0)
		if self.showType:
			lay.addWidget(lab,0,0,1,-1)
			lay.setRowStretch(0,-1)
		self.setLayout(lay)

		maxLabelWd=0.
		for entry in self.entries:
			w=self.mkWidget(entry)
			if w!=None: entry.widget=entry.widgets['value']=w
			# else: print 'No value widget for',entry.obj.__class__.__name__+'.'+entry.name
			#entry.widgets['value']=entry.widget # for code compat
			#if not entry.widgets['value']: entry.widgets['value']=entry.widget=QFrame() # avoid None widgets
			objPath=(self.path+'.'+entry.name) if self.path else None
			labelText,labelTooltip=self.getAttrLabelToolTip(entry)
			label=SerQLabel(self,labelText,tooltip=labelTooltip,path=objPath,elide=not self.labelIsVar)
			entry.widgets['label']=label
			if self.objManip and ('value' in entry.widgets):
				label.setContextMenuPolicy(Qt.CustomContextMenu)
				label.customContextMenuRequested.connect(lambda pos,entry=entry: self.objManipLabelMenu(entry,pos))
				label.setFocusPolicy(Qt.ClickFocus)
			entry.widgets['check']=QCheckBox('',self)
			ch=entry.widgets['check']
			ch.setVisible(self.showChecks)
			if entry.trait.readonly: ch.setEnabled(False)
			ch.clicked.connect(entry.toggleChecked)
			if entry.trait.unit:
				# frame for all unit-manipulating boxes
				entry.widgets['unit']=QFrame(self)
				unitLay=QHBoxLayout(entry.widgets['unit'])
				entry.unitLayout=unitLay
				unitLay.setSpacing(0); unitLay.setMargin(0)
				entry.widgets['unit'].setLayout(unitLay)
				assert len(entry.trait.unit)==len(entry.trait.altUnits)
				assert len(entry.trait.unit)==len(entry.trait.prefUnit)
				unitChoice=False
				for unit,pref,alt in zip(entry.trait.unit,entry.trait.prefUnit,entry.trait.altUnits):
					if alt: # there are alternative units, we give choice therefore
						unitChoice=True
						w=QComboBox(self)
						w.addItem(_unicodeUnit(unit))
						w.activated.connect(entry.unitChanged)
						for u,mult in alt: w.addItem(_unicodeUnit(u))
						# set preferred unit right away; when units are not shown, always use SI, however
						if unit!=pref[0]:
							# this is checked in c++, should never fail
							# print entry.trait.name,[i for i in range(len(alt)) if alt[i][0]==pref[0]]
							ii=[i for i in range(len(alt)) if alt[i][0]==pref[0]][0]
							w.setCurrentIndex(ii+1)
					else:
						w=QLabel(_unicodeUnit(unit),self)
					unitLay.addWidget(w)
				if unitChoice: entry.unitChanged() # postpone calling this at the very end
				entry.widgets['unit'].setVisible(self.showUnits)
			if entry.trait.buttons:
				bb=entry.trait.buttons[0]
				for i in range(0,len(bb),3):
					b=QPushButton(bb[i],self)
					l=QLabel(self.fontMetrics().elidedText(' '+bb[i+2],Qt.ElideRight,100))
					b.setToolTip('<code>'+bb[i+1]+'</code>')
					b.setStyleSheet('QPushButton {text-align: left; padding-left:5px; }')
					b.setFocusPolicy(Qt.NoFocus)
					l.setToolTip(bb[i+2])
					def callButton(cmd):
						exec cmd in globals(),dict(self=entry.obj)
					# first arg (foo) used by the dispatch
					# cmd=bb[i+1] binds the current value bb[i+1]
					b.clicked.connect(lambda foo,cmd=bb[i+1]: callButton(cmd)) 
					entry.widgets['buttons-%d'%(i/3)]=b
					entry.widgets['buttonLabels-%d'%(i/3)]=l 
				#print 'Buttons',entry.trait.name,entry.trait.buttons,entry.widgets
			self.entryGroups[entry.groupNo].entries.append(entry)
		for i,g in enumerate(self.entryGroups):
			hide=i>0
			if not onlyDefaultGroups:
				ex=g.makeExpander(not hide) # first group expanded, other hidden
				lay.addWidget(ex,lay.rowCount(),0,1,-1)
				lay.setRowStretch(lay.rowCount()-1,-1)
			for i,entry in enumerate(g.entries):
				try:
					def addButtonsNow():
						bb,ll=sorted([k for k in entry.widgets.keys() if k.startswith('buttons-')]),sorted([k for k in entry.widgets.keys() if k.startswith('buttonLabels-')])
						for b,l in zip(bb,ll):
							row=lay.rowCount()
							lay.addWidget(entry.widgets[b],row,self.gridCols['value'],1,1)
							lay.addWidget(entry.widgets[l],row,self.gridCols['label'],1,1)
							entry.widgets[l].setStyleSheet('background: palette(%s); '%('Base' if row%2 else 'AlternateBase'))
					# add buttons before
					if entry.trait.buttons and entry.trait.buttons[1]==True: addButtonsNow()
					row=lay.rowCount()
					entry.gridAndRow=lay,row
					lay.addWidget(entry.widgets['check'],row,self.gridCols['check'],1,1)
					lay.addWidget(entry.widgets['label'],row,self.gridCols['label'],1,1)
					entry.widgets['check'].setFocusPolicy(Qt.ClickFocus)
					# entry.widgets['label'].setFocusPolicy(Qt.NoFocus) # default
					maxLabelWd=max(maxLabelWd,entry.widgets['label'].width())
					if 'value' in entry.widgets:
						colSpan=(2 if 'unit' not in entry.widgets else 1) # use the unit column if there is no unit
						lay.addWidget(entry.widgets['value'],row,self.gridCols['value'],1,colSpan)
						# entry.widgets['value'].setFocusPolicy(Qt.StrongFocus) # default
					if 'unit' in entry.widgets:
						lay.addWidget(entry.widgets['unit'],row,self.gridCols['unit'])
						entry.widgets['unit'].setFocusPolicy(Qt.ClickFocus) # skip when keyboard-navigating
					lay.setRowStretch(row,2)
					for w in entry.widgets['label'],: # entry.widgets['value']:
						if not w or w.__class__==ObjectEditor: continue # nested editor not modified
						w.setStyleSheet('background: palette(%s); '%('Base' if row%2 else 'AlternateBase'))
					# add buttons after
					if entry.trait.buttons and entry.trait.buttons[1]==False: addButtonsNow()
				except RuntimeError:
					print 'ERROR while creating widget for entry %s (%s)'%(entry.name,objPath)
					import traceback
					traceback.print_exc()
		# close all groups except the first one			
		for g in self.entryGroups[1:]: g.toggleExpander()
		lay.setColumnMinimumWidth(self.gridCols['label'],maxLabelWd)
		lay.setSpacing(0)
		lay.addWidget(QFrame(self),lay.rowCount(),0,1,-1) # expander at the very end
		lay.setRowStretch(lay.rowCount()-1,10000)
		lay.setColumnStretch(self.gridCols['check'],-1)
		lay.setColumnStretch(self.gridCols['label'],2)
		lay.setColumnStretch(self.gridCols['value'],10)
		lay.setColumnStretch(self.gridCols['unit'],-1)
		self.refreshEvent()
	def refreshEvent(self):
		maxLabelWd=0.
		for e in self.entries:
			if self.hasSer: assert self.ser==e.obj
			if e.widget and not e.widget.hot:
				maxLabelWd=max(maxLabelWd,e.widgets['label'].width())
				# if there is a new instance of Object, we need to make new widget and replace the old one completely
				if type(e.widget)==ObjectEditor and e.widget.hasSer and e.widget.ser!=getattr(e.obj,e.name):
					# print 'New ObjectEditor (%s): '%e.name,e.widget.ser,'->',getattr(e.obj,e.name)
					# print e.widget.ser._cxxAddr,getattr(e.obj,e.name)._cxxAddr,getattr(e.obj,e.name)
					assert e.widget.ser==None or getattr(e.obj,e.name)==None or e.widget.ser._cxxAddr!=getattr(e.obj,e.name)._cxxAddr or (isinstance(getattr(e.obj,e.name),woo.pyderived.PyWooObject) and id(getattr(e.obj,e.name))!=id(e.widget.ser))
					e.widget.hide()
					e.widget=e.widgets['value']=self.mkWidget(e)
					grid,row=e.gridAndRow
					colSpan=(2 if 'unit' not in e.widgets else 1)
					grid.addWidget(e.widget,row,self.gridCols['value'],1,colSpan)
				# visibility might change if hideIf is defined
				if e.trait.hideIf or not e.visible: e.setVisible(None)
				e.widget.refresh()
		#self.layout().setColumnMinimumWidth(self.gridCols['label'],maxLabelWd)
		if self.labelIsVar: self.layout().setColumnStretch(self.gridCols['label'],-1)
		else: self.layout().setColumnStretch(self.gridCols['label'],2)
	def refresh(self): pass

def makeObjectLabel(ser,href=False,addr=True,boldHref=True,num=-1,count=-1):
	ret=u''
	if num>=0:
		if count>=0: ret+=u'%d/%d. '%(num,count)
		else: ret+=u'%d. '%num
	if href: ret+=(u' <b>' if boldHref else u' ')+woo.document.makeObjectHref(ser)+(u'</b> ' if boldHref else u' ')
	else: ret+=ser.__class__.__name__+' '
	if hasActiveLabel(ser): ret+=u' “'+unicode(ser.label)+u'”'
	# do not show address if there is a label already
	elif addr and ser!=None:
		ret+='0x%x'%ser._cxxAddr
	return ret

class SeqObjectComboBox(QFrame):
	def getItemType(self): return self.trait.pyType[0]
	def __init__(self,parent,getter,setter,T,trait,path=None,shrink=False,nesting=0):
		QFrame.__init__(self,parent)
		self.getter,self.setter,T,self.trait,self.path,self.shrink=getter,setter,T,trait,path,shrink
		if not hasattr(self.trait,'pyType'): self.trait.pyType=(T,) # this is for compat with C++ (?)
		self.layout=QVBoxLayout(self)
		self.nesting=nesting
		topLineFrame=QFrame(self)
		topLineLayout=QHBoxLayout(topLineFrame);
		for l in self.layout, topLineLayout: l.setSpacing(0); l.setContentsMargins(0,0,0,0)
		topLineFrame.setLayout(topLineLayout)
		labels=(u'+',u'−',u'↑',u'↓')
		tooltips=('Add','Delete','Move up','Move down')
		buttons=(self.newButton,self.killButton,self.upButton,self.downButton)=[QPushButton(label,self) for label in labels]
		buttonSlots=(None,self.killSlot,self.upSlot,self.downSlot) # same order as buttons
		for i,b in enumerate(buttons): b.setStyleSheet('QPushButton { font-size: 15pt; font-weight: bold; }'); b.setFixedWidth(25); b.setFixedHeight(30); b.setToolTip(tooltips[i])
		self.combo=QComboBox(self)
		self.combo.setSizeAdjustPolicy(QComboBox.AdjustToContents)
		for w in buttons[:2]+[self.combo,]+buttons[2:]: topLineLayout.addWidget(w)
		self.layout.addWidget(topLineFrame) # nested layout
		self.scroll=QScrollArea(self); self.scroll.setWidgetResizable(True)
		self.layout.addWidget(self.scroll)
		self.seqEdit=None # currently edited serializable
		self.setLayout(self.layout)
		self.hot=None # API compat with ObjectEditor
		self.setFrameShape(QFrame.Box); self.setFrameShadow(QFrame.Raised); self.setLineWidth(1)
		self.newDialog=None # is set when new dialog is created, and destroyed when it returns
		self.comboItemCount=0 # we use some inactive items with ranges instead of objects, so keep track of valid items separately
		# create menu for the "new" button
		newMenu=QMenu();
		newMenu.addAction(u'☘ New',self.newSlot)
		self.cloneAction=newMenu.addAction(u'≡ Clone',self.cloneSlot)
		newMenu.addAction(u'↥ Load',self.loadSlot)
		lib=makeLibraryBrowser(newMenu,lambda name,obj: self.libLoadSlot(name,obj),self.getItemType(),u'⇈ Library')
		self.newButton.setMenu(newMenu)

		# signals
		for b,slot in zip(buttons[1:],buttonSlots[1:]): b.clicked.connect(slot)
		self.combo.currentIndexChanged.connect(self.comboIndexSlot)
		self.refreshEvent()
		# periodic refresh
		self.refreshTimer=QTimer(self)
		self.refreshTimer.timeout.connect(self.refreshEvent)
		self.refreshTimer.start(500) # 1s should be enough
		#print 'SeqObject path is',self.path
	def comboIndexSlot(self,ix): # different seq item selected
		currSeq=self.getter();
		if ix>=len(currSeq): # this can happen with fake items which get activated when real item is deleted
			self.combo.setCurrentIndex(len(currSeq)-1)
			return
		if len(currSeq)==0: ix=-1
		logging.debug('%s comboIndexSlot len=%d, ix=%d'%(self.getItemType().__name__,len(currSeq),ix))
		self.downButton.setEnabled(ix<len(currSeq)-1)
		self.upButton.setEnabled(ix>0)
		self.combo.setEnabled(ix>=0)
		if ix>=0:
			ser=currSeq[ix]
			self.seqEdit=ObjectEditor(ser,parent=self,showType=seqObjectShowType,path=(self.path+'['+str(ix)+']') if self.path else None,nesting=self.nesting+1)
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
	# XXX: deprecated chunk
	# def serLabel(self,ser,i=-1):
	# 	return ('' if i<0 else str(i)+'. ')+str(ser)[1:-1].replace('instance at ','')
	def refreshEvent(self,forceIx=-1):
		try:
			currSeq=self.getter()
			comboEnabled=self.combo.isEnabled()
			if comboEnabled and len(currSeq)==0: self.comboIndexSlot(-1) # force refresh, otherwise would not happen from the initially empty state
			ix,cnt=self.combo.currentIndex(),self.comboItemCount # self.combo.count()
			# serializable currently being edited (which can be absent) or the one of which index is forced
			ser=(self.seqEdit.ser if self.seqEdit else None) if forceIx<0 else currSeq[forceIx] 
			if comboEnabled and len(currSeq)==cnt and (ix<0 or (ix<len(currSeq) and ser==currSeq[ix])): return
			if not comboEnabled and len(currSeq)==0: return
			logging.debug(self.getItemType().__name__+' rebuilding list from scratch')
			self.combo.clear()
			if len(currSeq)>0:
				prevIx=-1
				for i,s in enumerate(currSeq):
					extra=[]
					if self.trait.choice and i<len(self.trait.choice): extra.append(self.trait.choice[i])
					if self.trait.range and i>=self.trait.range[0]: extra.append('optional')
					extra=' (%s)'%('; '.join(extra)) if extra else ''
					self.combo.addItem(makeObjectLabel(s,num=i,count=len(currSeq),addr=False)+extra)
					if s==ser: prevIx=i
				self.comboItemCount=self.combo.count()
				# add extra (inactive, unselectable) items
				# when using range and descriptions
				if self.trait.range or self.trait.choice:
					for i in range(len(currSeq),max(self.trait.range[1],len(self.trait.choice))):
						extra=[]
						if self.trait.choice and i<len(self.trait.choice): extra.append(self.trait.choice[i])
						if self.trait.range and i>=self.trait.range[0]: extra.append('optional')
						extra=' (%s)'%('; '.join(extra)) if extra else ''
						self.combo.addItem(u'%d. −'%i+extra)
						# use the hack from http://theworldwideinternet.blogspot.cz/2011/01/disabling-qcombobox-items.html to deactivate items
						self.combo.setItemData(i,0,Qt.UserRole-1)
				if forceIx>=0: newIx=forceIx # force the index (used from newSlot to make the new element active)
				elif prevIx>=0: newIx=prevIx # if found what was active before, use it
				elif ix>=0: newIx=ix         # otherwise use the previous index (e.g. after deletion)
				else: newIx=0                  # fallback to 0
				logging.debug('%s setting index %d'%(self.getItemType().__name__,newIx))
				self.combo.setCurrentIndex(newIx)
			else:
				logging.debug('%s EMPTY, setting index 0'%(self.getItemType().__name__))
				self.combo.setCurrentIndex(-1)
			enableKill=(not self.trait.noGuiResize and len(currSeq)>(self.trait.range[0] if self.trait.range else 0))
			enableNew=(not self.trait.noGuiResize and (not self.trait.range or len(currSeq)<self.trait.range[1]))
			enableClone=enableNew and len(currSeq)>0
			self.killButton.setEnabled(enableKill)
			self.newButton.setEnabled(enableNew)
			self.cloneAction.setEnabled(enableClone)
		except RuntimeError as e:
			print 'Error refreshing sequence (path %s), ignored.'%self.path
			
	def newSlot(self):
		# print 'newSlot called'
		dialog=NewObjectDialog(self,self.getItemType())
		if not dialog.exec_(): return # cancelled
		ser=dialog.result()
		ix=self.combo.currentIndex()
		currSeq=list(self.getter()); currSeq.insert(ix,ser); self.setter(currSeq)
		logging.debug('%s new item created at index %d'%(self.getItemType().__name__,ix))
		self.refreshEvent(forceIx=ix)
	def loadSlot(self):
		f=QFileDialog.getOpenFileName(self,msg,'.')
		if not f: return # no file selected
		T=self.getItemType()
		try:
			obj=T.load(f) # be user friendly if garbage is being loaded
			# else: obj=woo._monkey.io.Object_load(None,f)
			currSeq=list(self.getter()); currSeq.insert(ix+1,obj); self.setter(currSeq)
		except Exception as e:
			import traceback
			traceback.print_exc()
			showExceptionDialog(self,e)
	def libLoadSlot(self,name,obj):
		ix=self.combo.currentIndex()
		currSeq=list(self.getter()); currSeq.insert(ix+1,obj); self.setter(currSeq)
		self.refreshEvent(forceIx=ix+1)
	def cloneSlot(self):
		ix=self.combo.currentIndex()
		currSeq=list(self.getter()); currSeq.insert(ix+1,currSeq[ix].deepcopy()); self.setter(currSeq)
		self.refreshEvent(forceIx=ix+1)
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
	def refresh(self): pass # API compat with ObjectEditor

SeqObject=SeqObjectComboBox


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

class NewObjectDialog(QDialog):
	def __init__(self,parent,baseClass,includeBase=True):
		import woo.system
		QDialog.__init__(self,parent)
		self.setWindowTitle('Create new object of type %s'%baseClass.__name__)
		self.layout=QVBoxLayout(self)
		self.combo=QComboBox(self)
		self.classes=list(woo.system.childClasses(baseClass,includeBase=False)); self.classes.sort()
		if includeBase:
			self.classes=[baseClass,None]+self.classes # None is for the separator, so that indices are the same
			self.combo.addItem(baseClass.__name__)
			self.combo.insertSeparator(1000)
		self.combo.addItems([c.__name__ for c in self.classes[(2 if includeBase else 0):]])
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
		self.ser=self.classes[index]() # instantiate the class
		self.scroll.setWidget(ObjectEditor(self.ser,self.scroll,showType=True))
		self.scroll.show()
	def result(self): return self.ser
	def sizeHint(self): return QSize(180,400)

class SeqFundamentalEditor(QFrame):
	# maximum length of the sequence; show ellipsis in the middle if longer
	# this should avoid freezes in the UI due to unexpectedly long data
	maxShowLen=30

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
		self.convSpec=None # cache value of unit conversions, for rows being added
		self.multiplier=None
		# ObjectEditor API compat
		self.hot=False
		self.rebuild()
		# periodic refresh
		self.refreshTimer=QTimer(self)
		self.refreshTimer.timeout.connect(self.refreshEvent)
		self.refreshTimer.start(500) # 1s should be enough
	def contextMenuEvent(self, event):
		# may return -1, which is OK
		index=self.localPositionToIndex(event.pos())
		seq=self.getter()
		if len(seq)==0: index=-1
		item=self.form.itemAt(index,QFormLayout.LabelRole)
		field=item.widget() if (index>=0 and item) else None
		menu=QMenu(self)
		actNew,actKill,actUp,actDown,actFromClip=[menu.addAction(name) for name in (u'☘ New',u'☠ Remove',u'↑ Up',u'↓ Down',u'↵ From clipboard')]
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
			actFromClip.triggered.connect(lambda: self.fromClipSlot(index))
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
	def localPositionToIndex(self,pos,isGlobal=False):
		gp=self.mapToGlobal(pos) if not isGlobal else pos
		for row in range(self.form.rowCount()):
			w,i=self.form.itemAt(row,QFormLayout.FieldRole),self.form.itemAt(row,QFormLayout.LabelRole)
			for wi in w.widget(),i.widget():
				x0,y0,x1,y1=wi.geometry().getCoords(); globG=QRect(self.mapToGlobal(QPoint(x0,y0)),self.mapToGlobal(QPoint(x1,y1)))
				if globG.contains(gp):
					return self.mapList2Seq(row)
		return -1
	def keyFocusIndex(self):
		w=QApplication.focusWidget()
		globPos=w.mapToGlobal(QPoint(.5*w.width(),.5*w.height()))
		return self.localPositionToIndex(globPos,isGlobal=True)
	def keyPressEvent(self,ev):
		isModified=ev.modifiers()&Qt.AltModifier
		if not isModified:
			if not QApplication.focusWidget(): return
			if ev.key()==Qt.Key_Return: QApplication.focusWidget().focusNextPrevChild(True)
			if ev.key()==Qt.Key_Up: QApplication.focusWidget().focusNextPrevChild(False)
			if ev.key()==Qt.Key_Down: QApplication.focusWidget().focusNextPrevChild(True)
			return
		if ev.key()==Qt.Key_Up:
			self.upSlot(self.keyFocusIndex()); ev.accept()
		elif ev.key()==Qt.Key_Down: self.downSlot(self.keyFocusIndex())
		elif ev.key()==Qt.Key_Delete:
			self.killSlot(self.keyFocusIndex())
			ev.accept()
		elif ev.key()==Qt.Key_Backspace:
			self.killSlot(self.keyFocusIndex()-1)
			ev.accept()
		elif ev.key()==Qt.Key_Enter or ev.key()==Qt.Key_Return:
			# insert after the current row
			self.newSlot(self.keyFocusIndex())
			ev.accept();
	def newSlot(self,i): # i is the index AFTER which the new row is inserted
		seq=self.getter();
		seq.insert(i+1,_fundamentalInitValues.get(self.itemType,self.itemType()))
		self.setter(seq)
		self.rebuild()
		if len(seq)>0:
			item=self.form.itemAt(i+1,QFormLayout.FieldRole)
			if item: item.widget().setFocus()
	def killSlot(self,i):
		seq=self.getter();
		if i<0: return
		assert(i<len(seq)); del seq[i]; self.setter(seq)
		self.refreshEvent()
		if len(seq)>0: self.form.itemAt(max(0,i if i<len(seq)-1 else i-1),QFormLayout.FieldRole).widget().setFocus()
	def upSlot(self,i):
		if i==0: return
		seq=self.getter(); assert(i<len(seq));
		prev,curr=seq[i-1:i+1];
		seq[i-1],seq[i]=curr,prev;
		self.setter(seq)
		self.refreshEvent(forceIx=i-1)
	def downSlot(self,i):
		seq=self.getter();
		if i==len(seq)-1: return
		assert(i<len(seq)-1);
		curr,nxt=seq[i:i+2]; seq[i],seq[i+1]=nxt,curr; self.setter(seq)
		self.refreshEvent(forceIx=i+1)
	def fromClipSlot(self,i):
		try:
			importables={Vector3:(float,3),Vector2:(float,2),Vector6:(float,6),Vector2i:(int,2),Vector2i:(int,3),Vector6i:(int,6),Matrix3:(float,9),float:(float,1),int:(int,1)}
			if self.itemType not in importables: raise NotImplementedError("Type %s is not text-importable"%(self.itemType.__name__))
			elementType,lineLen=importables[self.itemType]
			print 'Will import lines with %d item(s) of type %s'%(lineLen,elementType.__name__)
			# get txt from clipboard
			cb=QApplication.clipboard()
			txt=str(cb.text())
			print 'Got %d lines from clipboard:\n'%(len(txt.split('\n'))),txt
			# handle unit conversions here
			if self.multiplier:
				if isinstance(self.multiplier,tuple): mult=[1./m for m in self.multiplier]
				else: mult=[1./self.multiplier]*lineLen
				print 'Input will be scaled by',mult,' to match selected units'
			else: mult=[1.]*lineLen
			#
			seq=[]
			for i,ll in enumerate(txt.split('\n')):
				l=ll.split()
				if len(l)==0: continue # skip empty lines
				if len(l)!=lineLen: raise ValueError("Line %d has %d elements (should have %d)"%(i,len(l),lineLen))
				print 'Line tuple is',tuple([val for val in l])
				lineItems=[elementType(eval(val))*mult[i] for i,val in enumerate(l)]
				if lineLen>1: seq.append(tuple(lineItems))
				else: seq.append(lineItems[0]) # sequences of floats/ints are imported as sequence, not as sequence of tuples
			print 'Imported sequence',seq
		except Exception as e:
			import traceback
			traceback.print_exc()
			showExceptionDialog(self,e)
			return
		self.setter(seq)
		self.refreshEvent(forceIx=0)
	def mapList2Seq(self,l):
		'return sequence (object) index from given list (widget) index.'
		if not self.split: return l
		if l>self.split[0]: return l+self.split[1]-1  # 1 for the separator widget
	def mapSeq2List(self,s):
		'return list (widget) index from given sequence (object) index. Return -1 if the item is not shown in the UI due to splitting.'
		if not self.split: return s
		if s<self.split[0]: return s
		if s<self.split[1]: return -1
		if s>=self.split[1]: return s+(self.split[1]-self.split[0])+1 # 1 for the separator widget
	def recomputeSplit(self,currLen):
		l=currLen; ml=SeqFundamentalEditor.maxShowLen
		if l>ml:
			self.split=(ml/2,l-ml/2)
			self.splitLen=l
		else:
			self.split=None
			self.splitLen=0

	def renumber(self,currLen):
		if not self.split: return
		self.recomputeSplit()
		for row in range(self.form.rowCount()):
			l=self.form.itemAt(row,QFormLayout.LabelRole)
			l.widget().setText('%d. '%self.mapList2Seq(row))
		self.splitLen=currLen

	def rebuild(self):
		currSeq=self.getter()
		#print 'aaa',len(currSeq)
		# clear everything
		for row in range(self.form.rowCount()):
			logging.trace('counts',self.form.rowCount(),self.form.count())
			for wi in self.form.itemAt(row,QFormLayout.FieldRole),self.form.itemAt(row,QFormLayout.LabelRole):
				self.form.removeItem(wi)
				if not wi.widget(): continue
				logging.trace('deleting widget',wi.widget())
				# for some reason, deleting does not make the thing disappear visually; hiding does, however
				# FIXME: this might be the reason why ever-resizing sequences eat up RAM!!
				widget=wi.widget(); widget.hide(); del widget 
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
			def __call__(self):
				try:
					return self.getter()[self.index]
				except IndexError: return None
		class ItemSetter():
			def __init__(self,getter,setter,index): self.getter,self.setter,self.index=getter,setter,index
			def __call__(self,val):
				try:
					seq=self.getter(); seq[self.index]=val; self.setter(seq)
				except IndexError: pass

		self.recomputeSplit(len(currSeq))

		for i,item in enumerate(currSeq):
			if self.split and self.split[0]==i:
				# show separator
				l=QLabel(u'<b>⋮</b>'); l.font().setPointSize(32)
				self.form.insertRow(i,u'<b>⋮</b>',l)
				continue 
			li=self.mapSeq2List(i)
			# print 'item #%d mapped to field %d'%(i,li)
			if li<0: continue # in the split, do not show anything
			widget=Klass(self,ItemGetter(self.getter,i),ItemSetter(self.getter,self.setter,i))
			self.form.insertRow(i,'%d. '%li,widget)
			logging.debug('added item %d %s'%(i,str(widget)))
			# set units correctly
			if self.multiplier:
				widget.multiplier=self.multiplier
				widget.multiplierChanged(self.convSpec)
		if len(currSeq)==0: self.form.insertRow(0,'<i>empty</i>',QLabel('<i>(right-click for menu)</i>'))
		logging.debug('rebuilt, will refresh now')
		self.refreshEvent(dontRebuild=True) # avoid infinite recursion it the length would change meanwhile
	def refreshEvent(self,dontRebuild=False,forceIx=-1):
		currSeq=self.getter()
		#print 'bbb',len(currSeq)
		if not self.split and len(currSeq)!=self.form.rowCount():
			if dontRebuild: return # length changed behind our back, just pretend nothing happened and update next time instead
			self.rebuild()
			currSeq=self.getter()
		elif self.split and len(currSeq)!=self.splitLen: self.renumber(len(currSeq))
		for i in range(len(currSeq)):
			item=self.form.itemAt(i,QFormLayout.FieldRole)
			if not item: continue # some error condition, oh well
			widget=item.widget()
			logging.trace('got item #%d %s'%(i,str(widget)))
			if hasattr(widget,'hot') and not widget.hot: # it can be a QLabel as well
				widget.refresh()
			if forceIx>=0 and forceIx==i: widget.setFocus()
	def refresh(self): pass # ObjectEditor API
	# propagate multiplier change to children
	def multiplierChanged(self,convSpec):
		self.convSpec=convSpec # cache value should new rows be created
		self.setToolTip(convSpec)
		for row in range(self.form.count()//2):
			w=self.form.itemAt(row,QFormLayout.FieldRole).widget()
			if isinstance(w,QLabel): continue # label that the sequence is empty
			w.multiplier=self.multiplier
			w.multiplierChanged(convSpec)
	




