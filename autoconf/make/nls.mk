#
# NLS variables.
#

localedir = $(datadir)/locale
gnulocaledir = $(prefix)/share/locale

CATALOGS = @CATALOGS@
POFILES = @POFILES@
GMSGFMT = @GMSGFMT@
MSGFMT = @MSGFMT@
XGETTEXT = @XGETTEXT@
MSGMERGE = msgmerge
CATOBJEXT = @CATOBJEXT@
INSTOBJEXT = @INSTOBJEXT@

NLSDEFS = -DLOCALEDIR=\"$(localedir)\"
