#include <stdio.h>
#include "ics110b.h"

#define ICS110BASE 0x43510000
#define NUMREADS   512

/* Initialization routine for ICS110B sigma-delta analog-digital converter */
void
init_ics110b() {
  unsigned long * iptr;
  unsigned short * sptr;

  printf("ADC Initialization");
  fflush(stdout);
  /* Reset board */
  *(unsigned short *) (ICS110BASE + ICS_STATUS)  = 0x0080;
  /* Configure board as standalone master */
  *(unsigned short *) (ICS110BASE + ICS_CONTROL) = 0x0002;
  /* Configure to read from 2 channels */
  *(unsigned short *) (ICS110BASE + ICS_NCHANS)  = 0x0001;

  printf("."); fflush(stdout);
  /* Set internal clock to 10 kHz */
  iptr = (unsigned long *) (ICS110BASE + ICS_CLKFREQ);
  *iptr = 0x38001e05;
  tsleep(2);
  *iptr = 0x082e294b;
  tsleep(2);
  *iptr = 0x38001e04;
  tsleep(10);
  *iptr = 0x38001e00;
  tsleep(2);
  
  printf("."); fflush(stdout);
  /* Calibrate the Crystal CS5397 Analog-Digital Converter Chip */
  sptr = (unsigned short *) (ICS110BASE + ICS_ADC_CONF);
  *sptr = 0x8000; tsleep(10);
  *sptr = 0x8001; tsleep(10);
  *sptr = 0x80c0; tsleep(10);
  *sptr = 0x8044; tsleep(10);
  *sptr = 0x0000; tsleep(100);
  *sptr = 0x8000; tsleep(10);
  *sptr = 0x8001; tsleep(10);
  *sptr = 0x8000; tsleep(10);
  *sptr = 0x0000; tsleep(10);  
  
  printf("."); fflush(stdout);
  /* Disable on-chip high-pass filter */
  sptr = (unsigned short *) (ICS110BASE + ICS_ADC_CONF);
  *sptr = 0x8000; tsleep(10);
  *sptr = 0x8002; tsleep(10);
  *sptr = 0x800c; tsleep(10);
  *sptr = 0x0000; tsleep(10);

  printf("."); fflush(stdout);
  /* Set internal clock to 12.8 MHz (50 kHz data output rate) */
  iptr = (unsigned long *) (ICS110BASE + ICS_CLKFREQ);
  *iptr = 0x38001e05;
  tsleep(2);
  *iptr = 0x10381375;
  tsleep(2);
  *iptr = 0x38001e04;
  tsleep(10);
  *iptr = 0x38001e00;
  tsleep(2);  

  printf("."); fflush(stdout);
  /* Clear FIFO - shouldn't have to do this */
  *(unsigned short *) (ICS110BASE + ICS_STATUS) = 0x0040;

  /* Start acquiring */
  *(unsigned short *) (ICS110BASE + ICS_CONTROL) = 0x0003;

  printf("ADC initialization complete.\n");

}

void main() {
  int i, nreads = 10;
  unsigned long raw;
  unsigned short raw1, raw2;
  short adc1, adc2;

  init_ics110b();
  tsleep(10);
  /* stop acquiring */
  *(unsigned short *) (ICS110BASE + ICS_CONTROL) = 0x0002;

  printf("Print results of reads:\n");
  for (i=0;i<nreads;i++) {
    raw2 = (unsigned short) (raw & 0x0000ffff);
    raw1 = (unsigned short) (raw >> 16);

    if (raw1 & 0x8000) {
      adc1 = - (short) (~raw1);
    } else {
      adc1 = (short) raw1;
    }

    if (raw2 & 0x8000) {
      adc2 = - (short) (~raw2);
    } else {
      adc2 = (short) raw2;
    }

    printf("%8lx ==> 1 = %12d | 2 = %12d\n",raw,adc1,adc2);
  }
}
