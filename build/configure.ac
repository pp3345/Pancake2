dnl Worst build system ever here

AC_INIT([Pancake], [2.0], [support@pancakehttp.net], [pancake], [http://pancakehttp.net])
AC_PREREQ([2.59])
AC_CONFIG_AUX_DIR([aux-build])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([1.10 no-define foreign])
AC_CONFIG_HEADERS([config.h])

AC_GNU_SOURCE

PANCAKE_HAVE_SERVER_ARCHITECTURE=0
PANCAKE_REQUIRE_OPENSSL=0
PANCAKE_REQUIRE_BASE64_DECODER=0

AC_ARG_ENABLE([debug], 
				[AS_HELP_STRING([--enable-debug], [Create a debug build of Pancake])],
				[AC_DEFINE([PANCAKE_DEBUG], [1], [Pancake debug mode])])

AC_MSG_CHECKING([whether to enable debug mode])
if test "$enable_debug" != ""; then
	CFLAGS="-O0 -Wall -Wno-pointer-sign -Wno-parentheses -Wno-missing-braces -g"
	LDFLAGS="-rdynamic"
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

AC_PROG_CC
AM_PROG_CC_C_O

AM_PROG_LEX
if test Z"$LEX" != Zflex; then
AC_ERROR([Flex not found])
fi

AC_PROG_YACC
if test Z"$YACC" != "Zbison -y"; then
AC_ERROR([Bison not found])
fi

AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h stdint.h xlocale.h)
AC_C_CONST
AC_CHECK_FUNCS([newlocale uselocale freelocale])

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
AC_CHECK_HEADER([uthash.h], [], [AC_ERROR([uthash.h not found])])
#AC_CHECK_HEADER([utlist.h], [], [AC_ERROR([utlist.h not found])])
AC_CHECK_HEADER([execinfo.h], [AC_DEFINE([HAVE_EXECINFO_H], [], [Have execinfo.h])])
AC_CHECK_HEADER([errno.h],  [], [AC_ERROR([errno.h not found])])
AC_CHECK_HEADER([valgrind/valgrind.h], [AC_DEFINE([HAVE_VALGRIND_H], [], [Have valgrind/valgrind.h])])
AC_CHECK_HEADER([ucontext.h], [AC_DEFINE([HAVE_UCONTEXT_H], [], [Have ucontext.h])])
AC_CHECK_FUNC([sigaction], [AC_DEFINE([HAVE_SIGACTION], [], [Have sigaction()])])
AC_CHECK_FUNC([vasprintf], [],  [AC_ERROR([vasprintf is not available on your system])])
AC_CHECK_FUNC([accept4], [AC_DEFINE([HAVE_ACCEPT4], [], [Have accept4()])])
AC_CHECK_FUNC([itoa], [AC_DEFINE([HAVE_ITOA], [], [Have itoa()])])

if test "$PANCAKE_REQUIRE_BASE64_DECODER" == "1"; then
	AC_DEFINE([PANCAKE_BASE64_DECODER_DEPENDENCY], [1], [libb64 base64 decoder])
fi

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