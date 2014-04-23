AC_ARG_ENABLE([openssl], 
				[AS_HELP_STRING([--enable-openssl], [Enable OpenSSL cryptographic network layer module])],
				[])
				
if test "$enable_openssl" == "yes"; then
	PANCAKE_MODULE_HEADERS+="OpenSSL/PancakeOpenSSL.h "
	PANCAKE_MODULES+="PancakeOpenSSL "
	AC_DEFINE([PANCAKE_OPENSSL], [1], [OpenSSL module])
	
	AC_CHECK_HEADER([errno.h],  [], [AC_ERROR([errno.h not found])])
	
	AC_CHECK_HEADER([openssl/ssl.h],  [], [AC_ERROR([openssl/ssl.h not found])])
	AC_CHECK_HEADER([openssl/err.h],  [], [AC_ERROR([openssl/err.h not found])])
	AC_CHECK_HEADER([openssl/evp.h],  [], [AC_ERROR([openssl/evp.h not found])])
	AC_CHECK_HEADER([openssl/conf.h], [], [AC_ERROR([openssl/conf.h not found])])
	AC_CHECK_HEADER([openssl/engine.h], [], [AC_ERROR([openssl/engine.h not found])])
	AC_CHECK_LIB([ssl], [SSL_CTX_new], [], [AC_ERROR([OpenSSL library not found])])
fi