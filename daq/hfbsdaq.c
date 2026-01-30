/*
   $Id: hfbsdaq.c,v 1.2 2000/12/05 15:52:22 nickm Exp $

   Program : HFBSDAQ
   
             STRUCK, Dr. Matthias Kirsch (MKI) 1997
	     NCNR,   Dr. N. C. Maliszewskyj (NCM)
   Controls data NIST HFBS DAQ via interpretation of commands coming in 
   via socket communication from a LINUX host. 
   
   Compile with the command cdaq (using the OS-9 C-compiler) 
   on the heurikon
   
   Modifications:
   02/08/97 Installation MKI
   Aug 1997 NCM - finalized socket communication, general tidying up
   Mar 1998 NCM - added support for transferring histograms of different
                  sizes: TEACH=20x4098  STD=40x2048
   Jul 1998 NCM - Even out the timing of communications 
                    * return acknowledgement to host after init actions 
                      performed
		    * calculate timing of data collection
   Aug 1998 NCM - Add signal handling, make sockets blocking.
                  Actions to be performed asynchronously
   Sep 1998 NCM - Add means to return contents of IRQ counter as a way of
                  obtaining live time of data acquisition
                  Close zombie sockets - these led to terrible communications
                  problems.
   Nov 1998 NCM - Add infrastructure necessary to accommodate a new mode:
                      The hybrid mode is essentially standard mode except
                      binning is by PST position rather than velocity
   Feb 1999 NCM - Add support for sigma-delta ADC (ICS-110B-4B)		     
   Jun 1999 NCM - Altered infrastructure to support three major modes:
                      PST      binning 20x4096
                      time     binning 20x4096
                      velocity binning 20x4096
                  Load lookups when program starts and on demand
                  Changed frequency of heartbeat for time binning mode
   Oct 2000 NCM - Add support for Technology 80 Model 14 High Speed Encoder
   Nov 2000 NCM - Stripped out unneeded code, shunt lookup table generation
                  to an external program
*/
/*======================================================================*/
/* Standard IO */
#include <stdio.h>

/* networking/socket communication */
#include <sgstat.h>
#include <types.h>
#include <machine/types.h>
#include <time.h>
#include <inet/socket.h>
#include <inet/in.h>      
#include <inet/inet.h>
#include <inet/netdb.h>

/* module and daq specific stuff */
#include "mod_mmod.c"
#include "str7137.h"
#include "str7201.h"
#include "vsc16.h"
#include "ctrl.h"
#include "../vadm/hk4f.h"
#include "../vadm/vadm.h"

#define memBase 0x0f000000	/* base of DSP BRAM on VSB    */
#define ptroff1 0x0f002080 	/* Pointer offset 1 on VSB    */
#define ptroff2 0x0f002084	/* Pointer offset 2 on VSB    */
#define ptroff3 0x0f002088	/* Pointer offset 3 on VSB    */
#define ptroff4 0x0f00208c      /* Pointer offset 4 on VSB    */
#define lokBase 0x0f004000	/* BRAM+0x1000 on VSB         */
#define lokEner 0x0f010000      /* BRAM+0x4000 on VSB         */
#define lokTime 0x0f150000      /* Time of flight lookup table*/
#define lokMon  0x0f160000      /* Monitor  write ptr offset  */
#define lokDet  0x0f1a0000      /* Detector write ptr offset  */

#define hisBase 0x0f100000	/* base of 1 MB BRAM on VSB   */
#define ctrlreg 0x00800000	/* DSP ctrl reg on VSB        */
#define irqctr  0x0f002008	/* irq counter in BRAM on VSB */

#define RESET   0x1000
#define RUN     0x0010

#define SCALERBASE 0x43508000
#define LATCHBASE  0x43509000
#define VSC_BASE   0x43520000
#define IP_CARRIER 0xff046000   /* Industry Pack Carrier + Encoder */
#define IP_BASE    (IP_CARRIER + IP_A_IO_BASE)
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

#define SIGXFER       10
#define SIGTIME(sec) ((sec<<8) | 0x8000000)
#define SIGSECS       1
/* prototypes */
void init_str7137();
void init_str7201();
void init_vadm();
void init_vsc16();
void start_vsc16();
void stop_vsc16();
void read_vsc16();
void enc_omega();
void enc_lookup();
void initrun();
void dspload();
void loadlook();
void buildtlook();
void filesave();
void decode_command();
void phex();
int  recv_pkg();
int  send_vec();
void send_histos();
void send_scalers();
void dontblock();
void transferhist();
void initsock();

