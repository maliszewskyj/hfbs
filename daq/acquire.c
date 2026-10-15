static char rcsid[] = "$Id: acquire.c,v 2.0 2026/08/17 16:46:46 nickm Exp $";
/*
   TODO: Figure out transferring scaler data
         Set data acquisition mode remotely
	 Make data acquisition mode make sense
	 Transfer livetime
	 Transfer


   REMOTE Server needs to know only two modes: PST binning and Energy binning
 */
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
 *      acquire add ?array1? ?array2?
 *
 *      acquire timer
 *        reads IRQ counter to get a total live time
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
#include <errno.h>

#include "tcp_io.h"

extern int errno;
// char * host = "hfbsdaq.ncnr.nist.gov";
char * daqserver = "129.6.120.54";
#define SERVERPORT 10000
#define TBINDEFAULT  30

#define NTCHAN   4096
#define NDCHAN   20

#define ELEMENTS NTCHAN*NDCHAN


u_int32_t data_array[ELEMENTS];
time_t t12,t1,t2;

char * rdev = "HFBS_SHM_DATA";
#define HFBS_SHM_DATA_SIZE (NDCHAN*NTCHAN*sizeof(int))

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
#define TIME_BIN      30e-6

float * remote_hist;
float * pIncrement;
float * pTotal;
float Total[NDCHAN][NTCHAN];
float Increment[NDCHAN][NTCHAN];
float Energy[NTCHAN];
float Ordinal[NTCHAN];
int Remote[NDCHAN][NTCHAN];

double live_time=0.0;
int     remote_len;
/*
 * Acquisition modes: 0 = pst      binning (teach)
 *                    1 = energy   binning (time of flight)
 *                    2 = energy   binning (time of flight) DEFAULT
 *                    3 = scan             (energy binning with fill)
 *
 * The remote server only knows PST binning (0) and Energy binning (1)
 */
int     daq_mode=0;
int     ndchans[4] = {  20,  20,  20,  20};
int     smpdet[4]  = {  19,  19,  19,  19};
int     sum_mask[20] = { 0, 1, 1, 1, 1, 
			 1, 1, 1, 1, 1, 
			 1, 1, 1, 1, 1,
			 0, 0, 0, 0, 0};
#define CMD_MODE_PBN 0
#define CMD_MODE_TBN 2
#define CMD_MODE_EBN 1
#define CMD_MODE_SCN 3 // Really energy binning but with "stuffing"


int traceflag=0;
int timbin = TBINDEFAULT;
int test_pulse = 0;

