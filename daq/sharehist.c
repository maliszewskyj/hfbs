static char rcsid[] = "$Id: sharehist.c,v 2.0 2026/08/28 14:32:07 nickm Exp $";
/*
 * NAME
 *      sharehist - a loadable Tcl extension for accessing shared memory
 *
 * PURPOSE
 *      
 *
 * USAGE
 *
 *      sharehist narray - give narray access to shared memory segment
 *                         and semaphores
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <tcl.h>

/* Instrument-specific definitions */

#define VME_UNKNOWN    0
#define VME_SAMESTATE -1
#define VME_INVALID   -2
#define NTCHAN 4096
#define NDCHAN 20

int ndchans[4] = {  20,  20,  20,  20};
int ntchans[4] = {4096,4096,4096,4096};
int smpdet[4]  = {  19,  19,  19,  19};
int vme_mode=0;
int ntch=4096;
int semval;
float live_time=0.0;
static char mode[80];

/* Variables for shared memory */
key_t shkey,semkey;
int shmid,semid;
char * shmptr;

float Total[NDCHAN][NTCHAN];
float Counts[NDCHAN][NTCHAN];
float Energy[NTCHAN];
float PV[NTCHAN];
float X[NTCHAN];
float scratch[NTCHAN];

float * pTotal;
float * pCounts;
float * pEnergy;
float * pPV;
float * pNormalize;
float * pX;

void Calc_Vectors()
{
  int i;
  float erange;

  erange = 200.0;
  for (i=0;i<NTCHAN;i++) {
    Energy[i] = ((i - 2048)*erange)/2048.0;
  }
}


int share_update(Tcl_Interp * interp, int objc, char * objv[])
{
  char outval[80];
  unsigned int rawval;
  unsigned int Remote[NDCHAN][NTCHAN], *pRemote;
  int i;
  // Copy histogram directly from shared memory segment
   /* Read semaphores to determine data acquisition mode */
  if ((semval= semctl(semid,0,GETVAL,0)) < 0) {
    fprintf(stderr,"Error determining acquisition mode from semaphore.\n");
  } else {
    vme_mode = semval;
    //printf("Acquisition mode = %d\n",vme_mode);
  }

  /* Read semaphores to determine live acquisition time */
  if ((semval= semctl(semid,1,GETVAL,0)) < 0) {
    fprintf(stderr,"Error determining acquisition mode from semaphore.\n");
  } else {
    live_time = (float) semval;
  }

  memset(Remote,0,sizeof(Remote));
  pRemote = &Remote[0][0];
  memcpy(pTotal, shmptr, (NTCHAN*NDCHAN*sizeof(float)));
  memcpy(pRemote, shmptr, (NDCHAN*NTCHAN*sizeof(unsigned int)));
  
  switch(vme_mode) {
  case 0:sprintf(mode,"pst"); break;
  case 1:sprintf(mode,"time"); break;
  case 2:sprintf(mode,"velocity"); break;
  case 3:sprintf(mode,"scan"); break;
  default:
    Tcl_SetResult(interp,"Bad value for semaphore.",TCL_STATIC);
    return TCL_ERROR;
  }

  /*
   * Make sure that storage points to our shared memory segment.
   * If it doesn't, blow it away.
   */

  if (vme_mode == 3) {
    /* Walk through monitor channel looking for fill value */
    for (i=0;i<NTCHAN;i++) {
      //printf("pTotal[%d] = %f\n",i,*(pTotal+i));
      if (*(pTotal + i) < 0) break;
      rawval = *(pRemote + i);
    }
    ntch = i;
  } else {
    ntch = ntchans[vme_mode];
  }
  sprintf(outval,"%d",ntch);
  //printf("ntch = %d\n",ntch);
  Tcl_AppendResult(interp,outval,(char *) NULL);
  return TCL_OK;
}

