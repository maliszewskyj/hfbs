static char rcsid[] = "$Id: dsplink.c,v 1.6 2008/05/05 13:58:34 nickm Exp $";
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

//#include <ctrl.h>
/* sis1100/3100 PCI to VME specific */   
#include <sis1100_var.h>
#include <sharc_utils.h>
#include <sis3100_vme_calls.h>
/* SIS9200 DSP */
#include <sis9200.h>
/* front end modules */
#include <sis3600.h>	      
#include <sis3801.h>
#include <vsc16.h>

#include "dsplink.h"            /* Function prototypes */
#include "dspload.h"
#define TRACE(x...) { printf( x ); }

#define memBase 0x00000000	/* base of DSP BRAM     */
#define ptroff1 0x81202080 	/* Pointer offset 1     */
#define ptroff2 0x81202084	/* Pointer offset 2     */
#define ptroff3 0x81202088	/* Pointer offset 3     */
#define ptroff4 0x8120208C      /* Pointer offset 4     */
#define lokBase 0x81204000	/* INTRAM+0x1000 on     */
#define lokMRng 0x81208000      /* INTRAM+0x2000        */
#define lokDRng 0x8120c000      /* INTRAM+0x2000        */
#define lokEner 0x81210000      /* INTRAM+0x4000 on     */
#define lokTime 0x81240000      /* Time of flight lookup table (INTRAM+0x34000) */
#define lokMon  0x81250000      /* Monitor  write ptr offset  */
#define lokDet  0x81290000      /* Detector write ptr offset  */

#define hisBase 0x00080000	/* base of 1 MB BRAM on VSB   */
#define ctrlreg 0x00800000	/* DSP ctrl reg on VSB        */
#define irqctr  0x81202008	/* irq counter in BRAM on VSB */
#define INTRAM  0x81200000      /* Base address of DSP internal memory */

#define RESET   0x1000
#define RUN     0x0010

#define VSC_BASE     0x43520000
#define SIS3801_BASE 0x43508000
#define SCALERBASE   0x43508000
#define SIS3600_BASE 0x43509000
#define LATCHBASE    0x43509000
#define NPSTLOOK   4096
#define NADCLOOK   2048
#define NPOFFLOOK  65536
#define NTIMLOOK   16384

#define ENC_CONV   16384       /* Encoder conversion pulses/revolution */
#define PULSEPERREV 4096       /* Encoder conversion "cooked" to 12 bits */

#define EOS        0xefefefef
#define SERVERPORT 20000
#define MAXMSG      1024             
#define PKGMIN         5
#define BLKSIZE      256

#define WORDWRITTEN    4 /* CHECK ME OUT!!!!! */

#define OP_CMD         1
#define OP_ACK         2
#define OP_DAT         3
#define OP_ERR         4

#define ELEMENTS     4096*20


/*===========================================================================*/
/* Resource files                                                            */
/*===========================================================================*/
static char * pbinfile    = "/usr/local/hfbs/asm/pstbin.ldr";
static char * tbinfile    = "/usr/local/hfbs/asm/timbin.ldr";
static char * vbinfile    = "/usr/local/hfbs/asm/encbin.ldr";
static char * pstlook     = "/usr/local/hfbs/lookup/pstlook.tbl";
static char * pointerlook = "/usr/local/hfbs/lookup/pointer.tbl";
static char * geomfile    = "/usr/local/hfbs/lookup/geometry.tbl";


/*===========================================================================*/
/* Global variables                                                          */
/*===========================================================================*/
static char line[255];
extern int vme_fd;
extern int sdram_fd;
extern int sharc_fd;
u_int32_t sis3600_base=SIS3600_BASE;
u_int32_t sis3801_base=SIS3801_BASE;
static int scalers[16];
double omega = 50.0; /* Angular velocity of Doppler Drive */
extern int timbin;
extern int test_pulse;
u_int32_t empty_array[ELEMENTS];
extern unsigned long *data_ptr;
extern unsigned long *save_ptr;
extern unsigned long *ctrl_ptr,*look_ptr;
extern unsigned long *run_ptr,*new_run_ptr,*mode_ptr,*save_time_ptr;
extern unsigned long *rptr,*wptr,*mem;
extern unsigned long *timlook;
extern int traceflag;

