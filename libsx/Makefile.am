SUBDIRS = dialogs

lib_LTLIBRARIES=libsx.la
include_HEADERS=libsx.h

libsx_la_SOURCES=button.c check_mark.h colormap.c colorsel.c \
  dialog.c dialog.h dirlist.c draw.c drawing.c drawing.h drawingP.h \
  font.c freq.c grabpix.c libsx.c libsx_private.h list.c\
  menu.c misc.c popups.c resize.c scrollbar.c string_entry.c toggle.c\
  version.c xstuff.h
libsx_la_LDFLAGS = -version-info $(LIBSX_VERSION)

CLEANFILES = libsx.a

EXTRA_DIST = Makefile.no_auto

all-local: libsx.a

libsx.a: .libs/libsx.a
	$(LN_S) $< libsx.a
