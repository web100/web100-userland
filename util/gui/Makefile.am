bin_PROGRAMS = util

INCLUDES = @STRIP_BEGIN@ \
	@GTK_CFLAGS@ \
	-I$(top_srcdir)/lib \
	@STRIP_END@

util_LDADD = \
	$(top_builddir)/lib/libweb100.la \
	@GTK_LIBS@

util_SOURCES = \
	util.c \
	avd_list_wgt.c \
	avd_table_wgt.c \
	cpr_wgt.c \
	dtb_wgt.c \
	sockset_wgt.c \
	cnlst_wgt.c \
	sockdata.c \
	rtuner_wgt.c \
	stuner_wgt.c \
	web100obj.c \
	web100sync.c \
	ur_launch.c

noinst_HEADERS = \
	avd_list_wgt.h \
	avd_table_wgt.h \
	cpr_wgt.h \
	dtb_wgt.h \
	sockset_wgt.h \
	cnlst_wgt.h \
	sockdata.h \
	rtuner_wgt.h \
	stuner_wgt.h \
	web100obj.h \
	web100sync.h \
	ur_launch.h