/*===========================================================================*/
/* Load DSP     					  		     */
/*===========================================================================*/
int dsp_load(int mode){
    char* dsppath;
    int rc=0,clearrc,retcode=1;
    int count=0,loadcount=0;
    int offset;
    int retries;

    switch(mode){
    case 0: dsppath = pbinfile; break;
    case 1: dsppath = tbinfile; break;
    case 2: 
    case 3:
      dsppath = vbinfile; break;
    default:
      fprintf(stderr,"Unsupported mode:%d\n",mode);
      fprintf(stderr,"Supported modes: 0 (pstbin), 1 (timebin), 2 (velbin)\n");
      exit(-1);
    }

    retries = 0;
    while (retries<3) {
      if (traceflag) TRACE("resetting SHARC DSP\n");
      rc = reset_dsp(sharc_fd);

      if (traceflag) TRACE("loading DSP\n");
      rc = load_dsp_file(sharc_fd, dsppath);

      memset(empty_array,0,sizeof(empty_array));
      clearrc = s3100_sdram_write(sdram_fd, hisBase, empty_array, ELEMENTS) ;
      if(clearrc!=ELEMENTS*4) {
        if (traceflag) TRACE("error on clearing SDRAM\n");
        retries++;
	if (retries<3) TRACE("retrying\n");
      } else break;
    }

    if (retries>3) return -1;

    /* start SHARC */
    if (traceflag) TRACE("starting SHARC DSP\n");
    rc = start_dsp(sharc_fd);
    return rc;
}
/*==========================================================================*/
/* Initialize VSC16 scaler      					    */
/*==========================================================================*/
int init_vsc16() {
  int rcsum=0;
  int i;
  if (traceflag) TRACE("init VSC16\n");
  /* reset module */
  rcsum+=vme_A32D16_write(vme_fd,VSC_BASE + VSC_RESET,1);
  rcsum+=vme_A32D16_write(vme_fd,VSC_BASE + VSC_CONTROL,0);
  /* clear scaler array */
  for (i=0;i<16;i++) scalers[i] = 0;
  return rcsum;
}

/*==========================================================================*/
/* Start VSC16 scaler      					            */
/*==========================================================================*/
int start_vsc16(unsigned int preset) {
  int rcsum=0;
  /* preset channel 1 to 50 M counts */
  if (preset > 0) {
    rcsum+=vme_A32D16_write(vme_fd,VSC_BASE + VSC_DIRECT,1);
    rcsum+=vme_A32D32_write(vme_fd,VSC_BASE + 0xC0,preset);
  } else {
    rcsum+=vme_A32D16_write(vme_fd,VSC_BASE + VSC_DIRECT,0);
  }
  rcsum+=vme_A32D16_write(vme_fd,VSC_BASE + VSC_CONTROL,1);
  rcsum+=vme_A32D16_write(vme_fd,VSC_BASE + VSC_IRQMSK,1);
 return rcsum;
}

/*==========================================================================*/
/* Stop VSC16 scaler          					            */
/*==========================================================================*/
int stop_vsc16(void) {
  int rcsum=0;
  rcsum+=vme_A32D16_write(vme_fd,VSC_BASE + VSC_CONTROL,0);
  return rcsum;
}

/*==========================================================================*/
/* Read VSC16 scaler      					            */
/*==========================================================================*/
int read_vsc16(void) {
  int rcsum=0;
  int i;
  if (traceflag) TRACE("Counter values:");
  for (i=0;i<16;i++) {
      rcsum+=vme_A32D32_read(vme_fd,VSC_BASE + VSC_DATA(i),&scalers[i] );
      if ((i%4) == 0) if (traceflag) TRACE("\n");
      if (traceflag) TRACE(" %15d",scalers[i]);
  }
  if (traceflag) TRACE("\n");
  return rcsum;
}

/*==========================================================================*/
/* Determine the angular velocity of doppler drive                          */
/*==========================================================================*/

