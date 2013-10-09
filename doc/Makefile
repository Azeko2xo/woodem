.PHONY: gen extras upload uploadpng default

gen:
	rm -rf sphinx2/build/html sphinx2/build/doctrees sphinx2/build-extra sphinx2/source-extra
	rm -f sphinx2/source/{woo,wooExtra,wooMain}.*.rst
	cd sphinx2/source; PYTHONPATH=. woo -x --quirks=0 -R gen.py
	# python -c 'import webbrowser; webbrowser.open("file://${PWD}/sphinx2/build/html/index.html")'
extras:
	rm -rf sphinx2/build-extra sphinx2/source-extra
	cd sphinx2/source; PYTHONPATH=. woo -x -R gen.py --quirks=0 --only-extras
upload:
	rsync -r sphinx2/build/html/ bbeta:host/woodem/doc
	for ex in sphinx2/build-extra/*; do rsync -r $$ex/ bbeta:host/woodem/private/`basename $$ex`/doc; done
uploadpng:
	rsync -r sphinx2/build/html/ bbeta:host/woodem/doc-png
	for ex in sphinx2/build-extra/*; do rsync -r $$ex/ bbeta:host/woodem/private/`basename $$ex`/doc-png; done
default: gen upload
