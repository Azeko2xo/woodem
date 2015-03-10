# NOTE: all .py files from this directory are installed in py/woo/pre (as per /SConscript)
# they should not conflict with classes which are created in the woo.pre module (if any)

# frozen installation (via PyInstaller) is not able to auto-discover frozen preprocessors
# so they don't apper in the UI until they get imported by hand
# if they are listed here, they show up just fine
from . import horse
from . import cylTriax
from . import ell2d
from . import triax
from . import depot
