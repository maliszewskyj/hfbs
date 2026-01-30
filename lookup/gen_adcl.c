#include <stdio.h>
#include <math.h>

int debug = 0;
/* A few constants */
float v_bragg = 630.825;
float d_ms  = 2.25;
float d_sad = 4.10;
float d_sm  = 2.00;
float t_bin = 15e-6;
float d_scale, v_scale;
float d_off=0, v_off=0;

void
main(void){
  int v_idx, d_idx,i,j;
  int offset;
  float t_elastic, t_flight, delta_v, delta_d, d_fix;

  d_fix = d_ms;
  t_elastic = (d_fix) / v_bragg;
  d_scale = (0.082 * (10.0/8.0)) / 2048;
  v_scale = 20.0 / 4096;

  for(i=0;i<128;i++) {
    d_idx = (i & 0x80) ? (- (2048 - ((i&0x7f) << 4))) : (i << 4);
    delta_d = d_idx * d_scale;

    for(j=0;j<256;j++) {
    v_idx = (j & 0x80) ? (- (2048 - ((j&0x7f) << 4))) : (j << 4);
    delta_v = v_idx * v_scale;

      t_flight = (d_fix + delta_d) / (v_bragg + delta_v);
      offset = (int) ((t_flight - t_elastic) / t_bin);
      if (debug) printf("%3d %3d %4d ",i,j,offset);
      if (offset == 0) {
	printf(" 0\n");
      } else if (offset > 0) {
	printf(" 1\n");
      } else {
	printf("-1\n");
      }
    }
  }
}
