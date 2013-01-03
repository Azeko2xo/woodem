'''
Extend various c++ objects by defining python methods on them.
This module is imported automatically by woo and should not be used directly
'''
from . import io
from . import gts

# out-of-class docstrings for some classes
try: from . import extraDocs
except AttributeError:
	print 'WARN: Error importing woo._monkey.extraDocs'
	traceback.print_exc()
# attribute aliases
try: from . import aliases
except AttributeError:
	print 'WARN: Error importing woo._monkey.aliases.py'
	traceback.print_exc()

