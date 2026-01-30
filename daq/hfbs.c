/*===========================================================================*/
/*                                                                           */
/* File:             hfbs.c (HFBS code)                                      */
/*                                                                           */
/* OS:               LINUX (Kernel >= 2.4.4                                  */
/*                                                                           */
/* Description:      NIST HFBS readout code                                  */
/*                                                                           */
/* Version:          1.0                                                     */
/*                                                                           */
/*                                                                           */
/* Generated:        18.12.01                                                */
/*                                                                           */
/* Author:           MKI (Dr. Matthias Kirsch)                               */
/*                   based on hfbsdaq.c                                      */
/*                                                                           */
/* Last Change:      13.08.03 MKI                                            */
/* Last Change:      14.08.03 MKI     Installation                           */
/*---------------------------------------------------------------------------*/
/* SIS GmbH                                                                  */
/* Harksheider Str. 102A                                                     */
/* 22399 Hamburg                                                             */
/*                                                                           */
/* http://www.struck.de                                                      */
/*                                                                           */
/*===========================================================================*/

#define _GNU_SOURCE

#define memBase 0x00000000	/* base of DSP BRAM     */
#define ptroff1 0x81202080 	/* Pointer offset 1     */
#define ptroff2 0x81202084	/* Pointer offset 2     */
#define ptroff3 0x81202088	/* Pointer offset 3     */
#define ptroff4 0x8120208C      /* Pointer offset 4     */
#define lokBase 0x81204000	/* INTRAM+0x1000 on     */
#define lokEner 0x81210000      /* INTRAM+0x4000 on     */
#define lokTime 0x81240000      /* Time of flight lookup table (INTRAM+0x34000) */
#define lokMon  0x81250000      /* Monitor  write ptr offset  */
#define lokDet  0x81290000      /* Detector write ptr offset  */

#define hisBase 0x00080000	/* base of 1 MB BRAM on VSB   */
#define ctrlreg 0x00800000	/* DSP ctrl reg on VSB        */
#define irqctr  0x81202008	/* irq counter in BRAM on VSB */

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

#define OP_CMD         1
#define OP_ACK         2
#define OP_DAT         3
#define OP_ERR         4

#define ELEMENTS     4096*30


/*===========================================================================*/
/* Headers								     */
/*===========================================================================*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>      
#include <arpa/inet.h>
#include <netdb.h>

#include "header/ctrl.h"


/* sis1100/3100 PCI to VME specific */   
#include "../V1.0/linux-gnu/dev/pci/sis1100_var.h"
#include "../V1.0/sis3100_calls/sis3100_vme_calls.h" 
/* SIS9200 DSP */
#include "header/sis9200.h"
    
/* front end modules */
#include "header/sis3600.h"	      
#include "header/sis3801.h"
#include "header/str7137.h"
#include "header/str7201.h"
#include "header/vsc16.h"
#include "header/vadm.h"
/*  misc */
#include "header/rss.h"


/*===========================================================================*/
/* Prototypes					  			     */
/*===========================================================================*/
int init_sis3600(void);
int init_sis3801(void);
int init_vadm(void);
int reset_dsp(void);
int load_dsp(void);
int load_lookup(char* load_path,int load_len,unsigned int load_base);
int read_rss_conf(char* conf_path);

int init_vsc16(void);
int start_vsc16(void);
int stop_vsc16(void);
int read_vsc16(void);
int enc_omega(void);

void initrun(void);
void loadlook(void);
void buildtlook(void);
void enc_lookup(void);
void filesave(void);
void decode_command(int opcode,int prm,int val);
void dontblock(void);
void send_histos(void);
void send_scalers(void);
int send_vec(int blocknum, int blocksize);
int recv_pkg(int * len, int * opcode, int * prm, int *val);

/*===========================================================================*/
/* Resource files                                                            */
/*===========================================================================*/
char * pbinfile    = "hfbsasm/pstbin.ldr";
char * tbinfile    = "hfbsasm/timbin.ldr";
char * vbinfile    = "hfbsasm/encbin.ldr";
char * pstlook     = "pstlook.tbl";
char * enclook     = "enclook.tbl";
char * monlook     = "monlook.tbl";
char * detlook     = "detlook.tbl";
char * pointerlook = "pointer.tbl";
char line[255];

/*===========================================================================*/
/* Global variables                                                          */
/*===========================================================================*/
int p,p_sharc,p_sdram;
u_int32_t sis3600_base=SIS3600_BASE;
u_int32_t sis3801_base=SIS3801_BASE;

long *rlptr,*wlptr;    
int oldrun,histo,timeout;
time_t t1,t2,t12;
float omega; /* Angular velocity of Doppler Drive */ 

/* socket stuff */
struct sockaddr_in serv_addr, cli_addr;
int sockfd, newsock, n, clilen = sizeof(cli_addr);
long mesg[MAXMSG+PKGMIN];  

int mode;
int run;
int nofirq;
int new_run;
int save_time;
int test_pulse;
int nchans[3] = {4096,4096,4096};
int ndets[3]  = {  20,  20,  20};
int scalers[16];
int alarm_id=-1;