int enc_omega(void) {
  int rcsum=0;
  double scalefactor;
  int ival2;
  int duration;
  int rc=0;
  u_int16_t rdatum;
  int arm;

  if (traceflag) TRACE("Measuring Doppler Shaft angular velocity\n");
  scalefactor = ( 2 * 3.141592654 ) / 4096.0;
  duration = 5;

  rcsum+=init_vsc16();
  rcsum+=start_vsc16(duration*10000000);

  if (traceflag) TRACE("polling for armed flag to clear\n");
  arm=1;
  while((arm==1)&&(rc==0)) {
    rc=vme_A32D16_read(vme_fd,VSC_BASE + VSC_CONTROL,&rdatum);
    arm=rdatum&1;
  }

  rcsum+=read_vsc16();
  ival2 = scalers[15]; /* Choose last scaler as position counter */

  omega = scalefactor * (((double) ival2) / duration);
  if (traceflag) TRACE("  Accumulated %d counts in %d seconds\n",ival2,duration);
  if (traceflag) TRACE("  Angular velocity = %f radians/sec\n",omega);
  return rcsum;
}

/*===========================================================================*/
/* Initialize Latch      					  	     */
/*===========================================================================*/
int init_sis3600(void) {
  int rcsum=0;
  unsigned int period = (timbin * 10) - 1;

  /* KEY Reset */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_KEYRESET,SIS3600_KEYRESET);
  /* pipeline mode on */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_CTRL,SIS3600_CTRL_PIPEON);

  /* preset frequency register */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_FREQUENCY,0x800000);
  usleep(1);
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_FREQUENCY,period);
  /* output mode 3 , all outputs to pulser */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_CTRL,
                          SIS3600_CTRL_OUTMODE0ON+SIS3600_CTRL_OUTMODE1ON);
  /* enable external next*/
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_CTRL,SIS3600_CTRL_EXTLNEON);
  /* enable next logic */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_ENLNE,SIS3600_ENLNE);
  /* switch on user LED */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_CTRL,SIS3600_CTRL_LEDON);
  /* enable pulser -> start acquisition, moved to DSP code or other place */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_CTRL,SIS3600_CTRL_PULSERON); 
  return rcsum;
}

int sis3600_read(int * val) {
  int rcsum=0;
  /* KEY Reset */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_KEYRESET,SIS3600_KEYRESET);
  usleep(1000);
  /* enable next logic */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_ENLNE,SIS3600_ENLNE);

  /* Soft next clock */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_SOFTLNE,SIS3600_SOFTLNE);

  /* Now read from FIFO */
  rcsum+= vme_A32D32_read(vme_fd,sis3600_base+SIS3600_FIFO,val);

  return rcsum;
}

/*===========================================================================*/
/* Initialize Scaler          					  	     */
/*===========================================================================*/
int init_sis3801(int mode) {
  int rcsum=0;
  /* KEY Reset */
  rcsum+=vme_A32D32_write(vme_fd,sis3801_base+SIS3801_KEYRESET,SIS3801_KEYRESET);
  /* copy disable */
  if (mode==2 || mode==3) {
     if (traceflag) TRACE("19 scaler channels\n");
     rcsum+=vme_A32D32_write(vme_fd,sis3801_base+SIS3801_COPY,0xFFF80000);
  } else {
     if (traceflag) TRACE("18 scaler channels\n");
     rcsum+=vme_A32D32_write(vme_fd,sis3801_base+SIS3801_COPY,0xFFFC0000);
  }
  /* enable external LNE*/
  rcsum+=vme_A32D32_write(vme_fd,sis3801_base+SIS3801_CTRL,SIS3801_CTRL_EXTLNEON);
  /* enable LNE logic */
  rcsum+=vme_A32D32_write(vme_fd,sis3801_base+SIS3801_ENLNE,SIS3801_ENLNE);
  if (test_pulse==1) {
    /* enable 25 MHz test pulses */
    if (traceflag) TRACE("test pulses on\n");
    rcsum+=vme_A32D32_write(vme_fd,sis3801_base+SIS3801_CTRL,0x30); 
  }
  else {
    /* disable 25 MHz test pulses */
    if (traceflag) TRACE("test pulses off\n");
    rcsum+=vme_A32D32_write(vme_fd,sis3801_base+SIS3801_CTRL,0x3000); 
  }
  /* switch on user LED */
  rcsum+=vme_A32D32_write(vme_fd,sis3801_base+SIS3801_CTRL,SIS3801_CTRL_LEDON);
  return rcsum;
}

/*===========================================================================*/
/* reset DSP   			     		                             */
/*===========================================================================*/
int dsp_reset(void) {
    int retcode;
    int offset;
    /* Reset SHARC */
    if (traceflag) TRACE("resetting SHARC DSP\n");
    return reset_dsp(sharc_fd);
}

