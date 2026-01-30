/*
 * snapshot.c - print a "snapshot" of DSP memory locations
 */
#include <stdio.h>

void main() {
  unsigned long vresult, flagirqb, irqstat, nofirq, aseqstat;
  unsigned long latchraw, latchval, latchmsk;
  unsigned long ftlidx,mftlook, dftlook;
  unsigned long vadm1, vadm2;
  unsigned long monoff, detoff,extra;
  unsigned long monval,lodval,hidval;

  vresult  = *(unsigned long *) 0x0f002000;
  flagirqb = *(unsigned long *) 0x0f002004;
  nofirq   = *(unsigned long *) 0x0f002008;
  irqstat  = *(unsigned long *) 0x0f00200c;
  aseqstat = *(unsigned long *) 0x0f002010;

  monoff   = *(unsigned long *) 0x0f002014;
  detoff   = *(unsigned long *) 0x0f002018;
  extra    = *(unsigned long *) 0x0f00201c;

  vadm1    = *(unsigned long *) 0x0f002040;
  vadm2    = *(unsigned long *) 0x0f002044;

  latchraw = *(unsigned long *) 0x0f002048;
  latchval = *(unsigned long *) 0x0f00204c;
  latchmsk = *(unsigned long *) 0x0f002050;
  
  monval   = *(unsigned long *) 0x0f002054;
  lodval   = *(unsigned long *) 0x0f002058;
  hidval   = *(unsigned long *) 0x0f00205c;
  

  /* Print report */

  printf("VRESULT  = 0x%08x == %lu\n",vresult,vresult);
  printf("FLAGIRQB = 0x%08x == %lu\n",flagirqb,flagirqb);
  printf("NOFIRQ   = 0x%08x == %lu\n",nofirq,nofirq);
  printf("IRQSTAT  = 0x%08x == %lu\n",irqstat,irqstat);
  printf("ASEQSTAT = 0x%08x == %lu\n",aseqstat,aseqstat);
  printf("VADM1    = 0x%08x == %lu\n",vadm1,vadm1);
  printf("VADM2    = 0x%08x == %lu\n",vadm2,vadm2);
  printf("LATCHRAW = 0x%08x == %lu\n",latchraw,latchraw);
  printf("LATCHVAL = 0x%08x == %lu\n",latchval,latchval);
  printf("LATCHMSK = 0x%08x == %lu\n",latchmsk,latchmsk);
  printf("MONVAL   = 0x%08x == %lu\n",monval,monval);
  printf("LODVAL   = 0x%08x == %lu\n",lodval,lodval);
  printf("HIDVAL   = 0x%08x == %lu\n",hidval,hidval);
  printf("MONOFF   = 0x%08x == %lu\n",monoff,monoff);
  printf("DETOFF   = 0x%08x == %lu\n",detoff,detoff);
  printf("EXTRA    = 0x%08x == %lu\n",extra,extra);
}
