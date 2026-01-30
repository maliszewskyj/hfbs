static char rcsid[] = "$Id: acquire.c,v 1.8 2008/05/15 16:46:46 nickm Exp $";
/*
 * NAME
 *      acquire - a loadable Tcl extension for data acquisition
 *
 * PURPOSE
 *      
 *
 * USAGE
 *
 *      acquire clear
 *         clear histogram in local storage as well as that being collected
 *
 *      acquire update
 *         read histogram being collected and add to histogram stored locally
 *
 *      acquire start
 *
 *      acquire stop
 *
 *      acquire resume
 * 
 *      acquire add ?narray1? ?narray2?
 *
 *      acquire timer
 *        reads IRQ counter to get a total live time
 *      acquire load
 *        forces an upload of lookup tables
 *      acquire mode ?pst/time/velocity?
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <tcl.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "vsc16.h"
#include "dsplink.h"

#include <sis1100_var.h>
#include <sis3100_vme_calls.h>

#include "tcp_io.h"

#define TBINDEFAULT  30
#define ELEMENTS     4096*20

#define VSC_BASE     0x43520000
#define SIS3801_BASE 0x43508000
#define SCALERBASE   0x43508000
#define SIS3600_BASE 0x43509000
#define LATCHBASE    0x43509000

#define hisBase 0x00080000	/* base of 1 MB BRAM on VSB   */
#define INTRAM  0x81200000
#define irqctr    0x81202008	/* irq counter in INTRAM on SIS9200 */
#define latchval  0x8120204C	/* irq counter in INTRAM on SIS9200 */
#define latchmsk  0x81202050	/* irq counter in INTRAM on SIS9200 */
#define ptroff1 0x81202080 	/* Pointer offset 1     */
#define ptroff2 0x81202084	/* Pointer offset 2     */
#define ptroff3 0x81202088	/* Pointer offset 3     */
#define ptroff4 0x8120208C      /* Pointer offset 4     */

#define NTCHAN  4096
#define NDCHAN  20

unsigned long *data_ptr;
unsigned long *save_ptr;
unsigned long *ctrl_ptr,*look_ptr;
unsigned long *run_ptr,*new_run_ptr,*mode_ptr,*save_time_ptr;
unsigned long *rptr,*wptr,*mem;
unsigned long *timlook;

u_int32_t data_array[ELEMENTS];
time_t t12,t1,t2;

/* I really shouldn't have to do this, but /usr/include/bits/sem.h says I do */
#ifdef _SEM_SEMUN_UNDEFINED
union semun
   {
     int val;
     struct semid_ds *buf;
     unsigned short int *array;
     struct seminfo *__buf;
   };    
#endif

/* Instrument-specific definitions */
#define TIME_BIN      20e-6

#define VME_UNKNOWN       0
#define VME_SAMESTATE    -1
#define VME_INVALID      -2


float * remote_hist;
float * pIncrement;
float * pTotal;
float Total[NDCHAN][NTCHAN];
float Increment[NDCHAN][NTCHAN];
float Energy[NTCHAN];
float Ordinal[NTCHAN];
float Remote[NDCHAN][NTCHAN];

double live_time=0.0;
int     remote_len;
/*
 * Acquisition modes: 0 = pst      binning (teach)
 *                    1 = time     binning (time of flight)
 *                    2 = velocity binning (standard)
 *                    3 = scan             (standard again)
 */
int     ndchans[4] = {  20,  20,  20,  20};
int     smpdet[4]  = {  19,  19,  19,  19};
int     sum_mask[20] = { 0, 1, 1, 1, 1, 
			 1, 1, 1, 1, 1, 
			 1, 1, 1, 1, 1,
			 0, 0, 0, 0, 0};
int     vme_mode=0;
static char * vme_dev   = "/dev/sis1100_00remote";
static char * sdram_dev = "/dev/sis1100_00ram";
static char * sharc_dev = "/dev/sis1100_00dsp";
int vme_fd=0;
int sdram_fd=0;
int sharc_fd=0;
int traceflag=0;
int timbin = TBINDEFAULT;
int test_pulse = 0;

/* Variables for shared memory and semaphores */
key_t shkey, semkey;
int shmid, semid;
char * shmptr;
union semun semctl_arg;

char * daqserver = "192.168.120.15"; 
/* char * daqserver = "localhost"; */

int udpsock=0;
#define BROADCASTPORT 8080

/**********************************************************
 *
 * Actual operations performed on histograms.
 *
 */

