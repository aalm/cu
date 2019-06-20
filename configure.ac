# configure.ac

AC_INIT([cu], next-3.1)
AC_PREREQ([2.60])

#AC_CONFIG_AUX_DIR(etc)
AC_CONFIG_LIBOBJ_DIR(compat)
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_CANONICAL_HOST

# When CFLAGS isn't set at this stage and gcc is detected by the macro below,
# autoconf will automatically use CFLAGS="-O2 -g". Prevent that by using an
# empty default.
: ${CFLAGS=""}

# Save user CPPFLAGS, CFLAGS and LDFLAGS. We need to change them because
# AC_CHECK_HEADER doesn't give us any other way to update the include
# paths. But for Makefile.am we want to use AM_CPPFLAGS and friends.
SAVED_CFLAGS="$CFLAGS"
SAVED_CPPFLAGS="$CPPFLAGS"
SAVED_LDFLAGS="$LDFLAGS"

# Set up the compiler in two different ways and say yes we may want to install.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_PROG_CPP
AC_PROG_EGREP
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG
AC_USE_SYSTEM_EXTENSIONS

# Is this --enable-debug?
case "x$VERSION" in xnext*) enable_debug=yes;; esac
AC_ARG_ENABLE(
	debug,
	AC_HELP_STRING(--enable-debug, enable debug build flags),
)
AM_CONDITIONAL(IS_DEBUG, test "x$enable_debug" = xyes)

# Is this a static build?
AC_ARG_ENABLE(
	static,
	AC_HELP_STRING(--enable-static, create a static build)
)
if test "x$enable_static" = xyes; then
	test "x$PKG_CONFIG" != x && PKG_CONFIG="$PKG_CONFIG --static"
	AM_LDFLAGS="-static $AM_LDFLAGS"
	LDFLAGS="$AM_LDFLAGS $SAVED_LDFLAGS"
fi

# Is this gcc?
AM_CONDITIONAL(IS_GCC, test "x$GCC" = xyes)

# Check for various headers. Alternatives included from compat.h.
AC_CHECK_HEADERS([ \
	dirent.h \
	fcntl.h \
	inttypes.h \
	ndir.h \
	paths.h \
	stdint.h \
	sys/dir.h \
	sys/ndir.h \
	util.h \
])

# Check for functions that are replaced or omitted.
AC_CHECK_FUNCS([ \
	dirfd \
	sysconf \
])

# Check for functions with a compatibility implementation.
AC_REPLACE_FUNCS([ \
	closefrom \
	getprogname \
	strtonum \
])
# XXX AC_FUNC_STRNLEN ?

# Look for libevent.
PKG_CHECK_MODULES(
	LIBEVENT,
	libevent,
	[
		AM_CFLAGS="$LIBEVENT_CFLAGS $AM_CFLAGS"
		CFLAGS="$AM_CFLAGS $SAVED_CFLAGS"
		LIBS="$LIBEVENT_LIBS $LIBS"
		found_libevent=yes
	],
	[
		AC_SEARCH_LIBS(
			event_init,
			[event event-1.4 event2],
			found_libevent=yes,
			found_libevent=no
		)
	]
)
AC_CHECK_HEADER(
	event.h,
	,
	found_libevent=no
)
if test "x$found_libevent" = xno; then
	AC_MSG_ERROR("libevent not found")
fi

# Look for err and friends in err.h.
AC_CHECK_FUNC(err, found_err_h=yes, found_err_h=no)
AC_CHECK_FUNC(errx, , found_err_h=no)
AC_CHECK_FUNC(warn, , found_err_h=no)
AC_CHECK_FUNC(warnx, , found_err_h=no)
AC_CHECK_FUNC(verr, , found_err_h=no)
AC_CHECK_FUNC(verrx, , found_err_h=no)
if test "x$found_err_h" = xyes; then
	AC_CHECK_HEADER(err.h, , found_err_h=no)
else
	AC_LIBOBJ(err)
fi

# Look for stravis, compat/{vis,unvis}.c used if missing.
AC_CHECK_FUNC(stravis, found_stravis=yes, found_stravis=no)
if test "x$found_stravis" = xyes; then
	AC_MSG_CHECKING(if strnvis is broken)
	AC_EGREP_HEADER([strnvis\(char \*, const char \*, size_t, int\)],
			vis.h,
			AC_MSG_RESULT(no),
			[found_stravis=no])
	if test "x$found_stravis" = xno; then
		AC_MSG_RESULT(yes)
	fi
