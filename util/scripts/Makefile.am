bin_PROGRAMS = readall readvar deltavar writevar

NOGTK_LDADDS = @STRIP_BEGIN@ \
	$(top_builddir)/lib/libweb100.la \
	@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)/lib \
	@STRIP_END@

readall_SOURCES = readall.c
readall_LDADD = $(NOGTK_LDADDS)

readvar_SOURCES = readvar.c
readvar_LDADD = $(NOGTK_LDADDS)

deltavar_SOURCES = deltavar.c
deltavar_LDADD = $(NOGTK_LDADDS)

writevar_SOURCES = writevar.c
writevar_LDADD = $(NOGTK_LDADDS)
