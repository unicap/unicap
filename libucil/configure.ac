#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([libucil],[1.0],[arne@unicap-imaging.org])
AM_CONFIG_HEADER(config.h)

#   1. Start with version information of `0:0:0' for each libtool library.
#   2. Update the version information only immediately before a public release of your software. More frequent updates are unnecessary, and only guarantee that the current interface number gets larger faster.
#   3. If the library source code has changed at all since the last update, then increment revision (`c:r:a' becomes `c:r+1:a').

#   4. If any interfaces have been added, removed, or changed since the last update, increment current, and set revision to 0.
#   5. If any interfaces have been added since the last public release, then increment age.
#   6. If any interfaces have been removed since the last public release, then set age to 0. 
lt_major=6
lt_revision=0
lt_age=4

ucil_major_version=0
ucil_minor_version=9
ucil_micro_version=10
ucil_prerelease=
dist_version=$ucil_major_version.$ucil_minor_version.$ucil_micro_version$ucil_prerelease

#lt_major - lt_age
pkg_version=2

AM_INIT_AUTOMAKE(libucil, $dist_version)

AC_SUBST(lt_major)
AC_SUBST(lt_revision)
AC_SUBST(lt_age)
AC_SUBST(ucil_major_version)
AC_SUBST(ucil_minor_version)
AC_SUBST(ucil_micro_version)
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
# Gettext
#
GETTEXT_PACKAGE=ucil
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])

AM_GNU_GETTEXT([external])


GTK_DOC_CHECK(1.4)

# The unicap library
PKG_CHECK_MODULES(UNICAP, [libunicap])

pkg_modules="glib-2.0"
PKG_CHECK_MODULES(GLIB_PACKAGE, [$pkg_modules], HAVE_GLIB="yes", HAVE_GLIB="no")

# Disable deprecated APIs
AC_MSG_CHECKING([if GLib is version 2.11.0 or newer])
if $PKG_CONFIG --atleast-version=2.11.0 glib-2.0; then
  have_glib_2_11=yes
else
  have_glib_2_11=no
fi
AC_MSG_RESULT($have_glib_2_11)

PKG_CHECK_MODULES(PANGO_PACKAGE, "pangoft2", HAVE_PANGOFT2=yes, HAVE_PANGOFT2=no)

UCIL_CODEC_NONE="none"

UCIL_AVCODEC=""


AC_ARG_ENABLE(ucil-avcodec, 
		     AS_HELP_STRING( [--enable-ucil-avcodec],
				     [Enable avcodec support] ), 
	      {
		if test "x$enableval" != "xno"; then
		   USE_AVCODEC=true
		else
		   USE_AVCODEC=
		fi
	      }, 
	      USE_AVCODEC= )

UCIL_AVCODEC=""
if [[ $USE_AVCODEC ]] ; then
   PKG_CHECK_MODULES( AVCODEC_PACKAGE, "libavcodec", HAVE_AVCODEC=1, HAVE_AVCODEC= )
   
   if [[ $HAVE_AVCODEC ]]; then
      UCIL_AVCODEC="avcodec"
      UCIL_CODEC_NONE=""
      AC_DEFINE( [HAVE_AVCODEC], 1, Define to use avcodec )
   else
      AC_DEFINE( [HAVE_AVCODEC], 0, Define to use avcodec )
   fi
else
   AC_DEFINE( [HAVE_AVCODEC], 0, Define to use avcodec )
fi
AM_CONDITIONAL(HAVE_AVCODEC, test x$UCIL_AVCODEC == "xavcodec" )


AC_ARG_ENABLE(ucil-theora, 
		     AS_HELP_STRING( [--enable-ucil-theora],
				     [Enable ogg/theora support] ), 
	      {
		if test "x$enableval" != "xno"; then
		   USE_THEORA=true
		else
		   USE_THEORA=
		fi
	      }, 
	      USE_THEORA=true )

UCIL_THEORA=""
if [[ $USE_THEORA ]] ; then
   PKG_CHECK_MODULES(THEORA_PACKAGE, "ogg theora vorbis vorbisenc", HAVE_THEORA=1, HAVE_THEORA= )
   
   if [[ $HAVE_THEORA ]]; then
      UCIL_THEORA="ogg/theora"
      UCIL_CODEC_NONE=""
      AC_DEFINE( HAVE_THEORA, 1, Define to use theora video support )
   else
      AC_DEFINE( HAVE_THEORA, 0, Define to use theora video support )
   fi
else
   AC_DEFINE( HAVE_THEORA, 0, Define to use theora video support )
fi
AM_CONDITIONAL(HAVE_THEORA, test x$UCIL_THEORA == "xogg/theora" )


AC_ARG_ENABLE(ucil-alsa, 
		     AS_HELP_STRING( [--enable-ucil-alsa],
				     [Enable alsa sound support] ), 
	      {
		if test "x$enableval" != "xno"; then
		   USE_ALSA=true
		else
		   USE_ALSA=
		fi
	      }, 
	      USE_ALSA=true )

UCIL_ALSA=""
if [[ $USE_ALSA ]] ; then
   PKG_CHECK_MODULES(ALSA_PACKAGE, "alsa", HAVE_ALSA=1, HAVE_ALSA= )
   
   if [[ $HAVE_ALSA ]]; then
      UCIL_ALSA="alsa"
      UCIL_CODEC_NONE=""
      AC_DEFINE( HAVE_ALSA, 1, Define to use alsa sound support )
   else
      AC_DEFINE( HAVE_ALSA, 0, Define to use alsa sound support )
   fi
else
   AC_DEFINE( HAVE_ALSA, 0, Define to use alsa sound support )
