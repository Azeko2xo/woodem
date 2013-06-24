default:
	mpost woodem-6.mp
	# sudo apt-get install librsvg2-bin
	rsvg -w 256 -f png --keep-aspect-ratio woodem-6.woo.svg woo-icon.256.png
	rsvg -w 64 -f png --keep-aspect-ratio woodem-6.woo.svg woo-icon.64.png
	rsvg -w 256 -f png --keep-aspect-ratio woodem-6.woo-batch.svg woo-batch-icon.256.png
	# use 16-color palette for smaller size
	convert -flatten -colors 16 woo-icon.256.png woo-icon.256.ico
	convert -flatten -colors 16 -geometry 64x64\! woo-icon.64.png woo-icon.64.ico
	convert -flatten -colors 16 woo-batch-icon.256.png woo-batch-icon.256.ico
	cp woo-icon.64.ico ../../py/data/woo-favicon.ico
	cp woo-icon.64.ico ../../doc/sphinx2/source/_static/woo-favicon.ico
	cp woodem-6.woo.svg ../../gui/qt4/woo-logo.svg
	# make smaller rather than copy, so that SVG embedded in XHTML has the right size
	rsvg-convert woodem-6.woo.svg --width=120 --keep-aspect-ratio --format svg --output ../../py/data/woodem-6.woo.svg
	#cp woodem-6.woo.svg ../../py/data/woodem-6.woo.svg