int share_split(Tcl_Interp *interp, int objc, char * objv[]) 
{
  // array1 = Total
  // array2 = Counts
  // array3 = Normalize
  // array4 = X
  float *array1,*array2,*array3, *array4;

  float * wrk;
  int i, mode,len,rowlen,lstr;
  char * modestr, * str1, * str2, * str3, * str4;

  wrk = &scratch[0];
  array1 = pTotal;
  array2 = pCounts;
  array3 = pPV;
  array4 = pX;
  mode = 0;
  if (objc < 1) {
    Tcl_SetResult(interp,"Usage: share split <mode>",TCL_STATIC);
    return TCL_ERROR;
  }
  
  modestr = Tcl_GetStringFromObj(objv[0],&lstr);

  if (!strncmp("pst",modestr,lstr)) {
    mode = 0;
  } else if (!strncmp("time",modestr,lstr)) {
    mode = 1;
  } else if (!strncmp("velocity",modestr,lstr)) {
    mode = 2;
  } else if (!strncmp("scan",modestr,lstr)) {
    mode = 3;
  } else {
    Tcl_AppendResult(interp,"\"",modestr,"\" is an unrecognized mode",0);
    return TCL_ERROR;
  }

  /* Handle scans */
  if (mode == 3) {
    for (ntch=0;ntch<NTCHAN;ntch++) {
      if (*(array1 + ntch) < 0.0) break;
    }
    /* Cover our butts in case there are not any points in the scan */
    if (ntch == 0) {
      for (i=0;i<20;i++) 
	*(array1 + i * NTCHAN) =  0.0;
      for (i=19;i<20;i++) 
	*(array1 + i * NTCHAN) = 1.0;
      ntch = 1;
    } 
  } else {
    ntch = NTCHAN;
  }


  /* Fill in data and sample arrays */
  rowlen = NTCHAN;
  switch(mode){
  case 0: /* PST binning */
    memcpy(wrk,(array1 + 19 * rowlen), rowlen * sizeof(float)); 
    /* Fill in sample array */
    //for (i=0;i<17;i++) {
    //  memcpy((array3 + i * rowlen), wrk, rowlen * sizeof(float));
    //}
    memcpy(array3, wrk, rowlen *sizeof(float));
    /* Fill in data array */
    memcpy(array2,array1, 
	   17 * rowlen * sizeof(float));

    /* Fill in x position array */
    memcpy(array4,(array1 + 17 * rowlen), 
	   rowlen*sizeof(float));    
    break;
  case 1: /* Time binning */
    memcpy(wrk,(array1 + 19 * rowlen), rowlen * sizeof(float)); 
    /* Fill in sample array */
    //for (i=0;i<17;i++) {
    //  memcpy((array3 + i * rowlen), wrk, 
    //	     rowlen * sizeof(float));
    //}
    memcpy(array3,wrk,rowlen*sizeof(float));

    /* Fill in data array */
    memcpy(array2,array1, 
	   17 * rowlen * sizeof(float));

    /* Fill in x position array */
    memcpy(array4,(array1 + 17 * rowlen), 
	   rowlen*sizeof(float));    
    break;
  case 2: /* Velocity */
  case 3: /* Scan */
    //memcpy(wrk,(array1 + 18 * rowlen), ntch * sizeof(float));
    /* Fill in sample array */

    //memcpy(wrk,(array1 + 19 * rowlen), ntch * sizeof(float));
    //for (i=1;i<17;i++) {
    //  memcpy((array3 + i * NTCHAN), wrk, ntch * sizeof(float));
    //}
    memcpy(array3,(array1 + 19 * rowlen),ntch*sizeof(float)); // pV
    /* Fill in data array */
    for (i=0;i<17;i++) {
      memcpy((array2 + i * NTCHAN), (array1 + i * rowlen), ntch * sizeof(float));
    }

    for (i=0;i<ntch;i++) {
      *(array4 + i) = i; // Just ordinal
    }
    /* Fill in x position array */
    //memcpy(array4,(array1 + 17 * rowlen), ntch*sizeof(float));
    
    break;
  default:
    Tcl_SetResult(interp,"Bad mode number",TCL_STATIC);
    return TCL_ERROR;
  }

  /* Free work storage */
  return TCL_OK;
}

