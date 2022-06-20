ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS =								\
	panel-plugin							\
	po

distclean-local:
	rm -rf *.cache *~

EXTRA_DIST = 								\
	README.md								\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	po/.intltool-merge-cache.lock
