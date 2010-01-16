#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([libunicapgtk],[1.0],[arne@unicap-imaging.org])
AM_CONFIG_HEADER(config.h)

#   1. Start with version information of `0:0:0' for each libtool library.
#   2. Update the version information only immediately before a public release of your software. More frequent updates are unnecessary, and only guarantee that the current interface number gets larger faster.
#   3. If the library source code has changed at all since the last update, then increment revision (`c:r:a' becomes `c:r+1:a').
#   4. If any interfaces have been added, removed, or changed since the last update, increment current, and set revision to 0.
#   5. If any interfaces have been added since the last public release, then increment age.
#   6. If any interfaces have been removed since the last public release, then set age to 0. 
lt_major=3
lt_revision=11
lt_age=1

unicapgtk_major_version=0
unicapgtk_minor_version=9
unicapgtk_micro_version=8
unicapgtk_prerelease=
dist_version=$unicapgtk_major_version.$unicapgtk_minor_version.$unicapgtk_micro_version$unicapgtk_prerelease

#lt_major - lt_age
pkg_version=2

AM_INIT_AUTOMAKE(libunicapgtk, $dist_version)

AC_SUBST(lt_major)
AC_SUBST(lt_revision)
AC_SUBST(lt_age)
AC_SUBST(unicapgtk_major_version)
AC_SUBST(unicapgtk_minor_version)
AC_SUBST(unicapgtk_micro_version)
AC_SUBST(dist_version)
AC_SUBST(pkg_version)
	
AC_CONFIG_MACRO_DIR(m4)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_C_VOLATILE



# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([atexit bzero gettimeofday memset munmap strerror strncasecmp strstr])


AM_PROG_CC_C_O

#
# Intltool
#
IT_PROG_INTLTOOL([0.35.0])

#
# Gettext
#
GETTEXT_PACKAGE=unicapgtk
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])

AM_GNU_GETTEXT([external])


GTK_DOC_CHECK(1.4)


# The unicap library
PKG_CHECK_MODULES(UNICAP_PACKAGE, [libunicap libucil])

pkg_modules="gtk+-2.0 >= 2.6.0"
PKG_CHECK_MODULES(GTK_PACKAGE, [$pkg_modules], HAVE_GTK2="yes", HAVE_GTK2="no" )

pkg_modules="glib-2.0"
PKG_CHECK_MODULES(GLIB_PACKAGE, [$pkg_modules], HAVE_GLIB="yes", HAVE_GLIB="no")

# Disable deprecated APIs

GTK_CPPFLAGS="-Wall"

AC_MSG_CHECKING([if GLib is version 2.11.0 or newer])
if $PKG_CONFIG --atleast-version=2.11.0 glib-2.0; then
  have_glib_2_11=yes
else
  have_glib_2_11=no
fi
AC_MSG_RESULT($have_glib_2_11)

if test "x$have_glib_2_11" != "xyes"; then
  GTK_CPPFLAGS="${GTK_CPPFLAGS} -DG_DISABLE_DEPRECATED"
fi

AC_MSG_CHECKING([if GTK+ is version 2.9.0 or newer])
if $PKG_CONFIG --atleast-version=2.9.0 gtk+-2.0; then
  have_gtk_2_9=yes
else
  have_gtk_2_9=no
fi
AC_MSG_RESULT($have_gtk_2_9)

if test "x$have_gtk_2_9" != "xyes"; then
  GTK_CPPFLAGS="${GTK_CPPFLAGS} -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
fi

# Require multihead safe API
GTK_CPPFLAGS="${GTK_CPPFLAGS} -DGDK_MULTIHEAD_SAFE -DGTK_MULTIHEAD_SAFE"

AC_SUBST(GTK_CPPFLAGS)

PKG_CHECK_MODULES(PANGO_PACKAGE, "pangoft2", HAVE_PANGOFT2=yes, HAVE_PANGOFT2=no)

# X11 checks
AC_PATH_XTRA
if test "-DX_DISPLAY_MISSING" = "$X_CFLAGS"; then
  AC_MSG_ERROR(cannot find X11)
fi
# Force inclusion of the common path
if test "x`echo $X_LIBS | grep /usr/X11R6/lib`" = "x"; then
  X_LIBS="$X_LIBS -L/usr/X11R6/lib"
fi
AC_SUBST(X_CFLAGS)
AC_SUBST(X_PRE_LIBS)
AC_SUBST(X_EXTRA_LIBS)
AC_SUBST(X_LIBS)
AC_SUBST(x_libraries)

