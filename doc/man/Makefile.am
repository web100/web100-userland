dist-hook:
	doctext -mpath $(distdir) -quotefmt $(top_srcdir)/lib/*.[ch]

autogen_manpages := $(wildcard *.3)
normal_manpages := deltavar.1 readall.1 readvar.1 util.1 web100.7 web100-config.1 writevar.1

man_MANS := $(autogen_manpages) $(normal_manpages)
man3_MANS := $(autogen_manpages)

EXTRA_DIST += $(normal_manpages)