/*
 * Data as received from the heurikon are interleaved data/interval by
 * group. Shuffle the data around "detector" by "detector" to 
 * reorganize it into an un-interleaved version.
 */
void ShuffleData(void) 
{
  int dwidth;
  float Shuffle[NDCHAN][NTCHAN];
  float * shufflebase;

  dwidth = NTCHAN;
  shufflebase = &Shuffle[0][0];
  memset(Shuffle,0,sizeof(Shuffle));
  memmove(shufflebase, (remote_hist + 2 * dwidth), 
	  (4 * dwidth * sizeof(float)));
  memmove((remote_hist + 5 * dwidth), (remote_hist + 1 * dwidth),
	  (1 * dwidth * sizeof(float)));
  memmove((remote_hist + 1 * dwidth),shufflebase,
	  (4 * dwidth * sizeof(float)));

  memmove(shufflebase, (remote_hist + 7 * dwidth),
	  (12 * dwidth * sizeof(float)));
  memmove((remote_hist + 18 * dwidth), (remote_hist + 5 * dwidth),
	  (1 * dwidth * sizeof(float)));
  memmove((remote_hist + 5 * dwidth), shufflebase,
	  (12 * dwidth * sizeof(float)));

  /* Clear out reserved detector (#17) */
  memset((remote_hist + 17 * dwidth), 0, (dwidth * sizeof(float)));

}

/*
 * Calculate live time from sample histogram
 *
 */
void Calc_LiveTime(double * livetime) 
{
  float * samplebase;
  int i;
  *livetime = 0;
  
  samplebase = (remote_hist + 19*NTCHAN);
  for (i=0;i<NTCHAN;i++) {
    *livetime += (double) *(samplebase + i);
  }
  if (traceflag) printf("livetime: %f\n",*livetime);

  //*livetime *= timbin;
}

void Calc_Vectors()
{
  int i;
  float erange;

  erange = 200.0;
  for (i=0;i<NTCHAN;i++) {
    Ordinal[i] = i;
    Energy[i] = ((i - 2048)*erange)/2048.0;
  }
}

/*
 * Save data from remote histogram into local array
 */
int acq_update(Tcl_Interp * interp, int objc, char * objv[])
{
  int i, j;
  if (objc < 1) {
    Tcl_SetResult(interp,"Usage: acquire update ?arrayName?",TCL_STATIC);
    return TCL_ERROR;
  }
  if (traceflag) printf("acquire update\n");
  remote_len = NDCHAN * NTCHAN;

  /* Set acquisition mode semaphore so our clients know what mode is set */
  semctl_arg.val = vme_mode;
  if (semctl(semid,0,SETVAL,semctl_arg) < 0) {
    fprintf(stderr,"Error setting mode semaphore.\n");
  }

  /* Shuffle data into un-interleaved form if we're doing velocity binning */
  if ((vme_mode == CMD_MODE_VBN) || (vme_mode == CMD_MODE_SCN)) ShuffleData();

  Calc_LiveTime(&live_time);
  //memcpy(pIncrement,remote_hist,remote_len*sizeof(float));
  //memset(remote_hist,0,remote_len*sizeof(float));
  for(i=0;i<NDCHAN;i++) {
    for (j=0;j<NTCHAN;j++) {
      Increment[i][j] = Remote[i][j];
      //Remote[i][j] = 0;
    }
  }

  Tcl_SetResult(interp,"OK:\nAcquire update complete.",TCL_STATIC);
  if (traceflag) printf("...complete.\n");
  return TCL_OK;
}

int acq_clear(Tcl_Interp * interp, int objc, char * objv[]) 
{

  memset(Total,0,sizeof(Total));
  return TCL_OK;
}

// Initialize to "Fill" value = -1
int acq_fill(Tcl_Interp * interp, int objc, char * objv[]) 
{
  int i,j;
  for (i=0;i<NDCHAN;i++) {
    for (j=0;j<NTCHAN;j++) {
      Total[i][j] = -1;
    }
  }
  return TCL_OK;
}

