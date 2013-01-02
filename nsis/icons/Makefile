default:
	mpost woodem-6.mp
	# sudo apt-get install librsvg2-bin
	rsvg -w 256 -f png --keep-aspect-ratio woodem-6.woo.svg woo-icon.256.png
	rsvg -w 256 -f png --keep-aspect-ratio woodem-6.woo-batch.svg woo-batch-icon.256.png
	# use 16-color palette for smaller size
	convert -flatten -colors 16 woo-icon.256.png woo-icon.256.ico
	convert -flatten -colors 16 woo-batch-icon.256.png woo-batch-icon.256.ico
	cp woodem-6.woodem.svg ../../gui/qt4/woodem-logo.svg
