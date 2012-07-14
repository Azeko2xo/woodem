gen:
	rm -rf sphinx2/build/html
	cd sphinx2/source; PYTHONPATH=. woo -x -R gen.py || true
upload:
	rsync -r sphinx2/build/html/ bbeta:host/woodem/doc
default: gen upload