int acq_insert(Tcl_Interp * interp, int objc, char * objv[]) 
{
  int i, offset, listObjc, idet;
  Tcl_Obj ** listObjv;
  int slice[NDCHAN];
  float dval;

  if (objc < 2) {
    Tcl_SetResult(interp,"Usage: acquire insert ?offset? ?vals?",TCL_STATIC);
    return TCL_ERROR;
  }
  if (Tcl_GetIntFromObj(interp,objv[0],&offset) != TCL_OK) return TCL_ERROR;
  if ((offset < 0) || (offset > 4095)) {
    Tcl_SetResult(interp,"offset out of bounds",TCL_STATIC);
    return TCL_ERROR;
  }

  if (Tcl_ListObjLength(interp,objv[1],&idet) != TCL_OK) {
    Tcl_SetResult(interp,"slice must be a list of 20 numbers",TCL_STATIC);
    return TCL_ERROR;
  }
  if ((idet < 0) || (idet > 20) ) {
    Tcl_SetResult(interp,"slice must be a list of 20 numbers",TCL_STATIC);
    return TCL_ERROR;
  }

  if (Tcl_ListObjGetElements(interp,objv[1],&listObjc,&listObjv) != TCL_OK) {
    Tcl_SetResult(interp,"slice must be a list of 20 numbers",TCL_STATIC);
    /* May need to do garbage collection here */
    return TCL_ERROR;
  }

  for (i=0;i<20;i++) {
    if (Tcl_GetDoubleFromObj(interp,listObjv[i],&dval) == TCL_OK) {
      Total[i][offset] = dval;
    }
  }

  return TCL_OK;
}

int acq_start(Tcl_Interp * interp, int objc, char * objv[])
{
  int rc;
  int val;

  if (traceflag) printf("acquire start\n");
  time(&t2);
  t12=t1-t2;

  if ((rc=dsp_reset()) < 0) {
    Tcl_SetResult(interp,"error resetting DSP\n",TCL_STATIC);
    return TCL_ERROR;
  }
  init_sis3801(vme_mode);

  memset(data_array,0,sizeof(data_array));

  val = vme_mode;
  if ((rc=dsp_load(val))!=0) {
     printf("error on loading DSP\n");
     return TCL_ERROR;
  }
  init_vsc16();
  start_vsc16(0); 
  init_sis3600();
  if (traceflag) printf("...complete\n");
  return TCL_OK;

}

int acq_stop(Tcl_Interp * interp, int objc, char * objv[])
{
  int rc,readrc;
  int i,j,k;
  u_int32_t readvalue; 

  if (traceflag) printf("acquire stop\n");
  time(&t2);
  t12=t1-t2;

  rc=dsp_reset();
  reset_sis3801();
  reset_sis3600();
  stop_vsc16();

  memset(Remote,0,sizeof(Remote));
  readrc = s3100_sdram_read(sdram_fd, hisBase, data_array, ELEMENTS) ;
  if(readrc!=ELEMENTS*4) {
        printf("error on reading Histos from  SDRAM\n");
	Tcl_SetResult(interp,"Error reading Histos from SDRAM",TCL_STATIC);
	return TCL_ERROR;
  }
  for (k=0;k<ELEMENTS;k++) {
    i = k / NTCHAN;
    j = k % NTCHAN;
    Remote[i][j] = (float)data_array[k];
  }

  rc=s3100_sharc_read(sharc_fd, (latchval-INTRAM), &readvalue, 1) ;
  rc=s3100_sharc_read(sharc_fd, (latchmsk-INTRAM), &readvalue, 1) ;

  read_rings();
  if (traceflag) printf("...complete\n");
  return TCL_OK;

}

int acq_scalers(Tcl_Interp * interp, int objc, char * objv[])
{
  int retn,i;
  char buf[20];
  int scalers[16], ival;
  Tcl_Obj * optr;

  if (traceflag) printf("acquire scalers\n");
  retn = 0;
  optr = Tcl_NewListObj(0,NULL);
  for (i=0;i<16;i++) {
    if ((retn = vme_A32D32_read(vme_fd,VSC_BASE + VSC_DATA(i),(u_int32_t *)&ival))) {
      Tcl_SetResult(interp,"Error reading from scaler",TCL_STATIC);
      return TCL_ERROR;
    }
    Tcl_ListObjAppendElement(interp,optr,Tcl_NewIntObj(ival));
  }
  Tcl_SetObjResult(interp,optr);
  if (traceflag) printf("...complete\n");
  return TCL_OK;
}

int acq_latch(Tcl_Interp * interp, int objc, char * objv[]){
  int retn, val;

  retn = sis3600_read(&val);

  Tcl_SetObjResult(interp,Tcl_NewIntObj(val));
  return TCL_OK;
}

/*
 * Add two arrays together - 
 */
