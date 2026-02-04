
AOBJ = acquire.o dsplink.o dspload.o read_dsp_coff.c read_dsp_ldr.c read_dsp_raw.c
HOBJ = sharehist.o
MOBJ = mtxutil.o
#LIBS = -lefence
SISDIR = ../sis1100-linux-sis1100-2.16-0
TCLDIR = /usr/include/tcl8.6
INC = -I$(SISDIR)/dev/pci/ -I$(SISDIR)/sis3100_calls -I$(SISDIR)/sis3100_calls/header -I$(TCLDIR) -I/usr/local/hfbs/include -I.
CFLAGS=-g -fPIC -Wall -DCLIENT -I/usr/local/include $(INC)
LIBS = -lm -lefence
SISLIBS = -L$(SISDIR)/sis3100_calls/ -l_sis3100 

all: libs

libs: libacquire.so libsharehist.so

libacquire.so: $(AOBJ)
	$(CC) -shared -o libacquire.so $(AOBJ) $(LIBS) $(SISLIBS)

libsharehist.so: $(HOBJ)
	$(CC) -shared -o libsharehist.so $(HOBJ) $(LIBS)

libmtxutil.so: $(MOBJ)
	$(CC) -shared -o libmtxutil.so $(MOBJ) $(LIBS)

clean:
	rm -f *.o
realclean:
	rm -f *.o *.so

.c.o:
	$(CC) $(CFLAGS) -c $*.c
