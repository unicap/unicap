#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([libunicap],[1.0],[arne@unicap-imaging.org])
AM_CONFIG_HEADER(config.h)

#   1. Start with version information of `0:0:0' for each libtool library.
#   2. Update the version information only immediately before a public release of your software. More frequent updates are unnecessary, and only guarantee that the current interface number gets larger faster.
#   3. If the library source code has changed at all since the last update, then increment revision (`c:r:a' becomes `c:r+1:a').
#   4. If any interfaces have been added, removed, or changed since the last update, increment current, and set revision to 0.
#   5. If any interfaces have been added since the last public release, then increment age.
#   6. If any interfaces have been removed since the last public release, then set age to 0. 
lt_major=4
lt_revision=0
lt_age=2

unicap_major_version=0
unicap_minor_version=9
unicap_micro_version=12
unicap_prerelease=
dist_version=$unicap_major_version.$unicap_minor_version.$unicap_micro_version$unicap_prerelease

#lt_major - lt_age
pkg_version=2

AM_INIT_AUTOMAKE(libunicap, $dist_version)

AC_SUBST(lt_major)
AC_SUBST(lt_revision)
AC_SUBST(lt_age)
AC_SUBST(unicap_major_version)
AC_SUBST(unicap_minor_version)
AC_SUBST(unicap_micro_version)
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
GETTEXT_PACKAGE=unicap
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])

AM_GNU_GETTEXT([external])


GTK_DOC_CHECK(1.4)


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


# Check for raw1394
pkg_modules="libraw1394 <= 1.0.0"
PKG_CHECK_MODULES(LIBRAW1394_PACKAGE, [$pkg_modules], HAVE_LIBRAW1394="yes";LIBRAW1394_VERSION="RAW1394_1_0_API", HAVE_LIBRAW1394="no" )

if( test "$HAVE_LIBRAW1394" != "yes" ); then
    pkg_modules="libraw1394 >= 1.1.0"
    PKG_CHECK_MODULES(LIBRAW1394_PACKAGE, [$pkg_modules], HAVE_LIBRAW1394="yes";LIBRAW1394_VERSION="RAW1394_1_1_API", HAVE_LIBRAW1394="no" )
    if( test "$HAVE_LIBRAW1394" == "yes" ); then
	AC_DEFINE(RAW1394_1_1_API, 1, [Define if using the new raw1394 API])
    fi
else
    AC_DEFINE(RAW1394_1_0_API, 1, [Define if using the old raw1394 API])
fi
AC_SUBST(LIBRAW1394_PACKAGE_CFLAGS)
AC_SUBST(LIBRAW1394_PACKAGE_LIBS)
AC_SUBST(LIBRAW1394_VERSION)

AC_ARG_ENABLE(debug-vid21394, 
		     AS_HELP_STRING( [--enable-debug-vid21394],
				     [Enable debug output] ), 
	      {
		if test "x$enableval" != "xno"; then
		   AC_DEFINE( VID21394_DEBUG, 1, [Define to enable debug output] )
		else
		   AC_DEFINE( VID21394_DEBUG, 0, [Define to enable debug output] )
		fi
	      }, 
	      AC_DEFINE( VID21394_DEBUG, 0, [Define to enable debug output] ) )

AC_ARG_ENABLE(vid21394-bootload, 
				 AS_HELP_STRING( [--enable-vid21394-bootload], 
						 [Enable vid21394 bootload support ( dangerous! )] ), 
	      {
	        if test "x$enableval" != "xno"; then
		   AC_DEFINE( VID21394_BOOTLOAD, 1, [Define to enable bootload support] )
		else
		   AC_DEFINE( VID21394_BOOTLOAD, 0, [Define to enable bootload support] )
		fi
	      }, 
       	      AC_DEFINE( VID21394_BOOTLOAD, 0, [Define to enable bootload support] ) )

VID21394_VISCA="no"

AC_ARG_ENABLE(vid21394-visca, 
				 AS_HELP_STRING( [--enable-vid21394-visca], 
						 [Enable vid21394 visca protocol support] ), 
	      {
	        if test "x$enableval" != "xno"; then
		   AC_DEFINE( VID21394_VISCA, 1, [Define to enable vid21394 visca support] )
		   VID21394_VISCA="yes"
		else
		   AC_DEFINE( VID21394_VISCA, 0, [Define to enable vid21394 visca support] )
		fi
	      }, 
       	      AC_DEFINE( VID21394_VISCA, 0, [Define to enable vid21394 visca support] ) )

