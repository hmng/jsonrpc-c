AC_INIT([jsonrpc-c], [0.1], [hmng@farol.pt])
AC_CONFIG_SRCDIR([src/jsonrpc-c.c])

m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
LT_INIT
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AM_PROG_CC_C_O
AC_CONFIG_MACRO_DIRS([m4])

#AC_CHECK_LIB([ev], [ev_io_init],[], [AC_MSG_ERROR([libev was not found])])
#AC_CHECK_LIB([ev],[ev_sleep],[],[AC_MSG_ERROR([libev was not found])])

# stollen from lighttpd's configure.ac
AC_MSG_CHECKING([for libev support])
AC_ARG_WITH([libev],
    [AS_HELP_STRING([--with-libev@<:@=PATH@:>@],[Search for libev in PATH/include and PATH/lib])],
    [WITH_LIBEV=$withval],[WITH_LIBEV=yes])

LIBEV_CFLAGS="-lm"
LIBEV_LIBS=""

PKG_CHECK_MODULES([LIBEV], [libev], [], [
	# no pkg-config for libev, searching manually:
	if test "$WITH_LIBEV" != "yes"; then
		LIBEV_CFLAGS="-I$WITH_LIBEV/include"
		LIBEV_LIBS="-L$WITH_LIBEV/lib -lev"
	else
		AC_CHECK_HEADERS([ev.h],[
			AC_CHECK_LIB([ev], [ev_time], [
					LIBEV_LIBS="-lev"
				],[
					AC_MSG_ERROR([libev not found])
				]
			)],[
				AC_MSG_ERROR([libev not found])
			]
		)
	fi
])

AC_SUBST([LIBEV_CFLAGS])
AC_SUBST([LIBEV_LIBS])

AC_MSG_RESULT([
   LIBEV_CFLAGS: ${LIBEV_CFLAGS}
   LIBEV_LIBS: ${LIBEV_LIBS}
])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 include/Makefile
 src/Makefile
 example/Makefile
])
AC_OUTPUT

