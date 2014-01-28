# encoding: utf-8
from PyQt4.QtCore import *
from PyQt4.QtGui import *


class ExceptionDialog(QMessageBox):
	def __init__(self,parent,exc,t1=None,t2=None):
		QMessageBox.__init__(self,parent)
		if t1==None: t1=(exc.args[0] if len(exc.args)>0 else None)
		self.setText(u'<b>'+exc.__class__.__name__+':</b><br>\n'+unicode(t1))
		#QMessageBox.setTitle(self,xc.__class__.__name__)
		import traceback
		tbRaw=traceback.format_exc()
		# newlines are already <br> after Qt.convertFromPlainText, discard to avoid empty lines
		tb='<small><pre>'+Qt.convertFromPlainText(tbRaw).replace('\n','')+'</pre></small>'
		self.setInformativeText(t2 if t2 else tb)
		self.setDetailedText(tbRaw)
		self.setIcon(QMessageBox.Critical)
		self.setStandardButtons(QMessageBox.Ok)
		self.setDefaultButton(QMessageBox.Ok)
		self.setEscapeButton(QMessageBox.Ok)

def showExceptionDialog(parent,exc,t1=None,t2=None):
	# event loop brokne, modal dialogs won't work
	# just show and don't care anymore
	ExceptionDialog(parent,exc).show()
	# import traceback
	# QMessageBox.critical(parent,exc.__class__.__name__,'<b>'+exc.__class__.__name__+':</b><br>'+exc.args[0]+'+<br><small><pre>'+Qt.convertFromPlainText((traceback.format_exc()))+'</pre></small>')


if __name__=='__main__':
	import sys
	qapp=QApplication(sys.argv)
	e=ValueError('123, 234, 345','asdsd')
	showExceptionDialog(None,e)