unsigned long *data_ptr;
unsigned long *save_ptr;
unsigned long opcode,errmesg,errcode;
unsigned long *ctrl_ptr,*look_ptr;
unsigned long *run_ptr,*new_run_ptr,*mode_ptr,*save_time_ptr;
unsigned long *rptr,*wptr,*mem;
unsigned long *timlook;
int timbin = 30;
u_int32_t empty_array[ELEMENTS];

/*===========================================================================*/
/* Main      					  	                     */
/*===========================================================================*/
int main(int argc, char* argv[]) {
int rc;
u_int32_t blt_data[2];
u_int32_t readadd;
u_int32_t readdatum;
int histo,bin;
int repeat;
int i;

/* temp xxxx */
 mode=1;
 test_pulse=0;


/*
   if ((rc=read_rss_conf(CONFPATH)) != SUCCESS) {
     printf("error on reading configuration file\n");
     exit(-1);
   }
*/
   /* clear empty array */
   for(i=0;i<ELEMENTS;i++) {
     empty_array[i]=0;
   }

   /* open VME */ 
   if ((p=open("/tmp/sis1100", O_RDWR, 0))<0) {
     printf("error on opening VME environment\n");
     return -1;
   }
   /* open SHARC */
   if ((p_sharc=open("/tmp/sis3100sharc", O_RDWR, 0))<0) {
     printf("error on sharc open");
     return -1;
   }
   /* open SDRAM */
   if ((p_sdram=open("/tmp/sis3100sdram", O_RDWR, 0))<0) {
     printf("error on sdram open");
     return -1;
   }

   if ((rc=reset_dsp()) != SUCCESS) {
     printf("error on resetting DSP\n");
     exit(-1);
   }
/*
   if ((rc=load_lookup(beampath,BEAMLEN,BEAMBASE)) != SUCCESS) {
     printf("error on loading beam lookup table\n");
     exit(-1);
   }
   printf("... loaded\n");

   if ((rc=load_lookup(energypath,ENERGYLEN,ENERGYBASE)) != SUCCESS) {
     printf("error on loading energy lookup table\n");
     exit(-1);
   }
   printf("... loaded\n");
*/
   /* Latch KEY Reset */
   vme_A32D32_write(p,sis3600_base+SIS3600_KEYRESET,SIS3600_KEYRESET);

   /* initialize SIS3801 MCS */
   if ((rc=init_sis3801()) != SUCCESS) {
     printf("error on initializing SIS3801 scaler\n");
     return -1;
   }
   vme_A32D32_read(p,sis3801_base,&readdatum);
   printf("3801 CTRL: %8.8x \n",readdatum);


   loadlook();
   buildtlook();

   /* initialize VSC16 scaler */

   if ((rc=init_vsc16()) != SUCCESS) {
     printf("error on initializing VSC16 scaler, error code sum: %lx\n",rc);
     return -1;
   }

   printf("run encoder omega\n");
   enc_omega();


   if ((rc=load_dsp()) != SUCCESS) {
     printf("error on loading DSP code\n");
     exit(-1);
   }
   /* initialize SIS3600 latch */
   if ((rc=init_sis3600()) != SUCCESS) {
     printf("error on initializing SIS3600 latch\n");
     return -1;
   }

/*
   writeadd=0;
   blt_data[0]=0;
   blt_data[1]=0;

   for (histo=0;histo<24;histo++) {
       for (bin=0;bin<4096;bin++) {
          rc = s3100_sdram_write(p_sdram, writeadd, blt_data, 1) ;
	  writeadd+=4;
       }
   }
*/
         readadd=0;
   for (repeat=0;repeat<10000;repeat++) {

          rc = s3100_sharc_read(p_sharc, irqctr, blt_data, 1) ;
          printf("IRQ counter: 0x%lx:\n",blt_data[0]);
          rc = s3100_sdram_read(p_sdram, readadd+4, blt_data, 1) ;
          printf("base+1 (error): 0x%lx:\n",blt_data[0]);
          rc = s3100_sdram_read(p_sdram, readadd+8, blt_data, 1) ;
          printf("base+2 (error): 0x%lx:\n",blt_data[0]);
          rc = s3100_sdram_read(p_sdram, readadd+12, blt_data, 1) ;
          printf("base+3 (Latch FIFO Flags): 0x%lx:\n",blt_data[0]);
          rc = s3100_sdram_read(p_sdram, readadd+16, blt_data, 1) ;
          printf("base+4: 0x%lx:\n",blt_data[0]);
          sleep(1);
   }

#ifdef OUTPUT
   /* run control/display */
  printf("histo readout\n");
   for (repeat=0;repeat<10;repeat++) {
   printf("repeat loop: %d\n",repeat);
   readadd=0;
   for (histo=0;histo<24;histo++) {
       for (bin=0;bin<4096;bin++) {
          rc = s3100_sdram_read(p_sdram, readadd, blt_data, 1) ;
          if (rc!=1) {
              /* printf("sdram read ok\n"); */
          }
          else {
                printf("error on SDRAM read %08x\n",rc);
          }
          readadd+=4;
          if (blt_data[0]!=0) {
	      printf("histo: %d bin: %d datum: %d\n",histo,bin, blt_data[0]); 
	  }
       }
   }
   }
#endif

   /* close VME environment */
   close(p);
   /* close DSP */
   close(p_sharc);
   /* close SDRAM */
   close(p_sdram);
   return 0;
}
/*===========================================================================*/
/* Read Configuration File   					  	     */
/*===========================================================================*/
int read_rss_conf(char* conf_path){
    int rc=0;
    char line_in[128];
    char *buffer;
    char *rest;
    FILE *handle;
        printf("Read configuration file: %s \n",conf_path);	
        if ((handle = fopen(conf_path,"r"))==NULL) {
	     printf("file open error: %s\n",conf_path);
             return -1;
	}

	 while (fgets(line_in,128,handle)!=NULL) {
	      if ( line_in[0] != '#')
		  { 
		    if (buffer = (char *)strtok(line_in,">"))
			   rest = (char *)strtok(NULL,">");
			if (rest)  /* not an array */
			{
			    if (!strcmp(buffer, "SIS3801BASE")) {
			       sis3801_base = strtoul(rest,NULL,16);
			       printf("SIS3801 BASE: 0x%08X\n",sis3801_base);
			    }
			    if (!strcmp(buffer, "SCALERCHANNELS")) {
			       scaler_channels = atoi(rest);
			       printf("SCALER channels: %d\n",scaler_channels);
			    }
			    if (!strcmp(buffer, "SIS3600BASE")) {
			       sis3600_base  = strtol(rest,NULL,16);
			       printf("SIS3600 BASE: 0x%08X\n",sis3600_base);
			    }
			    if (!strcmp(buffer, "SIS3300BASE")) {
			       sis3300_base  = strtol(rest,NULL,16);
			       printf("SIS3300 BASE: 0x%08X\n",sis3300_base);
			    }
                            if (!strcmp(buffer, "PERIOD")) {
                               period = atoi(rest);
 			       printf("PERIOD: %d\n",period);
			    }
                            if (!strcmp(buffer, "DSPPATH")) {
                               sscanf(rest,"%s",dsppath);
 			       printf("DSP path: %s\n",dsppath);
			    }
                            if (!strcmp(buffer, "BEAMLOOKUP")) {
                               sscanf(rest,"%s",beampath);
			    }
                            if (!strcmp(buffer, "ENERGYLOOKUP")) {
                               sscanf(rest,"%s",energypath);
			    }
			}  	   
			else 	   /* is an array */
			{
			   /* not used for the time being */
			}  
			
		  }
	   }
	   fclose(handle);
	   return rc;
}
/*===========================================================================*/
/* Load DSP     					  		     */
/*===========================================================================*/
int load_dsp(void){
    char* dsppath;
    int rc=0,clearrc,retcode=1;
    int count=0,loadcount=0;
    int offset;
    int currentaddress ;
    char line_in[128];
    FILE *loaderfile;
    unsigned int tempword[0x10000];
    u_int32_t data ;
    u_int32_t addr ;

    switch(mode){
    case 0: dsppath = pbinfile; break;
    case 1: dsppath = tbinfile; break;
    case 2: dsppath = vbinfile; break;
    default:
      fprintf(stderr,"Unsupported mode:%d\n",mode);
      fprintf(stderr,"Supported modes: 0 (pstbin), 1 (timebin), 2 (velbin)\n");
      exit(-1);
    }
    loaderfile=fopen(dsppath,"r");
    if (loaderfile>0) {
       printf("loader file %s opened\n",dsppath);
       while (retcode>0) {
          tempword[count]= strtoul(line_in,NULL,16); 
	  retcode=fscanf(loaderfile,"0x%4x\n",&tempword[count]); 
          if (count<0x10000) {
             count++;
	  }
          else {
	     printf("load file size too big\n");
             return -1;
	  }
      }
      printf("load file length: %d\n",count);
    
    }
    else {
      printf("loader file %s not found\n",dsppath);
      return -1;
    }
    fclose(loaderfile);

    offset =  0x00000300 ;
    printf("loading SHARC DSP\n");

    currentaddress=SHARCRAM;
    while (loadcount<count) {  
       addr = D48REG;
       data = tempword[loadcount];

       if ((retcode = s3100_sharc_write(p_sharc, addr, &data, 0x1))!=WORDWRITTEN) { 
	   printf("s3100_sharc_write:   retcode = 0x%08x\n", retcode ); 
           return -1;
       }
       loadcount++;

       addr = currentaddress;
       data = ((tempword[loadcount+1] << 16 ) & 0xFFFF0000) + (tempword[loadcount] & 0x0000FFFF);

       if ((retcode = s3100_sharc_write(p_sharc, addr, &data, 0x1)) != WORDWRITTEN) {
          printf("s3100_sharc_write:   retcode = 0x%08x\n", retcode );
          return -1;
       }         
       currentaddress+=4;
       loadcount+=2;
    }

    clearrc = s3100_sdram_read(p_sdram, hisBase, empty_array, ELEMENTS) ;
    if(clearrc!=ELEMENTS*4) {
        printf("error on clearing SDRAM\n");
        exit(-1);
    }


    /* start SHARC */
    printf("starting SHARC DSP\n");
    if ((retcode = s3100_control_write(p, offset, 0x0100)) != SUCCESS) {    
       printf("s3100_control_write:   retcode = 0x%08x\n", retcode );
       return -1;
    }       
 return rc;
}
/*==========================================================================*/
/* Initialize VSC16 scaler      					    */
/*==========================================================================*/
int init_vsc16() {
  int rcsum=0;
  int i;
  printf("init VSC16\n");
  /* reset module */
  rcsum+=vme_A32D16_write(p,VSC_BASE + VSC_RESET,1);
  rcsum+=vme_A32D16_write(p,VSC_BASE + VSC_CONTROL,0);
  /* clear scaler array */
  for (i=0;i<16;i++) scalers[i] = 0;
  return rcsum;
}