fi
if test "x$found_stravis" = xyes; then
	AC_CHECK_DECL(
		VIS_DQ,
		,
		found_stravis=no,
		[
			#include <stdlib.h>
			#include <vis.h>
		]
)
fi
if test "x$found_stravis" = xyes; then
	AC_DEFINE(HAVE_VIS)
else
	AC_LIBOBJ(vis)
#	AC_LIBOBJ(unvis)
fi

# Look for getopt. glibc's getopt does not enforce argument order and the ways
# of making it do so are stupid, so just use our own instead.
AC_CHECK_FUNC(getopt, found_getopt=yes, found_getopt=no)
if test "x$found_getopt" != xno; then
	AC_MSG_CHECKING(if getopt is suitable)
	AC_EGREP_CPP(
		yes,
		[
			#include <features.h>
			#ifdef __GLIBC__
			yes
			#endif
		],
		[
			found_getopt=no
			AC_MSG_RESULT(no)
		],
		AC_MSG_RESULT(yes))
fi
if test "x$found_getopt" != xno; then
	AC_CHECK_DECLS(
		[optarg, optind, optreset],
		,
		found_getopt=no,
		[
			#include <unistd.h>
		])
fi
if test "x$found_getopt" != xno; then
	AC_DEFINE(HAVE_GETOPT)
else
	AC_LIBOBJ(getopt)
fi

# Look for __progname.
AC_MSG_CHECKING(for __progname)
AC_LINK_IFELSE([AC_LANG_SOURCE(
	[
		#include <stdio.h>
		#include <stdlib.h>
		extern char *__progname;
		int main(void) {
			const char *cp = __progname;
			printf("%s\n", cp);
			exit(0);
		}
	])],
	[AC_DEFINE(HAVE___PROGNAME) AC_MSG_RESULT(yes)],
	AC_MSG_RESULT(no)
)

# Look for program_invocation_short_name.
AC_MSG_CHECKING(for program_invocation_short_name)
AC_LINK_IFELSE([AC_LANG_SOURCE(
	[
		#include <errno.h>
		#include <stdio.h>
		#include <stdlib.h>
		int main(void) {
			const char *cp = program_invocation_short_name;
			printf("%s\n", cp);
			exit(0);
		}
	])],
	[AC_DEFINE(HAVE_PROGRAM_INVOCATION_SHORT_NAME) AC_MSG_RESULT(yes)],
	AC_MSG_RESULT(no)
)

# Look for fcntl(F_CLOSEM).
AC_CHECK_DECL(
	F_CLOSEM,
	AC_DEFINE(HAVE_FCNTL_CLOSEM),
	,
	[#include <fcntl.h>]
)

# Look for /proc/$$.
AC_MSG_CHECKING(for /proc/\$\$)
if test -d /proc/$$; then
	AC_DEFINE(HAVE_PROC_PID)
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

# Man page defaults to mdoc.
MANFORMAT=mdoc
AC_SUBST(MANFORMAT)

# Figure out the platform.
AC_MSG_CHECKING(platform)
case "$host_os" in
	*linux*)
		AC_MSG_RESULT(linux)
		PLATFORM=linux
		;;
	*netbsd*)
		AC_MSG_RESULT(netbsd)
		PLATFORM=netbsd
		;;
	*openbsd*)
		AC_MSG_RESULT(openbsd)
		PLATFORM=openbsd
		;;
	*)
		AC_MSG_RESULT(unknown)
		PLATFORM=unknown
		;;
esac
AC_SUBST(PLATFORM)
AM_CONDITIONAL(IS_LINUX, test "x$PLATFORM" = xlinux)
AM_CONDITIONAL(IS_NETBSD, test "x$PLATFORM" = xnetbsd)
AM_CONDITIONAL(IS_OPENBSD, test "x$PLATFORM" = xopenbsd)
AM_CONDITIONAL(IS_UNKNOWN, test "x$PLATFORM" = xunknown)

# Save our CFLAGS/CPPFLAGS/LDFLAGS for the Makefile and restore the old user
# variables.
AC_SUBST(AM_CPPFLAGS)
CPPFLAGS="$SAVED_CPPFLAGS"
AC_SUBST(AM_CFLAGS)
CFLAGS="$SAVED_CFLAGS"
AC_SUBST(AM_LDFLAGS)
LDFLAGS="$SAVED_LDFLAGS"

# autoconf should create a Makefile.
AC_OUTPUT(Makefile)
