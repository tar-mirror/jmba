#
# Rules for all phony targets.
#

.PHONY: all dep depend depclean make check test \
  clean distclean cvsclean \
  index maneg manhtml indent indentclean \
  doc dist release \
  install uninstall \
  help \
  rpm srpm deb \
  nls-update nls-clean nls-install nls-uninstall

all: $(alltarg) $(CATALOGS)

make:
	echo > $(srcdir)/autoconf/make/filelist.mk
	echo > $(srcdir)/autoconf/make/modules.mk
	cd $(srcdir); \
	bash autoconf/scripts/makemake.sh \
	     autoconf/make/filelist.mk \
	     autoconf/make/modules.mk
	sh ./config.status
	
dep depend: $(alldep)
	echo '#' > $(srcdir)/autoconf/make/depend.mk
	echo '# Dependencies.' >> $(srcdir)/autoconf/make/depend.mk
	echo '#' >> $(srcdir)/autoconf/make/depend.mk
	echo >> $(srcdir)/autoconf/make/depend.mk
	cat $(alldep) >> $(srcdir)/autoconf/make/depend.mk
	sh ./config.status

clean:
	rm -f $(allobj)
	find . -type f -name "*.c~" -exec rm -f '{}' ';'

depclean:
	rm -f $(alldep)

distclean: clean depclean
	rm -f $(alltarg) src/include/config.h
	rm -rf $(package)-$(version).tar* $(package)-$(version) BUILD-DEB
	rm -f *.rpm *.deb
	rm -f *.html config.*
	rm -f gmon.out
	rm Makefile

cvsclean: distclean nls-clean
	rm -f doc/lsm
	rm -f doc/$(package).spec
	rm -f doc/quickref.1
	rm -f doc/quickref.txt
	rm -f configure
	echo > $(srcdir)/autoconf/make/depend.mk
	echo > $(srcdir)/autoconf/make/filelist.mk
	echo > $(srcdir)/autoconf/make/modules.mk

indentclean:
	cd $(srcdir) && for FILE in $(allsrc); do rm -fv ./$${FILE}~; done

doc: doc/quickref.txt

index:
	(cd $(srcdir); sh autoconf/scripts/index.sh $(srcdir)) > index.html

manhtml:
	@man2html ./doc/quickref.1 \
	| sed -e '1,/<BODY/d' -e '/<\/BODY/,$$d' \
	      -e 's|<A [^>]*>&nbsp;</A>||ig' \
	      -e 's|<A [^>]*>\([^<]*\)</A>|\1|ig' \
	      -e '/<H1/d' -e 's|\(</H[0-9]>\)|\1<P>|ig' \
	      -e 's/<DL COMPACT>/<DL>/ig' \
	      -e 's/&lt;[0-9A-Za-z_.-]\+@[0-9A-Za-z_.-]\+&gt;//g' \
	      -e 's|<I>\(http://.*\)</I>|<A HREF="\1">\1</A>|ig' \
	| sed -e '1,/<HR/d' -e '/<H2>Index/,/<HR/d' \

maneg: $(package)
	@./$(package) -p \
	  | sed -e 's/^/ /' -e 's/\\/\\\\/g' \
	        -e 's/@PACKAGE@/@'"PACKAGE"'@/g' \
	        -e 's/@UCPACKAGE@/@'"UCPACKAGE"'@/g'

indent:
	cd $(srcdir) && indent -npro -kr -i8 -cd42 -c45 $(allsrc)

dist: doc nls-update
	rm -rf $(package)-$(version)
	mkdir $(package)-$(version)
	cp -dprf Makefile $(distfiles) $(package)-$(version)
	cd $(package)-$(version); $(MAKE) distclean
	cp -dpf doc/lsm             $(package)-$(version)/doc/
	cp -dpf doc/$(package).spec $(package)-$(version)/doc/
	cp -dpf doc/quickref.txt    $(package)-$(version)/doc/
	chmod 644 `find $(package)-$(version) -type f -print`
	chmod 755 `find $(package)-$(version) -type d -print`
	chmod 755 `find $(package)-$(version)/autoconf/scripts`
	chmod 755 $(package)-$(version)/configure
	chmod 755 $(package)-$(version)/debian/rules
	rm -rf DUMMY `find $(package)-$(version) -type d -name CVS`
	tar cf $(package)-$(version).tar $(package)-$(version)
	rm -rf $(package)-$(version)
	-cat $(package)-$(version).tar \
	 | bzip2 > $(package)-$(version).tar.bz2 \
	 || rm -f $(package)-$(version).tar.bz2
	$(DO_GZIP) $(package)-$(version).tar

