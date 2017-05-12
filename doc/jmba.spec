Summary: Junk Mail Buffering Agent
Name: jmba
Version: 0.5.5
Release: 1
Copyright: Artistic
Group: Development/Tools
Source: http://www.ivarch.com/programs/sources/jmba-0.5.5.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-root

%description
Junk Mail Buffering Agent (JMBA) is designed to queue potentially unwanted
email until the sender's email address has been verified, at which point the
original email is delivered.

JMBA is designed to be used in conjunction with a spam filter such as QSF
(http://www.ivarch.com/programs/qsf.shtml).  When the spam filter says it
thinks an email is spam, it can be passed to JMBA.  JMBA will queue it and
send an email to the sender containing a key; if the sender replies, the
original email is "unfrozen" from the queue and delivered.

%prep
%setup
mkdir BUILD
cd BUILD
CFLAGS="$RPM_OPT_FLAGS" sh ../configure \
  --prefix=/usr \
  --infodir=/usr/share/info \
  --mandir=/usr/share/man \
  --sysconfdir=/etc
cd ..

%build
make -C BUILD

%install
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf "$RPM_BUILD_ROOT"
[ -e "$RPM_BUILD_ROOT" ] || mkdir -m 755 "$RPM_BUILD_ROOT"
make -C BUILD install DESTDIR="$RPM_BUILD_ROOT"

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf "$RPM_BUILD_ROOT"

%pre

%post

%preun

%postun

%files
%defattr(-, root, root)
/usr/bin/jmba
%docdir /usr/share/man/man1
/usr/share/man/man1/*
/usr/share/locale/*/LC_MESSAGES/*
%doc README doc/NEWS doc/TODO doc/COPYING

%changelog
* Mon Feb  6 2006 Andrew Wood <andrew.wood@ivarch.com>
- Ignore files starting with "." in the queue directory.
- Include slightly more of the original message in replies.
- Added new {SENDER} tag in template.
- Added new {RECIPIENT} tag in template (sponsored by SpamDefy).

* Fri Jul  8 2005 Andrew Wood <andrew.wood@ivarch.com>
- Fixed a bug that prevented flood protection from working.

* Sat Jun 11 2005 Andrew Wood <andrew.wood@ivarch.com>
- Flood protection ("-f") was added and enabled by default, and the "-t"
- option is now enabled by default. Logging has also been improved and some
- minor subject line decoding bugs fixed.

* Fri Sep 24 2004 Andrew Wood <andrew.wood@ivarch.com>
- A new option "-x" has been added, which runs a command on each message as
- it is discarded from the queue.

* Thu Sep 23 2004 Andrew Wood <andrew.wood@ivarch.com>
- A new option "-S" has been added, which outputs the decoded subject line
- of an email.

* Wed Sep  8 2004 Andrew Wood <andrew.wood@ivarch.com>
- Accept message codes in the subject line, in case of blank replies.

* Tue Jul 15 2004 Andrew Wood <andrew.wood@ivarch.com>
- Corrected a bug in the default procmail recipe when used with "-t".

* Mon Jul 14 2004 Andrew Wood <andrew.wood@ivarch.com>
- An extra option "-t" has been added to ignore email that has the same
- value for both its "from" and "to" addresses.

* Wed May  5 2004 Andrew Wood <andrew.wood@ivarch.com>
- Additional template tags have been added to make replies more informative
- and flexible. Logging and the option of keeping discarded email have also
- been added.

* Sat Feb 14 2004 Andrew Wood <andrew.wood@ivarch.com>
- Support for alternate character sets, and minor portability fixes.

* Mon Feb  2 2004 Andrew Wood <andrew.wood@ivarch.com>
- Czech translation added courtesy of Ondrej Suchy

* Tue Dec 30 2003 Andrew Wood <andrew.wood@ivarch.com>
- Compatibilty with postfix was improved by updating the example recipe.

* Thu Nov 20 2003 Andrew Wood <andrew.wood@ivarch.com>
- first draft of spec file created
