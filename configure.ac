#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

AC_INIT([fb_flash],[2.0.0],[bcarrington@red-fone.com])
AC_DEFINE_UNQUOTED([FBFLASH_VERSION], ["2.0.0"], [Official Version Number])
AC_CONFIG_SRCDIR([fb_udp.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

# Checks for C compiler
AC_PROG_CC

# Enable `--without-readline'
AC_ARG_WITH([readline],
            [AS_HELP_STRING([--without-readline],[disable support for readline])],
            [],
            [with_readline=yes])

# Enable `--with-shared-libfb''
AC_ARG_WITH([with-shared-libfb],
            [AS_HELP_STRING([--with-shared-libfb], [use shared libfb instead of static version])],
            [with_shared_libfb=yes],
            [])

AC_ARG_WITH([with-public-only],
	    [AS_HELP_STRING([--with-public-only], [only allow public features])],
	    [with_public_only=yes],
	    [])

# Enable `--with-public-only'

PUBLICONLY=
 AS_IF([test "x$with_public_only" == xyes],
 [AC_DEFINE([PUBLIC_RELEASE], [1], [Define if this is a public release])]
 )

# Checks for libraries.
LIBREADLINE=
 AS_IF([test "x$with_readline" != xno],
   [AC_CHECK_LIB([readline], [readline],
			     [AC_SUBST([LIBREADLINE], ["-lreadline"])
			      AC_DEFINE([HAVE_LIBREADLINE], [1], [Define if you have libreadline])
			      ],
			      AC_MSG_FAILURE([test for readline failed (--without-readline to disable)]))])

LIBFB=
 AS_IF([test "x$with_shared_libfb" = xyes],
	AC_CHECK_LIB([fb], [libfb_init],
		     [AC_SUBST([LIBFB], ["-lfb"])
		     AC_DEFINE([HAVE_LIBFB], [1], [Define to 1 if you have libfb])
		     ],
		     AC_MSG_FAILURE([test for libfb failed (required)])),
		     # otherwise, we need to link static libfb and also link in libnet and libpcap
	# Check for static libfb
	AC_CHECK_FILE(/usr/local/lib/libfb.a,
	      [
	      AC_SUBST([LIBFB], ["/usr/local/lib/libfb.a"])
	      AC_DEFINE(HAVE_STATIC_LIBFB, 1, [Define to 1 if you have the static version of libfb in the right place.])
	      ],
	      [AC_MSG_ERROR([Couldn't find static libfb libraries])])
	# Check for shared libnet
	AC_CHECK_LIB([net], [libnet_write],
		     [AC_SUBST([LIBNET], ["-lnet"])
		     AC_DEFINE([HAVE_LIBNET], [1], [Define if you have libnet])
		     ],
		     AC_MSG_FAILURE([test for libnet failed (required)]))
	# Check for shared libpcap
	AC_CHECK_LIB([pcap], [pcap_next],
		     [AC_SUBST([LIBPCAP], ["-lpcap"])
		     AC_DEFINE([HAVE_LIBPCAP], [1], [Define if you have libpcap])
		     ],
		     AC_MSG_FAILURE([test for libpcap failed (required)]))
    )


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h unistd.h netdb.h time.h sys/types.h])

# Hack below, because I think config include files are clashing
AC_CHECK_HEADERS([stdbool.h])

# Library Headers
AC_CHECK_HEADERS([readline/readline.h readline/history.h libnet.h pcap.h libfb/fb_lib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostbyaddr gethostbyname memset socket strdup strtol strchr])
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
