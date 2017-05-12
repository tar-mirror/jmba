#
# Targets.
#
#

mainobjs := src/main.o src/md5.o src/library.o @NLSOBJ@

$(package): $(mainobjs)
	$(CC) $(LINKFLAGS) $(CFLAGS) -o $@ $(mainobjs) $(LIBS)

$(package)-static: $(mainobjs)
	$(CC) $(LINKFLAGS) $(CFLAGS) -static -o $@ $(mainobjs) $(LIBS)

# EOF