/*==========================================================================*/
/* Start VSC16 scaler      					            */
/*==========================================================================*/
int start_vsc16(void) {
  int rcsum=0;
  /* preset channel 1 to 50 M counts */
  rcsum+=vme_A32D16_write(p,VSC_BASE + VSC_DIRECT,1);
  rcsum+=vme_A32D32_write(p,VSC_BASE + 0xC0,50000000);
  rcsum+=vme_A32D16_write(p,VSC_BASE + VSC_CONTROL,1);
  rcsum+=vme_A32D16_write(p,VSC_BASE + VSC_IRQMSK,1);
 return rcsum;
}

/*==========================================================================*/
/* Stop VSC16 scaler          					            */
/*==========================================================================*/
int stop_vsc16(void) {
  int rcsum=0;
  rcsum+=vme_A32D16_write(p,VSC_BASE + VSC_CONTROL,0);
  return rcsum;
}

/*==========================================================================*/
/* Read VSC16 scaler      					            */
/*==========================================================================*/
int read_vsc16(void) {
  int rcsum=0;
  int i;
  printf("Counter values:");
  for (i=0;i<16;i++) {
      rcsum+=vme_A32D32_read(p,VSC_BASE + VSC_DATA(i),&scalers[i] );
      if ((i%4) == 0) printf("\n");
      printf(" %15d",scalers[i]);
  }
  printf("\n");
  return rcsum;
}

