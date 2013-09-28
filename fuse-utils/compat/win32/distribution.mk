## This file does not need automake. Include in the final Makefile.
## Copyright (c) 2013 Sergio Baldoví

## $Id$

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: serbalgi@gmail.com

package_win32=$(PACKAGE)-$(PACKAGE_VERSION)-win32
top_win32dir=$(top_builddir)/$(package_win32)

install-win32: all
	test -n "$(DESTDIR)" || { echo "ERROR: set DESTDIR path"; exit 1; }
	$(MKDIR_P) $(DESTDIR)/ || exit 1
#	Copy executables (we should manually copy the required libraries)
	list='$(bin_PROGRAMS)'; \
	for file in $$list; do \
	  if test -f "$(top_builddir)/.libs/$$file"; then \
	    cp "$(top_builddir)/.libs/$$file" $(DESTDIR); \
	  else \
	    cp "$(top_builddir)/$$file" $(DESTDIR); \
	  fi; \
	done
#	Get text files
	for file in AUTHORS ChangeLog COPYING README; \
	  do cp "$(top_srcdir)/$$file" "$(DESTDIR)/$$file.txt"; \
	done
#	Get manuals
	topsrcdirstrip=`echo "$(top_srcdir)/man" | sed 's/[].[^$$\\*]/\\\\&/g'`; \
	man_files=`for file in $(top_srcdir)/man/*.1; do echo "$$file"; done | \
	  sed -e "s|^$$topsrcdirstrip/||" -e "s|.1$$||"`; \
	for file in $$man_files; do \
	  if test -n "$(MAN2HTML)"; then \
	    $(MAN2HTML) -r "$(top_srcdir)/man/$$file.1" | sed '1d' > "$(DESTDIR)/$$file.html"; \
	  else \
	    test -z "$(GROFF)" || $(GROFF) -Thtml -man "$(top_srcdir)/man/$$file.1" > "$(DESTDIR)/$$file.html"; \
	  fi; \
	done
#	Convert to DOS line endings
	test -z "$(UNIX2DOS)" || find $(DESTDIR) -type f \( -name "*txt" -or -name "*html" \) -exec $(UNIX2DOS) -q {} \;

install-win32-strip: install-win32
	test -z "$(STRIP)" || { list='$(bin_PROGRAMS)'; \
	for file in $$list; do \
	  test -z "$(STRIP)" || $(STRIP) "$(DESTDIR)/$$file"; \
	done }

dist-win32-dir:
	$(MAKE) DESTDIR="$(top_win32dir)" install-win32-strip

dist-win32-zip: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32).zip
	rm -f -- $(top_builddir)/$(package_win32).zip.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
	cd $(top_win32dir) && \
	zip -q -9 -r $(abs_top_builddir)/$(package_win32).zip .
	-sha1sum $(top_builddir)/$(package_win32).zip > $(top_builddir)/$(package_win32).zip.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32).zip.sha1; }

dist-win32-7z: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32).7z
	rm -f -- $(top_builddir)/$(package_win32).7z.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
	cd $(top_win32dir) && \
	7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -bd $(abs_top_builddir)/$(package_win32).7z .
	-sha1sum $(top_builddir)/$(package_win32).7z > $(top_builddir)/$(package_win32).7z.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32).7z.sha1; }

dist-win32: dist-win32-zip dist-win32-7z

distclean-win32:
	if test -d "$(top_builddir)/$(package_win32)"; then \
	  rm -rf -- "$(top_builddir)/$(package_win32)"; \
	fi
	rm -f -- $(top_builddir)/$(package_win32).zip
	rm -f -- $(top_builddir)/$(package_win32).zip.sha1
	rm -f -- $(top_builddir)/$(package_win32).7z
	rm -f -- $(top_builddir)/$(package_win32).7z.sha1

.PHONY: install-win32 install-win32-strip \
	dist-win32 dist-win32-dir dist-win32-zip dist-win32-7z distclean-win32
