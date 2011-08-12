# encoding: utf-8
import yade.runtime
if not yade.runtime.hasDisplay: raise ImportError("Connecting to DISPLAY at Yade startup failed, unable to activate the qt4 interface.")

from PyQt4.QtGui import *
from PyQt4 import QtCore

from yade.qt.ui_controller import Ui_Controller

from yade.qt.Inspector import *
from yade import *
import yade.system, yade.config

from yade.qt._GLViewer import *  # imports Renderer() as well

maxWebWindows=1
"Number of webkit windows that will be cycled to show help on clickable objects"
webWindows=[] 
"holds instances of QtWebKit windows; clicking an url will open it in the window that was the least recently updated"
sphinxOnlineDocPath='https://www.yade-dem.org/sphinx/'
"Base URL for the documentation. Packaged versions should change to the local installation directory."


import os.path
# find if we have docs installed locally from package
sphinxLocalDocPath=yade.config.prefix+'/share/doc/yade'+yade.config.suffix+'/html/'
sphinxBuildDocPath=yade.config.sourceRoot+'/doc/sphinx/_build/html/'
# we prefer the packaged documentation for this version, if installed
if   os.path.exists(sphinxLocalDocPath+'/index.html'): sphinxPrefix='file://'+sphinxLocalDocPath
# otherwise look for documentation generated in the source tree
elif  os.path.exists(sphinxBuildDocPath+'/index.html'): sphinxPrefix='file://'+sphinxBuildDocPath
# fallback to online docs
else: sphinxPrefix=sphinxOnlineDocPath

sphinxDocWrapperPage=sphinxPrefix+'/yade.wrapper.html'


## object selection
def getSel(): return Renderer().selObj




