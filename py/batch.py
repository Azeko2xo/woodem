# encoding: utf-8

def wait():
	'Block the simulation if running inside a batch. Typically used at the end of script so that it does not finish prematurely in batch mode (the execution would be ended in such a case).'
	if runningInBatch(): woo.master.scene.wait()
def inBatch():
	'Tell whether we are running inside the batch or separately.'
	import os
	return 'WOO_BATCH' in os.environ

def writeResults(defaultDb='woo-results.sqlite',**kw):
	# increase every time the db format changes, to avoid errors
	formatVersion=1
	import woo
	S=woo.master.scene
	import os
	import sqlite3
	if inBatch():
		table,line,db=os.environ['WOO_BATCH'].split(':')
	else:
		table,line,db='',-1,defaultDb
	newDb=not os.path.exists(db)
	print 'Writing results to the database %s (%s)'%(db,'new' if newDb else 'existing')
	conn=sqlite3.connect(db,detect_types=sqlite3.PARSE_DECLTYPES)
	if newDb:
		c=conn.cursor()
		c.execute('create table batch (formatNumber integer, finished timestamp, batchtable text, batchTableLine integer, sceneId text, title text, pre text, tags text, plotData text, plots text, custom text)')
	else:	
		conn.row_factory=sqlite3.Row
		conn.execute('select * from batch')
		row=conn.cursor().fetchone()
		# row can be None if the db is empty
		if row and row['formatVersion']!=formatVersion:
			raise RuntimeError('database format mismatch: %s/batch/formatVersion==%s, but should be %s'%(db,row['formatVersion'],formatVersion))
	with conn:
		import json
		import woo.plot
		import datetime
		import woo.core
		# make sure keys are unicode objects (which is what json converts to!)
		# but preserve values using a 8-bit encoding)
		# this sort-of sucks, hopefully there is a better solution soon
		d2=dict([(key,val.decode('iso-8859-1')) for key,val in S.tags.items()])
		values=(	
			formatVersion, # formatVersion
			datetime.datetime.now(), # finished
			table, # batchTable
			line, # batchTableLine
			S.tags['id'], # sceneId
			S.tags['title'], # title
			S.pre.dumps(format='json'), # pre
			json.dumps(d2), # tags
			json.dumps(woo.plot.data), # plotData
			json.dumps(woo.plot.plots), # plots
			woo.core.WooJSONEncoder(indent=None).encode(kw) # custom
		)
		conn.execute('insert into batch values (?,?,?,?,?, ?,?,?,?,?, ?)',values)
	

def readParamsFromTable(tableFileLine=None,noTableOk=True,unknownOk=False,**kw):
	"""
	Read parameters from a file and assign them to __builtin__ variables.

	The format of the file is as follows (commens starting with # and empty lines allowed)::

		# commented lines allowed anywhere
		name1 name2 … # first non-blank line are column headings
					# empty line is OK, with or without comment
		val1  val2  … # 1st parameter set
		val2  val2  … # 2nd
		…

	Assigned tags (the ``title`` column is synthesized if absent,see :ref:`woo.utils.TableParamReader`); 

		s=woo.master.scene
		s.tags['title']=…                                      # assigns the title column; might be synthesized
		s.tags['params']="name1=val1,name2=val2,…"                   # all explicitly assigned parameters
		s.tags['defaultParams']="unassignedName1=defaultValue1,…"    # parameters that were left at their defaults
		s.tags['d.id']=s.tags['id']+'.'+s.tags['title']
		s.tags['id.d']=s.tags['title']+'.'+s.tags['id']

	All parameters (default as well as settable) are saved using :ref:`woo.utils.saveVars`\ ``('table')``.

	:param tableFile: text file (with one value per blank-separated columns)
	:param int tableLine: number of line where to get the values from
	:param bool noTableOk: if False, raise exception if the file cannot be open; use default values otherwise
	:param bool unknownOk: do not raise exception if unknown column name is found in the file, and assign it as well
	:return: dictionary with all parameters
	"""
	tagsParams=[]
	# dictParams is what eventually ends up in woo.params.table (default+specified values)
	dictDefaults,dictParams={},{}
	import os, __builtin__,re,math
	s=woo.master.scene
	if not tableFileLine and ('WOO_BATCH' not in os.environ or os.environ['WOO_BATCH']==''):
		if not noTableOk: raise EnvironmentError("WOO_BATCH is not defined in the environment")
		s.tags['line']='l!'
	else:
		if not tableFileLine: tableFileLine=os.environ['WOO_BATCH']
		env=tableFileLine.split(':')
		tableFile,tableLine=env[0],int(env[1])
		allTab=TableParamReader(tableFile).paramDict()
		if not allTab.has_key(tableLine): raise RuntimeError("Table %s doesn't contain valid line number %d"%(tableFile,tableLine))
		vv=allTab[tableLine]
		s.tags['line']='l%d'%tableLine
		s.tags['title']=vv['title']
		s.tags['idt']=s.tags['id']+'.'+s.tags['title']; s.tags['tid']=s.tags['title']+'.'+s.tags['id']
		# assign values specified in the table to python vars
		# !something cols are skipped, those are env vars we don't treat at all (they are contained in title, though)
		for col in vv.keys():
			if col=='title' or col[0]=='!': continue
			if col not in kw.keys() and (not unknownOk): raise NameError("Parameter `%s' has no default value assigned"%col)
			if vv[col]=='*': vv[col]=kw[col] # use default value for * in the table
			elif vv[col]=='-': continue # skip this column
			elif col in kw.keys(): kw.pop(col) # remove the var from kw, so that it contains only those that were default at the end of this loop
			#print 'ASSIGN',col,vv[col]
			tagsParams+=['%s=%s'%(col,vv[col])];
			dictParams[col]=eval(vv[col],math.__dict__)
	# assign remaining (default) keys to python vars
	defaults=[]
	for k in kw.keys():
		dictDefaults[k]=kw[k]
		defaults+=["%s=%s"%(k,kw[k])];
	s.tags['defaultParams']=",".join(defaults)
	s.tags['params']=",".join(tagsParams)
	dictParams.update(dictDefaults)
	saveVars('table',loadNow=True,**dictParams)
	return dictParams
	#return len(tagsParams)


