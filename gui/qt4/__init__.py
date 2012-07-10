# encoding: utf-8
import woo.runtime
if not woo.runtime.hasDisplay: raise ImportError("Connecting to DISPLAY at Yade startup failed, unable to activate the qt4 interface.")

from PyQt4.QtGui import *
from PyQt4 import QtCore

from woo.qt.ui_controller import Ui_Controller

from woo.qt.Inspector import *
from woo import *
import woo.system, woo.config

from woo.qt._GLViewer import *

from ExceptionDialog import *

#maxWebWindows=1
#"Number of webkit windows that will be cycled to show help on clickable objects"
#"holds instances of QtWebKit windows; clicking an url will open it in the window that was the least recently updated"
#webWindows=[] 
#webController=None
sphinxOnlineDocPath='http://www.woodem.eu/doc/'
"Base URL for the documentation. Packaged versions should change to the local installation directory."

import os.path
# find if we have docs installed locally from package
sphinxLocalDocPath=woo.config.prefix+'/share/doc/woo'+woo.config.suffix+'/html/'
sphinxBuildDocPath=woo.config.sourceRoot+'/doc/sphinx2/build/html/'
# we prefer the packaged documentation for this version, if installed
if   os.path.exists(sphinxLocalDocPath+'/index.html'): sphinxPrefix='file://'+sphinxLocalDocPath
# otherwise look for documentation generated in the source tree
elif  os.path.exists(sphinxBuildDocPath+'/index.html'): sphinxPrefix='file://'+sphinxBuildDocPath
# fallback to online docs
else: sphinxPrefix=sphinxOnlineDocPath

#sphinxDocWrapperPage=sphinxPrefix+'/woo.wrapper.html'


## object selection
def getSel(): return woo.gl.Renderer.selObj




def openUrl(url):
	import webbrowser
	webbrowser.open(url)
	#global webController
	#if not webController: webController=webbrowser.get()
	#webController.open(url)
	return

	# old version
	from PyQt4 import QtWebKit
	global maxWebWindows,webWindows
	reuseLast=False
	# use the last window if the class is the same and only the attribute differs
	try:
		reuseLast=(len(webWindows)>0 and str(webWindows[-1].url()).split('#')[-1].split('.')[2]==url.split('#')[-1].split('.')[2])
		#print str(webWindows[-1].url()).split('#')[-1].split('.')[2],url.split('#')[-1].split('.')[2]
	except: pass
	if not reuseLast:
		if len(webWindows)<maxWebWindows: webWindows.append(QtWebKit.QWebView())
		else: webWindows=webWindows[1:]+[webWindows[0]]
	web=webWindows[-1]
	web.load(QUrl(url)); web.setWindowTitle(url);
	if 0:
		def killSidebar(result):
			frame=web.page().mainFrame()
			frame.evaluateJavaScript("var bv=$('.bodywrapper'); bv.css('margin','0 0 0 0');")
			frame.evaluateJavaScript("var sbw=$('.sphinxsidebarwrapper'); sbw.css('display','none');")
			frame.evaluateJavaScript("var sb=$('.sphinxsidebar'); sb.css('display','none'); ")
			frame.evaluateJavaScript("var sb=$('.sidebar'); sb.css('width','0px'); ")
			web.loadFinished.disconnect(killSidebar)
		web.loadFinished.connect(killSidebar)
	web.show();	web.raise_()


controller=None

