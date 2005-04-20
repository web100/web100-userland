SUBDIRS = doc lib util python

bin_SCRIPTS = web100-config
CLEANFILES = web100-config

m4datadir = $(datadir)/aclocal
m4data_DATA = web100.m4

edit = @STRIP_BEGIN@ \
   sed -e 's,@WEB100_VERSION\@,$(WEB100_VERSION),g' \
       -e 's,@PREFIX\@,$(prefix),g' \
       -e 's,@EXEC_PREFIX\@,$(exec_prefix),g' \
       -e 's,@WEB100_INCLUDE_DIR\@,$(WEB100_INCLUDE_DIR),g' \
       -e 's,@LIBDIR\@,$(libdir),g' \
       @STRIP_END@

web100-config: $(srcdir)/web100-config.in
	rm -f web100-config web100-config.tmp
	$(edit) $(srcdir)/web100-config.in > web100-config.tmp
	chmod +x web100-config.tmp
	mv web100-config.tmp web100-config

EXTRA_DIST = web100-config.in web100.m4

distcleancheck:
	@:
distuninstallcheck:
	@:
