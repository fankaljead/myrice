AC_DEFUN([AM_PATH_OPENSSL],
[
AC_ARG_WITH([openssl-prefix],
            [  --with-openssl-prefix=PREFIX  Prefix where OpenSSL installed (optional)],
            [openssl_prefix=$withval],
            [openssl_prefix=""])

if test "x$openssl_prefix" = "x"; then
  openssl_prefix="/usr/local"
fi

LIBS_save=$LIBS
CPPFLAGS_save=$CPPFLAGS

PKG_CONFIG="$openssl_prefix/bin/pkg-config"
if test -x $PKG_CONFIG; then
  AC_MSG_CHECKING([checking availability of openssl using pkg-config])
  $PKG_CONFIG --exists openssl
  if test "$?" = "0"; then
    # Use pkg-config to detect LIBS and CFLAGS
    OPENSSL_LIBS=`$PKG_CONFIG --libs openssl`
    OPENSSL_CFLAGS=`$PKG_CONFIG --cflags openssl`

    LIBS="$OPENSSL_LIBS $LIBS"
    CPPFLAGS="$OPENSSL_CFLAGS $CPPFLAGS"
    have_openssl=yes
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no])
  fi
fi
if test "x$have_openssl" != "xyes"; then
  openssl_prefix_lib=$openssl_prefix/lib
  openssl_prefix_include=$openssl_prefix/include

  LIBS="-L$openssl_prefix_lib $LIBS"
  CPPFLAGS="-I$openssl_prefix_include $CPPFLAGS"

  AC_CHECK_LIB([ssl], [SSL_library_init], [have_openssl=yes LIBS="-lssl $LIBS"])
  if test "x$have_openssl" = "xyes"; then
    have_openssl=no
    AC_CHECK_LIB([crypto], [main], [have_openssl=yes; LIBS="-lcrypto $LIBS"])
    if test "x$have_openssl" = "xyes"; then
      OPENSSL_LIBS="-L$openssl_prefix_lib -lssl -lcrypto"
      OPENSSL_CFLAGS="-I$openssl_prefix_include"
    fi
  fi
fi

if test "x$have_openssl" = "xyes"; then
  AC_DEFINE([HAVE_LIBSSL], [1], [Define to 1 if you have openssl.])
  # check whether EVP_DigestInit_ex exists. Old openssl doesn't have it.
  AC_CHECK_FUNCS([EVP_DigestInit_ex], [have_digestinit_ex=yes])
  if test "x$have_digestinit_ex" = "x"; then
    AC_DEFINE([HAVE_OLD_LIBSSL], [1], [Define to 1 if you have old openssl.])
  fi
  AC_SUBST(OPENSSL_LIBS)
  AC_SUBST(OPENSSL_CFLAGS)
  # search for sha256 support
  AC_CHECK_FUNCS([EVP_sha256])
fi

LIBS=$LIBS_save
CPPFLAGS=$CPPFLAGS_save
])