class ControllerClass(QWidget,Ui_Controller):
	#from woo.gl import *
	#from woo import gl
	def __init__(self,parent=None):
		QWidget.__init__(self)
		self.setupUi(self)
		self.generator=None # updated automatically
		self.renderer=Renderer() # only hold one instance, managed by OpenGLManager
		self.genIndexLoad=-1
		self.genIndexCurr=-1
		self.addPreprocessors()
		self.addRenderers()
		self.movieButton.setEnabled(False)
		self.movieScene=None
		global controller
		controller=self
		self.setWindowTitle('Woo ('+woo.config.prettyVersion(lead=True)+(', debug' if woo.config.debug else '')+')')
		self.refreshTimer=QtCore.QTimer()
		self.refreshTimer.timeout.connect(self.refreshEvent)
		self.refreshTimer.start(200)
		self.stepLabel=self.iterLabel # name change
		self.stepPerSecTimeout=1 # how often to recompute the number of steps per second
		self.stepTimes,self.stepValues,self.stepPerSec=[],[],0 # arrays to keep track of the simulation speed
		self.dtEditUpdate=True # to avoid updating while being edited
		self.dtEdit.setEnabled(False)
		# show off with this one as well now
	def addPreprocessors(self):
		for c in woo.system.childClasses('Preprocessor'):
			self.generatorCombo.addItem(c)
		self.generatorCombo.insertSeparator(self.generatorCombo.count())
		# currently crashes...?
		self.generatorCombo.addItem('current simulation')
		self.genIndexCurr=self.generatorCombo.count()-1
		self.generatorCombo.addItem('load from file')
		self.genIndexLoad=self.generatorCombo.count()-1
	def addRenderers(self):
		from woo import gl
		self.displayCombo.addItem('Renderer'); afterSep=1
		for bc in [t for t in dir(gl) if t.endswith('Functor') ]:
			if afterSep>0: self.displayCombo.insertSeparator(10000); afterSep=0
			for c in woo.system.childClasses(bc) | set([bc]):
				try:
					inst=eval('gl.'+c+'()');
					if len(set(inst.dict().keys())-set(['label']))>0:
						self.displayCombo.addItem(c); afterSep+=1
				except (NameError,AttributeError): pass # functo which is not defined
	def inspectSlot(self):
		self.inspector=SimulationInspector(parent=None)
		self.inspector.show()
	def setTabActive(self,what):
		if what=='simulation': ix=0
		elif what=='display': ix=1
		elif what=='generator': ix=2
		elif what=='python': ix=3
		else: raise ValueErorr("No such tab: "+what)
		self.controllerTabs.setCurrentIndex(ix)
	def generatorComboSlot(self,genStr):
		"update generator parameters when a new one is selected"
		ix=self.generatorCombo.currentIndex()
		if ix==self.genIndexLoad:
			f=str(QFileDialog.getOpenFileName(self,'Load preprocessor from','.'))
			if not f:
				self.generator=None
				self.generatorArea.setWidget(QLabel('<i>No preprocessor loaded</i>'))
			else:
				self.generator=Preprocessor.load(f)
				self.generatorCombo.setItemText(self.genIndexLoad,'%s'%f)
		elif ix==self.genIndexCurr:
			# force deep-copy, to avoid bug #88
			self.generator=woo.master.scene.pre.deepcopy()
			if not self.generator:
				self.generatorArea.setWidget(QLabel('<i>No preprocessor in Scene.pre</i>'))
		else:
			if self.genIndexLoad>=0: self.generatorCombo.setItemText(self.genIndexLoad,'load')
			self.generator=eval('woo.pre.'+str(genStr)+'()')
		if self.generator:
			se=SerializableEditor(self.generator,parent=self.generatorArea,showType=True,labelIsVar=False,showChecks=True,showUnits=True) # TODO
			self.generatorArea.setWidget(se)
		#else:
		#	self.generatorArea.setWidget(QFrame(self))
	#def pythonComboSlot(self,cmd):
	#	try:
	#		code=compile(str(cmd),'<UI entry>','exec')
	#		exec code in globals()
	#	except:
	#		import traceback
	#		traceback.print_exc()
	def genSaveParamsSlot(self):
		if 0:
			formats=[('expr','Text, loadable (*.expr)'),('pickle','Pickle, loadable (*.pickle)'),('xml','XML (loadable)'),('HTML (write-only)','html')]
			dialog=QFileDialog(None,'Save preprocessor parameters to','.',';; '.join([fmt[1] for fmt in formats]))
			dialog.setAcceptMode(QFileDialog.AcceptSave)
			f=dialog.exec_()
			print 'chosen file',str(dialog.selectedFiles()[0])
			if not f: return # cancelled
		else:
			f=str(QFileDialog.getSaveFileName(self,'Save preprocessor parameters','.'))
			if not f: return
			self.generator.dump(f,format='expr')
	def generateSlot(self):
		try:
			QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)
			out=str(self.generatorFilenameEdit.text())
			mem=self.genMemoryCombo.currentIndex()==0
			auto=self.genOpenCheckbox.isChecked()
			pre=self.generator
			newScene=pre()
			if mem: newScene.saveTmp(out)
			elif out: newScene.save(out)
			else: pass # only generate, don't save
			if auto:
				if woo.master.scene.running:
					import sys
					sys.stdout.write('Stopping the current simulation...')
					woo.master.scene.stop()
					print ' ok'
				woo.master.scene=newScene
				controller.setTabActive('simulation')
		except Exception as e:
			import traceback
			traceback.print_exc()
			showExceptionDialog(self,e)
		finally:
			QApplication.restoreOverrideCursor()
	def movieCheckboxToggled(self,isOn):
		if isOn:
			# add SnapshotEngine to the current scene
			S=self.movieScene=woo.master.scene
			snap=woo.qt.SnapshotEngine(label='_snapshooter',fileBase=woo.master.tmpFilename(),stepPeriod=100,realPeriod=.5,ignoreErrors=False)
			S.engines=S.engines+[snap]
			se=SerializableEditor(snap,parent=self.movieArea,showType=True)
			self.movieArea.setWidget(se)
			# open new view if there is none			
			if len(views())==0: View()
			self.movieButton.setEnabled(True)
		else:
			self.movieScene=None
			if hasattr(woo,'_snapshooter'): del woo._snapshooter
			woo.master.scene.engines=[e for e in woo.master.scene.engines if type(e)!=woo.qt.SnapshotEngine]
			self.movieArea.setWidget(QFrame())
			self.movieButton.setEnabled(False)
	def movieButtonClicked(self):
		if not hasattr(woo,'_snapshooter'):
			print 'No woo._snapshooter, no movie will be created'
			return
		out=self.movieFileEdit.text()
		woo.utils.makeVideo(woo._snapshooter.snapshots,out=out,fps=self.movieFpsSpinbox.value(),kbps=self.movieBitrateSpinbox.value())
		import os.path
		url='file://'+os.path.abspath(out)
		print 'Video saved to',url
		import webbrowser
		webbrowser.open(url)
	def displayComboSlot(self,dispStr):
		from woo import gl
		ser=(self.renderer if dispStr=='Renderer' else eval('gl.'+str(dispStr)+'()'))
		path='woo.gl.'+dispStr
		se=SerializableEditor(ser,parent=self.displayArea,ignoredAttrs=set(['label']),showType=True,path=path)
		self.displayArea.setWidget(se)
	def loadSlot(self):
		f=QFileDialog.getOpenFileName(self,'Load simulation','')
		f=str(f)
		if not f: return # cancelled
		self.deactivateControls()
		woo.master.scene=Scene.load(f)
	def saveSlot(self):
		f=QFileDialog.getSaveFileName(self,'Save simulation','')
		f=str(f)
		if not f: return # cancelled
		woo.master.scene.save(f)
		import os.path
		assert os.path.exists(f),"File '%s' does not exist after saving?"%f
	def reloadSlot(self):
		self.deactivateControls()
		from woo import plot
		plot.splitData()
		woo.master.reload()
	## FIXME: this does not seem to work. Disabled setting dt from the gui until fixed
	def dtEditNoupdateSlot(self):
		#print 'dt not refreshed'
		self.dtEditUpdate=False
	def dtEditedSlot(self):
		#print 'dt refreshed'
		try:
			t=float(self.dtEdit.text())
			woo.master.scene.dt=t
		except ValueError: pass
		self.dtEdit.setText(str(woo.master.scene.dt))
		self.dtEditUpdate=True
	def playSlot(self):	woo.master.scene.run()
	def pauseSlot(self): woo.master.scene.stop()
	def stepSlot(self):
		S=woo.master.scene
		if self.multiStepSpinBox.value()==1 or S.subStepping: S.one()
		else:
			S.run() # don't block with multistep
			S.stopAtStep=S.step+self.multiStepSpinBox.value()
	def subStepSlot(self,value):
		self.multiStepSpinBox.setEnabled(not bool(value))
		woo.master.scene.subStepping=bool(value)
	def show3dSlot(self, show):
		vv=views()
		assert(len(vv) in (0,1))
		if show:
			if len(vv)==0: View()
		else:
			if len(vv)>0: vv[0].close()
	def setReferenceSlot(self):
		woo.gl.Gl1_DemField.updateRefPos=True
	def centerSlot(self):
		for v in views(): v.center()
	def setViewAxes(self,dir,up):
		try:
			v=views()[0]
			v.viewDir=dir
			v.upVector=up
			v.center()
		except IndexError: pass
	def xyzSlot(self): self.setViewAxes((0,0,-1),(0,1,0))
	def yzxSlot(self): self.setViewAxes((-1,0,0),(0,0,1))
	def zxySlot(self): self.setViewAxes((0,-1,0),(1,0,0))
	def refreshEvent(self):
		self.refreshValues()
		self.activateControls()
	def deactivateControls(self):
		self.realTimeLabel.setText('')
		self.virtTimeLabel.setText('')
		self.stepLabel.setText('')
		self.fileLabel.setText('<i>[loading]</i>')
		self.playButton.setEnabled(False)
		self.pauseButton.setEnabled(False)
		self.stepButton.setEnabled(False)
		self.subStepCheckbox.setEnabled(False)
		self.reloadButton.setEnabled(False)
		self.dtEdit.setEnabled(False)
		self.dtEditUpdate=True
	def activateControls(self):
		S=woo.master.scene
		hasSim=len(S.engines)>0
		running=S.running
		if hasSim:
			self.playButton.setEnabled(not running)
			self.pauseButton.setEnabled(running)
			self.reloadButton.setEnabled(S.lastSave is not None)
			self.stepButton.setEnabled(not running)
			self.subStepCheckbox.setEnabled(not running)
		else:
			self.playButton.setEnabled(False)
			self.pauseButton.setEnabled(False)
			self.reloadButton.setEnabled(False)
			self.stepButton.setEnabled(False)
			self.subStepCheckbox.setEnabled(False)
		## FIXME: dt editing broken, never enable editable
		#self.dtEdit.setEnabled(True)
		self.dtEdit.setEnabled(False)
		fn=S.lastSave
		self.fileLabel.setText(fn if fn else '<i>[no file]</i>')

	def refreshValues(self):
		scene=woo.master.scene
		rt=int(woo.master.realtime); t=scene.time; step=scene.step;
		assert(len(self.stepTimes)==len(self.stepValues))
		if len(self.stepTimes)==0: self.stepTimes.append(rt); self.stepValues.append(step); self.stepPerSec=0 # update always for the first time
		elif rt-self.stepTimes[-1]>self.stepPerSecTimeout: # update after a timeout
			if len(self.stepTimes)==1: self.stepTimes.append(self.stepTimes[0]); self.stepValues.append(self.stepValues[0]) # 2 values, first one is bogus
			self.stepTimes[0]=self.stepTimes[1]; self.stepValues[0]=self.stepValues[1]
			self.stepTimes[1]=rt; self.stepValues[1]=step;
			self.stepPerSec=(self.stepValues[-1]-self.stepValues[-2])/(self.stepTimes[-1]-self.stepTimes[-2])
		if not scene.running: self.stepPerSec=0
		stopAtStep=scene.stopAtStep
		subStepInfo=''
		if scene.subStepping:
			subStep=scene.subStep
			if subStep==-1: subStepInfo=u'→ <i>prologue</i>'
			elif subStep>=0 and subStep<len(scene.engines):
				e=scene.engines[subStep]; subStepInfo=u'→ %s'%(e.label if e.label else e.__class__.__name__)
			elif subStep==len(scene.engines): subStepInfo=u'→ <i>epilogue</i>'
			else: raise RuntimeError("Invalid scene.subStep value %d, should be ∈{-1,…,len(o.engines)}"%subStep)
			subStepInfo="<br><small>sub %d/%d [%s]</small>"%(subStep,len(scene.engines),subStepInfo)
		self.subStepCheckbox.setChecked(scene.subStepping) # might have been changed async
		if stopAtStep<=step:
			self.realTimeLabel.setText('%02d:%02d:%02d'%(rt//3600,(rt%3600)//60,rt%60))
			self.stepLabel.setText('#%ld, %.1f/s %s'%(step,self.stepPerSec,subStepInfo))
		else:
			if self.stepPerSec!=0:
				e=int((stopAtStep-step)/self.stepPerSec)
				eta='(ETA %02d:%02d:%02d)'%(e//3600,e//60,e%60)
			else: eta=u'(ETA −)'
			self.realTimeLabel.setText('%02d:%02d:%02d %s'%(rt//3600,rt//60,rt%60,eta))
			self.stepLabel.setText('#%ld / %ld, %.1f/s %s'%(scene.step,stopAtStep,self.stepPerSec,subStepInfo))
		if t!=float('inf') and t!=float('nan'):
			s=int(t); ms=int(t*1000)%1000; us=int(t*1000000)%1000; ns=int(t*1000000000)%1000
			self.virtTimeLabel.setText(u'%03ds%03dm%03dμ%03dn'%(s,ms,us,ns))
		else: self.virtTimeLabel.setText(u'[ ∞ ] ?!')
		## FIXME:
		##if self.dtEditUpdate:
		self.dtEdit.setText(str(scene.dt))
		self.show3dButton.setChecked(len(views())>0)
		##
		if self.movieScene:
			if self.movieScene!=woo.master.scene:
				self.movieCheckboxToggled(True) # as if it was clicked now

		
def Generator():
	global controller
	if not controller: controller=ControllerClass();
	controller.show(); controller.raise_()
	controller.setTabActive('generator')
def Controller():
	global controller
	if not controller: controller=ControllerClass();
	controller.show(); controller.raise_()
	controller.setTabActive('simulation')
def Inspector():
	global controller
	if not controller: controller=ControllerClass();
	controller.inspectSlot()
