#include <stdio.h>
#include "tech8014.h"

#define IP_CARRIER 0xff046000   /* Industry Pack Carrier + Encoder */
#define IP_BASE    (IP_CARRIER + IP_A_IO_BASE)
#define ENC_CONV   16384

float omega;

/* Routines for controlling Tech80 Model 14 High Speed Encoder */
void
enc_reset() {
  *(unsigned short *) (IP_BASE + GLOB_MULTI) = 0x3800;
}

void
enc_initlin() {
  *(unsigned short *) (IP_BASE + GLOB_CSR) = 0x8108;
  *(unsigned short *) (IP_BASE + GLOB_TSMODE) = 0x0001;
  *(unsigned short *) (IP_BASE + AXIS0_OFFSET + AXIS_IC) = 0x9000;
  *(unsigned short *) (IP_BASE + AXIS0_OFFSET + AXIS_CC) = 0x0504;
}

void
enc_initcir() {
  *(unsigned short *) (IP_BASE + GLOB_CSR) = 0x8108;
  *(unsigned short *) (IP_BASE + GLOB_TSMODE) = 0x0001;
  *(unsigned short *) (IP_BASE + AXIS0_OFFSET + AXIS_IC) = 0x9000;
  *(unsigned short *) (IP_BASE + AXIS0_OFFSET + AXIS_CC) = 0x0524;
}

void
enc_clear() {
  *(unsigned short *) (IP_BASE + AXIS0_OFFSET + AXIS_CSR) = 0x000c;
}

void
enc_capture() {
  *(unsigned short *) (IP_BASE + AXIS0_OFFSET + AXIS_CSR) = 0x0001;
}

/* Read encoder position */
int
enc_read() {
  unsigned short vals[2];
  unsigned int ival;

  vals[0] = *(unsigned short *) (IP_BASE + AXIS0_OFFSET + AXIS_OUT_HI);
  vals[1] = *(unsigned short *) (IP_BASE + AXIS0_OFFSET + AXIS_OUT_LO);

  ival = 0xffffff & ((vals[0] << 16) + vals[1]);
  return ival;
}

void
enc_verify() {
  unsigned short val;
  unsigned char ch;

  printf("   ");
  val = *(unsigned short *) (IP_BASE + 0x80);
  ch = (unsigned char) (0x00ff & val);
  putchar(ch);

  val = *(unsigned short *) (IP_BASE + 0x82);
  ch = (unsigned char) (0x00ff & val);
  putchar(ch);

  val = *(unsigned short *) (IP_BASE + 0x84);
  ch = (unsigned char) (0x00ff & val);
  putchar(ch);
  
  val = *(unsigned short *) (IP_BASE + 0x86);
  ch = (unsigned char) (0x00ff & val);
  putchar(ch);

  putchar('\n');

  val = *(unsigned short *) (IP_BASE + 0x88);
  ch = (unsigned char) (0x00ff & val);
  printf("   MFG ID   = 0x%02x\n",ch);

  val = *(unsigned short *) (IP_BASE + 0x8A);
  ch = (unsigned char) (0x00ff & val);
  printf("   MODEL NO = 0x%02x\n",ch);

  val = *(unsigned short *) (IP_BASE + 0x8C);
  ch = (unsigned char) (0x00ff & val);
  printf("   REVISION = 0x%02x\n",ch);

}

/* Determine the angular velocity of doppler drive */
void
enc_omega() {
  float scalefactor;
  int ival1,ival2;
  int duration;

  scalefactor = ( 2 * 3.141592654 ) / ENC_CONV;
  duration = 5;
  /* First determine whether the drive is moving at all */
  enc_capture();
  ival1 = enc_read();
  tsleep(50);
  enc_capture();
  ival2 = enc_read();
  if (ival1 == ival2) {
    omega = 0.0;
    return;
  }

  /* If the drive is still moving, determine angular velocity */
  /* Put encoder in linear positioning mode */
  enc_initlin();
  enc_clear();
  /* Clear and let it run for a few seconds, say 10 or so */
  /* Read number of counts, divide by time, multiply by 2pi/conversion */
  enc_capture();
  ival1 = enc_read();
  sleep(duration);
  enc_capture();
  ival2 = enc_read();
  
  omega = scalefactor * (((float) (ival2 - ival1)) / duration);

  /* Restore to circular positioning mode */
  enc_initcir();
}

/*

  enctest -c -r

 */


void
main(argc,argv)
int argc;
char * argv[];
{
  int i, val;
  char ch;
  int reset_flag=0, init_flag=0, capture_flag=0, read_flag=0;
  int verify_flag=0, clear_flag=0;

  /* Process command line options */
  if (argc >= 2) {
    for (i=1;i<argc;i++) {
      if (((ch = argv[i][0]) == '-') && (strlen(argv[i]) > 1)) {
	switch (ch = argv[i][1]) {
	case 'C':
	  clear_flag=1;
	  break;
	case 'c':
	  capture_flag=1;
	  break;
	case 'V':
	  verify_flag=1;
	  break;
	case 'r':
	  read_flag=1;
	  break;
	case 'R':
	  reset_flag=1;
	  break;
	case 'h':
	default:
	  fprintf(stderr,"Usage: enctest [-CcVrRh]\n");
	  exit(1);
	}
      }
    }
  }

  if (verify_flag) enc_verify();

  if (reset_flag) {
    enc_reset();
    enc_initcir();
  }

  if (clear_flag) enc_clear();

  if (capture_flag) enc_capture();

  if (read_flag) {
    val = enc_read();
    printf("%d\n",val);
  }

}
