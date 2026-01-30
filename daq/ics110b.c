#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ics110b.h"

#define ICS110BASE 0x4351000

/*
 * Configure ICS110B as a standalone master on the VME bus
 * following the example in the 
 *    ICS-110B operating manual E10534 Rev.C, p. 50
 *
 */
void
init_ics110b() {
  unsigned short sval, *sptr;
  unsigned int ival, *iptr;

  /* Perform a board reset */
  sptr = ICS110BASE + ICS_STATUS;
  *sptr = (unsigned short) ICS_BRDRST;

  /* 
   * Program the clock frequency register to set the sampling clock
   * frequency to 25.6 MHz
   * Follow the recipe provided in ICS110 Operating Manual p. 62
   */
  iptr = ICS110BASE + ICS_CLKFREQ;
  /* 1: Configure device and prepare it to receive the programming word */
  *iptr = (unsigned int) ICS_CR_ENPW;
  /* 2: Write programming word */
  *iptr = (unsigned int) ICS_PW_HIHZ;
  /* 3: Load programming word into device */
  *iptr = (unsigned int) ICS_CR_LDPW;
  /* 4: Wait at least 5 ms to device VCO frequency to stabilize */
  sleep(1);
  /* 5: Enable device output of new frequency */
  *iptr = (unsigned int) ICS_CR_ENFR;

  /* Configure the control register */

  /* Set NUM_CHANNELS = 2 */
  
  /* Reset ADC FIFO */
  
  /* Begin acquisition by setting CR<00> ACQUIRE=1 */
}
