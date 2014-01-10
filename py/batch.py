# encoding: utf-8

# for use with globals() when reading table
nan=float('nan')
from math import * 
from minieigen import *
import warnings

try:
	from lockfile import FileLock
except ImportError:
	class FileLock:
		'Dummy class if the `lockfile </https://pypi.python.org/pypi/lockfile>`_ module is not importable.'
		def __init__(self,f):
			warnings.warn("The 'lockfile' module is not importable, '%s' will not be locked. If this file is concurrently accessed by several simulations, it may get corrupted!"%f)
		def __enter__(self): pass
		def __exit__(self,*args): pass
	

# increase every time the db format changes, to avoid errors
# applies to both sqlite and hdf5 dbs
dbFormatVersion=3

## timeout for opening the database if locked
sqliteTimeout=1000

def wait():
	'If running inside a batch, start the master simulation (if not already running) and block until it stops by itself. Typically used at the end of script so that it does not finish prematurely in batch mode (the execution would be ended in such a case). Does nothing outisde of batch.'
	import woo
	S=woo.master.scene
	if inBatch():
		if not S.running: S.run()
		S.wait()
def inBatch():
	'Tell whether we are running inside the batch or separately.'
	import os
	return 'WOO_BATCH' in os.environ

def mayHaveStaleLock(db):
	import os.path
	if not os.path.splitext(db)[-1] in ('.h5','.hdf5','.he5','.hdf'): return
	return FileLock(db).is_locked()

def writeResults(scene,defaultDb='woo-results.hdf5',syncXls=True,dbFmt=None,series=None,quiet=False,postHooks=[],**kw):
	'''
	Write results to batch database. With *syncXls*, corresponding excel-file is re-generated.
	Series is a dicionary of 1d arrays written to separate sheets in the XLS. If *series* is `None`
	(default), `S.plot.data` are automatically added. All other ``**kw``
	arguments are serialized in the misc field, which then appears in the main XLS sheet.

	All data are serialized using json so that they can be read back in a language-independent manner.

	*postHooks* is list of functions (taking a single argument - the database name) which will be called
	once the database has been updated. They can be used in conjunction with :obj:`woo.batch.dbReadResults`
	to write aaggregate results from all records in the database.
	'''
	import woo, woo.plot, woo.core
	import os, os.path, datetime
	import numpy
	import json
	import logging
	S=scene
	if inBatch(): table,line,db=os.environ['WOO_BATCH'].split(':')
	else: table,line,db='',-1,defaultDb
	newDb=not os.path.exists(db)
	if not quiet: print 'Writing results to the database %s (%s)'%(db,'new' if newDb else 'existing')
	if dbFmt==None:
		ext=os.path.splitext(db)[-1]
		if ext in ('.sqlite','.db'): dbFmt='sqlite'
		elif ext in ('.h5','.hdf5','.he5','.hdf'): dbFmt='hdf5'
		else: raise ValueError("Unable to determine database format from file extension: must be *.h5, *.hdf5, *.he5, *.hdf, *.sqlite, *.db.")

	# make sure keys are unicode objects (which is what json converts to!)
	# but preserve values using a 8-bit encoding)
	# this sort-of sucks, hopefully there is a better solution soon
	unicodeTags=dict([(key,val.decode('iso-8859-1')) for key,val in S.tags.items()])
	# make sure series are 1d arrays
	if series==None:
		series={}; series.update([('plot/'+k,v) for k,v in S.plot.data.items()])
	for k,v in series.items():
		if isinstance(v,numpy.ndarray):
			if dbFmt=='sqlite': series[k]=v.tolist() # sqlite needs lists, hdf5 is fine with numpy arrays
		elif not hasattr(v,'__len__'): raise ValueError('series["%s"] not a sequence (__len__ not defined).'%k)
	if dbFmt=='sqlite':
		import sqlite3
		conn=sqlite3.connect(db,timeout=sqliteTimeout,detect_types=sqlite3.PARSE_DECLTYPES)
		if newDb:
			c=conn.cursor()
			c.execute('create table batch (formatVersion integer, finished timestamp, batchtable text, batchTableLine integer, sceneId text, title text, duration integer, pre text, tags text, plots text, misc text, series text)')
		else:	
			conn.row_factory=sqlite3.Row
			conn.execute('select * from batch')
			row=conn.cursor().fetchone()
			# row can be None if the db is empty
			if row and row['formatVersion']!=dbFormatVersion:
				raise RuntimeError('database format mismatch: %s/batch/formatVersion==%s, but should be %s'%(db,row['formatVersion'],dbFormatVersion))
		with conn:
			values=(	
				dbFormatVersion, # formatVersion
				datetime.datetime.now(), # finished
				table, # batchTable
				line, # batchTableLine
				S.tags['id'], # sceneId
				S.tags['title'], # title
				S.duration,
				(S.pre.dumps(format='json') if S.pre else None), # pre
				json.dumps(unicodeTags), # tags
				json.dumps(S.plot.plots), # plots
				woo.core.WooJSONEncoder(indent=None,oneway=True).encode(kw), # misc
				json.dumps(series) # series
			)
			conn.execute('insert into batch values (?,?,?,?,?, ?,?,?,?,?, ?,?)',values)
	elif dbFmt=='hdf5':
		import h5py
		try:
			hdf=h5py.File(db,'a',libver='latest')
		except IOError:
			import warnings
			warnings.warn("Error opening HDF5 file %s, moving to %s~~corrupt and creating a new one")
			import shutil
			shutil.move(db,db+'~~corrupt')
			hdf=h5py.File(db,'a',libver='latest')
		with FileLock(db):
			i=0
			while True:
				sceneId=S.tags['id']+('' if i==0 else '~%d'%i)
				if sceneId not in hdf: break
				i+=1
			# group for our Scene
			G=hdf.create_group(sceneId)
			G.attrs['formatVersion']=dbFormatVersion
			G.attrs['finished']=datetime.datetime.now().replace(microsecond=0).isoformat('_')
			G.attrs['batchTable']=table
			G.attrs['batchTableLine']=line
			G.attrs['sceneId']=S.tags['id']
			G.attrs['title']=S.tags['title']
			G.attrs['duration']=S.duration
			G.attrs['pre']=(S.pre.dumps(format='json') if S.pre else '')
			G.attrs['tags']=json.dumps(unicodeTags)
			G.attrs['plots']=json.dumps(S.plot.plots)
			G_misc=G.create_group('misc')
			for k,v in kw.items(): G_misc.attrs[k]=woo.core.WooJSONEncoder(indent=None,oneway=True).encode(v)
			G_series=G.create_group('series')
			for k,v in series.items():
				# hdf5 is smart enough to create sub-groups automatically if the name contains slashes
				G_series[k]=v
			hdf.close()
	else: raise ValueError('*fmt* must be one of "sqlite", "hdf5", None (autodetect based on suffix)')

	if syncXls:
		import re
		xls=db+'.xls'
		if not quiet: print 'Converting %s to file://%s'%(db,os.path.abspath(xls))
		dbToSpread(db,out=xls,dialect='xls')
	for ph in postHooks: ph(db)

