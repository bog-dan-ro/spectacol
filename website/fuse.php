<?php

/* fuse.php: the main Fuse homepage
   Copyright (c) 1999-2010 Darren Salt, Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

include ".inc/fuse.inc";

fuse_title ("Fuse - the Free Unix Spectrum Emulator", true);
fuse_menu_heading ("Fuse");

#<!-- ======================================================= -->

$img = mk_image('screens/f-awm.png', 330, 292);
fuse_section (NULL, "What is it?", <<<END_SECTION
  $img
  <p>Fuse (the Free Unix Spectrum Emulator) was originally, and somewhat unsurprisingly, a ZX Spectrum emulator for Unix. However, it has now also been ported to Mac OS X, which may or may not count as a Unix variant depending on your advocacy position. It has also been ported to Windows, the Wii, AmigaOS and MorphOS, which are definitely not Unix variants.</p>

END_SECTION
);

#<!-- ======================================================= -->

$img = mk_image('screens/f-large.png', 650, 532, 4);
fuse_section ("Features", "What features does it have?", <<<END_SECTION
  $img
  <ul>
   <li>Accurate 16K, 48K (including the NTSC variant), 128K, +2, +2A and +3 emulation.</li>
   <li>Working +3e, SE, TC2048, TC2068, TS2068, Pentagon 128, Pentagon "512" (Pentagon 128 modified for extra memory), Pentagon 1024 and Scorpion ZS 256 emulation.</li>
   <li>Runs at true Speccy speed on any computer you're likely to try it on.</li>
   <li>Support for loading from .tzx files, including accelerated loading.</li>
   <li>Sound (on Windows and Mac OS X, and on systems supporting ALSA, the Open Sound System, SDL or OpenBSD/Solaris's <tt>/dev/audio</tt>).</li>
   <li>Kempston joystick emulation.</li>
   <li>Emulation of the various printers you could attach to the Spectrum.</li>
   <li>Support for the RZX input recording file format, including 'competition mode'.</li>
   <li>Emulation of the DivIDE, Interface I, +D, Beta 128, Opus Discovery, Kempston mouse, Fuller audio box, Spectrum +3e, ZXATASP and ZXCF interfaces.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section ("What's lacking?", "What is it lacking?", <<<END_SECTION
  <ul>
   <li>Quite a lot! However, it's a lot better than it used to be...</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

$img = mk_image('screens/f-rzx.png', 330, 266);
fuse_section ("Download", "Downloads", <<<END_SECTION
  <p>Fuse is licensed under the <a href="COPYING">GNU General Public License</a>, version 2 or later. Please read this before downloading Fuse if you're not already familiar with it.</p>

  <h3>Unix</h3>
  <p>Packages are available for some Unix distributions; in general, any problems which are specific to the packages should be sent to the package maintainer.</p>
  <ul>
   <li>Unofficial packages of 1.0.0 for <a href="http://www.debian.org/">Debian</a> (or <a href="http://www.ubuntu.com/">Ubuntu</a>) by Alberto Garcia are available from <a href="http://people.igalia.com/berto/">his personal webpage</a>.</li>
   <li><a href="http://fedoraproject.org/index.html">Fedora</a> has <a href="https://admin.fedoraproject.org/pkgdb/acls/name/fuse-emulator">packages</a> of 1.0.0 available.</li>
   <li>Nokia's <a href="http://maemo.org/">Maemo</a> platform has a port of <a href="http://maemo.org/downloads/product/OS2008/fuse-emulator/">1.0.0</a> available by Alberto Garcia.</li>
   <li>The <a href="http://en.docs.pld-linux.org/">Polish Linux Distribution</a> has packages of Fuse 1.0.0 and the utilities.</li>
  </ul>
  <p>Packages of older versions of Fuse are also available for some other distributions:</p>
  <ul>
   <li><a href="http://www.freebsd.org/">FreeBSD</a> has a port of 0.10.0.2 available as <a href="http://www.FreeBSD.org/cgi/cvsweb.cgi/ports/emulators/fuse/">emulators/fuse</a>.</li>
   <li><a href="http://www.gentoo.org/">Gentoo</a> users have an <a href="http://packages.gentoo.org/package/app-emulation/fuse?full_cat">ebuild</a> of 0.10.0.2 available.</li>
   <li><a href="http://www.mandriva.com/">Mandriva</a> packages of 0.10.0.2 are available from the <a href="http://plf.zarb.org/">Penguin Liberation Front</a>.</li>
   <li><a href="http://www.netbsd.org/">NetBSD</a> users can get version 0.10.0.2 from <a href="http://www.pkgsrc.org/">pkgsrc</a> (the NetBSD Packages Collection) as <a href="http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/emulators/fuse/">emulators/fuse</a> and <a href="http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/emulators/fuse-utils/">emulators/fuse-utils</a>.</li>
   <li><a href="http://www.openbsd.org/">OpenBSD</a> users have version 0.10.0.2 available from ports as <a href="http://www.openbsd.org/cgi-bin/cvsweb/ports/emulators/fuse/">ports/emulators/fuse</a>, and <a href="http://www.openbsd.org/cgi-bin/cvsweb/ports/emulators/fuse-utils/">ports/emulators/fuse-utils/</a>, with thanks to Alexander Yurchenko.</li>
   <li><a href="http://www.suse.com/">SuSE</a> users have version 0.10.0.2 as the <a href="https://build.opensuse.org/package/show?package=Fuse&amp;project=Emulators">Fuse</a> package.</li>
  </ul>

  <h3>Mac OS X</h3>
  <p>A native port to OS X by <a href="mailto:fredm@spamcop.net">Fredrick Meunier</a> is available on its own SourceForge project <a href="http://fuse-for-macosx.sourceforge.net//">here</a>, as well as a Spotlight importer for Mac OS X 10.4 Tiger users. Alternatively, the original version of Fuse will compile on OS X 10.3 (Panther) or later.</p>

  <h3>Windows</h3>
  <p>A port to Windows of 0.10.0.2 by Marek Januszewski and Stuart Brady is available <a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-0.10.0.2-setup.exe?download">here</a> (<a href="http://prdownloads.sourceforge.net/fuse-emulator/fuse-0.10.0.2-setup.exe.sha1?download">SHA1 hash</a>). This is also mirrored at <a href="http://www.worldofspectrum.org/">World of Spectrum</a>: <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/pc/windows/fuse-0.10.0.2-setup.exe">Fuse</a> (<a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/pc/windows/fuse-0.10.0.2-setup.exe.sha1">SHA1 hash</a>).</p>

  <h3>AmigaOS 4</h3>
  <p>Chris Young has ported Fuse 0.10.0 to AmigaOS 4, with binaries available from Aminet as <a href="http://aminet.net/package/misc/emu/fuse.lha">misc/emu/fuse.lha</a>.</p>

  <h3>MorphOS</h3>
  <p>Q-Master has ported Fuse 0.10.0.1 to MorphOS, with <a href="http://www.amirus.org.ru/files/fuse_0.10.0.1_morphos.lha">binaries available from AmiRUS</a>.</p>

  <h3>PSP</h3>
  <p>Akop Karapetyan has ported Fuse to the PSP. Binaries and source, based on Fuse 0.10.0.1, are available from the <a href="http://psp.akop.org/fuse">Fuse PSP</a> page.</p>

  <h3>Wii</h3>
  <p>A Wii port, based on work by Bj&ouml;rn Giesler, is available from <a href="http://wiibrew.org/wiki/Fuse">WiiBrew</a>. This is based on what is essentially 0.10.0.2.</p>

  <h3>Gizmondo</h3>
  <p>A port of 0.9.0 to the Gizmondo tablet is available. The source is available via <a href="http://opensvn.csie.org/GizPorts/trunk/sdlport/fuzegiz/">csie.org</a>.</p>

  <h3>GP2X</h3>
  <p>Ben O'Steen has made a GP2X port, based on Fuse 0.6. Binaries and source are available from <a href="http://www.zen71790.zen.co.uk/#fuse">his homepage</a>.</p>

  <h3>XBox</h3>
  <p>Crabfists's has made an Xbox port, based on Fuse 0.6. Binaries and source are available from the <a href="http://sourceforge.net/projects/fusex/">FuseX project</a> at SourceForge.</p>

  <h3>PocketPC</h3>
  <p>Anders Holmberg's ported Fuse 0.4 to the PocketPC as <a href="http://pocketclive.emuunlim.com/">PocketClive</a>.</p>

  <h3>Smartphone</h3>
  <p>Keith Orbell's then ported PocketClive to the Smartphone as <a href="http://www.aooa27.dsl.pipex.com/FuseSP.htm">FuseSP</a>.</p>

END_SECTION
);

#<!-- ======================================================= -->

$img = mk_image('screens/f-nongtk.png', 330, 266);
fuse_section ("Source", "Source", <<<END_SECTION
  <h3>Installing Fuse</h3>
  $img
  <ul>
   <li>First, check the requirements below and ensure all the libraries you want/need are installed.</li>
   <li>Secondly, install <a href="libspectrum.php">libspectrum</a>.</li>
   <li>Get the <a href="http://sourceforge.net/projects/fuse-emulator/files/fuse/1.0.0.1/fuse-1.0.0.1.tar.gz/download">source code</a> (<a href="http://sourceforge.net/projects/fuse-emulator/files/fuse/1.0.0.1/fuse-1.0.0.1.tar.gz.sig/download">PGP signature</a>).</li>
   <li>The utilities which were previously packaged with Fuse are now available in their <a href="http://sourceforge.net/projects/fuse-emulator/files/fuse-utils/1.0.0/fuse-utils-1.0.0.tar.gz/download">own package</a> (<a href="http://sourceforge.net/projects/fuse-emulator/files/fuse-utils/1.0.0/fuse-utils-1.0.0.tar.gz.sig/download">PGP signature</a>). Note that
you'll still need <a href="libspectrum.php">libspectrum</a> installed to run these.</li>
   <li>The above are also mirrored at <a href="http://www.worldofspectrum.org/">World of Spectrum</a>: <a
href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-1.0.0.1.tar.gz">Fuse source</a> (<a
href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-1.0.0.1.tar.gz.sig">signature</a>), <a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-utils-1.0.0.tar.gz">utils source</a> (<a href="ftp://ftp.worldofspectrum.org/pub/sinclair/emulators/unix/fuse-utils-1.0.0.tar.gz.sig">signature</a>).</li>
   <li>The source code releases above are signed with the <a href="http://wwwkeys.pgp.net:11371/pks/lookup?op=get&amp;search=0xD0767AB9">Fuse Release Key, ID <tt>D0767AB9</tt></a>, which has fingerprint <tt>064E 0BA9 688F 2699 3267 B1E5 1043 EEEB D076 7AB9</tt>. This is different from the key used to sign the 0.6.0(.1) releases as I forgot the passphrase for that key <tt>:-(</tt>.</li>
  </ul>

  <h3>Requirements</h3>
  $img
  <dl>
   <dt>Required:</dt>
   <dd>
    <ul>
     <li>X, <a href="http://www.libsdl.org/">SDL</a>, <a href="http://www.svgalib.org/">svgalib</a> or framebuffer support. If you have <a href="http://www.gtk.org/">GTK+</a> installed, you'll get a (much) nicer user interface under X.</li>
     <li><a href="libspectrum.php">libspectrum</a>: the Spectrum emulator file format and information library.</li>
    </ul>
   </dd>
   <dt>Optional:</dt>
   <dd>
    <ul>
     <li><a href="http://www.gnu.org/directory/security/libgcrypt.html">libgcrypt</a>: the ability to digitally sign RZX files (note that Fuse requires version 1.1.42 or later).</li>
     <li><a href="http://www.libpng.org/pub/png/libpng.html">libpng</a>: the ability to save screenshots.</li>
     <li><a href="http://xmlsoft.org/">libxml2</a>: the ability to load and save Fuse's current configuration.</li>
     <li><a href="http://www.battlefieldlinux.com/wolfpack/libjsw/">libjsw</a>: allow joystick input to be used (not required for joystick emulation).</li>
     <li><a href="http://www.gzip.org/zlib/">zlib</a>: support for compressed RZX files.</li>
     <li><a href="http://www.bzip.org/">libbzip2</a>: support for certain compressed files.</li>
     <li><a href="http://www.68k.org/~michael/audiofile/">libaudiofile</a>: support for loading from .wav files.</li>
     <li>Versions previous to 0.10.0 used John Elliott's <a href="http://www.seasip.demon.co.uk/Unix/LibDsk/">lib765 and libdsk</a> for the +3 support. 0.10.0 and newer include this support natively, so these libraries are no longer necessary (or used).</li>
    </ul>
   </dd>
  </dl>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section (NULL, "What's new?", <<<END_SECTION
  <h3>1.0.0.1</h3>
  <ul>
    <li>Fix temporary breakpoints when using replacement Glib routines</li>
    <li>Ensure Amiga port can load files</li>
    <li>Fix SVGAlib compilation</li>
  </ul>

  <h3>1.0.0</h3>
  <ul>
    <li>Opus Discovery interface support</li>
    <li>Wii support</li>
    <li>Many improvements to Win32 UI</li>
    <li>NTSC Spectrum support</li>
    <li>Improved sound output, including when not running at 100% speed</li>
    <li>Fuller Audio Box support</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section (NULL, "Development", <<<END_SECTION
  <p>If you're just want news of new versions and the like, the (low volume) <a href="http://lists.sourceforge.net/lists/listinfo/fuse-emulator-announce">fuse-emulator-announce</a> list is available. If you're interested in the development of Fuse, this is coordinated via the <a href="http://lists.sourceforge.net/lists/listinfo/fuse-emulator-devel">fuse-emulator-devel</a> list and the <a href="http://sourceforge.net/projects/fuse-emulator/">project page</a> on SourceForge.</p>

  <p>The latest version of Fuse is always available by checking out the 'trunk/fuse' directory from the <a href="http://sourceforge.net/svn/?group_id=91293">Subversion repository</a> on SourceForge. Note that this isn't guaranteed to compile, let alone work properly. Also, don't expect any support for this version! (You'll also need libspectrum from Subversion; this is in the 'trunk/libspectrum' directory). Similarly, the utilities are available in the 'trunk/fuse-utils' directory.</p>

  <p>One thing which isn't in the SourceForge tracking system (and is now very outdated):</p>

  <ul>
   <li>David Gardner has produced a patch to give <a href="fuse-xvideo.patch.bz2">XVideo support</a> for the Xlib UI, allowing arbitrary sized windows.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_section ("Related projects", "Are there any related projects?", <<<END_SECTION
  <ul>
   <li><a href="libspectrum.php">libspectrum</a> is the library used by Fuse to handle various file formats.</li>
   <li>Mike Wynne's ZX81 emulator, <a href="http://www.chuntey.com/eightyone/">EightyOne</a> is also using Fuse's Z80 core.</li>
   <li>Matthew Westcott's <a href="http://matt.west.co.tt/spectrum/jsspeccy/">JSSpeccy</a> uses a Z80 core based on translating Fuse's core to Javascript.</li>
   <li>Alexander Shabarshin's <a href="http://robots.ural.net/nedopc/sprinter/">SPRINT</a>, an emulator of the <a href="http://www.interface1.net/zx/clones/peters.html">Peters Plus</a> super-Speccy, the <a href="http://www.petersplus.com/sprinter/">Sprinter</a>. SPRINT is using Fuse's Z80 core for its CPU emulation.</li>
   <li><a href="http://sourceforge.net/projects/z80ex/">z80ex</a>, a Z80 emulation library based on Fuse's Z80 core, used by zemu and <a href="http://pocketspeccy.narod.ru/">PocketSpeccy</a>.</li>
  </ul>

END_SECTION
);

#<!-- ======================================================= -->

fuse_related ("libspectrum", "libspectrum.php");

fuse_footer ();
?>