check test: $(package)
	@echo '*** No tests written yet ***'
	@exit 1

install: all doc nls-install
	$(srcdir)/autoconf/scripts/mkinstalldirs \
	  "$(DESTDIR)/$(bindir)"
	$(srcdir)/autoconf/scripts/mkinstalldirs \
	  "$(DESTDIR)/$(mandir)/man1"
	$(INSTALL) -m 755 $(package) \
	  "$(DESTDIR)/$(bindir)/$(package)"
	$(INSTALL) -m 644 doc/quickref.1 \
	  "$(DESTDIR)/$(mandir)/man1/$(package).1"
	-$(DO_GZIP) "$(DESTDIR)/$(mandir)/man1/$(package).1"

uninstall: nls-uninstall
	$(UNINSTALL) "$(DESTDIR)/$(bindir)/$(package)"
	$(UNINSTALL) "$(DESTDIR)/$(mandir)/man1/$(package).1"
	$(UNINSTALL) "$(DESTDIR)/$(mandir)/man1/$(package).1.gz"

rpmbuild:
	echo macrofiles: `rpm --showrc \
	  | grep ^macrofiles \
	  | cut -d : -f 2- \
	  | sed 's,^[^/]*/,/,'`:`pwd`/rpmmacros > rpmrc
	echo %_topdir `pwd`/rpm > rpmmacros
	rm -rf rpm
	mkdir rpm
	mkdir rpm/SPECS rpm/BUILD rpm/SOURCES rpm/RPMS rpm/SRPMS
	-cat /usr/lib/rpm/rpmrc /etc/rpmrc $$HOME/.rpmrc \
	 | grep -hsv ^macrofiles \
	 >> rpmrc

srpm:
	-test -e $(package)-$(version).tar.gz || $(MAKE) dist
	-test -e rpmrc || $(MAKE) rpmbuild
	rpmbuild $(RPMFLAGS) --rcfile=rpmrc -ts $(package)-$(version).tar.bz2
	mv rpm/SRPMS/*$(package)-*.rpm .
	rm -rf rpm rpmmacros rpmrc

rpm:
	-test -e $(package)-$(version).tar.gz || $(MAKE) dist
	-test -e rpmrc || $(MAKE) rpmbuild
	rpmbuild $(RPMFLAGS) --rcfile=rpmrc -tb $(package)-$(version).tar.bz2
	mv rpm/RPMS/*/$(package)-*.rpm .
	rm -rf rpm rpmmacros rpmrc

deb: dist
	rm -rf BUILD-DEB
	mkdir BUILD-DEB
	cd BUILD-DEB && tar xzf ../$(package)-$(version).tar.gz
	cd BUILD-DEB && cd $(package)-$(version) && dpkg-buildpackage -rfakeroot
	mv BUILD-DEB/*.deb .
	rm -rf BUILD-DEB

release: dist rpm srpm

help:
	@echo 'Makefile targets:'
	@echo ''
	@echo '  all           build all targets'
	@echo '  doc           build documentation'
	@echo '  install       install whole package'
	@echo '  uninstall     uninstall whole package'
	@echo '  check / test  run tests'
	@echo ''
	@echo '  rpm           build an RPM with rpm -bb $RPMFLAGS'
	@echo '  srpm          build an SRPM with rpm -bs $RPMFLAGS'
	@echo '  deb           build a Debian package'
	@echo ''
	@echo '  clean         delete .o and .c~ files'
	@echo '  depclean      delete .d (dependency) files'
	@echo '  distclean     remove all files not in source tarball'
	@echo '  cvsclean      remove all files not in CVS repository'
	@echo ''
	@echo '  make          regenerate compilation/linking rules'
	@echo '  dep / depend  rebuild dependencies'
	@echo '  index         generate HTML index of source code'
	@echo '  nls-update    update .po files'
	@echo '  indent        run indent(1) to enforce coding style'
	@echo ''
	@echo '  maneg         regenerate example for man page .in'
	@echo '  manhtml       convert man page to HTML'
	@echo ''
