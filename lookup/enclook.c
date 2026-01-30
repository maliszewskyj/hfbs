#include <stdio.h>
#include <math.h>
#include <getopt.h>

#define PULSEPERREV 4096 

void
calc_dandv(short i, short i_off, float omega, 
	   float * distance, float * velocity) {
  float pi = (float) 4 * atan(1.0);
  float theta, thetaprime;
  float amp, omegahalf;
  float dsign, vsign;

  amp = (256 * (0.09))/(15 * pi * pi + 8);
  omegahalf = omega / 2;
  
  theta = (float) (2 * pi * (i - i_off))/PULSEPERREV;
  if (theta < 0) theta += 2 * pi;
  if (theta > 2 * pi) theta -= 2 * pi;

  if (theta < (pi/2)) {
    thetaprime = theta;
    dsign = 1;
    vsign = 1;
  } else if (theta < pi) {
    thetaprime = pi - theta;
    dsign = 1;
    vsign = -1;
  } else if (theta < (3*pi/2)) {
    thetaprime = theta - pi;
    dsign = -1;
    vsign = 1;
  } else if (theta < 2 * pi) {
    thetaprime = 2 * pi - theta;
    dsign = -1;
    vsign = -1;
  }

  /* Now thetaprime should be between 0 and pi/2 */

  /* Calculate distance and velocity for triangle cam profile */
  if (thetaprime < (3*pi/16)) {
    *distance = dsign * amp * thetaprime * thetaprime / 2;
    *velocity = vsign * amp * omegahalf * thetaprime;
  } else if (thetaprime < (5 * pi/16)) {
    *distance = dsign * amp * ( (3 * pi * thetaprime) / 16
				- cos(8*thetaprime - 3*pi/2)/64
				+ 1/64.0
				- 9 * pi * pi / 512);
    *velocity = vsign * amp * omegahalf * ( 3 * pi / 16
				   + sin(8*thetaprime - 3*pi/2)/8);
  } else {
    *distance = dsign * amp * (pi * thetaprime / 2
		      - thetaprime * thetaprime / 2
		      + 1/32.0
		      - 17 * pi * pi / 256);
    *velocity = vsign * amp * omegahalf * ( pi / 2 - thetaprime);
  }

}

int
main(int argc, char * argv[]) {
  char ch;
  int detmon;
  short i, i_off;
  float omega;
  float dist, vel;
  int offset;

  /* Parameters */

  float v_bragg, d_ms, d_sad, d_sm, t_bin;
  float delta_e, delta_d, d_fix, t_elastic, t_flight;

  /* Dig up a number for neutron mass */
  float e_conv, e_range;
  unsigned int e_chan;

  v_bragg = 630.825;
  d_ms  = 2.246;
  d_sad = 4.121;
  d_sm  = 2.011;
  t_bin = 30e-6;
  e_conv = 5.2267e-3; /* Conversion to energy in ueV */
  e_range = 200.0;

  i_off = 0;
  omega = 50.0;
  detmon = 1;

  while ((ch = getopt(argc, argv, "w:o:m")) != -1)
    switch(ch) {
    case 'm':
      detmon = 0;
      break;
    case 'w':
      omega = atof(optarg);
      break;
    case 'o':
      i_off = (short) atoi(optarg);
      break;
    default:
      fprintf(stderr,"Usage: enclook -m -w omega -o offset\n");
      exit(1);
    }

  if (detmon) {
    d_fix = d_ms + d_sad;
  } else {
    d_fix = d_ms + d_sm;
  }
  t_elastic = d_fix / v_bragg;

  for (i = 0; i< PULSEPERREV; i++) {
    calc_dandv(i,i_off,omega,&dist,&vel);
    t_flight = (d_fix - dist) / (v_bragg + vel);
    offset = (int) ((t_flight - t_elastic) / t_bin);
    delta_e = e_conv * (2 * v_bragg * vel + vel * vel);

    e_chan = (delta_e/e_range) * 2048.0 + 2048;
    if (e_chan > 4095) e_chan = 4095;
    
    /* printf("%4d %3d %f\n",i,offset, delta_e); */
    printf("%4d %12.6f %12.6f %12.6f %4d\n",i, dist, vel, delta_e, e_chan);

  }

}