/*==========================================================================*/
/* Determine the angular velocity of doppler drive                          */
/*==========================================================================*/

int enc_omega(void) {
  int rcsum=0;
  float scalefactor;
  int ival2;
  int duration;
  int rc=0;
  u_int16_t rdatum;
  int arm;

  printf("Measuring Doppler Shaft angular velocity\n");
  scalefactor = ( 2 * 3.141592654 ) / 4096.0;
  duration = 5;

  rcsum+=init_vsc16();
  rcsum+=start_vsc16();

  printf("polling for armed flag to clear\n");
  arm=1;
  while((arm==1)&&(rc==0)) {
    rc=vme_A32D16_read(p,VSC_BASE + VSC_CONTROL,&rdatum);
    arm=rdatum&1;
  }

  rcsum+=read_vsc16();
  ival2 = scalers[15]; /* Choose last scaler as position counter */

  omega = scalefactor * (((float) ival2) / duration);
  printf("  Accumulated %d counts in %d seconds\n",ival2,duration);
  printf("  Angular velocity = %f radians/sec\n",omega);
  return rcsum;
}

/*===========================================================================*/
/* Initialize Latch      					  	     */
/*===========================================================================*/
int init_sis3600(void) {
  int rcsum=0;

  period=199;

  /* KEY Reset */
  rcsum+=vme_A32D32_write(p,sis3600_base+SIS3600_KEYRESET,SIS3600_KEYRESET);
  /* pipeline mode on */
  rcsum+=vme_A32D32_write(p,sis3600_base+SIS3600_CTRL,SIS3600_CTRL_PIPEON);
  /* preset frequency register */
  rcsum+=vme_A32D32_write(p,sis3600_base+SIS3600_FREQUENCY,period);
  /* output mode 3 , all outputs to pulser */
  rcsum+=vme_A32D32_write(p,sis3600_base+SIS3600_CTRL,
                          SIS3600_CTRL_OUTMODE0ON+SIS3600_CTRL_OUTMODE1ON);
  /* enable external next*/
  rcsum+=vme_A32D32_write(p,sis3600_base+SIS3600_CTRL,SIS3600_CTRL_EXTLNEON);
  /* enable next logic */
  rcsum+=vme_A32D32_write(p,sis3600_base+SIS3600_ENLNE,SIS3600_ENLNE);
  /* switch on user LED */
  rcsum+=vme_A32D32_write(p,sis3600_base+SIS3600_CTRL,SIS3600_CTRL_LEDON);
  /* enable pulser -> start acquisition, moved to DSP code or other place */
  rcsum+=vme_A32D32_write(p,sis3600_base+SIS3600_CTRL,SIS3600_CTRL_PULSERON); 
  return rcsum;
}

