bin_PROGRAMS = gutil
RC_FILES = web100.rc lcl.rc rmt.rc

INCLUDES = @STRIP_BEGIN@ \
	@GTK_CFLAGS@ \
	-I$(top_srcdir)/lib \
	-DWEB100_CONF_DIR=\"$(WEB100_CONF_DIR)\" \
	@STRIP_END@

gutil_LDADD = \
	$(top_builddir)/lib/libweb100.la \
	@GTK_LIBS@

gutil_SOURCES = \
	avd_list_wgt.c \
	avd_list_wgt.h \
	avd_table_wgt.c \
	avd_table_wgt.h \
	cnlst_wgt.c \
	cnlst_wgt.h \
	cpr_wgt.c \
	cpr_wgt.h \
	dtb_wgt.c \
	dtb_wgt.h \
	rtuner_wgt.c \
	rtuner_wgt.h \
	sockdata.c \
	sockdata.h \
	sockset_wgt.c \
	sockset_wgt.h \
	stuner_wgt.c \
	stuner_wgt.h \
	table.h \
	triage_wgt.c \
	triage_wgt.h \
	ur_launch.c \
	ur_launch.h \
	gutil.c \
	vars.h \
	vdt_wgt.c \
	vdt_wgt.h \
	wcgraph.c \
	wcgraph.h \
	wcpie.c \
	wcpie.h \
	web100obj.c \
	web100obj.h \
	web100sync.c \
	web100sync.h

confdir = $(WEB100_CONF_DIR)
conf_DATA = $(RC_FILES)

EXTRA_DIST += $(RC_FILES)