# Check for the Xv extension
XV_PRESENT=""
AC_CHECK_LIB(Xv, XvShmPutImage, XV_PRESENT="yes",, $X_LIBS -lXext)
if test "x$XV_PRESENT" = "xyes"; then
  AC_DEFINE(HAVE_LIBXV, 1, [Define if XVideo lib is present])
  X_XV_LIBS="-lXv -lXext"
  AC_SUBST(X_XV_LIBS)
fi

# Check for dl
DL_PRESENT=""
AC_CHECK_LIB( dl, dlopen, DL_PRESENT="yes",, $DL_LIBS -ldl )
if test "x$DL_PRESENT" = "xyes"; then
  AC_DEFINE(HAVE_LIBDL, 1, [Define if DL lib is present])
  DL_LIBS="-ldl"
  AC_SUBST(DL_LIBS)
fi

# Check for libm
M_PRESENT=""
AC_CHECK_LIB( m, sin, M_PRESENT="yes",, $M_LIBS -lm )
if test "x$M_PRESENT" = "xyes"; then
  AC_DEFINE(HAVE_LIBM, 1, [Define if libm is present])
  M_LIBS="-lm"
  AC_SUBST(M_LIBS)
fi

# Check for pthread
PTHREAD_PRESENT=""
AC_CHECK_LIB( pthread, pthread_create, PTHREAD_PRESENT="yes",, $PTHREAD_LIBS -lpthread )
if test "x$PTHREAD_PRESENT" = "xyes"; then
  AC_DEFINE(HAVE_LIBPTHREAD, 1, [Define if libpthread is present])
  PTHREAD_LIBS="-lpthread"
  AC_SUBST(PTHREAD_LIBS)
fi

# Check for rt
RT_PRESENT=""
AC_CHECK_LIB( rt, shm_open, RT_PRESENT="yes",, $RT_LIBS -lrt )
if test "x$RT_PRESENT" = "xyes"; then
  AC_DEFINE(HAVE_LIBRT, 1, [Define if librt is present])
  RT_LIBS="-lrt"
  AC_SUBST(RT_LIBS)
fi


AC_ARG_ENABLE(debug-unicapgtk, 
		     AS_HELP_STRING( [--enable-debug-unicapgtk],
				     [Enable debug output] ), 
	      {
		if test "x$enableval" != "xno"; then
		   AC_DEFINE( UNICAPGTK_DEBUG, 1, [Define to enable debug output] )
		else
		   AC_DEFINE( UNICAPGTK_DEBUG, 0, [Define to enable debug output] )
		fi
	      }, 
	      AC_DEFINE( UNICAPGTK_DEBUG, 0, [Define to enable debug output] ) )

AC_ARG_ENABLE(unicapgtk, 
	      AS_HELP_STRING( [--enable-unicapgtk], 
			      [build libunicapgtk: A widget set for easy GTK+ integration \[default=yes\]] ), 
	      {
		if test "x$enableval" != "xno"; then
			UNICAPGTK="yes";
		else
			UNICAPGTK="no";
		fi
	       },
	       UNICAPGTK="yes" )

if test "$UNICAPGTK" = "yes"; then
#	if test "$HAVE_GCONF" != "yes"; then
#		UNICAPGTK="no ( required GConf not found )"
#	fi
	if test "$HAVE_GTK2" != "yes"; then
		UNICAPGTK="no ( required GTK+ not found )"
	fi
	if test "x$XV_PRESENT" != "xyes"; then
		UNICAPGTK="no ( required Xv extensions not found )"
	fi

	if test "x$HAVE_PANGOFT2" != "xyes"; then
		UNICAPGTK="no ( required pangoft2 package not found )"
	fi
fi


AM_CONDITIONAL(BUILD_UNICAPGTK, test x$UNICAPGTK = xyes )

AC_CONFIG_FILES([ Makefile include/Makefile src/Makefile doc/Makefile doc/libunicapgtk/Makefile ])
AC_CONFIG_FILES([ include/unicapgtk_version.h po/Makefile.in])
AC_CONFIG_FILES([ libunicapgtk.pc ])
AC_OUTPUT

AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([              UnicapGTK $dist_version     ])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([  This software is distributed under the terms and ])
AC_MSG_RESULT([  conditions of the GNU GENERAL PUBLIC LICENSE ( GPL ). ])
AC_MSG_RESULT([  See the file COPYING for the full license text. ])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([])
AC_MSG_RESULT([  libunicapgtk:       $UNICAPGTK ])
AC_MSG_RESULT([])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([ ***  Please read the file README.troubleshooting   *** ])
AC_MSG_RESULT([ ***  if you have any trouble using this software.  *** ])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([])