/*===========================================================================*/
/* Initialize Scaler          					  	     */
/*===========================================================================*/
int init_sis3801(void) {
  int rcsum=0;
  /* KEY Reset */
  rcsum+=vme_A32D32_write(p,sis3801_base+SIS3801_KEYRESET,SIS3801_KEYRESET);
  /* copy disable */
  if (mode==2) {
     printf("19 scaler channels\n");
     rcsum+=vme_A32D32_write(p,sis3801_base+SIS3801_COPY,0xFFF80000);
  }
  else {
     printf("18 scaler channels\n");
     rcsum+=vme_A32D32_write(p,sis3801_base+SIS3801_COPY,0xFFFC0000);
  }
  /* enable external LNE*/
  rcsum+=vme_A32D32_write(p,sis3801_base+SIS3801_CTRL,SIS3801_CTRL_EXTLNEON);
  /* enable LNE logic */
  rcsum+=vme_A32D32_write(p,sis3801_base+SIS3801_ENLNE,SIS3801_ENLNE);
  if (test_pulse==1) {
    /* enable 25 MHz test pulses */
    printf("test pulses on\n");
    rcsum+=vme_A32D32_write(p,sis3801_base+SIS3801_CTRL,0x30); 
  }
  else {
    /* disable 25 MHz test pulses */
    printf("test pulses off\n");
    rcsum+=vme_A32D32_write(p,sis3801_base+SIS3801_CTRL,0x3000); 
  }
  /* switch on user LED */
  rcsum+=vme_A32D32_write(p,sis3801_base+SIS3801_CTRL,SIS3801_CTRL_LEDON);
  return rcsum;
}

/*===========================================================================*/
/* lookup table load routine    			     		     */
/*===========================================================================*/
int load_lookup(char* load_path,int load_len,unsigned int load_base) {
    int rc=0,retcode;
    int loop;
    unsigned int inword;
    FILE *handle;


    if ((handle = fopen(load_path,"r"))==NULL) {
	printf("file open error: %s\n",conf_path);
        return -1;
    }

    printf("load lookup, file: %s, len: %d, base: 0x%08x, ",
             load_path, load_len, load_base);
    for (loop=0;loop<load_len;loop++) {
	if(fscanf(handle,"%08x\n",&inword)!=1){
	    printf("error on line: %d\n",loop);
            return -1;
        }

        /* store datum to table */
        if ((retcode = s3100_sharc_write(p_sharc, load_base, &inword, 0x1))!=WORDWRITTEN) { 
	   printf("s3100_sharc_write:   retcode = 0x%08x\n", retcode ); 
           return -1;
        }

        load_base+=4;
    }
    fclose (handle);
    return rc;
}

/*===========================================================================*/
/* reset DSP   			     		                             */
/*===========================================================================*/
int reset_dsp(void) {
    int rc=0,retcode;
    int offset;
    /* Reset SHARC */
    printf("resetting SHARC DSP\n");
    offset =  0x00000300 ;
    if ((retcode = s3100_control_write(p, offset, 0x00000800)) != SUCCESS) {
	printf("s3100_control_write:   retcode = 0x%08x\n", retcode ); 
        return -1;
    } 
    if ((retcode = s3100_control_write(p, offset, 0x01000000)) != SUCCESS) {
	printf("s3100_control_write:   retcode = 0x%08x\n", retcode ); 
        return -1;
    } 
   return rc;
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
  printf("Building Time lookup table: timbin = %d usec.\n",timbin);
  sharc_address = lokTime;
  for (i=0;i<NTIMLOOK;i++) {
    val = (i * 30) / timbin;
    if (val > 4095) val = 4095;
    rc=s3100_sharc_write(p_sharc,sharc_address+=4,&val,1); 
    if (rc!=4) {
        printf("error on downloading time lookup table\n");
	exit(-1);
    }
  }
}

/*===========================================================================*/
/* filesave                                                                  */
/*===========================================================================*/
void filesave(void) {
  FILE *datafile;
  unsigned long *fptr;
    
  /* set pointer to begin of memory module base */
  fptr=save_ptr+1;
  printf("Write data to file...");
  fflush(stdout);
  if ((datafile = fopen("/h0/nist/data/actrun.dat", "w")) == NULL) {
    fprintf(stderr,"***Unable to open file***\n");
    return;
  }
  if (fwrite(fptr,sizeof(unsigned long),ndets[1]*nchans[1],datafile)
      != (ndets[1]*nchans[1])) {
    fprintf(stderr,"***Write attempt unsuccessful***\n");
  } else {
    printf("data written\n");
  }
    
  fclose(datafile);
}