fi
AM_CONDITIONAL(HAVE_ALSA, test x$UCIL_ALSA == "xalsa" )

AC_ARG_ENABLE(ucil-gstreamer, 
		     AS_HELP_STRING( [--enable-ucil-gstreamer],
				     [Enable GStreamer support] ), 
	      {
		if test "x$enableval" != "xno"; then
		   USE_GSTREAMER=true
		else
		   USE_GSTREAMER=
		fi
	      }, 
	      USE_GSTREAMER=true )

UCIL_GSTREAMER=""
if [[ $USE_GSTREAMER ]] ; then
   PKG_CHECK_MODULES(GSTREAMER_PACKAGE, "gstreamer-0.10", HAVE_GSTREAMER=1, HAVE_GSTREAMER= )
   GSTAPP_PRESENT=""
   AC_CHECK_LIB( gstapp-0.10, gst_app_src_push_buffer, GSTAPP_PRESENT="yes",, `pkg-config --libs gstreamer-0.10` -lgstapp-0.10)
   if test "x$GSTAPP_PRESENT" = "xyes"; then
     AC_DEFINE(HAVE_LIBGSTAPP, 1, [Define if gstapp lib is present])
     GSTAPP_LIBS="-lgstapp-0.10 -lgstvideo-0.10"
     AC_SUBST(GSTAPP_LIBS)
   else
     HAVE_GSTREAMER=
   fi
   
   if [[ $HAVE_GSTREAMER ]]; then
      UCIL_GSTREAMER="gstreamer"
      UCIL_CODEC_NONE=""
      AC_DEFINE( HAVE_GSTREAMER, 1, Define to use gstreamer )
   else
      AC_DEFINE( HAVE_GSTREAMER, 0, Define to use gstreamer )
   fi
else
   AC_DEFINE( HAVE_GSTREAMER, 0, Define to use gstreamer )
fi
AM_CONDITIONAL(HAVE_GSTREAMER, test x$UCIL_GSTREAMER == "xgstreamer" )

AC_ARG_ENABLE([gpl], 
              AS_HELP_STRING( [--enable-gpl],
	      		      [Allow external GPL licensed modules to be used] ),
	      [ case "${enableval}" in
	        yes) AC_DEFINE( ALLOW_GPL, 1, Define to allow external GPL licensed modules )
		     gpl=true ;;

		no) AC_DEFINE( ALLOW_GPL, 0, Define to allow external GPL licensed modules )
		    gpl=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-gpl]) ;;
	      esac],[gpl=false])
AM_CONDITIONAL([ALLOW_GPL], [text x$gpl = xtrue])



AC_ARG_ENABLE(ucil-png, 
		     AS_HELP_STRING( [--enable-ucil-png],
				     [Enable PNG support] ), 
	      {
		if test "x$enableval" != "xno"; then
		   USE_PNG=true
		else
		   USE_PNG=
		fi
	      }, 
	      USE_PNG=true )

UCIL_PNG=""
if [[ $USE_PNG ]] ; then
   PKG_CHECK_MODULES(PNG_PACKAGE, "libpng", HAVE_PNG=1, HAVE_PNG= )
   
   if [[ $HAVE_PNG ]]; then
      UCIL_PNG="png"
      AC_DEFINE( HAVE_PNG, 1, Define to compile in PNG support )
   else
      AC_DEFINE( HAVE_PNG, 0, Define to compile in PNG support )
   fi
else
   AC_DEFINE( HAVE_PNG, 0, Define to compile in PNG support )
fi
AM_CONDITIONAL(HAVE_PNG, test x$UCIL_PNG == "xpng" )





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


AC_ARG_ENABLE(debug-ucil, 
		     AS_HELP_STRING( [--enable-debug-ucil],
				     [Enable debug output] ), 
	      {
		if test "x$enableval" != "xno"; then
		   AC_DEFINE( UCIL_DEBUG, 1, [Define to enable debug output] )
		else
		   AC_DEFINE( UCIL_DEBUG, 0, [Define to enable debug output] )
		fi
	      }, 
	      AC_DEFINE( UCIL_DEBUG, 0, [Define to enable debug output] ) )


AC_ARG_ENABLE(ucil, 
	      AS_HELP_STRING( [--enable-ucil], 
			      [build libucil: unicap imaging library \[default=yes\]] ), 
	      {
		if test "x$enableval" != "xno"; then
			UCIL="yes";
		else
			UCIL="no";
		fi
	       },
	       UCIL="yes" )

if test "$UCIL" = "yes"; then 
	if test "$HAVE_GLIB" != "yes" ; then
	   UCIL="no ( missing glib )"
	fi
	if test "x$HAVE_PANGOFT2" != "xyes"; then
		UCIL="no ( required pangoft2 package not found )"
	fi
fi

AM_CONDITIONAL(BUILD_UCIL, test x$UCIL = xyes )

AC_CONFIG_FILES([ include/ucil_version.h libucil.pc ])
AC_CONFIG_FILES([ Makefile src/Makefile include/Makefile ])
AC_CONFIG_FILES([ doc/Makefile doc/libucil/Makefile ])
AC_OUTPUT

   

AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([              UCIL $dist_version     ])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([  This software is distributed under the terms and ])
AC_MSG_RESULT([  conditions of the GNU GENERAL PUBLIC LICENSE ( GPL ). ])
AC_MSG_RESULT([  See the file COPYING for the full license text. ])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([])
AC_MSG_RESULT([  libucil:            $UCIL ])
AC_MSG_RESULT([  UCIL modules: $UCIL_CODEC_NONE $UCIL_THEORA $UCIL_AVCODEC $UCIL_ALSA $UCIL_PNG $UCIL_GSTREAMER ])
AC_MSG_RESULT([])