/* Resource files */
char * pbinfile     = "/h0/nist/dsp/pstbin.dmp";
char * tbinfile     = "/h0/nist/dsp/timbin.dmp";
char * vbinfile    = "/h0/nist/dsp/encbin.dmp";
char * pstlook     = "/h0/nist/dsp/pstlook.tbl";
char * enclook     = "/h0/nist/dsp/enclook.tbl";
char * monlook     = "/h0/nist/dsp/monlook.tbl";
char * detlook     = "/h0/nist/dsp/detlook.tbl";
char * pointerlook = "/h0/nist/dsp/pointer.tbl";
char line[255];

unsigned long *data_ptr;
unsigned long *save_ptr;
unsigned long opcode,errmesg,errcode;
unsigned long *ctrl_ptr,*look_ptr;
unsigned long *run_ptr,*new_run_ptr,*mode_ptr,*save_time_ptr;
unsigned long *rptr,*wptr,*mem;
unsigned long *timlook;
unsigned long timbin = 30;

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

void
sighandler(signum) 
register int signum;
{
  switch(signum) {
  case 2:
    printf("Received SIGQUIT\n");
    close(sockfd);
    exit(2);
    break;
  case 3: 
    printf("Received SIGINT...");
    close(sockfd);
    printf("tidying up and exiting.\n");
    exit(0);
    break;
  case 4:
    printf("Received SIGHUP signal\n");
    close(sockfd);
    initsock();
    break;
  case 36:
    printf("Received SIGTERM signal\n");
    close(sockfd);
    exit(36);
    break;
  case 256: 
    transferhist();
    break;
  case 257:
    timeout = 1;
    break;
  case 512:
    printf("Received signal 512. Reinitialize socket.\n");
    close(sockfd);
    initsock();
    break;
  default:
    printf("Received unsupported signal %d\n",signum);
    break;
  }
}

void
main() {
  unsigned char* wrdptr; 
  unsigned long address;
  unsigned long *memMod_Ptr;
  int error,i,j,k,l;
  int len,op_code,prm,val;
        
  FILE *restfile,*lookupfile;
  int  count;

  /* set HK4F bus release to release when done    */
  /* otherwise the DSP is blocked from VME access */
  wrdptr=(unsigned char*)0xff0000d3;
  *wrdptr=0x40;
    
  /* generate or link to memory module for histos */
  data_ptr=(unsigned long*)malloc(sizeof(unsigned long)*ndets[1]*nchans[1]*4
				  +1);
  printf("data ptr: %8x\n",data_ptr);
   
  /* default values for run control      */
  mode=1;		/* teach mode    */
  new_run=1;	        /* start new run */
  run=0;		/* not running   */     
  save_time=2000000;    /*               */
  test_pulse=0;	        /* testpulses    */
   
  /* remember start of data module */
  save_ptr=data_ptr;

  printf("Set DSP to reset state\n");
  for(i=0;i<3;i++) {
    /* reset latch and scaler */
    /* latch key reset */
    wptr  = (unsigned long*)(LATCHBASE+S7137_KEY_RESET);
    *wptr = 1; 
    /* scaler key reset */
    wptr  = (unsigned long*)(SCALERBASE+S7201_KEY_RESET);
    *wptr = 1;
    /* dsp reset */
    mem = (unsigned long *) (memBase+ctrlreg);
    *mem=RESET;   
  } 

  initsock();                    /* Initialize socket communications */
  intercept(sighandler);         /* Set up signal handler */
  loadlook();                    /* Load lookup tables into DSP */
  buildtlook();

  time(&t1);
  t2 = t1;
  t12 = 0;
  omega = 0;
  printf("------>Initialization complete <------\n");
  printf("Waiting for run start or configuration\n"); 

  /* main program loop */   
  while(1) {


    /* wait for run start */
    while(run==0) {
      if ((newsock = 
	   accept(sockfd,(struct sockaddr *)&cli_addr,&clilen)) > NULL) {
	recv_pkg(&len,&op_code,&prm,&val);
	decode_command(op_code,prm,val);
	close(newsock); 
      }
    }

    rptr=(unsigned long*)irqctr; /* set read pointer to # of heartbeats */
    *rptr=0;                     /* set heartbeats to 0 */
     
    /*************************/
    /* Start Acquiring Data  */
    /*************************/
    if (run==1) {
      printf("Set DSP to run state\n");
      mem = (unsigned long *) (memBase+ctrlreg);
      *mem=RUN;
      time(&t1);
      if (!new_run) {
	/* In the case of resuming a previously started run fix up run time */
	t1 = t1 - t12;
      }
    }

    /* acquire until stop */

    while (run==1) {
      /* check on status change */
      if ((newsock = 
	   accept(sockfd,(struct sockaddr *)&cli_addr,&clilen)) > NULL) {
	recv_pkg(&len,&op_code,&prm,&val);
	decode_command(op_code,prm,val);
	close(newsock);
      }
    }
  }
}