def _checkHdf5sim(sim):
	if not 'formatVersion' in sim.attrs: raise RuntimeError('database %s: simulation %s does not define formatVersion?!')
	if sim.attrs['formatVersion']!=dbFormatVersion: raise RuntimeError('database format mismatch: %s: %s/formatVersion==%s, should be %s'%(db,sim,sim.attrs['formatVersion'],dbFormatVersion))


# return all series stored in the database
def dbReadResults(db,basicTypes=False):	
	'''Return list of dictionaries, representing database contents.

	:param basicTypes: don't reconstruct Woo objects from JSON (keep those as dicts) and don't return data series as numpy arrays.

	.. todo:: Nested (grouped) series are not read correctly from HDF5. Should be fixed either by flattening the hiearchy (like we do in :obj:`dbToSpread` and stuffing it into returned dict; or by reflecting the hierarchy in the dict returned.
	'''
	import numpy, sqlite3, json, woo.core
	try:
		import h5py
		hdf=h5py.File(db,'r',libver='latest')
	except (ImportError,IOError):
		# connect always succeeds, as it seems, even if the type is not sqlite3 db
		# in that case, it will fail at conn.execute below
		conn=sqlite3.connect(db,timeout=sqliteTimeout,detect_types=sqlite3.PARSE_DECLTYPES)
		hdf=None
	if hdf:
		with FileLock(db):
			ret=[]
			# iterate over simulations
			for simId in hdf:
				sim=hdf[simId]
				_checkHdf5sim(sim)
				rowDict={}
				for att in sim.attrs:
					if att in ('pre','tags','plots'):
						val=sim.attrs[att]
						if hasattr(val,'__len__') and len(val)==0: continue
						rowDict[att]=woo.core.WooJSONDecoder(onError='warn').decode(val)
					else: rowDict[att]=sim.attrs[att]
				rowDict['misc'],rowDict['series']={},{}
				for misc in sim['misc'].attrs: rowDict['misc'][misc]=woo.core.WooJSONDecoder(onError='warn').decode(sim['misc'].attrs[misc])
				for s in sim['series']: rowDict['series'][s]=sim['series'][s]
				ret.append(rowDict)
			## hdf.close()
			return ret
	else:
		# sqlite
		conn.row_factory=sqlite3.Row
		ret=[]
		for i,row in enumerate(conn.execute('SELECT * FROM batch ORDER BY finished')):
			rowDict={}
			for key in row.keys():
				# json-encoded fields
				if key in ('pre','tags','plots','misc'):
					if basicTypes: val=json.loads(row[key])
					else: val=woo.core.WooJSONDecoder(onError='warn').decode(row[key])
				elif key=='series':
					series=json.loads(row[key])
					assert type(series)==dict
					if basicTypes: val=series
					else: val=dict([(k,numpy.array(v)) for k,v in series.items()])
				else:
					val=row[key]
				if basicTypes and key=='finished': val=val.isoformat(sep='_')
				rowDict[key]=val
			ret.append(rowDict)
		conn.close() # don't occupy the db longer than necessary
		return ret