AM_CONDITIONAL(VID21394_VISCA, test x$VID21394_VISCA = xyes )

VID21394_21CF04="no"

AC_ARG_ENABLE(vid21394-21cf04, 
				 AS_HELP_STRING( [--enable-vid21394-21cf04], 
						 [Enable autodetection of DFK 21CF04 camera module ( default=yes )] ), 
	      {
	        if test "x$enableval" != "xno"; then
		   AC_DEFINE( VID21394_DETECT_21CF04, 1, [Define to enable autodetection of DFK 21CF04 camera module] )
		else
		   AC_DEFINE( VID21394_DETECT_21CF04, 0, [Define to enable autodetection of DFK 21CF04 camera module] )
		fi
	      }, 
       	      AC_DEFINE( VID21394_DETECT_21CF04, 1, [Define to enable autodetection of DFK 21CF04 camera module] ) )



AC_ARG_ENABLE(debug-dcam, 
		     AS_HELP_STRING( [--enable-debug-dcam],
				     [Enable debug output] ), 
	      {
		if test "x$enableval" != "xno"; then
		   AC_DEFINE( DCAM_DEBUG, 1, [Define to enable debug output] )
		else
		   AC_DEFINE( DCAM_DEBUG, 0, [Define to enable debug output] )
		fi
	      }, 
	      AC_DEFINE( DCAM_DEBUG, 0, [Define to enable debug output] ) )

AC_ARG_ENABLE(debug-euvccam, 
		     AS_HELP_STRING( [--enable-debug-euvccam],
				     [Enable debug output] ), 
	      {
		if test "x$enableval" != "xno"; then
		   AC_DEFINE( EUVCCAM_DEBUG, 1, [Define to enable debug output] )
		else
		   AC_DEFINE( EUVCCAM_DEBUG, 0, [Define to enable debug output] )
		fi
	      }, 
	      AC_DEFINE( EUVCCAM_DEBUG, 0, [Define to enable debug output] ) )

AC_ARG_ENABLE(debug-v4l, 
		     AS_HELP_STRING( [--enable-debug-v4l],
				     [Enable debug output] ), 
	      {
		if test "x$enableval" != "xno"; then
		   AC_DEFINE( V4L_DEBUG, 1, [Define to enable debug output] )
		else
		   AC_DEFINE( V4L_DEBUG, 0, [Define to enable debug output] )
		fi
	      }, 
	      AC_DEFINE( V4L_DEBUG, 0, [Define to enable debug output] ) )

AC_ARG_ENABLE(debug-v4l2, 
		     AS_HELP_STRING( [--enable-debug-v4l2],
				     [Enable debug output] ), 
	      {
		if test "x$enableval" != "xno"; then
		   AC_DEFINE( V4L2_DEBUG, 1, [Define to enable debug output] )
		else
		   AC_DEFINE( V4L2_DEBUG, 0, [Define to enable debug output] )
		fi
	      }, 
	      AC_DEFINE( V4L2_DEBUG, 0, [Define to enable debug output] ) )

AC_ARG_ENABLE(debug-thing, 
		     AS_HELP_STRING( [--enable-debug-thing],
				     [Enable debug output] ), 
	      {
		if test "x$enableval" != "xno"; then
		   AC_DEFINE( THING_DEBUG, 1, [Define to enable debug output] )
		else
		   AC_DEFINE( THING_DEBUG, 0, [Define to enable debug output] )
		fi
	      }, 
	      AC_DEFINE( THING_DEBUG, 0, [Define to enable debug output] ) )

AC_ARG_ENABLE(debug-unicap, 
		     AS_HELP_STRING( [--enable-debug-unicap],
				     [Enable debug output] ), 
	      {
		if test "x$enableval" != "xno"; then
		   AC_DEFINE( UNICAP_DEBUG, 1, [Define to enable debug output] )
		else
		   AC_DEFINE( UNICAP_DEBUG, 0, [Define to enable debug output] )
		fi
	      }, 
	      AC_DEFINE( UNICAP_DEBUG, 0, [Define to enable debug output] ) )

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

AC_ARG_ENABLE(vid21394, 
	      AS_HELP_STRING( [--enable-vid21394], 
		              [build plugin for video-2-1394 capture device \[default=yes\]] ),
	      {
	      	if test "x$enableval" != "xno"; then
			VID21394="yes";
	      	else
			VID21394="no";
	      	fi
	      },
	      VID21394="yes" )
