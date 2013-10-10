# encoding: utf-8


# signal import error witout display
if 1:
	import woo.runtime, wooMain
	if wooMain.options.forceNoGui:
		woo.runtime.hasDisplay=False
		raise ImportError("Woo was started with the -n switch; woo.qt4 insterface will not be enabled.")
	# ignore some warnings from Xlib
	import warnings
	warnings.filterwarnings('ignore',category=DeprecationWarning,module='Xlib')

	import sys
	if sys.platform=='win32':
		# assume display is always available at Windows
		woo.runtime.hasDisplay=True
	else:
		import Xlib.display
		# PyQt4's QApplication does exit(1) if it is unable to connect to the display
		# we however want to handle this gracefully, therefore
		# we test the connection with bare xlib first, which merely raises DisplayError
		try:
			# contrary to display.Display, _BaseDisplay does not check for extensions and that avoids spurious message "Xlib.protocol.request.QueryExtension" (bug?)
			Xlib.display._BaseDisplay();
			woo.runtime.hasDisplay=True
		except: 
			# usually Xlib.error.DisplayError, but there can be Xlib.error.XauthError etc as well
			# let's just pretend any exception means the display would not work
			woo.runtime.hasDisplay=False
			raise ImportError("Connecting to $DISPLAY failed, unable to activate the woo.qt4 interface.")
			

#import woo.runtime
#if not woo.runtime.hasDisplay:

useQtConsole=False # does not work yet
wooQApp=None
if 1:
	# API compatibility for PySide vs. PyQt
	# must be called before PyQt4 is ever imported
	# don't do that if someone imported PyQt4/PySide already
	# it would lead to crash
	import sys
	if woo.runtime.hasDisplay and ('PyQt4' not in sys.modules) and ('PySide' not in sys.modules):
		if woo.runtime.ipython_version>=11:
			import IPython.external.qt
			IPython.external.qt.prepare_pyqt4()

if 1: # initialize QApplication
	from PyQt4 import QtGui
	if woo.runtime.ipython_version==10:
		wooQApp=QtGui.QApplication(sys.argv)
	elif useQtConsole:
		from IPython.frontend.qt.console.qtconsoleapp import IPythonQtConsoleApp
		wooQApp=IPythonQtConsoleApp()
		wooQApp.initialize()
	else:
		# create an instance of InteractiveShell before the inputhook is created
		# see http://stackoverflow.com/questions/9872517/how-to-embed-ipython-0-12-so-that-it-inherits-namespace-of-the-caller for details
		# fixes http://gpu.doxos.eu/trac/ticket/40
		from IPython.frontend.terminal.embed import InteractiveShellEmbed
		from IPython.config.configurable import MultipleInstanceError
		try: ipshell=InteractiveShellEmbed.instance()
		except MultipleInstanceError:
			print 'Already running inside ipython, not embedding new instance.'
		# keep the qapp object referenced, otherwise 0.11 will die with "QWidget: Must construct QApplication before a QPaintDevice
		# see also http://ipython.org/ipython-doc/dev/interactive/qtconsole.html#qt-and-the-qtconsole
		import IPython.lib.inputhook #guisupport
		wooQApp=IPython.lib.inputhook.enable_gui(gui='qt4')
		#from IPython.lib.guisupport import start_event_loop_qt4
		#start_event_loop_qt4(wooQApp)
	#try:
	#	wooQApp.setStyleSheet(open(woo.config.resourceDir+'/qmc2-black-0.10.qss').read())
	#except IOError: pass # stylesheet not readable or whatever
	if sys.platform=='win32': 
		# don't use ugly windows theme, try something else
		for style in QtGui.QStyleFactory.keys():
			# the first one will be used
			if  style in ('Cleanlooks','Plastique'):
				QtGui.QApplication.setStyle(QtGui.QStyleFactory.create(style))
				QtGui.QApplication.setPalette(QtGui.QApplication.style().standardPalette())
				break
		
	
from PyQt4.QtGui import *
from PyQt4 import QtCore


from woo.qt.ui_controller import Ui_Controller

from woo.qt.Inspector import *
from woo import *
import woo.system, woo.config

try:
	from woo._qt import *
	from woo._qt._GLViewer import *
	OpenGL=True
	#import woo._qt._GLViewer, woo._qt
	#print woo
	# document those with woo.qt
	import _GLViewer
	_docInlineModules=(woo._qt,_GLViewer)