def dbToJSON(db,**kw):
	'''Return simulation database as JSON string.
	
	:param kw: additional arguments passed to `json.dumps <http://docs.python.org/3/library/json.html#json.dumps>`_.
	'''
	import woo.core
	return woo.core.WooJSONEncoder(indent=None,oneway=True).encode(dbReadResults(db,basicTypes=True),**kw)



def dbToSpread(db,out=None,dialect='xls',rows=False,series=True,ignored=('plotData','tags'),sortFirst=('title','batchtable','batchTableLine','finished','sceneId','duration'),selector=None):
	'''
	Select simulation results (using *selector*) stored in batch database *db*, flatten data for each simulation,
	and dump the data in the CSV format (using *dialect*: 'excel', 'excel-tab', 'xls') into file *out* (standard output
	if not given). If *rows*, every simulation is saved into one row of the CSV file (i.e. attributes are in columns),
	otherwise each simulation corresponds to one column and each attribute is in one row.
	
	If *out* ends with '.xls', the 'xls' dialect is forced regardless of the value given. The 'xls' format will refuse to write to standard output (*out* must be given).

	*ignored* fields are used to exclude large data from the dump: either database column of that name, or any attribute
	of that name. Attributes are flattened and path separated with '.'.

	*series* determines whether the `series` field will be written to a separate sheet, named by the sceneId. This is only supported with the `xls` dialect and raises error otherwise (unless `series` field is empty).

	Fields are sorted in their natural order (i.e. alphabetically, but respecting numbers), with *sortFirst* fields coming at the beginning.
	'''

	def flatten(obj,path='',sep='.',ret=None):
		'''Flatten possibly nested structure of dictionaries and lists (such as data decoded from JSON).
		Returns dictionary, where each object is denoted by its path, paths being separated by *sep*.
		Unicode strings are encoded to utf-8 strings (with encoding errors ignored) so that the result
		can be written with the csv module.
		
		Adapted from http://stackoverflow.com/questions/8477550/flattening-a-list-of-dicts-of-lists-of-dicts-etc-of-unknown-depth-in-python-n .
		'''
		if ret is None: ret={}
		if isinstance(obj,list):
			for i,item in enumerate(obj): flatten(item,(path+sep if path else '')+unicode(i),ret=ret)
		elif isinstance(obj,dict):
			for key,value in obj.items(): flatten(value,(path+sep if path else '')+unicode(key),ret=ret)
		elif isinstance(obj,(str,unicode)):
			#ret[path]=(obj.encode('utf-8','ignore') if isinstance(obj,unicode) else obj)
			ret[path]=unicode(obj)
		else:
			# other values passed as they are
			ret[path]=obj
		return ret

	def natural_key(string_):
		'''Return key for natural sorting (recognizing consecutive numerals as numbers):
		http://www.codinghorror.com/blog/archives/001018.html
		http://stackoverflow.com/a/3033342/761090
		'''
		import re
		return [int(s) if s.isdigit() else s for s in re.split(r'(\d+)', string_)]
	
	def fixSheetname(n):
		# truncate the name to 31 characters, otherwise there would be exception
		# see https://groups.google.com/forum/?fromgroups=#!topic/python-excel/QK4iJrPDSB8
		if len(n)>30: n=u'…'+n[-29:]
		# invald characters (is that documented somewhere?? those are the only ones I found manually)
		n=n.replace('[','_').replace(']','_').replace('*','_').replace(':','_').replace('/','_')
		return n

	import sqlite3,json,sys,csv,warnings,numpy
	allData={}
	# lowercase
	ignored=[i.lower() for i in ignored]
	sortFirst=[sf.lower() for sf in sortFirst]
	seriesData={}
	# open db and get rows
	try:
		import h5py
		hdf=h5py.File(db,'r',libver='latest')
	except (ImportError,IOError):
		# connect always succeeds, as it seems, even if the type is not sqlite3 db
		# in that case, it will fail at conn.execute below
		conn=sqlite3.connect(db,timeout=sqliteTimeout,detect_types=sqlite3.PARSE_DECLTYPES)
		hdf=None
	if hdf:
		with FileLock(db):
			ret=[]
			if selector: warnings.warn('selector parameter ignored, since the file is HDF5 (not SQLite)')
			# iterate over simulations
			for i,simId in enumerate(hdf):
				sim=hdf[simId]
				_checkHdf5sim(sim)
				rowDict={}
				for att in sim.attrs:
					val=sim.attrs[att]
					try: val=json.loads(val)
					except: pass
					rowDict[att]=val
				rowDict['misc']={}
				for att in sim['misc'].attrs:
					val=sim['misc'].attrs[att]
					try: val=json.loads(val)
					except: pass
					rowDict['misc'][att]=val
				series={}
				# we have to flatten series, since it may contain nested hdf5 groups
				# http://stackoverflow.com/a/6036037/761090
				def flat_group_helper(prefix,g):
					if prefix: prefix+='/'
					for name,sub in g.items():
						if isinstance(sub,h5py.Group):
							for j in flat_group_helper(prefix+name,sub): yield j
						else: yield (prefix+name,sub)
				def flat_group(g):
					import collections
					return collections.OrderedDict(flat_group_helper('',g))
				# print flat_group(sim['series']).keys()
				for sName,sVal in flat_group(sim['series']).items():
					# print sName
					series[sName]=numpy.array(sVal)
				seriesData[sim.attrs['title']+'_'+sim.attrs['sceneId']]=series
				# same as for sqlite3 below
				flat=flatten(rowDict)
				for key,val in flat.items():
					if key.lower() in ignored: continue
					if key not in allData: allData[key]=[None]*i+[val]
					else: allData[key].append(val)
			## hdf.close()
	else:
		conn=sqlite3.connect(db,timeout=sqliteTimeout,detect_types=sqlite3.PARSE_DECLTYPES)
		conn.row_factory=sqlite3.Row
		for i,row in enumerate(conn.execute(selector if selector!=None else 'SELECT * FROM batch ORDER BY title')):
			rowDict={}
			for key in row.keys():
				if key.lower() in ignored: continue
				val=row[key]
				# decode val from json, if it fails, leave it alone
				try: val=json.loads(val)
				except: pass
				if key!='series': rowDict[key]=val
				elif series: seriesData[row['title']+'_'+row['sceneId']]=val # set only if allowed
			flat=flatten(rowDict)
			for key,val in flat.items():
				if key.lower() in ignored: continue
				if key not in allData: allData[key]=[None]*i+[val]
				else: allData[key].append(val)
		conn.close() # don't occupy the db longer than necessary
	fields=sorted(allData.keys(),key=natural_key)
	# apply sortFirst
	fieldsLower=[f.lower() for f in fields]; fields0=fields[:] # these two have always same order
	for sf in reversed(sortFirst): # reverse so that the order of sortFirst is respected
		if sf in fieldsLower: # lowercased name should be put to the front
			field=fields0[fieldsLower.index(sf)] # get the case-sensitive one
			fields=[field]+[f for f in fields if f!=field] # rearrange

	if dialect.lower()=='xls' or (out and out.endswith('.xls')):
		if out==None: raise ValueError('The *out* parameter must be given when using the xls dialect (refusing to write binary to standard output).')
		# http://scienceoss.com/write-excel-files-with-python-using-xlwt/
		# http://www.youlikeprogramming.com/2011/04/examples-generating-excel-documents-using-pythons-xlwt/
		import xlwt, urllib
		wbk=xlwt.Workbook('utf-8')
		sheet=wbk.add_sheet(fixSheetname(db)) # truncate if too long, see below
		# cell styling 
		import datetime
		datetimeStyle=xlwt.XFStyle()
		datetimeStyle.num_format_str='yyyy-mm-dd_hh:mm:ss' # http://office.microsoft.com/en-us/excel-help/number-format-codes-HP005198679.aspx
		styleDict={datetime.datetime:datetimeStyle} # add styles for other custom types here
		# header style
		font=xlwt.Font()
		font.bold=True
		headStyle=xlwt.XFStyle()
		headStyle.font=font
		hrefStyle=xlwt.easyxf('font: underline single')
		# normal and transposed setters
		if rows: setCell=lambda r,c,data,style: sheet.write(r,c,data,style)
		else: setCell=lambda r,c,data,style: sheet.write(c,r,data,style)
		for col,field in enumerate(fields):
			# headers
			setCell(0,col,field,headStyle)
			# data
			for row,val in enumerate(allData[field]):
				style=styleDict.get(type(val),xlwt.Style.default_style)
				if isinstance(val,(str,unicode)) and (val.startswith('file://') or val.startswith('http://') or val.startswith('https://')):
					val=val.replace('"',"'")
					val=xlwt.Formula('HYPERLINK("%s","%s")'%(urllib.quote(val,safe=':/'),val))
					style=hrefStyle
				setCell(row+1,col,val,style)
				# print row,type(val),val
		# save data series
		if seriesData:
			for sheetName,dic in seriesData.items():
				sheet=wbk.add_sheet(fixSheetname(sheetName))
				# perhaps write some header here
				for col,colName in enumerate(sorted(dic.keys())):
					sheet.write(0,col,colName,headStyle)
					rowOffset=1 # length of header
					for row in range(0,len(dic[colName])):
						sheet.write(row+rowOffset,col,dic[colName][row])
		wbk.save(out)
	else:
		if seriesData: raise RuntimeError('Data series can only be written with the *xls* dialect')
		outt=(open(out,'w') if out else sys.stdout)
		import datetime
		def asStr(x):
			'Customize string conversions for some types'
			if type(x)==datetime.datetime: return x.strftime('%Y-%m-%d_%H:%M:%S') # as ISO, but without microseconds
			return x
		# write into CSV
		if rows:
			# one attribute per column
			writer=csv.DictWriter(outt,fieldnames=fields,dialect=dialect)
			writer.writeheader()
			for i in range(0,len(allData[fields[0]])):
				writer.writerow(dict([(k,asStr(allData[k][i])) for k in allData.keys()]))
		else:
			# one attribute per row
			writer=csv.writer(outt,dialect=dialect)
			for a in fields: writer.writerow([a]+[asStr(b) for b in allData[a]])

	