if test "$VID21394" = "yes"; then
	if test "$HAVE_LIBRAW1394" != "yes"; then
		AC_MSG_RESULT([ video-to-1394 plugin requires recent libraw1394! ])
		VID21394="no ( missing libraw1394 )"
	fi
fi

if test "x$VID21394" == "xyes"; then
   AC_DEFINE( BUILD_VID21394, 1, [Define to build vid21394 cpi] )
fi

AC_ARG_ENABLE(dcam, 
	      AS_HELP_STRING( [--enable-dcam], 
		              [build plugin for IIDC 1394 cameras \[default=yes\]] ),
	      {
	      	if test "x$enableval" != "xno"; then
			DCAM="yes";
	      	else
			DCAM="no";
	      	fi
	      },
	      DCAM="yes" )
if test "$DCAM" = "yes"; then
	if test "$HAVE_LIBRAW1394" != "yes"; then
		AC_MSG_RESULT([ IIDC 1394 camera plugin requires recent libraw1394! ])
		DCAM="no ( missing libraw1394 )"
	fi
fi

if test "x$DCAM" == "xyes"; then
   AC_DEFINE( BUILD_DCAM, 1, [Define to build dcam cpi] )
fi


AC_ARG_ENABLE(v4l, 
	      AS_HELP_STRING( [--enable-v4l], 
		              [build plugin for video-4-linux ( version 1 ) interface \[default=yes\]] ),
	      {
	      	if test "x$enableval" != "xno"; then
			V4L="yes";
	      	else
			V4L="no";
	      	fi
	      },
	      V4L="yes" )

if test "x$V4L" == "xyes"; then
   AC_DEFINE( BUILD_V4L, 1, [Define to build v4l cpi] )
fi


AC_ARG_ENABLE(v4l2, 
	      AS_HELP_STRING( [--enable-v4l2], 
		              [build plugin for video-4-linux ( version 2 ) interface \[default=yes\]] ),
	      {
	      	if test "x$enableval" != "xno"; then
			V4L2="yes";
	      	else
			V4L2="no";
	      	fi
	      },
	      V4L2="yes" )

if test "x$V4L2" == "xyes"; then
   AC_DEFINE( BUILD_V4L2, 1, [Define to build v4l2 cpi] )
fi

AC_ARG_ENABLE(libv4l, 
	      AS_HELP_STRING( [--enable-libv4l],
	      		      [use libv4l to communicate with v4l2 devices \[default=no\]] ),
	      {
		if test "x$enableval" != "xno"; then
		   USE_LIBV4L="yes";
		else
		   USE_LIBV4L="no";
		fi
	      },
	      USE_LIBV4L="no" )

if test "x$USE_LIBV4L" == "xyes"; then
   PKG_CHECK_MODULES(LIBV4L, libv4l2, HAVE_LIBV4L="yes", HAVE_LIBV4L="no")
   if test "x$HAVE_LIBV4L" == "xyes"; then
      AC_MSG_RESULT( enable enable enable )
      AC_DEFINE( [USE_LIBV4L], 1, Define to enable libv4l support )
   else
      AC_DEFINE( [USE_LIBV4L], 0, Define to enable libv4l support )
   fi
fi
AM_CONDITIONAL(HAVE_LIBV4L, test x$HAVE_LIBV4L == "xyes" )

AC_ARG_ENABLE(euvccam, 
	      AS_HELP_STRING( [--enable-euvccam], 
		              [build plugin for The Imaging Source USB CMOS cameras (experimental) \[default=yes\]] ),
	      {
	      	if test "x$enableval" != "xno"; then
			EUVCCAM="yes";
	      	else
			EUVCCAM="no";
	      	fi
	      },
	      EUVCCAM="yes" )

AC_ARG_ENABLE(thing, 
	      AS_HELP_STRING( [--enable-thing], 
		              [build plugin for a dummy device ( usefull for debugging ) \[default=no\]] ),
	      {
	      	if test "x$enableval" != "xno"; then
			THING="yes"
			THING_SUBDIR="thing"
	      	else
			THING="no";
	      	fi
	      },
	      THING="no" )

