AC_INIT([nqiv], [1.0.0-beta], [buzzingwires@outlook.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
CFLAGS=${CFLAGS} 
AC_PROG_CC
AC_ARG_ENABLE([cross-compile],
			  AS_HELP_STRING([--enable-cross-compile], [Enable cross compiling using MinGW on a posix system.]),
			  [AS_IF([test "$enable_cross_compile" = "yes"], [],
					 [test "$enable_cross_compile" = "no"], [],
					 [AC_MSG_ERROR([Bad value ${enable_cross_compile} for --enable-cross-compile], [])])],
			  [enable_cross_compiler=no])
AC_ARG_ENABLE([define-prefix],
			  AS_HELP_STRING([--enable-define-prefix], [Enable passing --define-prefix to package config, to choose path prefixes based on the location of the files. Useful for alternate toolchains, such as in the case of cross-compiling.]),
			  [AS_IF([test "$enable_define_prefix" = "yes"], [],
					 [test "$enable_define_prefix" = "no"], [],
					 [AC_MSG_ERROR([Bad value ${enable_define_prefix} for --enable-define_prefix], [])])],
			  [enable_define_prefix=no])
AC_ARG_ENABLE([coverage],
			  AS_HELP_STRING([--enable-coverage], [Pass --coverage for coverage analysis.]),
			  [AS_IF([test "$enable_coverage" = "yes"], [],
					 [test "$enable_coverage" = "no"], [],
					 [AC_MSG_ERROR([Bad value ${enable_coverage} for --enable-coverage], [])])],
			  [enable_coverage=no])
AC_ARG_ENABLE([no-jemalloc],
			  AS_HELP_STRING([--enable-no-jemalloc], [Do not look for or use the jemalloc allocator. This is especially useful on FreeBSD systems where jemalloc is already the default allocator.]),
			  [AS_IF([test "$enable_no_jemalloc" = "yes"], [],
					 [AC_MSG_ERROR([Bad value ${enable_no_jemalloc} for --enable-no-jemalloc], [])])],
			  [enable_no_jemalloc=no])
AM_CONDITIONAL([ISCROSS], [test "$enable_cross_compile" = "yes"])
AM_CONDITIONAL([DEFINEPREFIX], [test "$enable_define_prefix" = "yes"])
AM_CONDITIONAL([USECOV], [test "$enable_coverage" = "yes"])
AM_CONDITIONAL([USEJEMALLOC], [test "$enable_no_jemalloc" = "no"])
AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_OUTPUT