/*===========================================================================*/
/* Build time lookup table                                                   */
/*===========================================================================*/
void buildtlook(void) {
  int i;
  unsigned int sharc_address;
  unsigned int val;
  int rc;
  if (timbin < 30) timbin = 30;
  if (traceflag) TRACE("Building Time lookup table: timbin = %d usec.\n",timbin);
  sharc_address = (lokTime-INTRAM);
  for (i=0;i<NTIMLOOK;i++) {
    val = (i * 30) / timbin;
    if (val > 4095) val = 4095;
    rc=s3100_sharc_write(sharc_fd,sharc_address+=4,&val,1); 
    if (rc!=4) {
        if (traceflag) TRACE("error on downloading time lookup table\n");
	exit(-1);
    }
  }
}

void calc_dandv(int i, int i_off, double omega, 
		double *distance, double *velocity) 
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

void enc_lookup(void) {
  FILE * fp;
  int i, j, count, i_off, offset;
  long  *mptr,*dptr,*eptr;
  long  e_chan, m_offset, d_offset;
  double dist, vel;

  double v_bragg, d_ms, d_sad, d_sm, t_bin;
  double t_elastic_d, t_elastic_m;
  double delta_e, delta_d, d_fix, t_elastic, t_vary;
  int t_bin_d, t_bin_m;

  u_int32_t  e_chan_array[PULSEPERREV];
  u_int32_t  m_offset_array[PULSEPERREV];
  u_int32_t  d_offset_array[PULSEPERREV];
  /* Dig up a number for neutron mass */
  double e_conv, e_range;
  /* Flags */
  int use_ftoff, bin_uev;
  int writerc;

  v_bragg = 630.825;
  d_ms  = 2.25;
  d_sad = 4.12;
  d_sm  = -0.09;
  t_bin = 30e-6;
  e_conv = 5.2178e-3; /* Conversion to energy in ueV */
  e_range = 200.0;
  i_off = 868;
  //  omega = 50.0;       /* Cam frequency */

  use_ftoff = 1;
  bin_uev   = 1;

  if (traceflag) fprintf(stderr,"Using omega = %f\n",omega);

  /* Read file containing information about instrument geometry */
  if ((fp = fopen(geomfile,"r"))!= NULL) {
    if (traceflag) fprintf(stderr,"Reading geometry file\n");
    if (fgets(line,250,fp) != NULL) {
      // The following lines were masked out. Can't remember why.
            count = sscanf(line, "%lf %lf %lf %lf %lf %lf %d",
      		     &v_bragg, &e_conv, &e_range, 
      		     &d_ms, &d_sad, &d_sm, &offset);
       i_off = offset;
    }
    if (fgets(line,250,fp) != NULL) {
      count = sscanf(line, "%d %d", &bin_uev, &use_ftoff);
    }
    fclose(fp);
  }

  omega *= 2.0; /* Convert cam angular vel to monochromator angular vel */
  if (traceflag) { fp = fopen("/usr/local/hfbs/lookup/encbin.lok","w"); }
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

    e_chan_array[i]=e_chan;
    m_offset_array[i]=m_offset;
    d_offset_array[i]=d_offset;

    /*
      if (traceflag) printf("%4d %12.6f %12.6f %12.6f %4d %4d %4d\n",
      i, dist, vel, delta_e, e_chan, m_offset, d_offset);
    */

    if (traceflag) fprintf(fp,"%4d %4ld %4ld %4ld\n", i, e_chan, m_offset, d_offset);

  }
  if (traceflag) fclose(fp);
  if (traceflag) fprintf(stderr,"Writing lookups to DSP\n");
  writerc=0;
  if (traceflag) fprintf(stderr,"  1: Writing Detector Lookup: ");
  writerc += s3100_sharc_write(sharc_fd,(lokDet-INTRAM), d_offset_array,PULSEPERREV) ;
  if (traceflag) fprintf(stderr," %d\n",writerc);
  if (traceflag) fprintf(stderr,"  2: Writing Monitor  Lookup: ");
  writerc += s3100_sharc_write(sharc_fd,(lokMon-INTRAM), m_offset_array,PULSEPERREV) ;
  if (traceflag) fprintf(stderr," %d\n",writerc);
  if (traceflag) fprintf(stderr,"  3: Writing Energy   Lookup: ");
  writerc += s3100_sharc_write(sharc_fd,(lokTime-INTRAM), e_chan_array,PULSEPERREV) ;
  if (traceflag) fprintf(stderr," %d\n",writerc);
  if (writerc!=3*4*PULSEPERREV) {
    if (traceflag) TRACE("error on writing lookup table to DSP SRAM\n");
    exit -1;
 }