int acq_add(Tcl_Interp * interp, int objc, char * objv[]) 
{
  char * pArray1, * pArray2;
  int larray1, larray2;
  float * array1, * array2;
  int i;

  if (objc < 2) {
    Tcl_SetResult(interp,"Usage: acquire add Array1 Array2",TCL_STATIC);
    return TCL_ERROR;
  }
  pArray1 = Tcl_GetStringFromObj(objv[0],&larray1);
  pArray2 = Tcl_GetStringFromObj(objv[1],&larray2);

  // Set defaults
  array1 = pTotal;
  array2 = pIncrement;

  for (i=0;i<(NTCHAN*NDCHAN);i++) {
    *(array1 + i) += *(array2 + i);
  }

  // Fill in dimension scale
  for (i=0;i<NTCHAN;i++) {
    switch(vme_mode) {
    case CMD_MODE_VBN:
      Total[17][i] = Energy[i];
      break;
    default:
      Total[17][i] = Ordinal[i];
    }
  }
  return TCL_OK;
}

int acq_dsum(Tcl_Interp * interp, int objc, char * objv[])
{
  char * pArray1, buf[20];
  int which=0;
  float * array1, dsum[NDCHAN];
  int i,j,lstr;
  Tcl_Obj * optr;

  if (objc < 1) {
    Tcl_SetResult(interp,"Usage: acquire dsum Array",TCL_STATIC);
    return TCL_ERROR;
  }

  pArray1 = Tcl_GetStringFromObj(objv[0],&lstr);
  if (!strcmp(pArray1,"total")) { 
    which = 1;
  } 

  memset(dsum,0,sizeof(dsum));
  for (i=0;i<NDCHAN;i++) {
    for (j=0;j<NTCHAN;j++) {
      if (which) {
	dsum[i] += Total[i][j];
      } else {
	dsum[i] += Increment[i][j];
      }
    }
  }

  // Compose response
  optr = Tcl_NewListObj(0,NULL);
  for (i=0;i<NDCHAN;i++) {
    Tcl_ListObjAppendElement(interp,optr,Tcl_NewDoubleObj(dsum[i]));
  }
  Tcl_SetObjResult(interp,optr);
  return TCL_OK;
}

int acq_backfill(Tcl_Interp * interp, int objc, char * objv[])
{
  int listObjc, i;
  Tcl_Obj **listObjv;

  int index;
  double abscissa, dval, data[20];

  if (objc < 3) {
    Tcl_SetResult(interp,"Usage: acquire backfill ?index? ?abscissa? ?datalist?",TCL_STATIC);
    return TCL_ERROR;
  }

  index=0;
  abscissa=0.0;
  memset(data,0,sizeof(data));
  if (Tcl_GetIntFromObj(interp,objv[0],&index) != TCL_OK) return TCL_ERROR;
  if (Tcl_GetDoubleFromObj(interp,objv[1],&abscissa) != TCL_OK) return TCL_ERROR;

  if (traceflag) printf("acquire backfill %d %7.2f\n",index,abscissa);
  if (Tcl_ListObjGetElements(interp,objv[2],&listObjc, &listObjv) != TCL_OK) return TCL_ERROR;

  for(i=0;i<listObjc;i++) {
    if (i==20) break; // No more storage
    if (Tcl_GetDoubleFromObj(interp,listObjv[i],&dval) != TCL_OK) return TCL_ERROR;
    data[i] = dval;
  }

  // Bounds checking
  if ((index < 0) || (index >= 4096)) {
    Tcl_SetResult(interp,"index out of bounds 0 < index < 4096", TCL_STATIC);
    return TCL_ERROR;
  }

  // Now that we have what needs to be backfilled, add the data in
  for(i=0;i<20;i++) {
    Total[i][index] = data[i];
  }
  Total[17][index] = abscissa;
  if (traceflag) printf("...complete.\n");
  return TCL_OK;
}

// Write scale into shared memory segment
//      Options: ordinal energy (and should have time)
int acq_scale(Tcl_Interp * interp, int objc, char * objv[])
{
  char * str1;
  int lstr1,i;
  double *pData;
  int scale_energy;
  
  scale_energy = 0;

  if (objc < 1) {
    Tcl_SetResult(interp,
		  "Usage:\nacquire scale <array>",
		  TCL_STATIC);
    return TCL_ERROR;
  }

  str1 = Tcl_GetStringFromObj(objv[0],&lstr1);

  if (!strcmp(str1,"energy")) {
    scale_energy = 1;
  } else if (!strcmp(str1,"ordinal")) {
    scale_energy = 0;
  } else {
    fprintf(stderr,"array \"%s\" is not supported, using ordinal instead",str1);
    scale_energy = 0;
  }

  for(i=0;i<NTCHAN;i++) {
    Total[17][i] = (scale_energy) ? Energy[i] : Ordinal[i];
  }
  return TCL_OK;
}