def openUrl(url):
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
	#from yade.gl import *
	#from yade import gl
	def __init__(self,parent=None):
		QWidget.__init__(self)
		self.setupUi(self)
		self.generator=None # updated automatically
		self.renderer=Renderer() # only hold one instance, managed by OpenGLManager
		self.addPreprocessors()
		self.addRenderers()
		global controller
		controller=self
		self.refreshTimer=QtCore.QTimer()
		self.refreshTimer.timeout.connect(self.refreshEvent)
		self.refreshTimer.start(200)
		self.stepLabel=self.iterLabel # name change
		self.stepPerSecTimeout=1 # how often to recompute the number of steps per second
		self.stepTimes,self.stepValues,self.stepPerSec=[],[],0 # arrays to keep track of the simulation speed
		self.dtEditUpdate=True # to avoid updating while being edited
		# show off with this one as well now
	def addPreprocessors(self):
		for c in yade.system.childClasses('FileGenerator'):
			self.generatorCombo.addItem(c)
	def addRenderers(self):
		#from yade.gl import *
		from yade import gl
		self.displayCombo.addItem('Renderer'); afterSep=1
		for bc in [t for t in dir(gl) if t.endswith('Functor') ]:
			if afterSep>0: self.displayCombo.insertSeparator(10000); afterSep=0
			for c in yade.system.childClasses(bc) | set([bc]):
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
		gen=eval(str(genStr)+'()')
		self.generator=gen
		se=SerializableEditor(gen,parent=self.generatorArea,showType=True)
		self.generatorArea.setWidget(se)
	def pythonComboSlot(self,cmd):
		try:
			code=compile(str(cmd),'<UI entry>','exec')
			exec code in globals()
		except:
			import traceback
			traceback.print_exc()
	def generateSlot(self):
		filename=str(self.generatorFilenameEdit.text())
		if self.generatorMemoryCheck.isChecked():
			filename=':memory:'+filename
			print 'BUG: Saving to memory slots freezes Yade (cause unknown). Cross fingers.'
		#print 'Will save to ',filename
		self.generator.generate(filename)
		if self.generatorAutoCheck:
			O.load(filename)
			self.setTabActive('simulation')
			if len(views())==0:
				v=View(); v.center()
	def displayComboSlot(self,dispStr):
		from yade import gl
		ser=(self.renderer if dispStr=='Renderer' else eval('gl.'+str(dispStr)+'()'))
		path='yade.qt.Renderer()' if dispStr=='Renderer' else dispStr
		se=SerializableEditor(ser,parent=self.displayArea,ignoredAttrs=set(['label']),showType=True,path=path)
		self.displayArea.setWidget(se)
	def loadSlot(self):
		f=QFileDialog.getOpenFileName(self,'Load simulation','','Yade simulations (*.xml *.xml.bz2 *.xml.gz *.yade *.yade.gz *.yade.bz2);; *.*')
		f=str(f)
		if not f: return # cancelled
		self.deactivateControls()
		O.load(f)
	def saveSlot(self):
		f=QFileDialog.getSaveFileName(self,'Save simulation','','Yade simulations (*.xml *.xml.bz2 *.xml.gz *.yade *.yade.gz *.yade.bz2);; *.*')
		f=str(f)
		if not f: return # cancelled
		O.save(f)
	def reloadSlot(self):
		self.deactivateControls()
		from yade import plot
		plot.splitData()
		O.reload()
	def dtEditNoupdateSlot(self):
		#print 'dt not refreshed'
		self.dtEditUpdate=False
	def dtEditedSlot(self):
		#print 'dt refreshed'
		try:
			t=float(self.dtEdit.text())
			O.scene.dt=t
		except ValueError: pass
		self.dtEdit.setText(str(O.scene.dt))
		self.dtEditUpdate=True
	def playSlot(self):	O.run()
	def pauseSlot(self): O.pause()
	def stepSlot(self):
		if self.multiStepSpinBox.value()==1 or O.scene.subStepping: O.step()
		else:
			O.run() # don't block with multistep
			O.scene.stopAtStep=O.scene.step+self.multiStepSpinBox.value()
			#O.run(self.multiStepSpinBox.value(),True) # wait and block until done
	def subStepSlot(self,value):
		self.multiStepSpinBox.setEnabled(not bool(value))
		O.scene.subStepping=bool(value)
	def show3dSlot(self, show):
		vv=views()
		assert(len(vv) in (0,1))
		if show:
			if len(vv)==0: View()
		else:
			if len(vv)>0: vv[0].close()
	def setReferenceSlot(self):
		# sets reference periodic cell as well
		utils.setRefSe3()
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
		hasSim=len(O.scene.engines)>0
		running=O.running
		if hasSim:
			self.playButton.setEnabled(not running)
			self.pauseButton.setEnabled(running)
			self.reloadButton.setEnabled(O.scene.lastSave is not None)
			self.stepButton.setEnabled(not running)
			self.subStepCheckbox.setEnabled(not running)
		else:
			self.playButton.setEnabled(False)
			self.pauseButton.setEnabled(False)
			self.reloadButton.setEnabled(False)
			self.stepButton.setEnabled(False)
			self.subStepCheckbox.setEnabled(False)
		self.dtEdit.setEnabled(True)
		fn=O.scene.lastSave
		self.fileLabel.setText(fn if fn else '<i>[no file]</i>')

	def refreshValues(self):
		scene=O.scene
		rt=int(O.realtime); t=scene.time; step=scene.step;
		assert(len(self.stepTimes)==len(self.stepValues))
		if len(self.stepTimes)==0: self.stepTimes.append(rt); self.stepValues.append(step); self.stepPerSec=0 # update always for the first time
		elif rt-self.stepTimes[-1]>self.stepPerSecTimeout: # update after a timeout
			if len(self.stepTimes)==1: self.stepTimes.append(self.stepTimes[0]); self.stepValues.append(self.stepValues[0]) # 2 values, first one is bogus
			self.stepTimes[0]=self.stepTimes[1]; self.stepValues[0]=self.stepValues[1]
			self.stepTimes[1]=rt; self.stepValues[1]=step;
			self.stepPerSec=(self.stepValues[-1]-self.stepValues[-2])/(self.stepTimes[-1]-self.stepTimes[-2])
		if not O.running: self.stepPerSec=0
		stopAtStep=scene.stopAtStep
		subStepInfo=''
		if scene.subStepping:
			subStep=scene.subStep
			if subStep==-1: subStepInfo=u'→ <i>prologue</i>'
			elif subStep>=0 and subStep<len(O.scene.engines):
				e=scene.engines[subStep]; subStepInfo=u'→ %s'%(e.label if e.label else e.__class__.__name__)
			elif subStep==len(scene.engines): subStepInfo=u'→ <i>epilogue</i>'
			else: raise RuntimeError("Invalid O.scene.subStep value %d, should be ∈{-1,…,len(o.engines)}"%subStep)
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
		if t!=float('inf'):
			s=int(t); ms=int(t*1000)%1000; us=int(t*1000000)%1000; ns=int(t*1000000000)%1000
			self.virtTimeLabel.setText(u'%03ds%03dm%03dμ%03dn'%(s,ms,us,ns))
		else: self.virtTimeLabel.setText(u'[ ∞ ] ?!')
		if self.dtEditUpdate: self.dtEdit.setText(str(scene.dt))
		self.show3dButton.setChecked(len(views())>0)
		
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
