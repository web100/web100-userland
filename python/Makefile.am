all-am: Makefile python-build


if BUILD_PYTHON

python-build:
	$(PYTHON) setup.py build

python-install:
	$(PYTHON) setup.py install --install-purelib=${pythondir} --install-platlib=${pyexecdir}

python-clean:
	$(PYTHON) setup.py clean

else

python-build:

python-install:

python-clean:

endif


install-exec-hook: python-install

uninstall-local: python-clean