def readParamsFromTable(scene,under='table',tableFileLine=None,noTableOk=True,unknownOk=False,**kw):
	"""
	Read parameters from a file and assign them to :obj:`woo.core.Scene.lab` under the ``under`` pseudo-module (e.g. ``Scene.lab.table.foo`` and so on. This function is used for scripts (as opposed to preprocessors) running in a batch. The file format is described in :obj:`TableParamReader` (CSV or XLS).

	Assigned tags (the ``title`` column is synthesized if absent,see :obj:`woo.utils.TableParamReader`):: 

		S=woo.master.scene
		S.tags['title']=…                                            # assigns the title column; might be synthesized
		S.tags['params']="name1=val1,name2=val2,…"                   # all explicitly assigned parameters
		S.tags['defaultParams']="unassignedName1=defaultValue1,…"    # parameters that were left at their defaults
		S.tags['d.id']=s.tags['id']+'.'+s.tags['title']
		S.tags['id.d']=s.tags['title']+'.'+s.tags['id']

	:param tableFile: text file (with one value per blank-separated columns)
	:param under: name of pseudo-module under ``S.lab`` to save all values to (``table`` by default)
	:param int tableLine: number of line where to get the values from
	:param bool noTableOk: if False, raise exception if the file cannot be open; use default values otherwise
	:param bool unknownOk: do not raise exception if unknown column name is found in the file, and assign it as well
	:return: None
	"""
	tagsParams=[]
	# dictParams is what eventually ends up in S.lab.table.* (default+specified values)
	dictDefaults,dictParams={},{}
	import os, __builtin__,re,math,woo
	# create the S.lab.table pseudo-module
	S=scene
	S.lab._newModule(under)
	pseudoMod=getattr(S.lab,under)
	if not tableFileLine and ('WOO_BATCH' not in os.environ or os.environ['WOO_BATCH']==''):
		if not noTableOk: raise EnvironmentError("WOO_BATCH is not defined in the environment")
		S.tags['line']='l!'
	else:
		if not tableFileLine: tableFileLine=os.environ['WOO_BATCH']
		env=tableFileLine.split(':')
		tableFile,tableLine=env[0],int(env[1])
		if tableFile=='':
			if not noTableOk: raise RuntimeError("No table specified in WOO_BATCH, but noTableOk was not given.")
			else: return
		allTab=TableParamReader(tableFile).paramDict()
		if not allTab.has_key(tableLine): raise RuntimeError("Table %s doesn't contain valid line number %d"%(tableFile,tableLine))
		vv=allTab[tableLine]
		S.tags['line']='l%d'%tableLine
		S.tags['title']=str(vv['title'])
		S.tags['idt']=S.tags['id']+'.'+S.tags['title'];
		S.tags['tid']=S.tags['title']+'.'+S.tags['id']
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
			# when reading from XLS, data might be numbers; use eval only for strings, otherwise use the thing itself
			dictParams[col]=eval(vv[col],dict(woo=woo,**math.__dict__)) if type(vv[col]) in (str,unicode) else vv[col]
	# assign remaining (default) keys to python vars
	defaults=[]
	for k in kw.keys():
		dictDefaults[k]=kw[k]
		defaults+=["%s=%s"%(k,kw[k])];
	pseudoMod.defaultParams_=",".join(defaults)
	pseudoMod.explicitParams_=",".join(tagsParams)
	# save all vars to the pseudo-module
	dictDefaults.update(dictParams)
	for k,v in dictDefaults.items():
		setattr(pseudoMod,k,v)
	return None