/* Variables for shared memory and semaphores */
key_t shkey, semkey;
int shmid, semid;
char * shmptr;
union semun semctl_arg;

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
int acq_update(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
{
  int i, j, retn;
  int sockfd;
  //  int blocksize=8192;
  int blocksize=327680;
  unsigned int tmpval;
  unsigned int * tlptr;
  int * pRemote;
  
  
  if (objc < 1) {
    Tcl_SetResult(interp,"Usage: acquire update ?arrayName?",TCL_STATIC);
    return TCL_ERROR;
  }
  if (traceflag) printf("acquire update [from histogram server]\n");
  sockfd = net_open(daqserver,NULL,SERVERPORT);
  if (sockfd < 0) {
    Tcl_SetResult(interp,"Cannot open remote server",TCL_STATIC);
    return TCL_ERROR;
  }
  memset(Remote,0,sizeof(Remote));
  tmpval = htonl(blocksize);
  //printf("Requesting transfer. Blocksize = %d\n",blocksize);
  retn = sendPKG(sockfd,OP_CMD,CMD_XFER,&tmpval,sizeof(tmpval));
  //printf(" sendPKG: retn = %d\n",retn);
  
  //printf("Receiving histogram\n");
  pRemote = &Remote[0][0];
  retn = recvHIST(sockfd,pRemote,sizeof(Remote));
  //printf("  recvHIST: retn = %d\n",retn);
  
  net_close(sockfd);
  sockfd=-1;
  /* Set acquisition mode semaphore so our clients know what mode is set */
  semctl_arg.val = daq_mode;
  if (semctl(semid,0,SETVAL,semctl_arg) < 0) {
    fprintf(stderr,"Error setting mode semaphore: %s\n",strerror(errno));
  }
  
  /* Shuffle data into un-interleaved form if we're doing velocity binning */
  // if ((daq_mode == CMD_MODE_VBN) || (daq_mode == CMD_MODE_SCN)) ShuffleData();
  //Calc_LiveTime(&live_time);
  //memcpy(pIncrement,remote_hist,remote_len*sizeof(float));
  //memset(remote_hist,0,remote_len*sizeof(float));
  for(i=0;i<NDCHAN;i++) {
    for (j=0;j<NTCHAN;j++) {
      Increment[i][j] = Remote[i][j];
    }
  }

  Tcl_SetResult(interp,"OK:\nAcquire update complete.",TCL_STATIC);
  if (traceflag) printf("...complete.\n");
  return TCL_OK;
}

int acq_clear(Tcl_Interp * interp, int objc, Tcl_Obj * objv[]) 
{
  int sockfd;
  int val, lstr;
  char *strval;
  // NCM 9/10/2026 - trying to figure out why live plotting for scans looks wrong
  if (objc>0) {
    strval = Tcl_GetStringFromObj(objv[0],&lstr);
    if (!strcmp(strval,"total")) {
      memset(Total,0,sizeof(Total));
    }
  }

  sockfd = net_open(daqserver,NULL,SERVERPORT);

  val = 0; // NCMR - Figure out test for server actively acquiring
  if (val) {
    Tcl_SetResult(interp,"ERR: Acquisition in progress",TCL_STATIC);
    return TCL_ERROR;
  }
  
  if (sockfd < 0) {
    Tcl_SetResult(interp,"Cannot open port on histogrammer",TCL_STATIC);
    sockfd = 0;
    return TCL_ERROR;
  }

  sendPKG(sockfd,OP_CMD,CMD_CLEAR,0,0);
  net_close(sockfd);

  Tcl_SetResult(interp,"OK:\nHisto clear sent.",TCL_STATIC);
  return TCL_OK;
}

// Initialize to "Fill" value = -1
int acq_fill(Tcl_Interp * interp, int objc, Tcl_Obj * objv[]) 
{
  int i,j;
  for (i=0;i<NDCHAN;i++) {
    for (j=0;j<NTCHAN;j++) {
      Total[i][j] = -1; 
    }
  }
  return TCL_OK;
}

int acq_insert(Tcl_Interp * interp, int objc, Tcl_Obj * objv[]) 
{
  int i, offset, listObjc, idet;
  Tcl_Obj ** listObjv;
  int slice[NDCHAN];
  double dval;

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

int acq_start(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
{
  int sockfd;
  int val;

  if (traceflag) { printf("acquire start"); fflush(stdout); }
  sockfd = net_open(daqserver,NULL,SERVERPORT);

  if (sockfd < 0) {
    Tcl_SetResult(interp,"Cannot open port on histogrammer",TCL_STATIC);
    sockfd = 0;
    return TCL_ERROR;
  }

  sendPKG(sockfd,OP_CMD,CMD_ARM,0,0);
  net_close(sockfd);

  if (traceflag) printf("...complete\n");
  return TCL_OK;

}

int acq_stop(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
{
  int rc,readrc;
  int i,j,k;
  u_int32_t readvalue; 

  if (traceflag) { printf("acquire stop\n"); fflush(stdout); }
  int sockfd;
  int val;
  memset(Total,0,sizeof(Total));
  sockfd = net_open(daqserver,NULL,SERVERPORT);

  if (sockfd < 0) {
    Tcl_SetResult(interp,"Cannot open port on histogrammer",TCL_STATIC);
    sockfd = 0;
    return TCL_ERROR;
  }

  sendPKG(sockfd,OP_CMD,CMD_DISARM,0,0);
  net_close(sockfd);

  if (traceflag) { printf("...complete\n"); fflush(stdout); }
  return TCL_OK;

}

int acq_setmode(Tcl_Interp * interp, int mode)
{
  int sockfd;
  int val;
  int tmparr[2];

  if (traceflag) { printf("acquire mode %d",mode); fflush(stdout); }
  sockfd = net_open(daqserver,NULL,SERVERPORT);

  if (sockfd < 0) {
    Tcl_SetResult(interp,"Cannot open port on histogrammer",TCL_STATIC);
    sockfd = 0;
    return TCL_ERROR;
  }
  tmparr[0]=htonl(PRM_HST_MODE);
  tmparr[1]=htonl(mode);

  sendPKG(sockfd,OP_CMD,CMD_SET,tmparr,sizeof(tmparr));
  
  net_close(sockfd);

  if (traceflag) printf("...complete\n");
  return TCL_OK;

}


int acq_frames(int * nFrames)
{
  int retn, val,len,opcode,prm,tmpval;
  int sockfd;
  if (traceflag) printf("acquire frames\n");
  
  sockfd = net_open(daqserver,NULL,SERVERPORT);
  if (sockfd < 0) {
    sockfd = 0;
    return -1;
  }

  tmpval = htonl(PRM_NFRAMES);
  sendPKG(sockfd,OP_CMD,CMD_GET,&tmpval,sizeof(tmpval));
  retn = recvPKG(sockfd,&len,&opcode,&prm,(char *)&val);

  net_close(sockfd);
  if (retn <= 0) return -1;
  *nFrames = ntohl(val);
  return 1;
}

int acq_counting(int * counting)
{
  int retn, val,len,opcode,prm,tmpval;
  int sockfd;
  if (traceflag) printf("acquire counting\n");
  
  sockfd = net_open(daqserver,NULL,SERVERPORT);
  if (sockfd < 0) {
    sockfd = 0;
    return -1;
  }

  tmpval = htonl(PRM_STATUS);
  sendPKG(sockfd,OP_CMD,CMD_GET,&tmpval,sizeof(tmpval));
  usleep(100);
  retn = recvPKG(sockfd,&len,&opcode,&prm,(char *)&val);
  val = ntohl(val);
  net_close(sockfd);
  if (retn <= 0) return -1;
  if (traceflag) printf("   status = %d\n",val);
  *counting = (val & 0x1);
  return 1;
}

int acq_preset(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
{
  int retn,i;
  char buf[20];
  int tmparr[2], presetms;
  int len, opcode, prm;
  Tcl_Obj * optr;
  int sockfd;
  
  if (traceflag) printf("acquire preset\n");

  if (objc < 1) {
    Tcl_SetResult(interp,"Usage: acquire preset ?presetms?",TCL_STATIC);
    return TCL_ERROR;
  }

  if (Tcl_GetIntFromObj(interp,objv[0],&presetms) != TCL_OK) return TCL_ERROR;
  if ((presetms < 0) || (presetms > 3600000)) {
    Tcl_SetResult(interp,"Presetms must be between 0 and 3600000",TCL_STATIC);
    return TCL_ERROR;
  }
  
  sockfd = net_open(daqserver,NULL,SERVERPORT);
  if (sockfd < 0) {
    Tcl_SetResult(interp,"Cannot open port on histogrammer",TCL_STATIC);
    sockfd = 0;
    return TCL_ERROR;
  }

  tmparr[0] = htonl(PRM_TPRESET);
  tmparr[1] = htonl(presetms);
  sendPKG(sockfd,OP_CMD,CMD_SET,tmparr,sizeof(tmparr));
  net_close(sockfd);
  Tcl_SetResult(interp,"Set server preset to requested number of ms",TCL_STATIC);
  if (traceflag) printf("...complete\n");
  return TCL_OK;
}


int acq_scalers(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
{
  int retn,i;
  char buf[20];
  int scalers[16], ival,tmpval;
  int len, opcode, prm;
  Tcl_Obj * optr;
  int sockfd;
  
  if (traceflag) printf("acquire scalers\n");
  
  sockfd = net_open(daqserver,NULL,SERVERPORT);
  if (sockfd < 0) {
    Tcl_SetResult(interp,"Cannot open port on histogrammer",TCL_STATIC);
    sockfd = 0;
    return TCL_ERROR;
  }

  tmpval = htonl(PRM_SCALERS);
  sendPKG(sockfd,OP_CMD,CMD_GET,&tmpval,sizeof(tmpval));
  retn = recvPKG(sockfd,&len,&opcode,&prm,(char *)scalers);
  net_close(sockfd);
  optr = Tcl_NewListObj(0,NULL);
  for (i=0;i<16;i++) {
      // Fix byte order 
      scalers[i] = ntohl(scalers[i]);
      Tcl_ListObjAppendElement(interp,optr,Tcl_NewIntObj(scalers[i]));
  }
  Tcl_SetObjResult(interp,optr);

  if (traceflag) printf("...complete\n");
  return TCL_OK;
}

/*
 * Add two arrays together - 
 */
int acq_add(Tcl_Interp * interp, int objc, Tcl_Obj * objv[]) 
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
    switch(daq_mode) {
    case CMD_MODE_EBN:
      Total[17][i] = Energy[i];
      break;
    default:
      Total[17][i] = Ordinal[i];
    }
  }
  return TCL_OK;
}

int acq_dsum(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
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

int acq_backfill(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
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
int acq_scale(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
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
int acq_share(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
{
  int val;
  if (objc < 1) {
    Tcl_SetResult(interp,"Usage: acquire share ?array?",TCL_STATIC);
    return TCL_ERROR;
  }

  if (traceflag) printf("acquire share\n");
  // Copy our data to the shared memory segment
  pTotal = &Total[0][0];
  memcpy(shmptr,pTotal,(NTCHAN*NDCHAN*sizeof(float)));

  semctl_arg.val = (int) daq_mode;
  if (semctl(semid,0,SETVAL,semctl_arg) < 0) {
    fprintf(stderr,"acq_share semophore error: %s\n",strerror(errno));
  }
  /* Store livetime in semaphore so our clients can access it */
  val = (int) live_time;
  semctl_arg.val = val;
  if (semctl(semid,1,SETVAL,semctl_arg) < 0) {
    fprintf(stderr,"acq_share semophore error: %s\n",strerror(errno));
  }
  if (traceflag) printf("...complete\n");
  return TCL_OK;
}

int acq_write(Tcl_Interp * interp, int objc, Tcl_Obj * objv[])
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

int acq_udpbroad(Tcl_Interp *interp, int objc, Tcl_Obj * objv[])
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
  len = lstr;

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

int Tcl_Acquire(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj * objv[])
{
  int opcode, parm, val, retn;
  float fval;
  int counting;
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

  strval = Tcl_GetStringFromObj(objv[1],&lstr);

  if (!strcmp(strval,"counting")) {
    rc = acq_counting(&counting);
    if (rc <= 0) {
      Tcl_SetResult(interp,"Could not determine counting status",TCL_STATIC);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,Tcl_NewIntObj(counting));
    return TCL_OK;
  } else if (!strcmp(strval,"preset")) {
    return acq_preset(interp, (objc-2), objv+2);
  } else if (!strcmp(strval,"update")) {
    return acq_update(interp, (objc-2), (objv+2));
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
  } else if (!strcmp(strval,"mode")) {
    if (objc < 3) {
      Tcl_SetResult(interp,"Usage: acquire mode <pst/time/velocity/scan>",TCL_STATIC);
      return TCL_ERROR;
    }

    strval = Tcl_GetStringFromObj(objv[2],&lstr);
    if (!strncmp(strval,"pst",lstr)) {
      timbin=TBINDEFAULT;
      daq_mode = CMD_MODE_PBN;
    } else if (!strncmp(strval,"energy",lstr)) {
      timbin=TBINDEFAULT;
      daq_mode = CMD_MODE_EBN;
    } else if (!strncmp(strval,"velocity",lstr)) {
      timbin=TBINDEFAULT;
      daq_mode = CMD_MODE_EBN;
    } else if (!strncmp(strval,"scan",lstr)) {
      timbin=TBINDEFAULT;
      daq_mode = CMD_MODE_SCN;
    } else {
      Tcl_SetResult(interp,"Valid modes: pst velocity scan",
		    TCL_STATIC);
      return TCL_ERROR;
    }
    // Need to set acquisition mode
    if (acq_setmode(interp,daq_mode)==TCL_ERROR) return TCL_ERROR;
    
    
    semctl_arg.val = daq_mode;
    if (semctl(semid,0,SETVAL,semctl_arg) < 0) {
      Tcl_SetResult(interp,"Cannot initialize acquisition mode semaphore",
		    TCL_STATIC);
    }
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
    retn = acq_frames(&val);
    if (retn > 0) {
      fval = val * timbin * 1e-6;
      live_time = fval; //

      Tcl_SetObjResult(interp,Tcl_NewDoubleObj(fval));
      return TCL_OK;
    }
    Tcl_SetResult(interp,"Trouble read number of frames for timer",TCL_STATIC);
    return TCL_ERROR;
  } else if (!strcmp(strval,"frames")) {
    retn = acq_frames(&val);
    if (retn>0) {
      Tcl_SetObjResult(interp,Tcl_NewIntObj(val));
      return TCL_OK;
    }
    
    Tcl_SetResult(interp,"Trouble reading number of frames",TCL_STATIC);
    return TCL_ERROR;    
  } else if (!strcmp(strval,"scalers")) {
    return acq_scalers(interp,(objc-2), (objv+2));
  } else if (!strcmp(strval,"timebin")) {
    if (objc < 3) {
      Tcl_SetResult(interp,"Usage: acquire timebin <binwidth (usec)>",
		    TCL_STATIC);
      return TCL_ERROR;
    } 
    if (Tcl_GetIntFromObj(interp,objv[2],&timbin) != TCL_OK) return TCL_ERROR;
    else {
      // NCM Figure out out to set parameter for this
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

  semctl_arg.val = daq_mode;
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

  retn = acq_udpsock();

  /* Turn on debugging from Tcl */
  Tcl_LinkVar(interp,"traceflag",(char *)&traceflag,TCL_LINK_INT);
  Tcl_LinkVar(interp,"livetime",(char *)&live_time,TCL_LINK_DOUBLE);
  Tcl_CreateObjCommand(interp,"acquire",(Tcl_ObjCmdProc *)Tcl_Acquire,(ClientData)NULL, (Tcl_CmdDeleteProc*)acq_exit);

  Tcl_SetObjResult(interp,Tcl_NewStringObj(rcsid,strlen(rcsid)));
  return TCL_OK;
}
