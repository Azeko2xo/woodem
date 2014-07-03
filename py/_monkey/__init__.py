'''
Extend various (python/c++) objects by new methods/properties on them, or modify existing ones.
This approach is called `monkey patching <http://en.wikipedia.org/wiki/Monkey_patching>`_, whence the module name.

This module is imported automatically by Woo at startup and should not be used directly.
'''
from . import io
from . import gts
import traceback

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