def runPreprocessor(pre,preFile=None):
	"""Execute given :obj:`Preprocessor <woo.core.Preprocessor>`, modifying its attributes from batch (if running in batch). Each column from the batch table (except of environment variables starting with ``!``) must correspond to a preprocessor's attribute.

	Nested attributes are allowed, e.g. with :obj:`woo.pre.horse.FallingHorse`, a column named ``mat.tanPhi`` will modify horse's material's friction angle, using the default material object.
	"""

	def nestedSetattr(obj,attr,val):
		attrs=attr.split(".")
		for i in attrs[:-1]:
			obj=getattr(obj,i)
		setattr(obj,attrs[-1],val)

	# just run preprocessor in this case
	if not inBatch(): return pre()

	import os
	import woo,math
	tableFileLine=os.environ['WOO_BATCH']
	if tableFileLine:
		env=tableFileLine.split(':')
		tableFile,tableLine=env[0],int(env[1])
		if tableFile!='':
			allTab=TableParamReader(tableFile).paramDict()
			if not tableLine in allTab: raise RuntimeError("Table %s doesn't contain valid line number %d"%(tableFile,tableLine))
			vv=allTab[tableLine]
			# set preprocessor parameters first
			for name,val in vv.items():
				if name=='title': continue
				if val in ('*','-',''): continue
				nestedSetattr(pre,name,eval(val,globals(),dict(woo=woo,math=math))) # woo.unit
		else:
			vv={'title':tableFileLine}
	# check types, if this is a python preprocessor
	if hasattr(pre,'checkAttrTypes'): pre.checkAttrTypes()
	# run preprocessor
	S=pre()
	# set tags from batch
	if tableFileLine:
		S.tags['line']='l%d'%tableLine
		S.tags['title']=str(vv['title'])
	else:
		S.tags['line']='default'
		S.tags['title']=str(preFile if preFile else '[no file]')
	S.tags['idt']=(S.tags['id']+'.'+S.tags['title']).replace('/','_')
	S.tags['tid']=(S.tags['title']+'.'+S.tags['id']).replace('/','_')
	return S

