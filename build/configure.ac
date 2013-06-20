dnl Worst build system ever here

AC_INIT([Pancake], [2.0], [support@pancakehttp.net], [pancake], [http://pancakehttp.net])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 no-define foreign])
AC_CONFIG_HEADERS([config.h])

PANCAKE_HAVE_SERVER_ARCHITECTURE=0

AC_ARG_ENABLE([debug], 
				[AS_HELP_STRING([--enable-debug], [Create a debug build of Pancake])],
				[AC_DEFINE([PANCAKE_DEBUG], [1], [Pancake debug mode])])

if test "$enable_debug" != ""; then
	CFLAGS="-O0 -Wdouble-promotion -Wformat -Wimplicit-int -Wunused -Wtype-limits -g"
	LDFLAGS="-rdynamic"
fi

AC_PROG_CC

AC_ARG_WITH([config-path],
				[AS_HELP_STRING([--with-config-path=PATH], [Path to configuration file])],
				[])

if test "$with_config_path" != ""; then
	AC_DEFINE_UNQUOTED(PANCAKE_CONFIG_PATH, ["$with_config_path"], [Pancake configuration path])
fi

MODULES_AUTOCONF

if test "$PANCAKE_HAVE_SERVER_ARCHITECTURE" == "0"; then
	AC_ERROR([You need to build at least one server architecture])
fi

AC_CHECK_SIZEOF([long])
AC_CHECK_LIB([config], [config_init], [], [AC_ERROR([libconfig not found])])
AC_CHECK_HEADER([libconfig.h], [], [AC_ERROR([libconfig.h not found])])
AC_CHECK_HEADER([uthash.h], [], [AC_ERROR([uthash.h not found])])
AC_CHECK_HEADER([utlist.h], [], [AC_ERROR([utlist.h not found])])
AC_CHECK_HEADER([execinfo.h], [AC_DEFINE([HAVE_EXECINFO_H], [], [Have execinfo.h])])
AC_CHECK_HEADER([errno.h],  [], [AC_ERROR([errno.h not found])])
AC_CHECK_HEADER([valgrind/valgrind.h], [AC_DEFINE([HAVE_VALGRIND_H], [], [Have valgrind/valgrind.h])])
AC_CHECK_HEADER([ucontext.h], [AC_DEFINE([HAVE_UCONTEXT_H], [], [Have ucontext.h])])
AC_CHECK_FUNC([sigaction], [AC_DEFINE([HAVE_SIGACTION], [], [Have sigaction()])])
AC_CHECK_FUNC([vasprintf], [],  [AC_ERROR([vasprintf is not available on your system])])
AC_CHECK_FUNC([accept4], [AC_DEFINE([HAVE_ACCEPT4], [], [Have accept4()])])

if test -f "PancakeModules.c"; then
	rm PancakeModules.c
fi

echo "#include \"Pancake.h\"" >> PancakeModules.c
for header in $PANCAKE_MODULE_HEADERS;
do
	echo "#include \"$header\"" >> PancakeModules.c
done

i=0

for module in $PANCAKE_MODULES;
do
	let i++
done

let i++

echo "PancakeModule *PancakeModules@<:@$i@:>@;" >> PancakeModules.c

i=0

echo "void PancakeFetchModules() {" >> PancakeModules.c
for module in $PANCAKE_MODULES;
do
	echo "PancakeModules@<:@$i@:>@ = &$module;" >> PancakeModules.c
	let i++
done
echo "PancakeModules@<:@$i@:>@ = NULL;" >> PancakeModules.c
echo "}" >> PancakeModules.c

AC_CONFIG_FILES([Makefile])

AC_OUTPUT