/*===========================================================================*/
/* decode command                                                            */
/*===========================================================================*/
void decode_command(int opcode,int prm,int val) {	     
  int i,k;

  if (opcode==COMMAND) {
    histo=0;
    switch (prm) {
    case DAQRUN:
      if (val==2) {
	run=1;
	new_run=0;
      } else {
	run=val;
	new_run=1;  
      }
      if (run==oldrun) {
	errcode=-1;
	errmesg=-1;
	opcode=4;
      } else {
	errcode=0;
	errmesg=0;
	opcode=2;
	oldrun=run;

	if (run) {
	  initrun();
	} else {
	  /* set DSP to reset state */
	  printf("Set DSP to reset state\n");
	  time(&t2);
	  t12 = t2 - t1;

	  for(i=0;i<2;i++){
	    mem = (unsigned long *) (memBase+ctrlreg);
	    *mem=RESET;

	    /* reset latch and scaler */
	    /* latch key reset */
	    wptr=(unsigned long*)(LATCHBASE+S7137_KEY_RESET);
	    *wptr=1; 

	    /* scaler key reset */
	    wptr=(unsigned long*)(SCALERBASE+S7201_KEY_RESET);
	    *wptr=1;
	  }
	  stop_vsc16();
	  /* Read final histogram from BRAM at this point */
	  rptr=(unsigned long*)hisBase;
	  wptr=save_ptr+1;
	  printf("Final histo transfer to: %08lx\n",wptr);
	  for (i=0;i<ndets[mode];i++) {
	    for (k=0;k<nchans[mode];k++) {
	      *wptr++=*rptr++;  
	      /* tsleep here to keep VSB access to DSP at
		 moderate level */
	      /* tsleep(1); */
	    }
	  }
	  /*
	   * Omit saving data to file 
	   * filesave(); 
	   */
	  read_vsc16();
	  printf("Total collection time (secs): %ld\n",(long)(t2 - t1));
	  
	}
      } 
      break;
    case MODE:
      if ((val >= 0) && (val <= 2)) {
	if(run==0) {
	  mode=val;
	  switch(mode){
	  case 0: printf("Mode = pstbin.\n"); timbin=30; break;
	  case 1: printf("Mode = timbin.\n");            break;
	  case 2: printf("Mode = velbin.\n"); 
	    timbin=30; 
	    enc_omega(); /* Measure angular velocity prior to run */
	    enc_lookup();
	    break;
	  }
	  opcode=2;
	  errcode=0;
	  errmesg=0;
	} else {
	  opcode=4;
	  errcode=-1;
	  errmesg=-1;
	}
      } else {
	opcode=4;
	errcode=-1;
	errmesg=-2;
      }
      break;
    case SAVE_TIME:
      if (val<500000) {
	val=500000;
	opcode=4;
	errcode=-1;
	errmesg=-2;
      } else {
	save_time=val;
	opcode=2;
	errcode=0;
	errmesg=0;
      }
      break;
    case TEST_PULSE:
      if (run == 1) {
	opcode = 4;
	errcode=-1;
	errcode=-1;
	break;
      }
      if (val == 1) {
	test_pulse = 1;
      } else {
	test_pulse = 0;
      }
      opcode=2;
      errcode=0;
      errmesg=0;
      break;
    case GET_TIMER:
      opcode = 3;
      errcode = 0;
      errmesg = (*(unsigned long*)irqctr)*timbin; /* Send # usecs */
      break;
    case LOAD_LOOKUP: 
      loadlook();
      buildtlook();
      opcode=2;
      errcode=0;
      errmesg=0;
      break;      
    case GET_TIMBIN:
      timbin = val;
      buildtlook();
      opcode=2;
      errcode=0;
      errmesg=0;
      break;
    case SEND_SCALERS: 
      send_scalers();
      break;
    case SEND_HISTOS: 
      if (run) {
	time(&t2);
	printf("Total collection time (secs): %ld\n",(long)(t2 - t1));      
      }
      send_histos();
      histo=1;
      break;
    default:
      opcode=4;
      errcode=-1;
      errmesg=0;
      break;
    }
    /* Send response package */
    if (histo!=1) {
      mesg[0]=ERROR_LEN;
      mesg[1]=opcode;
      mesg[2]=errcode;
      mesg[3]=errmesg;
      mesg[4]=EOS;
      if (write(newsock,mesg,ERROR_LEN) != ERROR_LEN) {     
	printf("serv:write");
	exit(-1);
      } 
    }
  }
}
/**********************************************/
/* Send histograms to LINUX host upon request */
/**********************************************/
void send_scalers(void) {
  int blocknum, blocksize, retn;
  int len, opcode, prm, val,n;
  int written;
  
  blocknum=0;
  blocksize=16;
  
  printf("Sending scaler values.\n");

  mesg[0] = (blocksize + 2) * sizeof(long);
  mesg[1] = OP_DAT;
  mesg[2] = blocknum;
  memcpy((mesg + 3),&scalers[0],blocksize * sizeof(long));
  n = mesg[0] + sizeof(long);  
  written=write(newsock,mesg,n);
  if (written!=n) {
    fprintf(stderr,"serv:write");
    fprintf(stderr,"bytes written: %d\n",written);
    exit(-1);
  } 

  retn = recv_pkg(&len,&opcode,&prm,&val);
  if ((retn < 0) || (opcode != OP_ACK)) { /* An error occurred */
    printf("**ERROR**\n");
    return;
  }

  /*
   * Send an acknowledge packet to indicate that 
   * we've transferred the whole batch
   */
  
  mesg[0]=ERROR_LEN - sizeof(long);
  mesg[1]=OP_ACK;
  mesg[2]=0;
  mesg[3]=0;
  mesg[4]=EOS;
  if (write(newsock,mesg,ERROR_LEN) != ERROR_LEN) {     
    printf("serv:write");
    exit(-1);
  } 
}