/*   fclose(fp); */






/*   eptr = (long *) lokTime; */
/*   mptr = (long *) lokMon; */
/*   dptr = (long *) lokDet; */

/*   printf("Calculating lookup tables for encoder mode\n"); */
/*   sprintf(line,"enclook %f",omega); */
/*   printf("Spawning: %s\n",line); */
/*   system(line); */

/*   printf("Reading encoder lookup table\n"); */
/*   if ((fp = fopen(enclook, "r")) == NULL) { */
/*     fprintf(stderr,"could not open lookup file %s\n",enclook); */
/*     exit(-1); */
/*   } */

/*   for (j=0;j<4096;j++) { */
/*     if (fgets(line,250,fp) == NULL) { */
/*       fprintf(stderr,"***Error loading encoder lookup table!***\n"); */
/*       exit(-1); */
/*     } */
/*     count = sscanf(line,"%d %ld %ld %ld",&i,&e_chan,&m_offset, &d_offset); */
/*     *eptr++ = e_chan; */
/*     *mptr++ = m_offset; */
/*     *dptr++ = d_offset; */
/*     /\* printf("%4d %4d %4d %4d\n",i,e_chan,m_offset,d_offset); *\/ */
/*   } */
/*   fclose(fp); */
/*   printf("Ready.\n"); */
}

/*************************************/
/* load lookup table                 */      
/*************************************/
void loadlook(void) {
  FILE *lookupfile;
  unsigned int sharc_address;
  int j, count;
  int rc;
  unsigned int lookval[NPSTLOOK];
  unsigned int ptr1,ptr2,ptr3,ptr4;


  /* Load lookup tables. Only really necessary for standard and hybrid modes */
  if (traceflag) TRACE("Downloading PST lookup tables\n");
  if ((lookupfile = fopen(pstlook, "r")) == NULL) {
    fprintf(stderr,"could not open lookup file %s\n",pstlook);
    exit(-1);
  }
  sharc_address=lokBase;
  for (j=0;j<NPSTLOOK;j++) {
    count=fscanf(lookupfile,"%x",&lookval[j]);
    if (count!=1) {
        if (traceflag) TRACE("error on reading from pstlook\n");
        fprintf(stderr,"***Error loading PST lookup table!***\n");
	exit(-1);
    }
    //    rc=s3100_sharc_write(sharc_fd,sharc_address+=4,&ptr1,1); 
    //    if (rc!=4) {
    //        if (traceflag) TRACE("error on downloading time lookup table\n");
    //	exit(-1);
    //    }
  }
  fclose(lookupfile);

  rc = s3100_sharc_write(sharc_fd,(lokBase-INTRAM),lookval,NPSTLOOK);
  if (rc!=4*NPSTLOOK) {
    if (traceflag) TRACE("error on downloading time lookup table\n");
    exit(-1);
  }
  /*
  printf("Downloading monitor write pointer offset lookup table\n");
  if ((lookupfile = fopen(monlook, "r")) == NULL) {
    fprintf(stderr,"could not open lookup file %s\n",adclook);
    exit(-1);
  }
  wlptr=(long*)lokMon;
  for (j=0;j<NPOFFLOOK;j++) {
    if(fgets(line,250,lookupfile) == NULL) {
      fprintf(stderr,"***Error loading monitor FTOFF lookup table!***\n");
      exit(-1);
    }
    count = sscanf(line," %d ",wlptr++);
  }
  fclose(lookupfile);
  */

  if (traceflag) TRACE("Downloading pointer positions\n");
  if ((lookupfile = fopen(pointerlook, "r")) == NULL) {
    fprintf(stderr,"could not open pointer file %s\n",pointerlook);
    exit(-1);
  }
  fscanf(lookupfile,"%d",&ptr1);
  fscanf(lookupfile,"%d",&ptr2);
  fscanf(lookupfile,"%d",&ptr3);
  fscanf(lookupfile,"%d",&ptr4);

  if (traceflag) TRACE("ptr1: %d\n",ptr1);
  if (traceflag) TRACE("ptr2: %d\n",ptr2);
  if (traceflag) TRACE("ptr3: %d\n",ptr3);
  if (traceflag) TRACE("ptr4: %d\n",ptr4);
   
  rc=s3100_sharc_write(sharc_fd,(ptroff1-INTRAM),&ptr1,1);
  rc=s3100_sharc_write(sharc_fd,(ptroff2-INTRAM),&ptr2,1);
  rc=s3100_sharc_write(sharc_fd,(ptroff3-INTRAM),&ptr3,1);
  rc=s3100_sharc_write(sharc_fd,(ptroff4-INTRAM),&ptr4,1);

  fclose(lookupfile);

#ifdef whatever
  run_ptr       = ctrl_ptr + RUN_OFF;
  mode_ptr      = ctrl_ptr + MODE_OFF;
  save_time_ptr = ctrl_ptr + SAVE_TIME_OFF;
  new_run_ptr   = ctrl_ptr + NEW_RUN_OFF;
  if (traceflag) TRACE("run_ptr: %ld\n",*run_ptr);
#endif
}

