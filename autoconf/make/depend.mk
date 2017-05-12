#
# Dependencies.
#

src/library/gettext.d src/library/gettext.o: src/library/gettext.c src/include/config.h src/include/library/gettext.h 
src/library/getopt.d src/library/getopt.o: src/library/getopt.c src/include/config.h src/include/library/gettext.h 
src/library/memmem.d src/library/memmem.o: src/library/memmem.c src/include/config.h src/include/library/gettext.h 
src/main/license.d src/main/license.o: src/main/license.c src/include/config.h src/include/library/gettext.h 
src/main/bounce.d src/main/bounce.o: src/main/bounce.c src/include/config.h src/include/library/gettext.h src/include/options.h src/include/md5.h src/include/library/memmem.h 
src/main/decode.d src/main/decode.o: src/main/decode.c src/include/config.h src/include/library/gettext.h src/include/options.h 
src/main/help.d src/main/help.o: src/main/help.c src/include/config.h src/include/library/gettext.h 
src/main/init.d src/main/init.o: src/main/init.c src/include/config.h src/include/library/gettext.h src/include/options.h 
src/main/options.d src/main/options.o: src/main/options.c src/include/config.h src/include/library/gettext.h src/include/options.h src/include/library/getopt.h 
src/main/log.d src/main/log.o: src/main/log.c src/include/config.h src/include/library/gettext.h src/include/options.h 
src/main/main.d src/main/main.o: src/main/main.c src/include/config.h src/include/library/gettext.h src/include/options.h 
src/main/runqueue.d src/main/runqueue.o: src/main/runqueue.c src/include/config.h src/include/library/gettext.h src/include/options.h 
src/main/recipe.d src/main/recipe.o: src/main/recipe.c src/include/config.h src/include/library/gettext.h src/include/options.h 
src/main/version.d src/main/version.o: src/main/version.c src/include/config.h src/include/library/gettext.h 
src/main/store.d src/main/store.o: src/main/store.c src/include/config.h src/include/library/gettext.h src/include/options.h src/include/md5.h src/include/library/memmem.h 
src/md5.d src/md5.o: src/md5.c src/include/md5.h 