/**********************************************/
/* Send histograms to LINUX host upon request */
/**********************************************/
void send_histos(void) {
  int blocknum, retn;
  int len, opcode, prm, val;
  int nblocks;
  
  blocknum=0;
  nblocks = (nchans[mode] / BLKSIZE) * ndets[mode];
  
  printf("Sending histogram [%d blocks of size %d]:",nblocks,BLKSIZE);
  while(blocknum < nblocks ){
    /* printf("..%d",blocknum);fflush(stdout); */
    putchar('.'); fflush(stdout);
    send_vec(blocknum,BLKSIZE);
    retn = recv_pkg(&len,&opcode,&prm,&val);
    if ((retn < 0) || (opcode != OP_ACK)) { /* An error occurred */
      printf("**ERROR**\n");
      return;
    }
    blocknum = prm + 1;
  }
  printf("\n");
  /*
   * Send an acknowledge packet to indicate that 
   * we've transferred the entire histogram.
   */
  
  mesg[0]=ERROR_LEN - sizeof(long);
  mesg[1]=OP_ACK;
  mesg[2]=0;
  mesg[3]=0;
  mesg[4]=EOS;
  if (write(newsock,mesg,ERROR_LEN) != ERROR_LEN) {     
    printf("serv:write");
    exit(-1);
  } 
}

/*******************************************************/
/* Set socket option to make server socket nonblocking */
/*******************************************************/
void dontblock(void) {
/*
  struct sgbuf buf;
  _gs_opt(sockfd,&buf);
  buf.sg_noblock=1;
  _ss_opt(sockfd,&buf);
*/
}


/*******************************************************/
/* send vector                                         */
/*******************************************************/
int send_vec(int blocknum, int blocksize) {
  unsigned long * hptr;
  int written;

  mesg[0] = (blocksize + 2) * sizeof(long);
  mesg[1] = OP_DAT;
  mesg[2] = blocknum;
  hptr = (save_ptr + 1) + blocknum * BLKSIZE;
  memcpy((mesg + 3),hptr,BLKSIZE * sizeof(long));
  n = mesg[0] + sizeof(long);  
  written=write(newsock,mesg,n);
  if (written!=n) {
    fprintf(stderr,"serv:write");
    fprintf(stderr,"bytes written: %d\n",written);
    exit(-1);
  } 
  return 0;
}

/*******************************************************/
/* receive package */
/*******************************************************/
int recv_pkg(int * len, int * opcode, int * prm, int *val)

{
  int retn;

  /* I think this is where we get stuck */

  n=-1;
  timeout=0;
  /* alm_set(257,5000); */
  while(n<0){
    n = read(newsock,mesg,sizeof(mesg));
  }

  if (!timeout) {
    /* if (alarm_id >= 0) alm_delete(alarm_id);*/
  } else {
    alarm_id = -1;
    printf("\n**TCP timeout**\n");
    return -1; /* Timeout */
  }

  if (( *len    = mesg[0]) < ((PKGMIN -1 ) * sizeof(long))) {
    return -2; /* Short package */
  }

  *opcode = mesg[1];
  *prm    = mesg[2];
  *val    = mesg[3];
  if ((retn = mesg[4]) != EOS) { return -3; } /* Bad format */
  return 0;
}

/*************************************/
/* load lookup table                 */      
/*************************************/
void loadlook(void) {
  FILE *lookupfile;
  unsigned int sharc_address;
  int j, count;
  int rc;
  unsigned int ptr1,ptr2,ptr3,ptr4;


  /* Load lookup tables. Only really necessary for standard and hybrid modes */
  printf("Downloading PST lookup tables\n");
  if ((lookupfile = fopen(pstlook, "r")) == NULL) {
    fprintf(stderr,"could not open lookup file %s\n",pstlook);
    exit(-1);
  }
  sharc_address=lokBase;
  for (j=0;j<NPSTLOOK;j++) {
    count=fscanf(lookupfile,"%x",&ptr1);
    if (count!=1) {
        printf("error on reading from pstlook\n");
        fprintf(stderr,"***Error loading PST lookup table!***\n");
	exit(-1);
    }
    rc=s3100_sharc_write(p_sharc,sharc_address+=4,&ptr1,1); 
    if (rc!=4) {
        printf("error on downloading time lookup table\n");
	exit(-1);
    }
  }
  fclose(lookupfile);


  /*
  printf("Downloading velocity lookup table\n");
  if ((lookupfile = fopen(adclook, "r")) == NULL) {
    fprintf(stderr,"could not open lookup file %s\n",adclook);
    exit(-1);
  }
  wptr=(unsigned long*)lokEner;
  for (j=0;j<NADCLOOK;j++) {
    if(fgets(line,250,lookupfile) == NULL) {
      fprintf(stderr,"***Error loading velocity lookup table!***\n");
      exit(-1);
    }
    count = sscanf(line," %d ",wptr++);
  }
  fclose(lookupfile);
  */

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

  /*
  printf("Downloading detector write pointer offset lookup table\n");
  if ((lookupfile = fopen(detlook, "r")) == NULL) {
    fprintf(stderr,"could not open lookup file %s\n",adclook);
    exit(-1);
  }
  wlptr=(long*)lokDet;
  for (j=0;j<NPOFFLOOK;j++) {
    if(fgets(line,250,lookupfile) == NULL) {
      fprintf(stderr,"***Error loading detector FTOFF lookup table!***\n");
      exit(-1);
    }
    count = sscanf(line," %d ",wlptr++);
  }
  fclose(lookupfile);
  */
  
  printf("Downloading pointer positions\n");
  if ((lookupfile = fopen(pointerlook, "r")) == NULL) {
    fprintf(stderr,"could not open pointer file %s\n",pointerlook);
    exit(-1);
  }
  fscanf(lookupfile,"%d",&ptr1);
  fscanf(lookupfile,"%d",&ptr2);
  fscanf(lookupfile,"%d",&ptr3);
  fscanf(lookupfile,"%d",&ptr4);

  printf("ptr1: %d\n",ptr1);
  printf("ptr2: %d\n",ptr2);
  printf("ptr3: %d\n",ptr3);
  printf("ptr4: %d\n",ptr4);
   
  rc=s3100_sharc_write(p_sharc,ptroff1,&ptr1,1);
  rc=s3100_sharc_write(p_sharc,ptroff2,&ptr2,1);
  rc=s3100_sharc_write(p_sharc,ptroff3,&ptr3,1);
  rc=s3100_sharc_write(p_sharc,ptroff4,&ptr4,1);
  printf("rc sharcwrite: %x\n",rc);
  fclose(lookupfile);


  run_ptr       = ctrl_ptr + RUN_OFF;
  mode_ptr      = ctrl_ptr + MODE_OFF;
  save_time_ptr = ctrl_ptr + SAVE_TIME_OFF;
  new_run_ptr   = ctrl_ptr + NEW_RUN_OFF;
  printf("run_ptr: %lx\n",run_ptr);

}