except ImportError:
	OpenGL=False
	if 'opengl' in woo.config.features: raise RuntimeError("Woo was compiled with the 'opengl' feature, but OpenGL modules (woo._qt, woo._qt._GLViewer) could not be imported?")

from ExceptionDialog import *

#maxWebWindows=1
#"Number of webkit windows that will be cycled to show help on clickable objects"
#"holds instances of QtWebKit windows; clicking an url will open it in the window that was the least recently updated"
#webWindows=[] 
#webController=None

def onSelection(obj):
	print 'woo.qt.onSelection:',obj,'was selected'
	print '   Set woo.gl.Renderer.selFunc to your own callback function.'

def openUrl(url):
	import webbrowser
	webbrowser.open(url)
	return

controller=None

class ControllerClass(QWidget,Ui_Controller):
	#from woo.gl import *
	#from woo import gl
	def __init__(self,parent=None):
		QWidget.__init__(self)
		self.setupUi(self)
		self.generator=None # updated automatically
		if OpenGL:
			self.renderer=Renderer() # only hold one instance, managed by OpenGLManager
			self.addRenderers()
		self.genIndexLoad=-1
		self.genIndexCurr=-1
		self.genChecks,self.genVars=False,False
		self.refreshPreprocessors()
		self.fillAboutData()
		self.generatorComboSlot(None)
		self.movieButton.setEnabled(False)
		self.lastScene=None
		self.traceCheckboxToggled(isOn=None) # detect on/off
		self.movieActive=False
		self.movieFileEdit.setText(woo.utils.fixWindowsPath(str(self.movieFileEdit.text()))) # fix /tmp on windows
		self.tracerActive=False
		self.inspector=None
		global controller
		controller=self
		self.setWindowTitle('Woo ('+woo.config.prettyVersion(lead=True)+(', debug' if woo.config.debug else '')+')')
		self.refreshTimer=QtCore.QTimer()
		self.refreshTimer.timeout.connect(self.refreshEvent)
		self.refreshTimer.start(200)
		self.stepLabel=self.iterLabel # name change
		self.stepPerSecTimeout=1 # how often to recompute the number of steps per second
		self.stepTimes,self.stepValues,self.stepPerSec=[],[],0 # arrays to keep track of the simulation speed
		#
		if not OpenGL:
			self.show3dButton.setText("[no OpenGL]")
			self.show3dButton.setEnabled(False)
		# show off with this one as well now
	def refreshPreprocessors(self):
		self.generatorCombo.clear()
		self.preprocessorObjects=[]
		# import all preprocessors
		import pkgutil, woo.pre, sys
		for importer,modname,ispkg in pkgutil.iter_modules(woo.pre.__path__):
			try: __import__('woo.pre.'+modname,fromlist='woo.pre')
			except:
				# this error is spurious with frozen installs, but probably meaningful otherwise
				# so we silently skip over when frozen
				if not hasattr(sys,'frozen'): print "(Error importing woo.pre."+modname+", ignoring.)"	
		#
		preps=[]
		for c in woo.system.childClasses(woo.core.Preprocessor):
			preps.append((c.__module__,c.__name__,c))
		preps.sort()
		for pre in preps:
			self.generatorCombo.addItem(pre[1]+('' if pre[0]=='woo.pre' else ' ('+pre[0]+')'))
			self.preprocessorObjects.append(pre[2])
		self.generatorCombo.insertSeparator(self.generatorCombo.count())
		# currently crashes...?
		self.generatorCombo.addItem('current simulation')
		self.genIndexCurr=self.generatorCombo.count()-1
		self.generatorCombo.addItem('load from file')
		self.genIndexLoad=self.generatorCombo.count()-1
		# refresh the view
		self.generatorComboSlot(0)
	def addRenderers(self):
		from woo import gl
		self.displayCombo.addItem('Renderer'); afterSep=1
		# put GlFieldFunctor at the beginning artificially
		for bc in ['GlFieldFunctor']+[t for t in dir(gl) if t.endswith('Functor') and t!='GlFieldFunctor']:
			if afterSep>0: self.displayCombo.insertSeparator(10000); afterSep=0
			for c in woo.system.childClasses(bc) | set([bc]):
				try:
					inst=eval('gl.'+c+'()');
					if len(set(inst.dict().keys())-set(['label']))>0:
						self.displayCombo.addItem(c); afterSep+=1
				except (NameError,AttributeError): pass # functo which is not defined
	def fillAboutData(self):	
		import woo, woo.config, platform, textwrap, types, pkg_resources, collections
		extras=[]
		try:
			import wooExtra
			ExInfo=collections.namedtuple('ExInfo','name, mod, version, distributor')
			for exName in dir(wooExtra):
				mod=getattr(wooExtra,exName)
				if type(mod)!=types.ModuleType: continue
				modName=mod.__name__
				ver=pkg_resources.get_distribution(modName).version
				distributor=unicode(getattr(mod,'distributor') if hasattr(mod,'distributor') else u'−')
				extras.append(ExInfo(name=exName,mod=mod,version=ver,distributor=distributor))
		except ImportError: pass # no wooExtra modules installed
		self.aboutGeneralLabel.setText('''<h4>System data</h4><table cellpadding='2px' rules='all' width='100%'>
			<tr><td>user</td><td>{user}</td></tr>
			<tr><td>cores</td><td>{nCores}</td></tr>
			<tr><td>version</td><td>{version} ({buildDate}{flavor})</td></tr>
			<tr><td>platform</td><td>{platform}</td></tr>
			<tr><td>features&nbsp;</td><td>{features}</td></tr>
			<tr><td>extras</td><td>{extraModules}</td></tr>
		</table>
		'''.format(user=woo.master.scene.tags['user'].decode('utf-8'),nCores=woo.master.numThreads,platform='<br>'.join(textwrap.wrap(platform.platform().replace('-',' '),40)),version=woo.config.version+'/'+woo.config.revision+(' (debug)' if woo.config.debug else ''),features=', '.join(woo.config.features),buildDate=woo.config.buildDate,flavor=((', flavor "'+woo.config.flavor+'"') if woo.config.flavor else ''),extraModules='<br>'.join(['{e.name} ({e.version})'.format(e=e) for e in extras])))
		if extras:
			self.aboutExtraLabel.setText(u"<h4>Extra modules</h4><table cellpadding='10px' width='100%'>"+u''.join([u'<tr><td>wooExtra.<b>{e.name}&nbsp;</b><br>{e.version}</td><td>{e.distributor}</td></tr>'.format(e=e) for e in extras])+'</table')
	def inspectSlot(self):
		if not self.inspector:
			self.inspector=SimulationInspector(parent=None)
			self.inspector.show()
		else:
			#self.inspector.hide()
			self.inspector=None
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
			if not woo.master.scene.pre:
				self.generatorArea.setWidget(QLabel('<i>No preprocessor in Scene.pre</i>'))
				self.generator=None
			else:
				# force deep-copy, to avoid bug #88
				self.generator=woo.master.scene.pre.deepcopy()
		else:
			if self.genIndexLoad>=0: self.generatorCombo.setItemText(self.genIndexLoad,'load')
			self.generator=(self.preprocessorObjects[ix]() if ix<len(self.preprocessorObjects) else None)
		if self.generator:
			# keep var/check settings
			w=self.generatorArea.widget()
			if w and isinstance(w,ObjectEditor): self.genVars,self.genChecks=w.labelIsVar,w.showChecks
			se=ObjectEditor(self.generator,parent=self.generatorArea,showType=True,labelIsVar=self.genVars,showChecks=self.genChecks,showUnits=True,objManip=True) # TODO
			self.generatorArea.setWidget(se)
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
			self.generator.dump(f,format='json' if f.endswith('.json') else 'expr')
	def generateSlot(self):
		try:
			QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)
			out=str(self.generatorFilenameEdit.text())
			mem=self.genMemoryCombo.currentIndex()==0
			auto=self.genOpenCheckbox.isChecked()
			pre=self.generator
			newScene=pre()
			if not isinstance(newScene,woo.core.Scene): raise RuntimeError('Preprocessor must return a Scene when called, not '+str(newScene))
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
		S=woo.master.scene
		if isOn:
			# add SnapshotEngine to the current scene
			snap=woo.qt.SnapshotEngine(label='snapshooter_',fileBase=woo.master.tmpFilename(),stepPeriod=100,realPeriod=.5,ignoreErrors=False)
			S.engines=S.engines+[snap]
			se=ObjectEditor(snap,parent=self.movieArea,showType=True)
			self.movieArea.setWidget(se)
			# open new view if there is none			
			if len(views())==0: View()
			self.movieButton.setEnabled(True)
			self.movieActive=True
		else:
			if hasattr(S.lab,'snapshooter_'): del S.lab.snapshooter_
			woo.master.scene.engines=[e for e in S.engines if type(e)!=woo.qt.SnapshotEngine]
			self.movieArea.setWidget(QFrame())
			self.movieButton.setEnabled(False)
			self.movieActive=False
	def movieButtonClicked(self):
		S=woo.master.scene
		if not hasattr(S.lab,'snapshooter_'):
			print 'No S.lab.snapshooter_, no movie will be created'
			return
		out=str(self.movieFileEdit.text())
		woo.utils.makeVideo(S.lab.snapshooter_.snapshots,out=out,fps=self.movieFpsSpinbox.value(),kbps=self.movieBitrateSpinbox.value())
		import os.path
		url='file://'+os.path.abspath(out)
		print 'Video saved to',url
		import webbrowser
		webbrowser.open(url)
	def resetTraceClicked(self):
		S=woo.master.scene
		if not hasattr(S.lab,'tracer_'):
			print 'No S.lab.tracer_, will not reset'
			return
		S.lab.tracer_.resetNodesRep(setupEmpty=True)
	def tracerGetEngine(self,S):
		'Find tracer engine, return it, or return None; raise exception with multiple tracers'
		tt=[e for e in S.engines if isinstance(e,woo.dem.Tracer)]
		if len(tt)>1: raise RuntimeError('Multiple tracer engines in scene?')
		if len(tt)==1: return tt[0]
		return None
	def traceCheckboxToggled(self,isOn):
		S=woo.master.scene
		# auto-detect on/off
		if isOn==None:
			tr=self.tracerGetEngine(woo.master.scene)
			isOn=bool(tr and not tr.dead)
			# will call this function again, and also set the checkbox state properly
			self.traceCheckbox.setChecked(isOn) 
			return 
		# go ahead now
		if isOn:
			# is there a tracer already?
			tracer=self.tracerGetEngine(S)
			if not tracer:
				tracer=woo.dem.Tracer(label='tracer_',stepPeriod=100,realPeriod=.2)
				S.engines=S.engines+[tracer]
			tracer.dead=False
			S.ranges=S.ranges+[woo.dem.Tracer.lineColor]
			self.tracerArea.setWidget(ObjectEditor(tracer,parent=self.tracerArea,showType=True))
			self.resetTraceButton.setEnabled(True)
			woo.gl.Gl1_DemField.glyph=woo.gl.Gl1_DemField.glyphKeep
			self.tracerActive=True
		else:
			tracer=self.tracerGetEngine(S)
			if tracer:
				tracer.dead=True
				#hasattr(woo,'_tracer'):
				tracer.resetNodesRep(setupEmpty=False)
				#del woo._tracer
			#S.engines=[e for e in S.engines if type(e)!=woo.dem.Tracer]
			S.ranges=[r for r in S.ranges if r!=woo.dem.Tracer.lineColor]
			self.tracerArea.setWidget(QFrame())
			self.resetTraceButton.setEnabled(False)
			self.tracerActive=False
		
	def displayComboSlot(self,dispStr):
		from woo import gl
		ser=(self.renderer if dispStr=='Renderer' else eval('gl.'+str(dispStr)+'()'))
		path='woo.gl.'+dispStr
		se=ObjectEditor(ser,parent=self.displayArea,ignoredAttrs=set(['label']),showType=True,path=path)
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
		p=woo.master.scene.plot
		p.splitData()
		woo.master.scene.stop()
		woo.master.reload()
		# assign including previous data
		# this should perhaps not be the default?
		woo.master.scene.plot=p 
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
	def plotSlot(self):
		woo.master.scene.plot.plot()
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
	def activateControls(self):
		S=woo.master.scene
		hasSim=len(S.engines)>0
		running=S.running
		if hasSim:
			self.playButton.setEnabled(not running)
			self.pauseButton.setEnabled(running)
			self.reloadButton.setEnabled(bool(S.lastSave))
			self.stepButton.setEnabled(not running)
			self.subStepCheckbox.setEnabled(not running)
		else:
			self.playButton.setEnabled(False)
			self.pauseButton.setEnabled(False)
			self.reloadButton.setEnabled(False)
			self.stepButton.setEnabled(False)
			self.subStepCheckbox.setEnabled(False)
		self.traceCheckbox.setEnabled(S.hasDem)
		fn=S.lastSave
		self.fileLabel.setText(fn if fn else '[no file]')

		#
		#
		self.plotButton.setEnabled(len(S.plot.plots)>0)

	def refreshValues(self):
		S=woo.master.scene
		rt=int(woo.master.realtime); t=S.time; step=S.step;
		assert(len(self.stepTimes)==len(self.stepValues))
		if len(self.stepTimes)==0: self.stepTimes.append(rt); self.stepValues.append(step); self.stepPerSec=0 # update always for the first time
		elif rt-self.stepTimes[-1]>self.stepPerSecTimeout: # update after a timeout
			if len(self.stepTimes)==1: self.stepTimes.append(self.stepTimes[0]); self.stepValues.append(self.stepValues[0]) # 2 values, first one is bogus
			self.stepTimes[0]=self.stepTimes[1]; self.stepValues[0]=self.stepValues[1]
			self.stepTimes[1]=rt; self.stepValues[1]=step;
			self.stepPerSec=(self.stepValues[-1]-self.stepValues[-2])/(self.stepTimes[-1]-self.stepTimes[-2])
		if not S.running: self.stepPerSec=0
		stopAtStep=S.stopAtStep
		subStepInfo=''
		if S.subStepping:
			subStep=S.subStep
			if subStep==-1: subStepInfo=u'→ <i>prologue</i>'
			elif subStep>=0 and subStep<len(S.engines):
				e=S.engines[subStep]; subStepInfo=u'→ %s'%(e.label if e.label else e.__class__.__name__)
			elif subStep==len(S.engines): subStepInfo=u'→ <i>epilogue</i>'
			else: raise RuntimeError("Invalid S.subStep value %d, should be ∈{-1,…,len(o.engines)}"%subStep)
			subStepInfo="<br><small>sub %d/%d [%s]</small>"%(subStep,len(S.engines),subStepInfo)
		self.subStepCheckbox.setChecked(S.subStepping) # might have been changed async
		if stopAtStep<=step:
			self.realTimeLabel.setText('%02d:%02d:%02d'%(rt//3600,(rt%3600)//60,rt%60))
			self.stepLabel.setText('#%ld, %.1f/s %s'%(step,self.stepPerSec,subStepInfo))
		else:
			if self.stepPerSec!=0:
				e=int((stopAtStep-step)/self.stepPerSec)
				eta='(ETA %02d:%02d:%02d)'%(e//3600,e//60,e%60)
			else: eta=u'(ETA −)'
			self.realTimeLabel.setText('%02d:%02d:%02d %s'%(rt//3600,rt//60,rt%60,eta))
			self.stepLabel.setText('#%ld / %ld, %.1f/s %s'%(S.step,stopAtStep,self.stepPerSec,subStepInfo))
		if t!=float('inf') and t!=float('nan'):
			s=int(t); ms=int(t*1000)%1000; us=int(t*1000000)%1000; ns=int(t*1000000000)%1000
			self.virtTimeLabel.setText(u'%03ds%03dm%03dμ%03dn'%(s,ms,us,ns))
		else: self.virtTimeLabel.setText(u'[ ∞ ] ?!')
		self.dtLabel.setText(str(S.dt))
		if OpenGL: self.show3dButton.setChecked(len(views())>0)
		self.inspectButton.setChecked(self.inspector!=None)
		# import sys
		# sys.stderr.write(str(self.lastScene)+'/'+str(S)+'\n')
		##
		if self.lastScene!=S:
			self.lastScene=S
			# set movie and trace using last state
			if OpenGL:
				self.movieCheckboxToggled(self.movieActive)
				self.traceCheckboxToggled(isOn=None)
			#
			#
			ix=self.simPageLayout.indexOf(self.customArea)
			row,col,rows,cols=self.simPageLayout.getItemPosition(ix) # find position, so that we know where to place the next one
			self.simPageLayout.removeWidget(self.customArea)
			self.customArea.setParent(None)
			self.customArea=QScrollArea()
			sp=QSizePolicy(QSizePolicy.Expanding,QSizePolicy.Expanding); sp.setVerticalStretch(40)
			self.customArea.setSizePolicy(sp)
			self.simPageLayout.addWidget(self.customArea,row,col)
			#print 'New QScrollArea created'
			if S.uiBuild:
				import __main__
				glob=globals(); glob.update(__main__.__dict__)
				exec S.uiBuild in glob, {'S':S,'area':self.customArea}

		
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
