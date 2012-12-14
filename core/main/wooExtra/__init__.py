# in the base installation, wooExtra contains nothing.
# Having it empty here lets us import wooExtra without worrying
# that wooExtra might actually not exist since no wooExtra.* is installed
__import__('pkg_resources').declare_namespace(__name__)