/*
 * Reassign the storage of the indicated narray to our shared memory segment
 */
int acq_share(Tcl_Interp * interp, int objc, char * objv[])
{

  if (objc < 1) {
    Tcl_SetResult(interp,"Usage: acquire share ?array?",TCL_STATIC);
    return TCL_ERROR;
  }

  if (traceflag) printf("acquire share\n");
  // Copy our data to the shared memory segment
  pTotal = &Total[0][0];
  memcpy(shmptr,pTotal,(NTCHAN*NDCHAN*sizeof(float)));

  /* Store livetime in semaphore so our clients can access it */
  semctl_arg.val = (int) live_time;
  if (semctl(semid,1,SETVAL,semctl_arg) < 0) {
    fprintf(stderr,"Error setting mode semaphore.\n");
  }
  if (traceflag) printf("...complete\n");
  return TCL_OK;
}

int acq_write(Tcl_Interp * interp, int objc, char * objv[])
{
  char * str1, * str2, * str3;
  int lstr1, lstr2, lstr3;
  int i, j, offset;
  FILE * fp;
  if (objc < 2) {
    Tcl_SetResult(interp,
		  "Usage:\nacquire write <array> <filename> ?header?",
		  TCL_STATIC);
    return TCL_ERROR;
  }

  str1 = Tcl_GetStringFromObj(objv[0],&lstr1);
  str2 = Tcl_GetStringFromObj(objv[1],&lstr1);


  if ((fp = fopen(str2,"w")) == NULL) {
    Tcl_SetResult(interp,"ERR:\nCan't open file",TCL_STATIC);
    return TCL_ERROR;
  }

  if (objc >= 3) {
    str3 = Tcl_GetStringFromObj(objv[2],&lstr3);
    fprintf(fp,"%s\n",str3);
  }

  for (i=0;i<NDCHAN;i++) {
    fprintf(fp,"# Detector %d\n",i);
    for (j=0;j<NTCHAN;j++) {
      offset = i * NTCHAN + j;
      fprintf(fp,"%4d %10.2f\n",j,*(pTotal + offset));
    }
  }
  fflush(fp);
  fclose(fp);
  return TCL_OK;
}

/* Examine contents of DSP shadowed locations */
int acq_peek() {
  dmp_regs();
  return TCL_OK;
}

int acq_dump() {
  dmp_look();
  return TCL_OK;
}

void acq_exit(ClientData clientData) {
  /* Free shared memory segment */
  if (shmctl(shmid,IPC_RMID,0)<0) {
    fprintf(stderr,"Could not free shared memory segment.\n");
  }

  /* Free semaphore */
  if (semctl(semid,0,IPC_RMID,0)<0) {
    fprintf(stderr,"Could not free acquisition mode semaphore.\n");
  }

}

