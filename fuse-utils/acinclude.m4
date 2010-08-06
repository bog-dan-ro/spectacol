# Configure paths for GLIB
# Owen Taylor     97-11-3

# Taken from Glib 1.2.10 by PAK 2004/07/06

dnl AM_PATH_GLIB([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GLIB, and define GLIB_CFLAGS and GLIB_LIBS, if "gmodule" or 
dnl gthread is specified in MODULES, pass to glib-config
dnl
AC_DEFUN([AM_PATH_GLIB],
[dnl 
dnl Get the cflags and libraries from the glib-config script
dnl
AC_ARG_WITH(glib-prefix,[  --with-glib-prefix=PFX   Prefix where GLIB is installed (optional)],
            glib_config_prefix="$withval", glib_config_prefix="")
AC_ARG_WITH(glib-exec-prefix,[  --with-glib-exec-prefix=PFX Exec prefix where GLIB is installed (optional)],
            glib_config_exec_prefix="$withval", glib_config_exec_prefix="")
AC_ARG_ENABLE(glibtest, [  --disable-glibtest       Do not try to compile and run a test GLIB program],
		    , enable_glibtest=yes)

  if test x$glib_config_exec_prefix != x ; then
     glib_config_args="$glib_config_args --exec-prefix=$glib_config_exec_prefix"
     if test x${GLIB_CONFIG+set} != xset ; then
        GLIB_CONFIG=$glib_config_exec_prefix/bin/glib-config
     fi
  fi
  if test x$glib_config_prefix != x ; then
     glib_config_args="$glib_config_args --prefix=$glib_config_prefix"
     if test x${GLIB_CONFIG+set} != xset ; then
        GLIB_CONFIG=$glib_config_prefix/bin/glib-config
     fi
  fi

  for module in . $4
  do
      case "$module" in
         gmodule) 
             glib_config_args="$glib_config_args gmodule"
         ;;
         gthread) 
             glib_config_args="$glib_config_args gthread"
         ;;
      esac
  done

  AC_PATH_PROG(GLIB_CONFIG, glib-config, no)
  min_glib_version=ifelse([$1], ,0.99.7,$1)
  AC_MSG_CHECKING(for GLIB - version >= $min_glib_version)
  no_glib=""
  if test "$GLIB_CONFIG" = "no" ; then
    no_glib=yes
  else
    GLIB_CFLAGS=`$GLIB_CONFIG $glib_config_args --cflags`
    GLIB_LIBS=`$GLIB_CONFIG $glib_config_args --libs`
    glib_config_major_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    glib_config_minor_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    glib_config_micro_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_glibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GLIB_CFLAGS"
      LIBS="$GLIB_LIBS $LIBS"
dnl
dnl Now check if the installed GLIB is sufficiently new. (Also sanity
dnl checks the results of glib-config to some extent
dnl
      rm -f conf.glibtest
      AC_TRY_RUN([
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.glibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_glib_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_glib_version");
     exit(1);
   }

  if ((glib_major_version != $glib_config_major_version) ||
      (glib_minor_version != $glib_config_minor_version) ||
      (glib_micro_version != $glib_config_micro_version))
    {
      printf("\n*** 'glib-config --version' returned %d.%d.%d, but GLIB (%d.%d.%d)\n", 
             $glib_config_major_version, $glib_config_minor_version, $glib_config_micro_version,
             glib_major_version, glib_minor_version, glib_micro_version);
      printf ("*** was found! If glib-config was correct, then it is best\n");
      printf ("*** to remove the old version of GLIB. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If glib-config was wrong, set the environment variable GLIB_CONFIG\n");
      printf("*** to point to the correct copy of glib-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
  else if ((glib_major_version != GLIB_MAJOR_VERSION) ||
	   (glib_minor_version != GLIB_MINOR_VERSION) ||
           (glib_micro_version != GLIB_MICRO_VERSION))
    {
      printf("*** GLIB header files (version %d.%d.%d) do not match\n",
	     GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     glib_major_version, glib_minor_version, glib_micro_version);
    }
  else
    {
      if ((glib_major_version > major) ||
        ((glib_major_version == major) && (glib_minor_version > minor)) ||
        ((glib_major_version == major) && (glib_minor_version == minor) && (glib_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GLIB (%d.%d.%d) was found.\n",
               glib_major_version, glib_minor_version, glib_micro_version);
        printf("*** You need a version of GLIB newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GLIB is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the glib-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GLIB, but you can also set the GLIB_CONFIG environment to point to the\n");
        printf("*** correct copy of glib-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_glib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_glib" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$GLIB_CONFIG" = "no" ; then
       echo "*** The glib-config script installed by GLIB could not be found"
       echo "*** If GLIB was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GLIB_CONFIG environment variable to the"
       echo "*** full path to glib-config."
     else
       if test -f conf.glibtest ; then
        :
       else
          echo "*** Could not run GLIB test program, checking why..."
          CFLAGS="$CFLAGS $GLIB_CFLAGS"
          LIBS="$LIBS $GLIB_LIBS"
          AC_TRY_LINK([
#include <glib.h>
#include <stdio.h>
],      [ return ((glib_major_version) || (glib_minor_version) || (glib_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GLIB or finding the wrong"
          echo "*** version of GLIB. If it is not finding GLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the GTK package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps gtk gtk-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GLIB was incorrectly installed"
          echo "*** or that you have moved GLIB since it was installed. In the latter case, you"
          echo "*** may want to edit the glib-config script: $GLIB_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GLIB_CFLAGS=""
     GLIB_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GLIB_CFLAGS)
  AC_SUBST(GLIB_LIBS)
  rm -f conf.glibtest
])

# Configure paths for GLIB
# Owen Taylor     1997-2001

# Taken from Glib 2.4.2 by PAK 2004/07/06

dnl AM_PATH_GLIB_2_0([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GLIB, and define GLIB_CFLAGS and GLIB_LIBS, if gmodule, gobject or 
dnl gthread is specified in MODULES, pass to pkg-config
dnl
AC_DEFUN([AM_PATH_GLIB_2_0],
[dnl 
dnl Get the cflags and libraries from pkg-config
dnl
AC_ARG_ENABLE(glibtest, [  --disable-glibtest      do not try to compile and run a test GLIB program],
		    , enable_glibtest=yes)

  pkg_config_args=glib-2.0
  for module in . $4
  do
      case "$module" in
         gmodule) 
             pkg_config_args="$pkg_config_args gmodule-2.0"
         ;;
         gobject) 
             pkg_config_args="$pkg_config_args gobject-2.0"
         ;;
         gthread) 
             pkg_config_args="$pkg_config_args gthread-2.0"
         ;;
      esac
  done

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

  no_glib=""

  if test x$PKG_CONFIG != xno ; then
    if $PKG_CONFIG --atleast-pkgconfig-version 0.7 ; then
      :
    else
      echo *** pkg-config too old; version 0.7 or better required.
      no_glib=yes
      PKG_CONFIG=no
    fi
  else
    no_glib=yes
  fi

  min_glib_version=ifelse([$1], ,2.0.0,$1)
  AC_MSG_CHECKING(for GLIB - version >= $min_glib_version)

  if test x$PKG_CONFIG != xno ; then
    ## don't try to run the test against uninstalled libtool libs
    if $PKG_CONFIG --uninstalled $pkg_config_args; then
	  echo "Will use uninstalled version of GLib found in PKG_CONFIG_PATH"
	  enable_glibtest=no
    fi

    if $PKG_CONFIG --atleast-version $min_glib_version $pkg_config_args; then
	  :
    else
	  no_glib=yes
    fi
  fi

  if test x"$no_glib" = x ; then
    GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
    GOBJECT_QUERY=`$PKG_CONFIG --variable=gobject_query glib-2.0`
    GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`

    GLIB_CFLAGS=`$PKG_CONFIG --cflags $pkg_config_args`
    GLIB_LIBS=`$PKG_CONFIG --libs $pkg_config_args`
    glib_config_major_version=`$PKG_CONFIG --modversion glib-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    glib_config_minor_version=`$PKG_CONFIG --modversion glib-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    glib_config_micro_version=`$PKG_CONFIG --modversion glib-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_glibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GLIB_CFLAGS"
      LIBS="$GLIB_LIBS $LIBS"
dnl
dnl Now check if the installed GLIB is sufficiently new. (Also sanity
dnl checks the results of pkg-config to some extent)
dnl
      rm -f conf.glibtest
      AC_TRY_RUN([
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.glibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_glib_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_glib_version");
     exit(1);
   }

  if ((glib_major_version != $glib_config_major_version) ||
      (glib_minor_version != $glib_config_minor_version) ||
      (glib_micro_version != $glib_config_micro_version))
    {
      printf("\n*** 'pkg-config --modversion glib-2.0' returned %d.%d.%d, but GLIB (%d.%d.%d)\n", 
             $glib_config_major_version, $glib_config_minor_version, $glib_config_micro_version,
             glib_major_version, glib_minor_version, glib_micro_version);
      printf ("*** was found! If pkg-config was correct, then it is best\n");
      printf ("*** to remove the old version of GLib. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If pkg-config was wrong, set the environment variable PKG_CONFIG_PATH\n");
      printf("*** to point to the correct configuration files\n");
    } 
  else if ((glib_major_version != GLIB_MAJOR_VERSION) ||
	   (glib_minor_version != GLIB_MINOR_VERSION) ||
           (glib_micro_version != GLIB_MICRO_VERSION))
    {
      printf("*** GLIB header files (version %d.%d.%d) do not match\n",
	     GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     glib_major_version, glib_minor_version, glib_micro_version);
    }
  else
    {
      if ((glib_major_version > major) ||
        ((glib_major_version == major) && (glib_minor_version > minor)) ||
        ((glib_major_version == major) && (glib_minor_version == minor) && (glib_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GLIB (%d.%d.%d) was found.\n",
               glib_major_version, glib_minor_version, glib_micro_version);
        printf("*** You need a version of GLIB newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GLIB is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the pkg-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GLIB, but you can also set the PKG_CONFIG environment to point to the\n");
        printf("*** correct copy of pkg-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_glib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_glib" = x ; then
     AC_MSG_RESULT(yes (version $glib_config_major_version.$glib_config_minor_version.$glib_config_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$PKG_CONFIG" = "no" ; then
       echo "*** A new enough version of pkg-config was not found."
       echo "*** See http://www.freedesktop.org/software/pkgconfig/"
     else
       if test -f conf.glibtest ; then
        :
       else
          echo "*** Could not run GLIB test program, checking why..."
          ac_save_CFLAGS="$CFLAGS"
          ac_save_LIBS="$LIBS"
          CFLAGS="$CFLAGS $GLIB_CFLAGS"
          LIBS="$LIBS $GLIB_LIBS"
          AC_TRY_LINK([
#include <glib.h>
#include <stdio.h>
],      [ return ((glib_major_version) || (glib_minor_version) || (glib_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GLIB or finding the wrong"
          echo "*** version of GLIB. If it is not finding GLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GLIB is incorrectly installed."])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GLIB_CFLAGS=""
     GLIB_LIBS=""
     GLIB_GENMARSHAL=""
     GOBJECT_QUERY=""
     GLIB_MKENUMS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GLIB_CFLAGS)
  AC_SUBST(GLIB_LIBS)
  AC_SUBST(GLIB_GENMARSHAL)
  AC_SUBST(GOBJECT_QUERY)
  AC_SUBST(GLIB_MKENUMS)
  rm -f conf.glibtest
])

# Configure paths for the Audio File Library
# Bertrand Guiheneuf 98-10-21
# stolen from esd.m4 in esound :
# Manish Singh    98-9-30
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_AUDIOFILE([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for Audio File Library, and define AUDIOFILE_CFLAGS and AUDIOFILE_LIBS.
dnl
AC_DEFUN([AM_PATH_AUDIOFILE],
[dnl 
dnl Get compiler flags and libraries from the audiofile-config script.
dnl
AC_ARG_WITH(audiofile-prefix,[  --with-audiofile-prefix=PFX   Prefix where Audio File Library is installed (optional)],
            audiofile_prefix="$withval", audiofile_prefix="")
AC_ARG_WITH(audiofile-exec-prefix,[  --with-audiofile-exec-prefix=PFX Exec prefix where Audio File Library is installed (optional)],
            audiofile_exec_prefix="$withval", audiofile_exec_prefix="")
AC_ARG_ENABLE(audiofiletest, [  --disable-audiofiletest       Do not try to compile and run a test Audio File Library program], , enable_audiofiletest=yes)

  if test x$audiofile_exec_prefix != x ; then
     audiofile_args="$audiofile_args --exec-prefix=$audiofile_exec_prefix"
     if test x${AUDIOFILE_CONFIG+set} != xset ; then
        AUDIOFILE_CONFIG=$audiofile_exec_prefix/bin/audiofile-config
     fi
  fi
  if test x$audiofile_prefix != x ; then
     audiofile_args="$audiofile_args --prefix=$audiofile_prefix"
     if test x${AUDIOFILE_CONFIG+set} != xset ; then
        AUDIOFILE_CONFIG=$audiofile_prefix/bin/audiofile-config
     fi
  fi

  AC_PATH_PROG(AUDIOFILE_CONFIG, audiofile-config, no)
  min_audiofile_version=ifelse([$1], ,0.2.5,$1)
  AC_MSG_CHECKING(for Audio File Library - version >= $min_audiofile_version)
  no_audiofile=""
  if test "$AUDIOFILE_CONFIG" = "no" ; then
    no_audiofile=yes
  else
    AUDIOFILE_LIBS=`$AUDIOFILE_CONFIG $audiofileconf_args --libs`
    AUDIOFILE_CFLAGS=`$AUDIOFILE_CONFIG $audiofileconf_args --cflags`
    audiofile_major_version=`$AUDIOFILE_CONFIG $audiofile_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    audiofile_minor_version=`$AUDIOFILE_CONFIG $audiofile_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    audiofile_micro_version=`$AUDIOFILE_CONFIG $audiofile_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_audiofiletest" = "xyes" ; then
      AC_LANG_SAVE
      AC_LANG_C
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $AUDIOFILE_CFLAGS"
      LIBS="$LIBS $AUDIOFILE_LIBS"
dnl
dnl Now check if the installed Audio File Library is sufficiently new. 
dnl (Also checks the sanity of the results of audiofile-config to some extent.)
dnl
      rm -f conf.audiofiletest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <audiofile.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.audiofiletest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_audiofile_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_audiofile_version");
     exit(1);
   }

   if (($audiofile_major_version > major) ||
      (($audiofile_major_version == major) && ($audiofile_minor_version > minor)) ||
      (($audiofile_major_version == major) && ($audiofile_minor_version == minor) && ($audiofile_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'audiofile-config --version' returned %d.%d.%d, but the minimum version\n", $audiofile_major_version, $audiofile_minor_version, $audiofile_micro_version);
      printf("*** of the Audio File Library required is %d.%d.%d. If audiofile-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If audiofile-config was wrong, set the environment variable AUDIOFILE_CONFIG\n");
      printf("*** to point to the correct copy of audiofile-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_audiofile=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
       AC_LANG_RESTORE
     fi
  fi
  if test "x$no_audiofile" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$AUDIOFILE_CONFIG" = "no" ; then
       cat <<END
*** The audiofile-config script installed by the Audio File Library could
*** not be found.  If the Audio File Library was installed in PREFIX, make
*** sure PREFIX/bin is in your path, or set the AUDIOFILE_CONFIG
*** environment variable to the full path to audiofile-config.
END
     else
       if test -f conf.audiofiletest ; then
        :
       else
          echo "*** Could not run Audio File Library test program; checking why..."
          AC_LANG_SAVE
          AC_LANG_C
          CFLAGS="$CFLAGS $AUDIOFILE_CFLAGS"
          LIBS="$LIBS $AUDIOFILE_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <audiofile.h>
],      [ return 0; ],
        [ cat <<END
*** The test program compiled, but did not run.  This usually means that
*** the run-time linker is not finding Audio File Library or finding the
*** wrong version of Audio File Library.
***
*** If it is not finding Audio File Library, you'll need to set your
*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point
*** to the installed location.  Also, make sure you have run ldconfig if
*** that is required on your system.
***
*** If you have an old version installed, it is best to remove it, although
*** you may also be able to get things to work by modifying
*** LD_LIBRARY_PATH.
END
        ],
        [ echo "*** The test program failed to compile or link. See the file config.log"
          echo "*** for the exact error that occured. This usually means the Audio File"
          echo "*** Library was incorrectly installed or that you have moved the Audio"
          echo "*** File Library since it was installed. In the latter case, you may want"
          echo "*** to edit the audiofile-config script: $AUDIOFILE_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
          AC_LANG_RESTORE
       fi
     fi
     AUDIOFILE_CFLAGS=""
     AUDIOFILE_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(AUDIOFILE_CFLAGS)
  AC_SUBST(AUDIOFILE_LIBS)
  rm -f conf.audiofiletest
])

# iconv.m4 serial 11 (gettext-0.18.1)
dnl Copyright (C) 2000-2002, 2007-2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

AC_DEFUN([AM_ICONV_LINKFLAGS_BODY],
[
  dnl Prerequisites of AC_LIB_LINKFLAGS_BODY.
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  AC_REQUIRE([AC_LIB_RPATH])

  dnl Search for libiconv and define LIBICONV, LTLIBICONV and INCICONV
  dnl accordingly.
  AC_LIB_LINKFLAGS_BODY([iconv])
])

AC_DEFUN([AM_ICONV_LINK],
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  dnl Search for libiconv and define LIBICONV, LTLIBICONV and INCICONV
  dnl accordingly.
  AC_REQUIRE([AM_ICONV_LINKFLAGS_BODY])

  dnl Add $INCICONV to CPPFLAGS before performing the following checks,
  dnl because if the user has installed libiconv and not disabled its use
  dnl via --without-libiconv-prefix, he wants to use it. The first
  dnl AC_TRY_LINK will then fail, the second AC_TRY_LINK will succeed.
  am_save_CPPFLAGS="$CPPFLAGS"
  AC_LIB_APPENDTOVAR([CPPFLAGS], [$INCICONV])

  AC_CACHE_CHECK([for iconv], [am_cv_func_iconv], [
    am_cv_func_iconv="no, consider installing GNU libiconv"
    am_cv_lib_iconv=no
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      [am_cv_func_iconv=yes])
    if test "$am_cv_func_iconv" != yes; then
      am_save_LIBS="$LIBS"
      LIBS="$LIBS $LIBICONV"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        [am_cv_lib_iconv=yes]
        [am_cv_func_iconv=yes])
      LIBS="$am_save_LIBS"
    fi
  ])
  if test "$am_cv_func_iconv" = yes; then
    AC_CACHE_CHECK([for working iconv], [am_cv_func_iconv_works], [
      dnl This tests against bugs in AIX 5.1, HP-UX 11.11, Solaris 10.
      am_save_LIBS="$LIBS"
      if test $am_cv_lib_iconv = yes; then
        LIBS="$LIBS $LIBICONV"
      fi
      AC_TRY_RUN([
#include <iconv.h>
#include <string.h>
int main ()
{
  /* Test against AIX 5.1 bug: Failures are not distinguishable from successful
     returns.  */
  {
    iconv_t cd_utf8_to_88591 = iconv_open ("ISO8859-1", "UTF-8");
    if (cd_utf8_to_88591 != (iconv_t)(-1))
      {
        static const char input[] = "\342\202\254"; /* EURO SIGN */
        char buf[10];
        const char *inptr = input;
        size_t inbytesleft = strlen (input);
        char *outptr = buf;
        size_t outbytesleft = sizeof (buf);
        size_t res = iconv (cd_utf8_to_88591,
                            (char **) &inptr, &inbytesleft,
                            &outptr, &outbytesleft);
        if (res == 0)
          return 1;
      }
  }
  /* Test against Solaris 10 bug: Failures are not distinguishable from
     successful returns.  */
  {
    iconv_t cd_ascii_to_88591 = iconv_open ("ISO8859-1", "646");
    if (cd_ascii_to_88591 != (iconv_t)(-1))
      {
        static const char input[] = "\263";
        char buf[10];
        const char *inptr = input;
        size_t inbytesleft = strlen (input);
        char *outptr = buf;
        size_t outbytesleft = sizeof (buf);
        size_t res = iconv (cd_ascii_to_88591,
                            (char **) &inptr, &inbytesleft,
                            &outptr, &outbytesleft);
        if (res == 0)
          return 1;
      }
  }
#if 0 /* This bug could be worked around by the caller.  */
  /* Test against HP-UX 11.11 bug: Positive return value instead of 0.  */
  {
    iconv_t cd_88591_to_utf8 = iconv_open ("utf8", "iso88591");
    if (cd_88591_to_utf8 != (iconv_t)(-1))
      {
        static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
        char buf[50];
        const char *inptr = input;
        size_t inbytesleft = strlen (input);
        char *outptr = buf;
        size_t outbytesleft = sizeof (buf);
        size_t res = iconv (cd_88591_to_utf8,
                            (char **) &inptr, &inbytesleft,
                            &outptr, &outbytesleft);
        if ((int)res > 0)
          return 1;
      }
  }
#endif
  /* Test against HP-UX 11.11 bug: No converter from EUC-JP to UTF-8 is
     provided.  */
  if (/* Try standardized names.  */
      iconv_open ("UTF-8", "EUC-JP") == (iconv_t)(-1)
      /* Try IRIX, OSF/1 names.  */
      && iconv_open ("UTF-8", "eucJP") == (iconv_t)(-1)
      /* Try AIX names.  */
      && iconv_open ("UTF-8", "IBM-eucJP") == (iconv_t)(-1)
      /* Try HP-UX names.  */
      && iconv_open ("utf8", "eucJP") == (iconv_t)(-1))
    return 1;
  return 0;
}], [am_cv_func_iconv_works=yes], [am_cv_func_iconv_works=no],
        [case "$host_os" in
           aix* | hpux*) am_cv_func_iconv_works="guessing no" ;;
           *)            am_cv_func_iconv_works="guessing yes" ;;
         esac])
      LIBS="$am_save_LIBS"
    ])
    case "$am_cv_func_iconv_works" in
      *no) am_func_iconv=no am_cv_lib_iconv=no ;;
      *)   am_func_iconv=yes ;;
    esac
  else
    am_func_iconv=no am_cv_lib_iconv=no
  fi
  if test "$am_func_iconv" = yes; then
    AC_DEFINE([HAVE_ICONV], [1],
      [Define if you have the iconv() function and it works.])
  fi
  if test "$am_cv_lib_iconv" = yes; then
    AC_MSG_CHECKING([how to link with libiconv])
    AC_MSG_RESULT([$LIBICONV])
  else
    dnl If $LIBICONV didn't lead to a usable library, we don't need $INCICONV
    dnl either.
    CPPFLAGS="$am_save_CPPFLAGS"
    LIBICONV=
    LTLIBICONV=
  fi
  AC_SUBST([LIBICONV])
  AC_SUBST([LTLIBICONV])
])

dnl Define AM_ICONV using AC_DEFUN_ONCE for Autoconf >= 2.64, in order to
dnl avoid warnings like
dnl "warning: AC_REQUIRE: `AM_ICONV' was expanded before it was required".
dnl This is tricky because of the way 'aclocal' is implemented:
dnl - It requires defining an auxiliary macro whose name ends in AC_DEFUN.
dnl   Otherwise aclocal's initial scan pass would miss the macro definition.
dnl - It requires a line break inside the AC_DEFUN_ONCE and AC_DEFUN expansions.
dnl   Otherwise aclocal would emit many "Use of uninitialized value $1"
dnl   warnings.
m4_define([gl_iconv_AC_DEFUN],
  m4_version_prereq([2.64],
    [[AC_DEFUN_ONCE(
        [$1], [$2])]],
    [[AC_DEFUN(
        [$1], [$2])]]))
gl_iconv_AC_DEFUN([AM_ICONV],
[
  AM_ICONV_LINK
  if test "$am_cv_func_iconv" = yes; then
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL([am_cv_proto_iconv], [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], [am_cv_proto_iconv_arg1=""], [am_cv_proto_iconv_arg1="const"])
      am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"])
    am_cv_proto_iconv=`echo "[$]am_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
    AC_MSG_RESULT([
         $am_cv_proto_iconv])
    AC_DEFINE_UNQUOTED([ICONV_CONST], [$am_cv_proto_iconv_arg1],
      [Define as const if the declaration of iconv() needs const.])
  fi
])
