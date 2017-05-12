# Automatically generated module linking rules
#
# Creation time: Mon Feb  6 10:16:41 GMT 2006

src/library.o:  src/library/getopt.o src/library/gettext.o src/library/memmem.o
	$(LD) $(LDFLAGS) -o $@  src/library/getopt.o src/library/gettext.o src/library/memmem.o

src/main.o:  src/main/bounce.o src/main/decode.o src/main/help.o src/main/init.o src/main/license.o src/main/log.o src/main/main.o src/main/options.o src/main/recipe.o src/main/runqueue.o src/main/store.o src/main/version.o
	$(LD) $(LDFLAGS) -o $@  src/main/bounce.o src/main/decode.o src/main/help.o src/main/init.o src/main/license.o src/main/log.o src/main/main.o src/main/options.o src/main/recipe.o src/main/runqueue.o src/main/store.o src/main/version.o