/*===========================================================================*/
/* Reset Scaler          					  	     */
/*===========================================================================*/
int reset_sis3801(void) {
  int rcsum=0;
  /* KEY Reset */
  rcsum+=vme_A32D32_write(vme_fd,sis3801_base+SIS3801_KEYRESET,SIS3801_KEYRESET);
  return rcsum;
}

/*===========================================================================*/
/* Reset Latch          					  	     */
/*===========================================================================*/
int reset_sis3600(void) {
  int rcsum=0;

  /* KEY Reset */
  rcsum+=vme_A32D32_write(vme_fd,sis3600_base+SIS3600_KEYRESET,SIS3600_KEYRESET);
  return rcsum;
}

int read_rings(void) {
  int rcsum,i;
  FILE * fp;
  u_int32_t ringval[4096];

  rcsum = s3100_sharc_read(sharc_fd,(lokMRng-INTRAM), ringval, 4096);
  if ((fp = fopen("mring.dat","w")) == NULL) return -1;
  for (i=0;i<4096;i++) fprintf(fp,"%4d\n",ringval[i]);
  fclose(fp);

  rcsum = s3100_sharc_read(sharc_fd,(lokDRng-INTRAM), ringval, 4096);
  if ((fp = fopen("dring.dat","w")) == NULL) return -1;
  for (i=0;i<4096;i++) fprintf(fp,"%4d\n",ringval[i]);
  fclose(fp);

  return 0;
}

void dmp_regs(void) {
  u_int32_t addr, val[24], errf;
  int i, rc;

  //addr = INTRAM + 0x2000;
  addr = 0x2000;
  rc = s3100_sharc_read(sharc_fd,addr,val,24);

  
  //  printf("NOFIRQ   = 0x%08x %u\n",val[2],val[2]);
  //  printf("MONOFF   = 0x%08x %u\n",val[5],val[5]);
  //  printf("DETOFF   = 0x%08x %u\n",val[5],val[6]);
  
  //  printf("LATCHRAW = 0x%08x %u\n",val[12],val[12]);
  //  printf("LATCHVAL = 0x%08x %u\n",val[13],val[13]);
  //  printf("LATCHMSK = 0x%08x %u\n",val[14],val[14]);
  for (i=0;i<24;i++) {
    addr += 4;
    printf("INTRAM + 0x%03x *(0x%08x) = 0x%08x %d\n",0x800 + i,addr,val[i],val[i]);
  }

  //  addr = INTRAM + 0x20C0;
  addr = 0x20C0;
  rc = s3100_sharc_read(sharc_fd,addr,&errf,1);
  printf("ERRORFLG = 0x%08x %u\n",errf,errf);
}

void dmp_look(void) {
  u_int32_t addr, val[4096];
  int rc, i;
  FILE *fp;
  rc = s3100_sharc_read(sharc_fd,(lokBase-INTRAM),val,4096);
  fp = fopen("dsplook.dat","w");
  for (i=0;i<4096;i++) {
    fprintf(fp," %x \n",val[i]);
  }
  fclose(fp);
  
}