void
initsock() {
  /* get socket descriptor */
  if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0) {
    printf("server: socket\n");
    exit(-1);
  } 

  /* bind to socket */
  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port        = htons(SERVERPORT);
  if (bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0) {
    printf("server: bind\n");
    exit(-1);
  } 
   
  /* For the mean time, work with blocking sockets */
  /* dontblock(); */
 
  /* listen to socket */
  listen(sockfd,15); 
}

void
loadlook() {
  FILE *lookupfile;
  int i, j, k, count;

  /* Load lookup tables. Only really necessary for standard and hybrid modes */
  printf("Downloading PST lookup tables\n");
  if ((lookupfile = fopen(pstlook, "r")) == NULL) {
    fprintf(stderr,"could not open lookup file %s\n",pstlook);
    exit(-1);
  }
  wptr=(unsigned long*)lokBase;
  for (j=0;j<NPSTLOOK;j++) {
    if (fgets(line,250,lookupfile) == NULL) {
      fprintf(stderr,"***Error loading PST lookup table!***\n");
      exit(-1);
    }
    count = sscanf(line," %x ",wptr++);
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
  wptr=(unsigned long*)ptroff1;
  fgets(line,250,lookupfile);
  count = sscanf(line," %d ",wptr);
  printf("  P1 = %d\n",*wptr);
  wptr=(unsigned long*)ptroff2;
  fgets(line,250,lookupfile);
  count = sscanf(line," %d ",wptr);
  printf("  P2 = %d\n",*wptr);
  wptr=(unsigned long*)ptroff3;
  fgets(line,250,lookupfile);
  count = sscanf(line," %d ",wptr);
  printf("  P3 = %d\n",*wptr);
  wptr=(unsigned long*)ptroff4;
  fclose(lookupfile);

  run_ptr       = ctrl_ptr + RUN_OFF;
  mode_ptr      = ctrl_ptr + MODE_OFF;
  save_time_ptr = ctrl_ptr + SAVE_TIME_OFF;
  new_run_ptr   = ctrl_ptr + NEW_RUN_OFF;
  printf("run_ptr: %8x\n",(unsigned long)run_ptr);

}

/* Build time lookup table */
void buildtlook() {
  int i;
  unsigned long val;
  if (timbin < 30) timbin = 30;
  printf("Building Time lookup table: timbin = %d usec.\n",timbin);
  wptr = (unsigned long *) lokTime;
  for (i=0;i<NTIMLOOK;i++) {
    val = (i * 30) / timbin;
    if (val > 4095) val = 4095;
    *wptr++ = val;
  }
}

void initrun() {
  FILE * restfile;
  int i, j, k;

  printf("Initializing for run.\n");
  dspload();

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
  init_str7201();                /* Initialise multiscaler */
  init_str7137();                /* Initialise latch */

}

/*
 * Load ascii-formatted code to DSP. 
 * Note that errors at this level are FATAL.
 */
void 
dspload() {
  unsigned long int word1, word2, word3, word4;
  unsigned long *mem, tmp1,tmp2;
  unsigned long address;
  char * dmpfile;
  FILE * inputfile;
  int len, i, count, nerror;

  switch(mode){
  case 0: dmpfile = pbinfile; break;
  case 1: dmpfile = tbinfile; break;
  case 2: dmpfile = vbinfile; break;
  default:
    fprintf(stderr,"Unsupported mode:%d\n",mode);
    fprintf(stderr,"Supported modes: 0 (pstbin), 1 (timebin), 2 (velbin)\n");
    exit(-1);
  }

  printf("Loading DSP code from file: %s\n",dmpfile);

  if ((inputfile = fopen(dmpfile,"r")) == NULL) {
    fprintf("Can't open input file %s.\n",dmpfile); 
    exit(-2);
  }

  /* reset DSP */
  mem = (unsigned long *) (memBase+ctrlreg);
  *mem=RESET;
    
  /* Skip 2 lines of comments */
  fgets(line,250,inputfile);
  fgets(line,250,inputfile);

  address = (unsigned long)memBase;    
  printf("starting at %08X\n",address);
  mem = (unsigned long *) address;
  len=0;
  data_ptr=save_ptr;
    
  do {
    fgets(line,250,inputfile);
    count = sscanf(line,"%X %X %X %X",  &word1, &word2, &word3, &word4);

    if (count <= 0)
      continue;

    if (count>0) {
      *mem = word1;
      *data_ptr++ = word1;
      mem++;
      len++;
    }

    if (count>1) {
      *mem = word2;
      *data_ptr++ = word2;
      mem++;
      len++;
    }
    if (count>2) {
      *mem = word3;
      *data_ptr++ = word3;
      mem++;   	
      len++;
    }
    if (count>3) {
      *mem = word4;
      *data_ptr++ = word4;
      mem++;
      len++;
    }

    if (count>4)
      printf("File format error\n");
  } while (!feof(inputfile)); /* while !feof */

  fclose(inputfile);
  printf("Program length %d longwords\n",len);
  if (len>1024) {
    fprintf(stderr,"Program too long for DSP shadow memory!\n");
    exit(-1);
  }
  printf("Verify program...");
  address = (unsigned long)memBase;    
  mem = (unsigned long *) address;
  data_ptr=save_ptr;
  nerror=0;
  for (i=0;i<len;i++) {
    tmp1=*mem;
    tmp2=*data_ptr;
    if(tmp1 != tmp2) {
      printf("\npos %lx written %lx read %lx\n",i,tmp2,tmp1);
      nerror++;
    }
    mem++;
    data_ptr++;
  }
  if (nerror != 0) {
    fprintf(stderr,"verify ended with %d errors\n",nerror);
    exit(-1);
  } else {
    printf("Verify ok.\n");
  }
}

/* Initialisation routine for STR7137 latch */
void 
init_str7137() { 
  unsigned long* latptr;
  printf("Initializing latch...");
  /* ext_next enable BIT16 */
  /* pipeline mode   BIT05    */
  /* output mode 3   BIT02/03 */
  /* set user LED    BIT00    */
  latptr=(unsigned long*)(LATCHBASE+S7137_CTRL);
  *latptr=0x1002D;

  /* set output frequency */   
  latptr=(unsigned long*)(LATCHBASE+S7137_FREQ);
  *latptr=0x800000;
  tsleep(1);

  /* set output interval to "timbin" microseconds */
  *latptr = (timbin * 10) - 1;

  /* enable external next key address */
  latptr=(unsigned long*)(LATCHBASE+S7137_EN_NXT);
  *latptr=1;

  /* enable out pulse (starts to trigger DAQ) */
  latptr=(unsigned long*)(LATCHBASE+S7137_CTRL);
  *latptr=0x10;
  tsleep(1);
  printf("latch init complete\n");
}


/* Initialisation routine for STR7201 multiscaler */
void 
init_str7201() { 
  unsigned long* scalptr;
  printf("Initializing scaler...");
  /* ext_next enable BIT16    */
  /* pipeline mode   BIT05    */
  /* output mode 0   BIT02/03 */
  /* set user LED    BIT00    */
  scalptr=(unsigned long*)(SCALERBASE+S7201_CTRL);
  *scalptr=0x10001;
  /* enable channels 0-17 */   
  scalptr=(unsigned long*)(SCALERBASE+S7201_COPY);
  if (mode == 2) {
    printf("read 19 chans...");
    *scalptr=0xFFF80000; /* Read 19 channels */
  } else {
    printf("read 18 chans...");
    *scalptr=0xFFFC0000; /* Read 18 channels */
  }
  /* enable 25 MHz if testpulse set */
  scalptr=(unsigned long*)(SCALERBASE+S7201_CTRL);
  if (test_pulse==1) {
    printf("test pulsing enabled...");
    /* enable test */
    *scalptr=0x10;
    *scalptr=0x20;
  } else {   
    /* Enabled interrupt sources ? */
    *scalptr=0x1000;
    *scalptr=0x2000;
  }

  /* enable external next key address */
  scalptr=(unsigned long*)(SCALERBASE+S7201_EN_NXT);
  *scalptr=1;
  tsleep(1);
  printf("scaler init complete\n");
}

/* Initialization routine for VADMs */
void 
init_vadm() {
  unsigned short *wptr,*rptr;       /* general write and read pointers     */

  printf("VADM initialisation...");
  printf("VADM1...");

  /* set fast mode */
  wptr=(unsigned short *)(D16BASE+VADM1BASE+DI);
  *wptr=0;

  /* select input channel 0 */
  wptr=(unsigned short *)(D16BASE+VADM1BASE+ADMUX);
  /* *wptr=0x400;*/
  *wptr = 0x400;
  printf("VADM2...");

  /* set fast mode */
  wptr=(unsigned short *)(D16BASE+VADM2BASE+DI);
  *wptr=0;

  /* select input channel 0 */
  wptr=(unsigned short *)(D16BASE+VADM2BASE+ADMUX);
  /* *wptr=0x400; */
  *wptr = 0x400;
  printf("VADM init complete\n");
}

/* Determine the angular velocity of doppler drive */
void
enc_omega() {
  float scalefactor;
  int ival1,ival2;
  int duration;

  printf("Measuring Doppler Shaft angular velocity\n");
  scalefactor = ( 2 * 3.141592654 ) / 4096.0;
  duration = 5;

  init_vsc16();
  start_vsc16();
  sleep(duration);
  stop_vsc16();
  read_vsc16();
  ival2 = scalers[15]; /* Choose last scaler as position counter */

  omega = scalefactor * (((float) ival2) / duration);
  printf("  Accumulated %d counts in %d seconds\n",ival2,duration);
  printf("  Angular velocity = %f radians/sec\n",omega);

}

/* Calculate flight time offset and energy lookups for encoder positions */
void
enc_lookup() {
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

/* Routines for controlling VSC16 TTL scaler */
void
init_vsc16() {
  int i;
  /* reset module */
  *(unsigned short *) (VSC_BASE + VSC_RESET)   = 1;
  *(unsigned short *) (VSC_BASE + VSC_CONTROL) = 0;
  for (i=0;i<16;i++) scalers[i] = 0;
}

void
start_vsc16() {
  *(unsigned short *) (VSC_BASE + VSC_CONTROL) = 1;
}

void
stop_vsc16() {
  *(unsigned short *) (VSC_BASE + VSC_CONTROL) = 0;
}

void
read_vsc16() {
  unsigned int val;
  int i;
  printf("Counter values:");
  for (i=0;i<16;i++) {
    scalers[i] = *(unsigned int *) (VSC_BASE + VSC_DATA(i));
    if ((i%4) == 0) printf("\n");
    printf(" %15d",scalers[i]);
  }
  printf("\n");
}

void 
filesave() {
  FILE *datafile;
  int i,j,k;
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

void
transferhist() 
{
  int i,k;
  rptr=(unsigned long*)irqctr;
  nofirq=*rptr; 

  rptr=(unsigned long*)hisBase;
  wptr=save_ptr+1;
  printf("histo transfer to: %08lx - IRQ = %ld\n",wptr,nofirq);
       
  for (i=0;i<ndets[mode];i++) {
    for (k=0;k<nchans[mode];k++) {
      *wptr++=*rptr++;  
      /* tsleep here to keep VSB access to DSP at moderate level */
      tsleep(1);
    }
  }
}

/* Decode incoming packets from LINUX host */
void 
decode_command(opcode,prm,val) 
     int opcode, prm, val;
{	     
  int i,j,k;

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

int
recv_pkg(len, opcode, prm, val)
     int * len, * opcode, * prm, * val;
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

void
phex(input,len) 
     char * input;
     int len;
{
  char ch;
  int i;

  for (i=0;i<len;i++) {
    ch = *(input + i);
    if ((i>0) && !(i%4)) printf("- ");
    printf("%02x ",ch);
  }
  printf("\n");
}

/* Send portions of the data histogram to the host computer */

int
send_vec(blocknum,blocksize)
     int blocknum, blocksize;
{
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

/* Send histograms to LINUX host upon request */
void 
send_histos() {
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

/* Send histograms to LINUX host upon request */
void 
send_scalers() {
  int blocknum, blocksize, retn;
  int len, opcode, prm, val,n;
  int nblocks,written;
  
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


/* Set socket option to make server socket nonblocking */
void dontblock() {
  struct sgbuf buf;
  _gs_opt(sockfd,&buf);
  buf.sg_noblock=1;
  _ss_opt(sockfd,&buf);
}
