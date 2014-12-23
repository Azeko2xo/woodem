.PHONY: gen extras upload uploadpng default

gen:
	rm -rf sphinx2/build/html sphinx2/build/doctrees sphinx2/build-extra sphinx2/source-extra
	rm -f sphinx2/source/{woo,wooExtra,wooMain}.*.rst
	cd sphinx2/source; PYTHONPATH=. woo -x --fake-display  --quirks=0 -R gen.py
	# python -c 'import webbrowser; webbrowser.open("file://${PWD}/sphinx2/build/html/index.html")'
sphinxonly:
	cd sphinx2/source; PYTHONPATH=. woo -x --fake-display --quirks=0 gen.py
extras:
	rm -rf sphinx2/build-extra sphinx2/source-extra
	cd sphinx2/source; PYTHONPATH=. woo -x --fake-display -R gen.py --quirks=0 --only-extras
upload:
	rsync -r sphinx2/build/html/ woodem:woo-doc/
	for ex in sphinx2/build-extra/*; do rsync -r $$ex/ woodem:woo-private/`basename $$ex`/doc/; done
uploadpng:
	rsync -r sphinx2/build/html/ woodem:woo-doc-png/
	for ex in sphinx2/build-extra/*; do rsync -r $$ex/ woodem:woo-private/`basename $$ex`/doc-png/; done
default: gen upload