def runPreprocessor(pre,preFile=None):
	def nestedSetattr(obj,attr,val):
		attrs=attr.split(".")
		for i in attrs[:-1]:
			obj=getattr(obj,i)
		setattr(obj,attrs[-1],val)

	# just run preprocessor in this case
	if not inBatch(): return pre()

	import os
	import woo
	tableFileLine=os.environ['WOO_BATCH']
	if tableFileLine:
		env=tableFileLine.split(':')
		tableFile,tableLine=env[0],int(env[1])
		allTab=TableParamReader(tableFile).paramDict()
		if not tableLine in allTab: raise RuntimeError("Table %s doesn't contain valid line number %d"%(tableFile,tableLine))
		vv=allTab[tableLine]

		# set preprocessor parameters first
		for name,val in vv.items():
			if name=='title': continue
			if val in ('*','-'): continue
			nestedSetattr(pre,name,eval(val,globals(),dict(woo=woo))) # woo.unit
	# run preprocessor
	S=pre()
	# set tags from batch
	if tableFileLine:
		S.tags['line']='l%d'%tableLine
		S.tags['title']=vv['title']
	else:
		S.tags['line']='default'
		S.tags['title']=preFile if preFile else '[no file]'
	S.tags['idt']=(S.tags['id']+'.'+S.tags['title']).replace('/','_')
	S.tags['tid']=(S.tags['title']+'.'+S.tags['id']).replace('/','_')
	return S

class TableParamReader():
	"""Class for reading simulation parameters from text file.

Each parameter is represented by one column, each parameter set by one line. Colums are separated by blanks (no quoting).

First non-empty line contains column titles (without quotes).
You may use special column named 'title' to describe this parameter set;
if such colum is absent, title will be built by concatenating column names and corresponding values (``param1=34,param2=12.22,param4=foo``)

* from columns ending in ``!`` (the ``!`` is not included in the column name)
* from all columns, if no columns end in ``!``.
* columns containing literal - (minus) will be ignored

Empty lines within the file are ignored (although counted); ``#`` starts comment till the end of line. Number of blank-separated columns must be the same for all non-empty lines.

A special value ``=`` can be used instead of parameter value; value from the previous non-empty line will be used instead (works recursively).

This class is used by :ref:`woo.utils.readParamsFromTable`.
	"""
	def __init__(self,file):
		"Setup the reader class, read data into memory."
		import re
		# read file in memory, remove newlines and comments; the [''] makes lines 1-indexed
		ll=[re.sub('\s*#.*','',l[:-1]) for l in ['']+open(file,'r').readlines()]
		# usable lines are those that contain something else than just spaces
		usableLines=[i for i in range(len(ll)) if not re.match(r'^\s*(#.*)?$',ll[i])]
		headings=ll[usableLines[0]].split()
		# use all values of which heading has ! after its name to build up the title string
		# if there are none, use all columns
		if not 'title' in headings:
			bangHeads=[h[:-1] for h in headings if h[-1]=='!'] or headings
			headings=[(h[:-1] if h[-1]=='!' else h) for h in headings]
		usableLines=usableLines[1:] # and remove headinds from usableLines
		values={}
		for l in usableLines:
			val={}
			for i in range(len(headings)):
				val[headings[i]]=ll[l].split()[i]
			values[l]=val
		lines=values.keys(); lines.sort()
		# replace '=' by previous value of the parameter
		for i,l in enumerate(lines):
			for j in values[l].keys():
				if values[l][j]=='=':
					try:
						values[l][j]=values[lines[i-1]][j]
					except IndexError,KeyError:
						raise RuntimeError("The = specifier on line %d refers to nonexistent value on previous line?"%l)
		#import pprint; pprint.pprint(headings); pprint.pprint(values)
		# add descriptions, but if they repeat, append line number as well
		if not 'title' in headings:
			descs=set()
			for l in lines:
				dd=','.join(head.replace('!','')+'='+('%g'%values[head] if isinstance(values[l][head],float) else str(values[l][head])) for head in bangHeads if values[l][head].strip()!='-').replace("'",'').replace('"','')
				if dd in descs: dd+='__line=%d__'%l
				values[l]['title']=dd
				descs.add(dd)
		self.values=values

	def paramDict(self):
		"""Return dictionary containing data from file given to constructor. Keys are line numbers (which might be non-contiguous and refer to real line numbers that one can see in text editors), values are dictionaries mapping parameter names to their values given in the file. The special value '=' has already been interpreted, ``!`` (bangs) (if any) were already removed from column titles, ``title`` column has already been added (if absent)."""
		return self.values

if __name__=="__main__":
	tryTable="""head1 important2! !OMP_NUM_THREADS! abcd
	1 1.1 1.2 1.3
	'a' 'b' 'c' 'd'  ### comment

	# empty line
	1 = = g
"""
	file='/tmp/try-tbl.txt'
	f=open(file,'w')
	f.write(tryTable)
	f.close()
	from pprint import *
	pprint(TableParamReader(file).paramDict())