int acq_udpsock() 
{
  struct sockaddr_in cli_addr;
  int yes = 1;

  /* Create socket */
  if ((udpsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    return -1;
  }

  /* Bind socket to local address */
  memset((char *)&cli_addr,0,sizeof(cli_addr));
  cli_addr.sin_family = AF_INET;
  cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  cli_addr.sin_port        = 0;
  
  if ((bind(udpsock,(struct sockaddr *) &cli_addr, sizeof(cli_addr)))<0) {
    /* Can't bind to local address */
    close(udpsock);
    udpsock=-2;
    return -2;
  }
  if ((setsockopt(udpsock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(int))) < 0) {
    /* Can't set BROADCAST flag on UDP socket */
    close(udpsock);
    udpsock=-3;
    return -3;
  }
  return 0; /* Success */
}

int acq_udpbroad(Tcl_Interp *interp, int objc, char * objv[])
{
  struct sockaddr_in serv_addr;
  int retn;
  int len, lstr;
  char obuf[65536],*output, *strval;

  memset((char *)&serv_addr, 0, sizeof(serv_addr));
  memset(obuf,0,sizeof(obuf));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_BROADCAST;
  serv_addr.sin_port        = htons(BROADCASTPORT);

  if (objc < 1) {
      Tcl_SetResult(interp,"Usage: acquire broadcast <message>",TCL_STATIC);
      return TCL_ERROR;    
  }

  strval = Tcl_GetStringFromObj(objv[0],&lstr);
  strcpy(obuf,strval);

  if (udpsock <= 0) {
    Tcl_SetResult(interp,"Broadcast socket not initialized",TCL_STATIC);
    return TCL_ERROR;
  }

  if (traceflag) {
    printf("acq_udpbroad: sending %d bytes\n",len);
    printf("acq_udpbroad: sending \"%s\"\n",obuf);
  }
  output = &obuf[0];
  retn = sendto(udpsock, output, len, 0, 
		(struct sockaddr *)&serv_addr, sizeof(serv_addr)) ;
  if (traceflag) printf("acq_udpbroad: sendto returned %d\n",retn);
  if (retn != len) {
    Tcl_SetResult(interp,"Unable to broadcast message",TCL_STATIC);    
    return TCL_ERROR;
  }
  
  return TCL_OK;
}

/* 
 *  Interface to Tcl level - fork to requested subfunction
 *
 *
 * acquire update
 *         dump
 *         add
 *         clear
 *         write
 *         start
 *         stop
 *         mode
 *         dsum
 *         scalers
 *         latch
 *         pointers
 */

int Tcl_Acquire(ClientData clientData, Tcl_Interp *interp, int objc, char * objv[])
{
  int opcode, parm, val, retn;
  float fval;
  static char rmsg[80];
  //unsigned int to_sec;
  u_int32_t readvalue;
  unsigned int rc;
  char * strval;
  int lstr;

  Tcl_ResetResult(interp); // I shouldn't need to do this, but I'm chasing a memory leak
  if (objc < 2) {
    Tcl_SetResult(interp,"usage: acquire ?options? ?parameters?",TCL_STATIC);
    return TCL_ERROR;
  }

  parm = 0;
  val = 0;
  //to_sec = 30;

  strval = Tcl_GetStringFromObj(objv[1],&lstr);

  if        (!strcmp(strval,"update")) {
    return acq_update(interp, (objc-2), (objv+2));
  } else if (!strcmp(strval,"peek")) {
    return acq_peek();
  } else if (!strcmp(strval,"dump")) {
    // Dump lookup tables
    return acq_dump();
  } else if (!strcmp(strval,"share")) {
    if (objc < 3) {
      Tcl_SetResult(interp,"usage: acquire share <narrayname>",TCL_STATIC);
      return TCL_ERROR;
    }
    return acq_share(interp, (objc-2), (objv+2));
  } else if (!strcmp(strval,"add")) {
    return acq_add(interp,(objc-2),(objv+2));
  } else if (!strcmp(strval,"dsum")) {
    return acq_dsum(interp, (objc-2),(objv+2));
  } else if (!strcmp(strval,"clear")) {
    return acq_clear(interp, (objc-2), (objv+2));
  } else if (!strcmp(strval,"fill")) {
    return acq_fill(interp, (objc-2), (objv+2));
  } else if (!strcmp(strval,"backfill")) {
    return acq_backfill(interp, (objc-2), (objv+2));
  } else if (!strcmp(strval,"scale")) {
    return acq_scale(interp, (objc-2), (objv+2));
  } else if (!strcmp(strval,"insert")) {
    return acq_insert(interp,(objc-2), (objv+2));
  } else if (!strcmp(strval,"start")) {
    live_time = 0;
    return acq_start(interp,(objc-2), (objv+2));
  } else if (!strcmp(strval,"stop")) {
    return acq_stop(interp,(objc-2), (objv+2));

  } else if (!strcmp(strval,"resume")) {
    opcode = OP_CMD;
    parm   = CMD_RUN;
    val    = CMD_RUN_RSM;
  } else if (!strcmp(strval,"load")) {
      loadlook();
      buildtlook();
      Tcl_SetResult(interp,"Lookups loaded",TCL_STATIC);
      return TCL_OK;
  } else if (!strcmp(strval,"mode")) {
    if (objc < 3) {
      Tcl_SetResult(interp,"Usage: acquire mode <pst/time/velocity/scan>",TCL_STATIC);
      return TCL_ERROR;
    }

    strval = Tcl_GetStringFromObj(objv[2],&lstr);
    if (!strncmp(strval,"pst",lstr)) {
        timbin=TBINDEFAULT;
        vme_mode = CMD_MODE_PBN;
    } else if (!strncmp(strval,"time",lstr)) {
        vme_mode = CMD_MODE_TBN;
    } else if (!strncmp(strval,"velocity",lstr)) {
        timbin=TBINDEFAULT;
        vme_mode = CMD_MODE_VBN;
        enc_omega();
        enc_lookup();
    } else if (!strncmp(strval,"scan",lstr)) {
      timbin=TBINDEFAULT;
      enc_omega();
      enc_lookup();
      vme_mode = CMD_MODE_SCN;
    } else {
      Tcl_SetResult(interp,"Valid modes: pst time velocity scan",
		    TCL_STATIC);
      return TCL_ERROR;
    }    
    loadlook();
    return TCL_OK;
  } else if (!strcmp(strval,"write")) {
    return acq_write(interp,(objc-2), (objv+2));
  } else if (!strcmp(strval,"test")) {
    if (objc < 3) {
      Tcl_SetResult(interp,"Usage: acquire test <on/off>",TCL_STATIC);
      return TCL_ERROR;
    }
    retn = 0;
    strval = Tcl_GetStringFromObj(objv[2],&lstr);
    if (!strcmp(strval,"on")) {
      test_pulse=1;
      Tcl_SetResult(interp,"Test pulses will be on during next run",TCL_STATIC);
      return TCL_OK;
    } else if (!strcmp(strval,"off")) {
      test_pulse=0;
      Tcl_SetResult(interp,"Test pulses will be off during next run",TCL_STATIC);
      return TCL_OK;
    } else {
      Tcl_SetResult(interp,"Valid modes: on off",TCL_STATIC);
      return TCL_ERROR;
    }
    return TCL_OK;
  } else if (!strcmp(strval,"timer")) {
    rc=s3100_sharc_read(sharc_fd, (irqctr-INTRAM), &readvalue, 1) ;
      if (rc==4) {
        fval=timbin*1e-6*readvalue;
	Tcl_SetObjResult(interp,Tcl_NewDoubleObj(fval));
        return TCL_OK;
      } else return TCL_ERROR;
  } else if (!strcmp(strval,"ticks")) {
    rc=s3100_sharc_read(sharc_fd, (irqctr-INTRAM), &readvalue, 1) ;
      if (rc>0) {
	Tcl_SetObjResult(interp,Tcl_NewIntObj(readvalue));
        return TCL_OK;
      } else return TCL_ERROR;
  } else if (!strcmp(strval,"pointers")) {

    readvalue = 0;
    rc = s3100_sharc_read(sharc_fd, (ptroff1-INTRAM), &readvalue, 1);
    printf("ptr1 = %d\n",readvalue);
    rc = s3100_sharc_read(sharc_fd, (ptroff2-INTRAM), &readvalue, 1);
    printf("ptr2 = %d\n",readvalue);
    rc = s3100_sharc_read(sharc_fd, (ptroff3-INTRAM), &readvalue, 1);
    printf("ptr3 = %d\n",readvalue);
    rc = s3100_sharc_read(sharc_fd, (ptroff4-INTRAM), &readvalue, 1);
    printf("ptr4 = %d\n",readvalue);
    return TCL_OK;
  } else if (!strcmp(strval,"scalers")) {
    return acq_scalers(interp,(objc-2), (objv+2));
  } else if (!strcmp(strval,"latch")) {
    return acq_latch(interp, (objc-2), (objv+2));
  } else if (!strcmp(strval,"timebin")) {
    if (objc < 3) {
      Tcl_SetResult(interp,"Usage: acquire timebin <binwidth (usec)>",
		    TCL_STATIC);
      return TCL_ERROR;
    } 
    if (Tcl_GetIntFromObj(interp,objv[2],&timbin) != TCL_OK) return TCL_ERROR;
    else {
      buildtlook();
    }
  } else if (!strcmp(strval,"broadcast")) {
      return acq_udpbroad(interp,(objc-2),(objv+2));
  } else {
    Tcl_SetResult(interp,
	"Recognized options: share start stop resume mode test timebin timer ticks update",
	TCL_STATIC);
    return TCL_ERROR;
  }

  /* Write some meaningful response messages */
  if (!strcmp(strval,"start")) {
    Tcl_SetResult(interp,"OK:\nAcquisition started",TCL_STATIC);
  } else if (!strcmp(strval,"stop")) {
    Tcl_SetResult(interp,"OK:\nAcquisition stopped",TCL_STATIC);
  } else if (!strcmp(strval,"resume")) {
    Tcl_SetResult(interp,"OK:\nAcquisition resumed",TCL_STATIC);
  } else if (!strcmp(strval,"mode")) {
    Tcl_SetResult(interp,"OK:\nAcquisition mode set",TCL_STATIC);
  } else if (!strcmp(strval,"load")) {
    Tcl_SetResult(interp,"OK:\nLookup tables loaded",TCL_STATIC);
  } else if (!strcmp(strval,"timebin")) {
    Tcl_SetResult(interp,"OK:\nTime channel width set",TCL_STATIC);
  } else if (!strcmp(strval,"test")) {
    Tcl_SetResult(interp,"OK\nTest mode set",TCL_STATIC);
  }

  return TCL_OK;        
}


/*
 *  Tcl Wrapper: automatically called by the package loader
 *               The library name is capitalized.
 */

int Acquire_Init(Tcl_Interp *interp) {
  int retn;

  /* Allocate storage for data */
  /* Create a unique key via a call to ftok() */
  shkey = ftok("/tmp",'a');
  semkey = ftok("/tmp",'b');
  live_time = 0.0;

  if ((sdram_fd = open(sdram_dev,O_RDWR, 0)) <= 0) {
    Tcl_SetResult(interp, "error opening sdram link", TCL_STATIC);
    return TCL_ERROR;
  }

  if ((sharc_fd = open(sharc_dev,O_RDWR, 0)) <= 0) {
    Tcl_SetResult(interp, "error opening sharc link", TCL_STATIC);
    return TCL_ERROR;
  }

  if ((vme_fd = open(vme_dev,O_RDWR, 0)) <= 0) {
    Tcl_SetResult(interp, "error opening VME bridge", TCL_STATIC);
    return TCL_ERROR;
  }

  /* Open the shared memory segment, creating it if necessary */
  if ((shmid = shmget(shkey, NDCHAN * NTCHAN * sizeof(float), 
		      IPC_CREAT|0666)) < 0) {
    Tcl_SetResult(interp,"Cannot allocate shared memory",TCL_STATIC);
    return TCL_ERROR;
  }

  /* Attach (map) the shared memory segment into the current process */
  if ((shmptr = shmat(shmid,0,0)) < 0) {
    perror("shmat");
    exit(1);
  }

  memset(shmptr,0,NDCHAN*NTCHAN*sizeof(float));

  /* Open the acquisition mode semaphore */
  if ((semid = semget(semkey, 2, 0666 | IPC_CREAT)) < 0) {
    Tcl_SetResult(interp,"Cannot allocate acquisition mode semaphore",
		  TCL_STATIC);
    return TCL_ERROR;
  }

  semctl_arg.val = vme_mode;
  if (semctl(semid,0,SETVAL,semctl_arg) < 0) {
    Tcl_SetResult(interp,"Cannot initialize acquisition mode semaphore",
		  TCL_STATIC);
    return TCL_ERROR;
  }

  // Allocate memory for various histograms
  //pTotal     = (float *) ckalloc(NTCHAN * NDCHAN * sizeof(float));
  //pIncrement = (float *) ckalloc(NTCHAN * NDCHAN * sizeof(float));
  pTotal = &Total[0][0];
  pIncrement = &Increment[0][0];
  remote_hist = &Remote[0][0];
  memset(Total,0,sizeof(Total));
  memset(Increment,0,sizeof(Increment));
  Calc_Vectors();

  printf("sdram_dev = \"%s\" sdram_fd = %d\n",sdram_dev,sdram_fd);
  printf("sharc_dev = \"%s\" sharc_fd = %d\n",sharc_dev,sharc_fd);
  printf("vme_dev   = \"%s\" vme_fd   = %d\n",vme_dev,vme_fd);
  retn = acq_udpsock();

  /* Turn on debugging from Tcl */
  Tcl_LinkVar(interp,"traceflag",(char *)&traceflag,TCL_LINK_INT);
  Tcl_LinkVar(interp,"livetime",(char *)&live_time,TCL_LINK_DOUBLE);
  Tcl_CreateObjCommand(interp,"acquire",(Tcl_ObjCmdProc *)Tcl_Acquire,(ClientData)NULL, (Tcl_CmdDeleteProc*)acq_exit);

  Tcl_SetObjResult(interp,Tcl_NewStringObj(rcsid,strlen(rcsid)));
  return TCL_OK;
}