int share_dsum(Tcl_Interp *interp, int objc, const char * objv[])
{
  float data[NTCHAN];
  int mask[20] = {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
  int i,j,lmask, listObjc,ival;
  char outval[20];
  Tcl_Obj ** listObjv, * optr;

  // Process input mask

  if (objc > 0) {
    if (Tcl_ListObjLength(interp,objv[0],&lmask) != TCL_OK) {
      Tcl_SetResult(interp,"Introduce mask as a list",TCL_STATIC);
      return TCL_ERROR;
    }
    if (lmask < 17) {
      Tcl_SetResult(interp,"Specify at least 17 detectors",TCL_STATIC);
      return TCL_ERROR;
    }
    if (lmask > 20) {
      Tcl_SetResult(interp,"Too many elements in mask",TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ListObjGetElements(interp,objv[0],&listObjc,&listObjv) != TCL_OK) {
      Tcl_SetResult(interp,"mask must be at least 17 numbers",TCL_STATIC);
      return TCL_ERROR;
    }
    for (i=0;i<listObjc;i++) {
      if (Tcl_GetIntFromObj(interp,listObjv[i],&ival) != TCL_OK) {
	Tcl_SetResult(interp,"expecting integers for mask",TCL_STATIC);
	return TCL_ERROR;
      }
      mask[i] = (ival==0) ? 0 : 1;
    }
  }

  memset(data,0,sizeof(data));
  for (i=0;i<NTCHAN;i++){
    for (j=0;j<17;j++) {
      if (mask[j]) {
	data[i] += Counts[j][i];
      }
    }
  }

  optr = Tcl_NewListObj(0,NULL);
  for (i=0;i<NTCHAN;i++) {
    Tcl_ListObjAppendElement(interp,optr,Tcl_NewDoubleObj(data[i]));
  }
  Tcl_SetObjResult(interp,optr);

  return TCL_OK;
}

int share_ndsum(Tcl_Interp * interp, int objc, const char * objv[])
{
  float data[NTCHAN], normfactor,val;
  int mask[20] = {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
  int i,j,ival,lmask, lstr, listObjc;
  Tcl_Obj **listObjv, * optr;
  char * strval;
  int norm;

  norm = 0;

  if (objc > 0) {
    if (Tcl_ListObjLength(interp,objv[0],&lmask) != TCL_OK) {
      Tcl_SetResult(interp,"Introduce mask as a list",TCL_STATIC);
      return TCL_ERROR;
    }
    if (lmask < 17) {
      Tcl_SetResult(interp,"Specify at least 17 detectors",TCL_STATIC);
      return TCL_ERROR;
    }
    if (lmask > 20) {
      Tcl_SetResult(interp,"Too many elements in mask",TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ListObjGetElements(interp,objv[0],&listObjc,&listObjv) != TCL_OK) {
      Tcl_SetResult(interp,"mask must be at least 17 numbers",TCL_STATIC);
      return TCL_ERROR;
    }
    for (i=0;i<listObjc;i++) {
      if (Tcl_GetIntFromObj(interp,listObjv[i],&ival) != TCL_OK) {
	Tcl_SetResult(interp,"expecting integers for mask",TCL_STATIC);
	return TCL_ERROR;
      }
      mask[i] = (ival==0) ? 0 : 1;
    }
  }

  if (objc > 1) {
    strval = Tcl_GetStringFromObj(objv[1],&lstr);
    if (!strcmp(strval,"sample")) {
      norm += 1;
    } else if (!strcmp(strval,"monitor")) {
      norm += 2;
    }
  }

  if (objc > 2) {
    strval = Tcl_GetStringFromObj(objv[2],&lstr);
    if (!strcmp(strval,"sample")) {
      norm += 1;
    } else if (!strcmp(strval,"monitor")) {
      norm += 2;
    }
  }


  memset(data,0,sizeof(data));
  for (i=0;i<NTCHAN;i++){
    for (j=0;j<17;j++) {
      if (mask[j]) {
	data[i] += Counts[j][i];
      }
    }
  }

  if (norm) {
    for (i=0;i<NTCHAN;i++) {
      normfactor = 1;
      if (norm & 1) {
	// Normalize by sampling histogram
	val = (PV[i] < 0.0000001) ? 0 : 833.33/PV[i] ;
	normfactor *= val;
      }
      if (norm & 2) {
	// Normalize by monitor
	val = (Counts[0][i] < 0.0000001) ? 1 : 1/(Counts[0][i]);
	normfactor *= val;
      }
      data[i] *= normfactor;
    }
  }

  optr = Tcl_NewListObj(0,NULL);
  for (i=0;i<NTCHAN;i++) {
    Tcl_ListObjAppendElement(interp,optr,Tcl_NewDoubleObj(data[i]));
  }
  Tcl_SetObjResult(interp,optr);

  return TCL_OK;
}

int share_energy(Tcl_Interp * interp, int objc, const char * objv[])
{
  Tcl_Obj * optr;
  int i;
  optr = Tcl_NewListObj(0,NULL);
  for (i=0;i<NTCHAN;i++) {
    Tcl_ListObjAppendElement(interp,optr,Tcl_NewDoubleObj(Energy[i]));
  }
  Tcl_SetObjResult(interp,optr);
  return TCL_OK;
}

int share_pv(Tcl_Interp * interp, int objc, const char * objv[])
{
  Tcl_Obj * optr;
  int i;
  optr = Tcl_NewListObj(0,NULL);
  for (i=0;i<NTCHAN;i++) {
    Tcl_ListObjAppendElement(interp,optr,Tcl_NewDoubleObj(PV[i]));
  }
  Tcl_SetObjResult(interp,optr);
  return TCL_OK;
}

int share_det(Tcl_Interp * interp, int objc, const char * objv[])
{
  Tcl_Obj * optr;
  int i;
  int det;

  if (objc < 1) {
    Tcl_SetResult(interp,"Error: Specify detector",TCL_STATIC);
    return TCL_ERROR;
  }

  if (Tcl_GetIntFromObj(interp,objv[0],&det) != TCL_OK) return TCL_ERROR;

  if ((det < 0) || (det > 19)) {
    Tcl_SetResult(interp,"Detector out of range",TCL_STATIC);
    return TCL_ERROR;
  }

  optr = Tcl_NewListObj(0,NULL);
  for (i=0;i<NTCHAN;i++) {
    Tcl_ListObjAppendElement(interp,optr,Tcl_NewDoubleObj(Total[det][i]));
  }
  Tcl_SetObjResult(interp,optr);
  return TCL_OK;
}


void share_exit(ClientData clientData) {
  printf("adios!\n");
}

/*
 * Reassign the storage of the indicated narray to our shared memory segment
 */
int Tcl_ShareHist(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[])
{
  char * strval;
  int lstr;
  if (objc < 2) {
    Tcl_SetResult(interp,"Usage: share <command>",TCL_STATIC);
    return TCL_ERROR;
  }

  strval = Tcl_GetStringFromObj(objv[1],&lstr);
  if (!strcmp("update",strval)) {
    return share_update(interp,objc-2,objv+2);
  } else if (!strcmp("split",strval)) {
    return share_split(interp,objc-2,objv+2);
  } else if (!strcmp("dsum",strval)) {
    return share_dsum(interp,objc-2,objv+2);
  } else if (!strcmp("ndsum",strval)) {
    return share_ndsum(interp,objc-2,objv+2);
  } else if (!strcmp("energy",strval)) {
    return share_energy(interp,objc-2,objv+2);
  } else if (!strcmp("pv",strval)) {
    return share_pv(interp,objc-2,objv+2);
  } else if (!strcmp("det",strval)) {
    return share_det(interp,objc-2,objv+2);
  }

  return TCL_OK;
}


/*
 *  Tcl Wrapper: automatically called by the package loader
 *               The library name is capitalized.
 */

int Sharehist_Init(Tcl_Interp *interp) {

  /* Allocate storage for data */
  /* Create a unique key via a call to ftok() */
  shkey = ftok("/tmp",'a');
  semkey = ftok("/tmp",'b');

  /* Open the shared memory segment, failing if we can't */
  if ((shmid = shmget(shkey, NDCHAN * NTCHAN * sizeof(float), 
		      SHM_RDONLY)) < 0) {
    Tcl_SetResult(interp,"Cannot allocate shared memory",TCL_STATIC);
    return TCL_ERROR;
  }

  /* Attach (map) the shared memory segment into the current process */
  if ((shmptr = shmat(shmid,0,0)) < 0) {
    perror("shmat");
    exit(1);
  }

  /* Open the acquisition mode semaphore, failing if we can't */
  if ((semid = semget(semkey,2,0)) <0 ) {
    Tcl_SetResult(interp,"Cannot obtain mode semaphore",TCL_STATIC);
    return TCL_ERROR;
  }

  pTotal  = &Total[0][0];
  pCounts = &Counts[0][0];
  pPV = &PV[0];
  pX = &X[0];
  memset(Total,0,sizeof(Total));
  memset(Counts,0,sizeof(Counts));
  Calc_Vectors();

  Tcl_LinkVar(interp,"vmemode",(char *)&vme_mode,TCL_LINK_INT);
  Tcl_LinkVar(interp,"livetime",(char *)&live_time,TCL_LINK_DOUBLE);
  Tcl_CreateObjCommand(interp,"share",(Tcl_ObjCmdProc *)Tcl_ShareHist,(ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
  Tcl_SetObjResult(interp,Tcl_NewStringObj(rcsid,strlen(rcsid)));
  return TCL_OK;
}
