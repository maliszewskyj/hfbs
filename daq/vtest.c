#include <stdio.h>
#include "vsc16.h"
#define VSC_BASE 0x43520000

void
initvsc () {
  /* reset module */
  *(unsigned short *) (VSC_BASE + VSC_RESET)   = 1;
  *(unsigned short *) (VSC_BASE + VSC_CONTROL) = 0;
}

void
armvsc () {
  printf("Arm counters.\n");
  *(unsigned short *) (VSC_BASE + VSC_CONTROL) = 1;
}

void
disarmvsc () {
  printf("Disarm counters.\n");
  *(unsigned short *) (VSC_BASE + VSC_CONTROL) = 0;
}

void main() {
  int val;
  int i;

  initvsc();
  armvsc();
  sleep(10);
  disarmvsc();
  for (i=0;i<16;i++) {
    val = *(unsigned int *) (VSC_BASE + VSC_DATA(i));
    printf("Counter[%2d] = %d\n",i,val);
  }
  
}