AC_ARG_ENABLE(static-cpi, 
			      AS_HELP_STRING( [--enable-static-cpi],  
					      [Static link the cpi modules ( default=no )] ),
	      {
	        if test "x$enableval" == "xyes"; then
		   AC_DEFINE( ENABLE_STATIC_CPI, 1, [Define to enable static cpi])
		   ENABLE_STATIC_CPI="yes";
		fi   
	      }, ENABLE_STATIC_CPI="no" )

AM_CONDITIONAL(ENABLE_STATIC_CPI, test x$ENABLE_STATIC_CPI = xyes )

AC_ARG_ENABLE(thread-locking, 
			       AS_HELP_STRING( [--enable-thread-locking],
					       [ (default=yes) ] ), 
	       {
	         if test "x$enableval" == "xyes"; then
		    AC_DEFINE( UNICAP_THREAD_LOCKING, 1, [Define to enable thread locking] )
		 fi
	       }, AC_DEFINE( UNICAP_THREAD_LOCKING, 1, [Define to enable thread locking] ) )

AC_ARG_ENABLE(unicap-threads, 
			      AS_HELP_STRING( [--enable-unicap-threads], 
					      [ (default=yes) ] ), 
	       {
	         if test "x$enableval" == "xyes"; then
		    AC_DEFINE( UNICAP_THREADS, 1, [Define to enable threads] )
		 fi
	       }, AC_DEFINE( UNICAP_THREADS, 1, [Define to enable threads] ) )


AM_CONDITIONAL(BUILD_VID21394, test x$VID21394 = xyes )
AM_CONDITIONAL(BUILD_DCAM, test x$DCAM = xyes )
AM_CONDITIONAL(BUILD_V4L, test x$V4L = xyes )
AM_CONDITIONAL(BUILD_V4L2, test x$V4L2 = xyes )
AM_CONDITIONAL(BUILD_EUVCCAM, test x$EUVCCAM = xyes)
AM_CONDITIONAL(BUILD_THING, test x$THING = xyes )

AC_CONFIG_FILES([ include/unicap_version.h po/Makefile.in include/Makefile src/Makefile common/Makefile ])
AC_CONFIG_FILES([ Makefile libunicap.pc ])
AC_CONFIG_FILES([ cpi/vid21394/Makefile ])
AC_CONFIG_FILES([ cpi/dcam/Makefile ])
AC_CONFIG_FILES([ cpi/v4l/Makefile ])
AC_CONFIG_FILES([ cpi/v4l2cpi/Makefile ])
AC_CONFIG_FILES([ cpi/euvccam/Makefile ])
AC_CONFIG_FILES([ cpi/thing/Makefile ])
AC_CONFIG_FILES([ cpi/Makefile cpi/include/Makefile ])
AC_CONFIG_FILES([ doc/libunicap/Makefile doc/Makefile ])
AC_CONFIG_FILES([ data/Makefile ])
AC_OUTPUT

if test x$VID21394_VISCA = xyes; then
	VID21394="$VID21394 with visca support"
fi

if test x$HAVE_LIBV4L == xyes && test x$V4L2 == xyes; then
   V4L2="yes, with libv4l2 support"
fi
   

AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([              Unicap $dist_version     ])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([  This software is distributed under the terms and ])
AC_MSG_RESULT([  conditions of the GNU GENERAL PUBLIC LICENSE ( GPL ). ])
AC_MSG_RESULT([  See the file COPYING for the full license text. ])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([])
AC_MSG_RESULT([Configuration: ])
AC_MSG_RESULT([  libraw1394 version: $LIBRAW1394_VERSION ])
AC_MSG_RESULT([  installation goes to: $prefix ])
#AC_MSG_RESULT([  static cpi: $ENABLE_STATIC_CPI ])
AC_MSG_RESULT([])
AC_MSG_RESULT([Plugins: ])
AC_MSG_RESULT([  video-to-1394:      $VID21394 ])
AC_MSG_RESULT([  IIDC 1394 camera:   $DCAM ])
AC_MSG_RESULT([  video-4-linux:      $V4L ])
AC_MSG_RESULT([  video-4-linux v2:   $V4L2 ])
AC_MSG_RESULT([  euvccam:            $EUVCCAM ])
AC_MSG_RESULT([])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([ ***  Please read the file README.troubleshooting   *** ])
AC_MSG_RESULT([ ***  if you have any trouble using this software.  *** ])
AC_MSG_RESULT([ ****************************************************** ])
AC_MSG_RESULT([])