class TableParamReader():
	r"""Class for reading simulation parameters from text file.

Each parameter is represented by one column, each parameter set by one line. Colums are separated by blanks (no quoting).

First non-empty line contains column titles (without quotes).
You may use special column named 'title' to describe this parameter set;
if such colum is absent, title will be built by concatenating column names and corresponding values (``param1=34,param2=12.22,param4=foo``)

* from columns ending in ``!`` (the ``!`` is not included in the column name)
* from all columns, if no columns end in ``!``.
* columns containing literal - (minus) will be ignored

Empty lines within the file are ignored (although counted); ``#`` starts comment till the end of line. Number of blank-separated columns must be the same for all non-empty lines.

A special value ``=`` can be used instead of parameter value; value from the previous non-empty line will be used instead (works recursively); in XLS, *empty* cell is treated the same as ``=``.

This class is used by :obj:`woo.utils.readParamsFromTable`.

>>> tryData=[
... 	['head1','important2!','!OMP_NUM_THREADS!','abcd'],
... 	[1,1.1,1.2,1.3,],
... 	['a','b','c','d','###','comment'],
... 	['# empty line'],
... 	[1,'=','=','g']
... ]
>>> import woo
>>> tryFile=woo.master.tmpFilename()
>>> # write text
>>> f1=tryFile+'.txt'
>>> txt=open(f1,'w')
>>> for ll in tryData: txt.write(' '.join([str(l) for l in ll])+'\n')
>>> txt.close()
>>> 
>>> # write xls
>>> import xlwt,itertools
>>> f2=tryFile+'.xls'
>>> xls=xlwt.Workbook(); sheet=xls.add_sheet('test')
>>> for r in range(len(tryData)):
... 	for c in range(len(tryData[r])):
... 		sheet.write(r,c,tryData[r][c])
>>> xls.save(f2)
>>> 
>>> from pprint import *
>>> pprint(TableParamReader(f1).paramDict())
{2: {'!OMP_NUM_THREADS': '1.2',
     'abcd': '1.3',
     'head1': '1',
     'important2': '1.1',
     'title': 'important2=1.1,OMP_NUM_THREADS=1.2'},
 3: {'!OMP_NUM_THREADS': 'c',
     'abcd': 'd',
     'head1': 'a',
     'important2': 'b',
     'title': 'important2=b,OMP_NUM_THREADS=c'},
 5: {'!OMP_NUM_THREADS': 'c',
     'abcd': 'g',
     'head1': '1',
     'important2': 'b',
     'title': 'important2=b,OMP_NUM_THREADS=c__line=5__'}}
>>> pprint(TableParamReader(f2).paramDict())
{1: {u'!OMP_NUM_THREADS': '1.2',
     u'abcd': '1.3',
     u'head1': '1.0',
     u'important2': '1.1',
     'title': u'important2=1.1,OMP_NUM_THREADS=1.2'},
 2: {u'!OMP_NUM_THREADS': u'c',
     u'abcd': u'd',
     u'head1': u'a',
     u'important2': u'b',
     'title': u'important2=b,OMP_NUM_THREADS=c'},
 4: {u'!OMP_NUM_THREADS': u'c',
     u'abcd': u'g',
     u'head1': '1.0',
     u'important2': u'b',
     'title': u'important2=b,OMP_NUM_THREADS=c__line=4__'}}

	"""
	def __init__(self,file):
		"Setup the reader class, read data into memory."
		import re
		if file.lower().endswith('.xls'):
			import xlrd
			xls=xlrd.open_workbook(file)
			sheet=xls.sheet_by_index(0)
			maxCol=0
			rows=[] # rows actually containing data (filled in the loop)
			for row in range(sheet.nrows):
				# find first non-empty and non-comment cell
				lastDataCol=-1
				for col in range(sheet.ncols):
					c=sheet.cell(row,col)
					empty=(c.ctype in (xlrd.XL_CELL_EMPTY,xlrd.XL_CELL_BLANK) or (c.ctype==xlrd.XL_CELL_TEXT and c.value.strip()==''))
					comment=(c.ctype==xlrd.XL_CELL_TEXT and re.match(r'^\s*(#.*)?$',c.value)) 
					if comment: break # comment cancels all remaining cells on the line
					if not empty: lastDataCol=col
				if lastDataCol>=0:
					rows.append(row)
					maxCol=lastDataCol
			# rows and cols with data
			cols=range(maxCol+1)
			#print 'maxCol=%d,cols=%s'%(maxCol,cols)
			# iterate through cells, define rawHeadings, headings, values
			rawHeadings=[sheet.cell(rows[0],c).value for c in cols]
			headings=[(h[:-1] if (h and h[-1]=='!') else h) for h in rawHeadings] # without trailing bangs
			values={}
			for r in rows[1:]:
				vv={}
				for c in cols:
					v=sheet.cell(r,c).value
					if type(v)!=unicode: v=str(v)
					vv[c]=v
				values[r]=dict([(headings[c],vv[c]) for c in cols])
		else:
			# text file, space separated
			# read file in memory, remove newlines and comments; the [''] makes lines 1-indexed
			ll=[re.sub('\s*#.*','',l[:-1]) for l in ['']+open(file,'r').readlines()]
			# usable lines are those that contain something else than just spaces
			usableLines=[i for i in range(len(ll)) if not re.match(r'^\s*(#.*)?$',ll[i])]
			rawHeadings=ll[usableLines[0]].split()
			headings=[(h[:-1] if h[-1]=='!' else h) for h in rawHeadings] # copy of headings without trailing bangs (if any)
			# use all values of which heading has ! after its name to build up the title string
			# if there are none, use all columns
			usableLines=usableLines[1:] # and remove headindgs from usableLines
			values={}
			for l in usableLines:
				val={}
				for i in range(len(headings)):
					val[headings[i]]=ll[l].split()[i]
				values[l]=val
		#
		# each format has to define the following:
		#   values={lineNumber:{key:val,key:val,...],...} # keys are column headings
		#   rawHeadings=['col1title!','col2title',...]    # as in the file
		#   headings=['col1title','col2title',...]        # with trailing bangs removed
		#

		# replace empty cells or '=' by the previous value of the parameter
		lines=values.keys(); lines.sort()
		for i,l in enumerate(lines):
			for j in values[l].keys():
				if values[l][j] in ('=',''):
					try:
						values[l][j]=values[lines[i-1]][j]
					except IndexError,KeyError:
						raise ValueError("The = specifier for '%s' on line %d, refers to nonexistent value on previous line?"%(j,l))
		# add descriptions, but if they repeat, append line number as well
		if not 'title' in headings:
			bangHeads=[h[:-1] for h in rawHeadings if (h and h[-1]=='!')] or headings
			descs=set()
			for l in lines:
				ddd=[]
				for head in bangHeads:
					val=values[l][head]
					if type(val) in (str,unicode) and val.strip() in ('','-','*'): continue # default value used
					ddd.append(head.replace('!','')+'='+('%g'%val if isinstance(val,float) else str(val)))
				dd=','.join(ddd).replace("'",'').replace('"','')
				#dd=','.join(head.replace('!','')+'='+('%g'%values[head] if isinstance(values[l][head],float) else str(values[l][head])) for head in bangHeads if (values[l][head].strip()!='-').replace("'",'').replace('"','')
				if dd in descs: dd+='__line=%d__'%l
				values[l]['title']=dd
				descs.add(dd)
		self.values=values

	def paramDict(self):
		"""Return dictionary containing data from file given to constructor. Keys are line numbers (which might be non-contiguous and refer to real line numbers that one can see in text editors), values are dictionaries mapping parameter names to their values given in the file. The special value '=' has already been interpreted, ``!`` (bangs) (if any) were already removed from column titles, ``title`` column has already been added (if absent)."""
		return self.values

if __name__=="__main__":
	## this is now in the doctest as well
	tryData=[
		['head1','important2!','!OMP_NUM_THREADS!','abcd'],
		[1,1.1,1.2,1.3,],
		['a','b','c','d','###','comment'],
		['# empty line'],
		[1,'=','=','g']
	]
	tryFile='/tmp/try-tbl'

	# write text
	f1=tryFile+'.txt'
	txt=open(f1,'w')
	for ll in tryData: txt.write(' '.join([str(l) for l in ll])+'\n')
	txt.close()

	# write xls
	import xlwt,itertools
	f2=tryFile+'.xls'
	xls=xlwt.Workbook(); sheet=xls.add_sheet('test')
	for r in range(len(tryData)):
		for c in range(len(tryData[r])):
			sheet.write(r,c,tryData[r][c])
	xls.save(f2)

	from pprint import *
	pprint(TableParamReader(f1).paramDict())
	pprint(TableParamReader(f2).paramDict())



