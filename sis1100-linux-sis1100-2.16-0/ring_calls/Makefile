CC           := gcc
WFLAGS       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat
#                -Wuninitialized -Werror
CPPFLAGS     := -I..
CFLAGS       := -g -ansi $(WFLAGS)

srcdir       := .

lib_ring_calls.a:  ring_calls.o 
	ar cr $@ $^

ring_calls.o: ring_calls.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^

clean:
	rm -f *.o
	rm -f *.a

depend:
	cp Makefile Makefile.bak
	sed -e '/^# DO NOT DELETE THIS LINE/,$$d' < Makefile.bak > Makefile
	echo '# DO NOT DELETE THIS LINE' >> Makefile
	echo ' ' >> Makefile
	for i in $(cfiles) ; do \
	  echo checking $$i ; \
	  $(CC) -M $(CPPFLAGS) $(srcdir)/$$i >> Makefile ; \
	done
# DO NOT DELETE THIS LINE
