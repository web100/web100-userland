all-am: Makefile python-build

if BUILD_PYTHON

python-build: $(srcdir)/libweb100.py $(srcdir)/libweb100_wrap.cxx
	test -e libweb100.py || $(LN_S) $(srcdir)/libweb100.py .
	test -e libweb100_wrap.cxx || $(LN_S) $(srcdir)/libweb100_wrap.cxx .
	$(PYTHON) $(srcdir)/setup.py build_ext --include-dirs=$(top_srcdir)/lib:$(top_srcdir)/python
	$(PYTHON) $(srcdir)/setup.py build_py

python-install:
	$(PYTHON) $(srcdir)/setup.py install --install-purelib=${pythondir} --install-platlib=${pyexecdir}

python-clean:
	$(PYTHON) $(srcdir)/setup.py clean

libweb100.py libweb100_wrap.cxx: $(top_srcdir)/python/libweb100.i $(top_srcdir)/python/web100_readbuf.h
	$(SWIG) -python -I$(top_srcdir)/lib -o libweb100_wrap.cxx $(srcdir)/libweb100.i

SWIG_OUTPUTS = libweb100.py libweb100_wrap.cxx

# Maintainer-clean the swig generated files, however, include them in EXTRA_DIST
MAINTAINERCLEANFILES = $(SWIG_OUTPUTS)

else

python-build:

python-install:

python-clean:

endif


install-exec-hook: python-install

uninstall-local: python-clean

EXTRA_DIST=Web100.py libweb100.i libweb100.py libweb100_wrap.cxx readall.py setup.py web100_readbuf.h
