from distutils.core import setup, Extension
import os

MODULES_UNICAP='libunicap libucil glib-2.0 pygobject-2.0'
MODULES_UNICAPGTK=MODULES_UNICAP + ' libunicapgtk'


def parse_cflags(cflags):
    """
    Returns (Includes,extra_cflags)
    """
    return ([x[2:] for x in cflags.split() if x[0:2] == '-I'], [x for x in cflags.split() if x[0:2] != '-I'])

def parse_libs(libs):
    """
    Returns (Libdirs,Libs)
    """
    return ([x[2:] for x in libs.split() if x[0:2] == '-L'], [x[2:] for x in libs.split() if x[0:2] == '-l'])

pkgcfg = os.popen( 'pkg-config --cflags ' + MODULES_UNICAP )
pkgcflags_unicap = pkgcfg.readline().strip()
pkgcfg.close()
pkgcfg = os.popen( 'pkg-config --cflags ' + MODULES_UNICAPGTK )
pkgcflags_unicapgtk = pkgcfg.readline().strip()
pkgcfg.close()
pkgcfg = os.popen( 'pkg-config --libs ' + MODULES_UNICAP )
pkglibs_unicap = pkgcfg.readline().strip()
pkgcfg.close()
pkgcfg = os.popen( 'pkg-config --libs ' + MODULES_UNICAPGTK )
pkglibs_unicapgtk = pkgcfg.readline().strip()
pkgcfg.close()


iflags_unicap, extra_cflags_unicap = parse_cflags(pkgcflags_unicap)
libdirs_unicap, libs_unicap = parse_libs(pkglibs_unicap)


unicapmodule = Extension('unicap',
			 sources = ['unicapmodule.c', 'unicapdevice.c', 'unicapimagebuffer.c', 'utils.c'],
			 include_dirs = iflags_unicap,
			 extra_compile_args = extra_cflags_unicap + ["-O0","-g"],
			 library_dirs = libdirs_unicap,
			 libraries = libs_unicap )

iflags_unicapgtk, extra_cflags_unicapgtk = parse_cflags(pkgcflags_unicapgtk)
libdirs_unicapgtk, libs_unicapgtk = parse_libs(pkglibs_unicapgtk)
unicapgtkmodule = Extension( name = 'unicapgtk',
			     sources = ['unicapgtkmodule.c',
					'pyunicapgtk_video_display.c',
					'pyunicapgtk_property_dialog.c',
					'pyunicapgtk_device_property.c',
					'pyunicapgtk_device_selection.c', 
					'utils.c'],
			     include_dirs = iflags_unicapgtk,
			     extra_compile_args = extra_cflags_unicapgtk + ["-O0","-g"],
			     library_dirs = libdirs_unicapgtk,
			     libraries = libs_unicapgtk )


setup (name = 'Unicap',
       version = '1.0',
       description = 'unicap package',
       ext_modules = [unicapmodule])

setup (name='UnicapGtk',
       version = '1.0',
       description = 'unicapgtk package',
       ext_modules = [unicapgtkmodule] )
