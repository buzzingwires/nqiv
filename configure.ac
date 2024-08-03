AC_INIT([nqiv], [2024-06-18-01], [buzzingwires@outlook.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
CFLAGS=${CFLAGS} 
AC_PROG_CC
AC_ARG_ENABLE([cross-compile],
			  AS_HELP_STRING([--enable-cross-compile], [Enable cross compiling using MinGW on a posix system.]),
			  [AS_IF([test "$enable_cross_compile" = "yes"], [],
					 [test "$enable_cross_compile" = "no"], [],
					 [AC_MSG_ERROR([Bad value ${enable_cross_compile} for --enable-cross-compile], [])])],
			  [enable_cross_compiler=no])
AM_CONDITIONAL([ISCROSS], [test "$enable_cross_compile" = "yes"])
AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_OUTPUT