/*************************************************************************/
/* Calculate flight time offset and energy lookups for encoder positions */
/*************************************************************************/
void enc_lookup(void) {
  FILE * fp;
  int i, j, count;
  long  *mptr,*dptr,*eptr;
  long  e_chan, m_offset, d_offset;

  eptr = (long *) lokTime;
  mptr = (long *) lokMon;
  dptr = (long *) lokDet;

  printf("Calculating lookup tables for encoder mode\n");
  sprintf(line,"enclook %f",omega);
  printf("Spawning: %s\n",line);
  system(line);

  printf("Reading encoder lookup table\n");
  if ((fp = fopen(enclook, "r")) == NULL) {
    fprintf(stderr,"could not open lookup file %s\n",enclook);
    exit(-1);
  }

  for (j=0;j<4096;j++) {
    if (fgets(line,250,fp) == NULL) {
      fprintf(stderr,"***Error loading encoder lookup table!***\n");
      exit(-1);
    }
    count = sscanf(line,"%d %d %d %d",&i,&e_chan,&m_offset, &d_offset);
    *eptr++ = e_chan;
    *mptr++ = m_offset;
    *dptr++ = d_offset;
    /* printf("%4d %4d %4d %4d\n",i,e_chan,m_offset,d_offset); */
  }
  fclose(fp);
  printf("Ready.\n");
}

/********************************************************************************/
/*    Init run                                                                  */
/********************************************************************************/
void initrun(void) {
  FILE * restfile;
  int j, k;

  printf("Initializing for run.\n");
  load_dsp();

  /* clear histo memory or download old histos */
  if (new_run==1) {
    printf("Clear BRAM for new run\n");
    wptr=(unsigned long*)hisBase;
    for (j=0;j<ndets[1];j++) {
      for (k=0;k<nchans[1];k++) {
	*wptr++=0;
        /* tsleep(1); */
      }
    }
  } else {
    /*
     * I don't think this will work properly -
     * need to write values read back into bram as well as into
     *  save_ptr
     */
    printf("Restoring data from disk\n");
    if ((restfile = fopen("/h0/nist/data/actrun.dat", "r")) == NULL) {
      fprintf(stderr,"could not open saved file\n");
      exit(-1);
    }
    wptr=save_ptr+1;
    if (fread(wptr,sizeof(unsigned long),ndets[1]*nchans[1],restfile) !=
	(ndets[1]*nchans[1])) {
      fprintf(stderr,"***File restore error***\n");
    } else {
      rptr = save_ptr +1;
      wptr=(unsigned long*)hisBase;
      for (j=0;j<ndets[1];j++) {
	for (k=0;k<nchans[1];k++) {
	  *wptr++=*rptr++;
	  /* tsleep(1); */
	}
      }
      printf("   Restore successful.\n");
    }
    fclose(restfile);
  }    


  /* init VME modules                                       */
  /* do not change the position of the STR7137 latch init,  */
  /* it has to be last, as it will start triggering the DAQ */
  init_vadm();                   /* initialise ADCs */
  init_vsc16();
  start_vsc16();
  init_sis3801();                /* Initialise multiscaler */
  init_sis3600();                /* Initialise latch */

}

/************************************/
/* Initialization routine for VADMs */
/************************************/
int init_vadm(void) {
  int rcsum=0;

  printf("VADM initialisation...");
  printf("VADM1...");

  /* set fast mode */
  rcsum+=vme_A32D32_write(p,VADM1BASE+DI,0);

  /* select input channel 0 */
  rcsum+=vme_A32D32_write(p,VADM1BASE+ADMUX,0x400);
  printf("VADM2...");

  /* set fast mode */
  rcsum+=vme_A32D32_write(p,VADM2BASE+DI,0);

  /* select input channel 0 */
  rcsum+=vme_A32D32_write(p,VADM2BASE+ADMUX,0x400);
  printf("VADM init completed ");
  if (rcsum!=0) {
     printf("with errors");
  }
  else {
     printf("successfully");
  }
  printf("\n");
  return rcsum;
}
