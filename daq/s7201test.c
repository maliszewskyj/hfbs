#include <stdio.h>
#include "str7201.h"

#define SCALERBASE 0x43508000
#define NUMREADS   5
int test_pulse = 0;

/* Initialisation routine for STR7201 multiscaler */
void 
init_str7201() { 
  unsigned long* scalptr;
  printf("Initializing scaler...");

  /* Reset scaler */
  *(unsigned long*) (SCALERBASE + S7201_KEY_RESET) = 0x0;

  /* Set up to read channels 0-19 */
  *(unsigned long*) (SCALERBASE + S7201_COPY) = 0xFFF00000;
  
  /* Disable next logic */
  *(unsigned long *) (SCALERBASE + 0x2c) = 0x0;
  /* Clear FIFO */
  *(unsigned long *) (SCALERBASE + 0x20) = 0x0;
  /* Enable next logic */
  *(unsigned long *) (SCALERBASE + 0x28) = 0x0;

  tsleep(1);
  printf("scaler init complete\n");
}

void main(argc, argv)
int argc;
char * argv[];
{
  int i, j, n, nreads=NUMREADS, nchans=20, waitsec=5;
  long val;

  printf("STR7201 at base address 0x%08X\n",
	 (unsigned int)SCALERBASE); 

  /* Process command line options */

  if (argc >= 2) {
    for (i=1;i<argc;i++) {
      if (!strncmp(argv[i],"-t",2)) {
	if (argc > i + 1) {
	  i++;
	  nreads = 0;
	  n = sscanf(argv[i],"%d",&waitsec);
	  if ((n != 1) || (waitsec <= 0)) {
	    printf("Specify a wait time in seconds\n");
	    exit(0);
	  }
	} else if (!strncmp(argv[i],"-n",2)) {
	  if (argc > i + 1) {
	    i++;
	    nreads = 0;
	    n = sscanf(argv[i],"%d",&nreads);
	    if ((n != 1) || (nreads < 1) || (nreads > NUMREADS)) {
	      printf("Specify a number less than %d.\n",(int)NUMREADS);
	      exit(0);
	    }
	  } else {
	    printf("Specify the number of points to read.\n");
	    exit(0);
	  }
	} else {
	  printf("Usage: s7201test [[-t wait] [-n npts]]\n");
	  printf("       -t time   Interval over which to count\n");
	  printf("       -n npts   Number of points to read (def=512)\n");
	  exit(0);
	}
      }
    }
  }
  init_str7201();
  *(unsigned long *) (SCALERBASE + 0x24) = 0x0;
  tsleep(200);

  /* Read from FIFO */
  for (i=0;i<nreads;i++) {
    printf("Read %d\n",i);
    *(unsigned long *) (SCALERBASE + 0x24) = 0x0;
    tsleep(200);
    for (j=0;j<nchans;j++) {
      val = *(unsigned long *) (SCALERBASE + S7201_FIFO);
      printf("  Chan(%02d) = %8d\n",j,val);
    }
  }
}

