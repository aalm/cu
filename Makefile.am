# Makefile.am

# Obvious program stuff.
bin_PROGRAMS = cu
CLEANFILES = cu.1.tmp

# Distribution tarball options.
EXTRA_DIST = \
	compat/*.[ch] \
	cu.1
#EXTRA_DIST += osdep-*.c

# Additional object files.
LDADD = $(LIBOBJS)

# Set flags for gcc.
if IS_GCC
AM_CFLAGS += -std=gnu99 -O2 -Wno-unused-parameter
if IS_DEBUG
AM_CFLAGS += -g
AM_CFLAGS += -Wno-long-long -Wall -W -Wformat=2
AM_CFLAGS += -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations
AM_CFLAGS += -Wwrite-strings -Wshadow -Wpointer-arith -Wsign-compare
AM_CFLAGS += -Wundef -Wbad-function-cast -Winline -Wcast-align
AM_CFLAGS += -Wdeclaration-after-statement -Wno-pointer-sign -Wno-attributes
AM_CFLAGS += -Wno-unused-result
AM_CPPFLAGS += -DDEBUG
endif
AM_CPPFLAGS += -iquote.
endif

# Set flags for NetBSD.
if IS_NETBSD
AM_CPPFLAGS += -D_OPENBSD_SOURCE
endif

# List of sources.
dist_cu_SOURCES = \
	command.c \
	compat.h \
	cu.c \
	cu.h \
	error.c \
	input.c \
	xmodem.c
#nodist_cu_SOURCES = osdep-@PLATFORM@.c

# Install cu.1 in the right format.
if IS_LINUX
install-exec-hook:
	sed -e "s|@DEFLINE@|ttyS|g" -e "s|@DEFLINEU@|ttyS0|g" \
		$(srcdir)/cu.1 >$(srcdir)/cu.1.tmp
	$(mkdir_p) $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(srcdir)/cu.1.tmp $(DESTDIR)$(mandir)/man1/cu.1
else
if IS_NETBSD
install-exec-hook:
	sed -e "s|@DEFLINE@|dty|g" -e "s|@DEFLINEU@|dty00|g" \
		$(srcdir)/cu.1 >$(srcdir)/cu.1.tmp
	$(mkdir_p) $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(srcdir)/cu.1.tmp $(DESTDIR)$(mandir)/man1/cu.1
else
if IS_OPENBSD
install-exec-hook:
	sed -e "s|@DEFLINE@|cua|g" -e "s|@DEFLINEU@|cua00|g" \
		$(srcdir)/cu.1 >$(srcdir)/cu.1.tmp
	$(mkdir_p) $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(srcdir)/cu.1.tmp $(DESTDIR)$(mandir)/man1/cu.1
else
install-exec-hook:
	sed -e "s|@DEFLINE@|tty|g" -e "s|@DEFLINEU@|tty00|g" \
		$(srcdir)/cu.1 >$(srcdir)/cu.1.tmp
	$(mkdir_p) $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(srcdir)/cu.1.tmp $(DESTDIR)$(mandir)/man1/cu.1
endif
endif
endif
