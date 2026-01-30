#include <stdio.h>
#include <math.h>

#define PULSEPERREV 4096 

int debug = 0;

#include <stdlib.h>
char *geomfile = "geometry.tbl";
char *outfile  = "enclook.tbl";

void
calc_dandv(i, i_off, omega, distance, velocity) 
short i, i_off;
double omega, *distance, *velocity;
{
  double pi = (double) 4 * atan(1.0);
  double theta, thetaprime;
  double amp, omegahalf;
  double dsign, vsign;

  amp = (256 * (0.09))/(15 * pi * pi + 8);
  omegahalf = omega/2; 
  
  theta = (double) (2 * pi * (i - i_off))/PULSEPERREV;
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
    dsign = 1;
    vsign = 1;
  } else if (theta < 2 * pi) {
    thetaprime = 2 * pi - theta;
    dsign = 1;
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
  *distance -= 0.045;

}

void
main(argc, argv)
int argc;
char *argv[];
{
  FILE *fp;
  char ch;
  int count, offset;
  short i, i_off;
  long m_offset, d_offset;
  double omega; /* Cam shaft angular velocity */
  double dist, vel;
  char line[255];

  /* Parameters */

  double v_bragg, d_ms, d_sad, d_sm, t_bin;
  double t_elastic_d, t_elastic_m;
  double delta_e, delta_d, d_fix, t_elastic, t_vary;
  int t_bin_d, t_bin_m;

  /* Dig up a number for neutron mass */
  double e_conv, e_range;
  int e_chan;

  /* Flags */
  int use_ftoff, bin_uev;

  v_bragg = 630.825;
  d_ms  = 2.246;
  d_sad = 4.121;
  d_sm  = 2.011;
  t_bin = 30e-6;
  e_conv = 5.2267e-3; /* Conversion to energy in ueV */
  e_range = 150.0;
  i_off = 0;
  omega = 15.0;       /* Cam frequency */

  use_ftoff = 1;
  bin_uev   = 1;

  if (argc >= 3) debug = 1;
  if (argc >= 2) {
    omega = (double) atof(argv[1]);
  }


  /* Read file containing information about instrument geometry */
  if ((fp = fopen(geomfile,"r"))!= NULL) {
    if (debug) fprintf(stderr,"Reading geometry file\n");
    if (fgets(line,250,fp) != NULL) {
      count = sscanf(line, "%f %f %f %f %f %f %d",
		     &v_bragg, &e_conv, &e_range, 
		     &d_ms, &d_sad, &d_sm, &offset);
      i_off = (short) offset;
    }
    if (fgets(line,250,fp) != NULL) {
      count = sscanf(line, "%d %d", &bin_uev, &use_ftoff);
    }
    fclose(fp);
  }

  t_elastic_d = (d_sad + d_ms) / v_bragg;
  t_bin_d = (int) (t_elastic_d / t_bin);

  t_elastic_m = (d_sm + d_ms) / v_bragg;
  t_bin_m = (int) (t_elastic_m / t_bin);

  if (debug) {
    fprintf(stderr,"Using the following parameters:\n");
    fprintf(stderr,"  omega   = %f\n",omega);
    fprintf(stderr,"  i_off   = %d\n",i_off);
    fprintf(stderr,"  v_bragg = %f\n",v_bragg);
    fprintf(stderr,"  e_conv  = %f\n",e_conv); 
    fprintf(stderr,"  e_range = %f\n",e_range);
    fprintf(stderr,"  d_ms    = %f\n",d_ms);
    fprintf(stderr,"  d_sad   = %f\n",d_sad);
    fprintf(stderr,"  d_sm    = %f\n",d_sm);
    fprintf(stderr,"  t_bin   = %f\n",t_bin);
    fprintf(stderr,"  t_e_d   = %f\n",t_elastic_d);
    fprintf(stderr,"  t_bin_d = %d\n",t_bin_d);
    fprintf(stderr,"  t_e_m   = %f\n",t_elastic_m);
    fprintf(stderr,"  t_bin_m = %d\n",t_bin_m);
  }

  fp = fopen(outfile,"w");
  omega *= 2.0; /* Convert cam angular vel to monochromator angular vel */
  for (i = 0; i< PULSEPERREV; i++) {
    calc_dandv(i,i_off,omega,&dist,&vel);

    d_fix = d_ms + d_sad;
    t_elastic = d_ms / v_bragg;
    t_vary = (d_ms - 2.0*dist) / (v_bragg + vel);
    d_offset = (int) ((t_vary - t_elastic) / t_bin);
    
    d_fix = d_ms + d_sm;
    t_elastic = (d_ms + d_sm) / v_bragg;
    t_vary = (d_ms + d_sm - 2.0*dist) / (v_bragg + vel);
    m_offset = (int) ((t_vary - t_elastic) / t_bin);

    delta_e = e_conv * vel * (2 * v_bragg + vel);

    if (bin_uev) {
      e_chan = (delta_e/e_range) * 2048.0 + 2048;
      if (e_chan < 0)    e_chan = 0;
      if (e_chan > 4095) e_chan = 4095;
    } else {
      e_chan = (int) (i - i_off);
      /* Wrap the resulting number around the extremes of the range */
      while (e_chan < 0)    e_chan += 4096;
      while (e_chan > 4095) e_chan -= 4096;
    }
    
    if (!use_ftoff) { /* Turn off the flight time offset correction */
      m_offset = 0;
      d_offset = 0;
    }


    if (debug) printf("%4d %12.6f %12.6f %12.6f %4d %4d %4d\n",
		       i, dist, vel, delta_e, e_chan, m_offset, d_offset);

    fprintf(fp,"%4d %4d %4d %4d\n", i, e_chan, m_offset, d_offset);

  }
  fclose(fp);

}
