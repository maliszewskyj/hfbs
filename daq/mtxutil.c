static char rcsid[] = 
"$Id: mtxutil.c,v 1.4 2008/06/02 13:42:21 nickm Exp $";
/*
 * NAME
 *      mtxutil - a loadable Tcl extension for manipulating narrays
 *
 * PURPOSE
 *      
 *
 * USAGE
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <tcl.h>
#include <narray.h>

static int     ndchans[4] = {  20,  20,  20,  20};
static int     ntchans[4] = {4096,4096,4096,4096};

int Mtx_Split(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj * objv[]) {
  Tcl_CmdInfo cmdInfo;
  NArray *array1,*array2,*array3, *array4;
  NArrayFloat * wrk;
  int i, mode,len,rowlen,ntch,lstr;
  char * modestr, * str1, * str2, * str3, * str4;
  mode = 0;
  if (objc < 5) {
    Tcl_SetResult(interp,
	  "Usage: mtx split <mode> NArray1 NArrayCts NarrayNrm NarrayX",
	  TCL_STATIC);
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

  /* Set up Narray1 */
  str1 = Tcl_GetStringFromObj(objv[1],&lstr);
  if (Tcl_GetCommandInfo(interp,str1,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str1,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array1 = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array1->closure.interp!= interp)
      || (array1->n_dims != 2)) {
    Tcl_AppendResult(interp,"\"",str1,"\""," must be a 2D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  /* Handle scans */
  if (mode == 3) {
    for (ntch=0;ntch<ntchans[mode];ntch++) {
      if (*(array1->storage + ntch) < (NArrayFloat) 0.0) break;
    }
    /* Cover our butts in case there are not any points in the scan */
    if (ntch == 0) {
      for (i=0;i<20;i++) 
	*(array1->storage + i * ntchans[mode]) = (NArrayFloat) 0.0;
      for (i=19;i<20;i++) 
	*(array1->storage + i * ntchans[mode]) = (NArrayFloat) 1.0;
      ntch = 1;
    } 
  } else {
    ntch = ntchans[mode];
  }

  /* Set up Narray2 */
  str2 = Tcl_GetStringFromObj(objv[2],&lstr);
  if (Tcl_GetCommandInfo(interp,str2,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str2,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array2 = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array2->closure.interp!= interp)
      || (array2->n_dims != 2)) {
    Tcl_AppendResult(interp,"\"",str2,"\""," must be a 2D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  /* Set up Narray3 */
  str3 = Tcl_GetStringFromObj(objv[3],&lstr);
  if (Tcl_GetCommandInfo(interp,str3,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str3,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array3 = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array3->closure.interp!= interp)
      || (array3->n_dims != 2)) {
    Tcl_AppendResult(interp,"\"",str3,"\""," must be a 2D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  /* Set up Narray4 */
  str4 = Tcl_GetStringFromObj(objv[4],&lstr);
  if (Tcl_GetCommandInfo(interp,str4,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str4,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array4 = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array4->closure.interp!= interp)
      || (array4->n_dims != 2)) {
    Tcl_AppendResult(interp,"\"",str4,"\""," must be a 2D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  /* resize the result narrays */
  ckfree(array2->storage);
  array2->length  = 17 * ntch;
  array2->dim_length[0] = 17;
  array2->dim_length[1] = ntch;
  array2->storage =
	    (NArrayFloat*) ckalloc(sizeof(NArrayFloat) * array2->length);
  memset(array2->storage, 0, sizeof(NArrayFloat) * array2->length);

  ckfree(array3->storage);
  array3->length  = 17 * ntch;
  array3->dim_length[0] = 17;
  array3->dim_length[1] = ntch;
  array3->storage =
	    (NArrayFloat*) ckalloc(sizeof(NArrayFloat) * array3->length);
  memset(array3->storage, 0, sizeof(NArrayFloat) * array3->length);

  ckfree(array4->storage);
  array4->length  = ntch;
  array4->dim_length[0] = 1;
  array4->dim_length[1] = ntch;
  array4->storage =
	    (NArrayFloat*) ckalloc(sizeof(NArrayFloat) * array4->length);
  memset(array4->storage, 0, sizeof(NArrayFloat) * array4->length);


  /* Allocate work storage */
  wrk = (NArrayFloat*) ckalloc(sizeof(NArrayFloat) * ntch);

  /* Fill in data and sample arrays */
  rowlen = ntchans[mode];
  switch(mode){
  case 0: /* PST binning */
    memcpy(wrk,(array1->storage + 19 * rowlen), rowlen * sizeof(NArrayFloat)); 
    /* Fill in sample array */
    for (i=0;i<17;i++) {
      memcpy((array3->storage + i * rowlen), wrk, 
	     rowlen * sizeof(NArrayFloat));
    }
    /* Fill in data array */
    memcpy(array2->storage,array1->storage, 
	   17 * rowlen * sizeof(NArrayFloat));

    /* Fill in x position array */
    memcpy(array4->storage,(array1->storage + 17 * rowlen), 
	   rowlen*sizeof(NArrayFloat));    
    break;
  case 1: /* Time binning */
    memcpy(wrk,(array1->storage + 19 * rowlen), rowlen * sizeof(NArrayFloat)); 
    /* Fill in sample array */
    for (i=0;i<17;i++) {
      memcpy((array3->storage + i * rowlen), wrk, 
	     rowlen * sizeof(NArrayFloat));
    }

    /* Fill in data array */
    memcpy(array2->storage,array1->storage, 
	   17 * rowlen * sizeof(NArrayFloat));

    /* Fill in x position array */
    memcpy(array4->storage,(array1->storage + 17 * rowlen), 
	   rowlen*sizeof(NArrayFloat));    
    break;
  case 2: /* Velocity */
  case 3: /* Scan */
    memcpy(wrk,(array1->storage + 18 * rowlen), ntch * sizeof(NArrayFloat));
    /* Fill in sample array */
    memcpy((array3->storage), wrk, ntch * sizeof(NArrayFloat));
    memcpy(wrk,(array1->storage + 19 * rowlen), ntch * sizeof(NArrayFloat));
    for (i=1;i<17;i++) {
      memcpy((array3->storage + i * ntch), wrk, ntch * sizeof(NArrayFloat));
    }
    /* Fill in data array */
    for (i=0;i<17;i++) {
      memcpy(wrk, (array1->storage + i * rowlen), ntch * sizeof(NArrayFloat));
      memcpy((array2->storage + i * ntch), wrk, ntch * sizeof(NArrayFloat));
    }

    /* Fill in x position array */
    memcpy(array4->storage,(array1->storage + 17 * rowlen), 
	   ntch*sizeof(NArrayFloat));
    break;
  default:
    Tcl_SetResult(interp,"Bad mode number",TCL_STATIC);
    ckfree(wrk);
    return TCL_ERROR;
  }

  /* Free work storage */
  ckfree(wrk);
  return TCL_OK;
}
int Mtx_Add(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj * objv[]) {
  Tcl_CmdInfo cmdInfo;
  NArray *array1,*array2;
  char * str1, * str2;
  int i, lstr;

  if (objv < 2) {
    Tcl_SetResult(interp,"Usage: mtx add NArray1 NArray2",TCL_STATIC);
    return TCL_ERROR;
  }

  /* Set up Narray1 */
  str1 = Tcl_GetStringFromObj(objv[0],&lstr);
  if (Tcl_GetCommandInfo(interp,str1,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str1,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array1 = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array1->closure.interp!= interp)
      || (array1->n_dims != 2)) {
    Tcl_AppendResult(interp,"\"",str1,"\""," must be a 2D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  /* Set up Narray2 */
  str2 = Tcl_GetStringFromObj(objv[1],&lstr);
  if (Tcl_GetCommandInfo(interp,str2,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str2,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array2 = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array2->closure.interp!= interp)
      || (array2->n_dims != 2)) {
    Tcl_AppendResult(interp,"\"",str2,"\""," must be a 2D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  if ((array1->dim_length[0] != array2->dim_length[0]) ||
      (array1->dim_length[1] != array2->dim_length[1]) ||
      (array1->length        != array2->length)) {
    Tcl_SetResult(interp,"Array dimensions do not match",TCL_STATIC);
    return TCL_ERROR;
  }

  for (i=0;i<array1->length;i++) {
    *(array1->storage + i) += *(array2->storage + i);
  }
  return TCL_OK;
}

int Mtx_Norm(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj * objv[]) {
  Tcl_CmdInfo cmdInfo;
  NArrayFloat val;
  NArray *array1, *array2;
  double scale;
  char * str1, * str2;
  int i, lstr;

  if (objc < 2) {
    Tcl_SetResult(interp,
		  "Usage: mtx normalize NArray1 NArray2 ?scale?",TCL_STATIC);
    return TCL_ERROR;
  }

  /* Get scale factor */
  scale = 1.0;
  if (objc >= 3) {
    if (Tcl_GetDoubleFromObj(interp,objv[2],&scale) != TCL_OK) return TCL_ERROR;
    if (scale == 0.0 ) {
      Tcl_SetResult(interp,"Supply nonzero scale factor.",TCL_STATIC);
      return TCL_ERROR;
    }
  }

  /* Set up Narray1 */
  str1 = Tcl_GetStringFromObj(objv[0],&lstr);
  if (Tcl_GetCommandInfo(interp,str1,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str1,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array1 = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array1->closure.interp!= interp)
      || (array1->n_dims != 2)) {
    Tcl_AppendResult(interp,"\"",str1,"\""," must be a 2D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  /* Set up Narray2 */
  str2 = Tcl_GetStringFromObj(objv[1],&lstr);
  if (Tcl_GetCommandInfo(interp,str2,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str2,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array2 = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array2->closure.interp!= interp)
      || (array2->n_dims != 2)) {
    Tcl_AppendResult(interp,"\"",str2,"\""," must be a 2D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  if ((array1->dim_length[0] != array2->dim_length[0]) ||
      (array1->dim_length[1] != array2->dim_length[1]) ||
      (array1->length        != array2->length)) {
    Tcl_SetResult(interp,"Array dimensions do not match",TCL_STATIC);
    return TCL_ERROR;
  }

  for (i=0;i<array1->length;i++) {
    if ((val = *(array2->storage + i)) < 1.0e-9) val = 1.0e-9;
    *(array1->storage + i) /= val;
    *(array1->storage + i) *= scale;
  }
  return TCL_OK;
}

/*
 *  mtx write <filename> <narray> <mode> ?header?
 */

int Mtx_Write(ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj * objv[]) {
  Tcl_CmdInfo cmdInfo;
  NArray * array;
  char * str1, * str2, * str3;
  int i, j, offset, lstr;
  FILE * fp;

  if (objc < 2) {
    Tcl_SetResult(interp,
		  "Usage:\nmtx write <narray> <filename> ?header?",
		  TCL_STATIC);
    return TCL_ERROR;
  }

  str1 = Tcl_GetStringFromObj(objv[0],&lstr);
  if (Tcl_GetCommandInfo(interp,str1,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str1,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array->closure.interp!= interp)
      || (array->n_dims != 2)) {
    Tcl_AppendResult(interp,"\"",str1,"\""," must be a 2D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  str2 = Tcl_GetStringFromObj(objv[1],&lstr);
  if ((fp = fopen(str2,"w")) == NULL) {
    Tcl_SetResult(interp,"ERR:\nCan't open file",TCL_STATIC);
    return TCL_ERROR;
  }

  if (objc >= 3) {
    str3 = Tcl_GetStringFromObj(objv[2],&lstr);
    fprintf(fp,"%s\n",str3);
  }

  for (i=0;i<array->dim_length[0];i++) {
    fprintf(fp,"# Detector %d\n",i);
    for (j=0;j<array->dim_length[1];j++) {
      offset = i * array->dim_length[1] + j;
      fprintf(fp,"%4d %10.2f\n",j,*(array->storage + offset));
    }
  }
  fclose(fp);

  
  return TCL_OK;
}

int Mtx_Print1D(ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj * objv[]) {
  Tcl_CmdInfo cmdInfo;
  NArray * array;
  int i,  offset;
  char output[65536], scratch[20], *str1;
  int len, maxidx, lstr;
  
  offset = 0;
  if (objc < 1) {
    Tcl_SetResult(interp,
		  "Usage:\nmtx print1d <narray> ?<offset>? ?<length>?",
		  TCL_STATIC);
    return TCL_ERROR;
  } 
  
  if (objc >= 2) {
    if (Tcl_GetIntFromObj(interp,objv[1],&offset)!=TCL_OK) return TCL_ERROR;
    if (offset < 0) {
      Tcl_SetResult(interp,"Specify offset > 0",TCL_STATIC);
      return TCL_ERROR;
    }
  }

  str1 = Tcl_GetStringFromObj(objv[0],&lstr);
  if (Tcl_GetCommandInfo(interp,str1,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str1,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array->closure.interp!= interp)
      || (array->n_dims != 1)) {
    Tcl_AppendResult(interp,"\"",str1,"\""," must be a 1D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  maxidx = array->dim_length[0];
  if (objc >= 3) {
    if (Tcl_GetIntFromObj(interp,objv[2],&len)!=TCL_OK) return TCL_ERROR;
    if (len <= 0) {
      Tcl_SetResult(interp,"Specify length >= 0",TCL_STATIC);
      return TCL_ERROR;
    }
    if ((len + offset) > array->dim_length[0]) {
      maxidx = array->dim_length[0];
    } else {
      maxidx = len + offset;
    }
  }


  memset(output,0,sizeof(output));
  sprintf(output,"%.0f",*(array->storage + offset));
  for(i=offset+1;i<maxidx;i++) {
    sprintf(scratch,",%.0f",*(array->storage+i));
    strcat(output,scratch);
  }
  Tcl_AppendResult(interp,output,(char *)NULL);
  return TCL_OK;
}

int Mtx_Rebin1D(ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj * objv[]) {
  Tcl_CmdInfo cmdInfo;
  NArray * array;
  int i, j, idx, offset, nbins, binsize, lstr;
  char output[65536], scratch[20];
  char * str1;
  float outarray[4096];
  int oidx, maxidx;

  offset = 0;
  binsize = 1;
  nbins = 1;
  if (objc < 1) {
    Tcl_SetResult(interp,
		  "Usage:\nmtx rebin <narray> ?<offset>? ?<nbins>? ?<binsize>?",
		  TCL_STATIC);
    return TCL_ERROR;
  } 
  
  if (objc >= 2) {
    if (Tcl_GetIntFromObj(interp,objv[1],&offset)!=TCL_OK) return TCL_ERROR;
    if (offset < 0) {
      Tcl_SetResult(interp,"Specify offset > 0",TCL_STATIC);
      return TCL_ERROR;
    }
  }

  str1 = Tcl_GetStringFromObj(objv[0],&lstr);
  if (Tcl_GetCommandInfo(interp,str1,&cmdInfo) != 1) {
    Tcl_AppendResult(interp,"Cannot get info for NArray ",
		     "\"",str1,"\"",(char *)NULL);
    return TCL_ERROR;
  }
  if (((array = (NArray*) (cmdInfo.clientData)) == NULL) 
      || (array->closure.interp!= interp)
      || (array->n_dims != 1)) {
    Tcl_AppendResult(interp,"\"",str1,"\""," must be a 1D NArray",
		     (char *)NULL);
    return TCL_ERROR;
  }

  maxidx = array->dim_length[0];
  if (objc >= 3) {
    if (Tcl_GetIntFromObj(interp,objv[2],&nbins)!=TCL_OK) return TCL_ERROR;
    if (nbins <= 0) {
      Tcl_SetResult(interp,"Specify nbins >= 0",TCL_STATIC);
      return TCL_ERROR;
    }
    /* Insert boundary logic here as well? */
    if (nbins > 4096) {
      Tcl_SetResult(interp,"Maximum number of bins is 4096",TCL_STATIC);
      return TCL_ERROR;
    }
  }

  if (objc >= 4) {
    if (Tcl_GetIntFromObj(interp,objv[3],&binsize)!=TCL_OK) return TCL_ERROR;
    if (binsize <= 0) {
      Tcl_SetResult(interp,"Specify binsize >= 0",TCL_STATIC);
      return TCL_ERROR;
    }
    /* Insert boundary logic here as well? */
  }

  maxidx = offset + nbins * binsize;
  if (maxidx >= array->dim_length[0]) {
    maxidx = array->dim_length[0] - 1;
  }

  /* Sum the data */
  memset(outarray,0,sizeof(outarray));
  idx = offset;
  for(i=0;i<nbins;i++) {
    for (j=0;j<binsize;j++) {
      outarray[i] += *(array->storage+idx);
      idx++;
      if (idx > maxidx) break;
    }
  }

  /* Print the result */
  memset(output,0,sizeof(output));
  sprintf(output,"%.0f",outarray[0]);
  for(i=1;i<nbins;i++) {
    sprintf(scratch,",%.0f",outarray[i]);
    strcat(output,scratch);
  }  

  Tcl_AppendResult(interp,output,(char *)NULL);
  return TCL_OK;
}

unsigned int linearIndex(int ind[], NArray *array){
  int i, lastDim = array->n_dims-1;
  int offset = ind[0];

  for (i=1; i <= lastDim; i++) {
    offset = ind[i] + array->dim_length[i]* offset;
  }
  return offset;
}

int Mtx_List(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj * objv[]) {
  int dim = objc-1;
  NArray * array;
  int * inds = (int*)calloc(sizeof(int),dim);
  char * str1, *str, ch, * list;
  int i, running, lstr;
  Tcl_CmdInfo cmdInfo;
  float cur;
  Tcl_Obj *optr;
  
  if (dim < 1) {
    Tcl_AppendResult(interp,"too few args: should be \"mtx list array ?n1 ?n2 .. x ?nk vector\"",NULL);
    return TCL_ERROR;
  }

  str1 = Tcl_GetStringFromObj(objv[0],&lstr);
  if (   (Tcl_GetCommandInfo(interp,str1,&cmdInfo) != 1) 
      || ((array = (NArray*) (cmdInfo.clientData)) == NULL)
      || (array->closure.interp!=interp)
      || (array->n_dims !=  dim) ) {
    Tcl_AppendResult(interp,"\"",str1,"\" must be an NArray, consistent with provided dimensions",NULL);
    return TCL_ERROR;
  }
  
  for (i=0; i<dim; i++) {
    str = Tcl_GetStringFromObj(objv[1+i],&lstr);
    ch = str[0];
    if ((ch == 'x') || (ch == 'X')){
      running=i;
    } else  if (Tcl_GetIntFromObj(interp, objv[1+i], &inds[i]) != TCL_OK) {
      ckfree(inds);
      return TCL_ERROR;
    } else if (inds[i] >= array->dim_length[i]) {
      Tcl_AppendResult(interp,"value of index, ",str,", exceeds the array dimension",NULL);
      return TCL_ERROR;      
    }
  }
  
  // Output will be a list

  optr = Tcl_NewListObj(0,NULL);

  inds[running]=0;
  for (i=0; i<array->dim_length[running]; i++) {
    inds[running]=i;
    cur = array->storage[linearIndex(inds,array)];
    Tcl_ListObjAppendElement(interp,optr,Tcl_NewDoubleObj(cur));
  }
  Tcl_SetObjResult(interp,optr);
  return TCL_OK;
}


/*
 * Reassign the storage of the indicated narray to our shared memory segment
 */
int Tcl_Mtx(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj * objv[]) {

  int len;
  char * str;

  if (objc < 2) {
    Tcl_SetResult(interp,"Usage: mtx <options>",TCL_STATIC);
    return TCL_ERROR;
  }
  
  str = Tcl_GetStringFromObj(objv[1],&len);
  if (!strncmp("split",str,len)) {
    return Mtx_Split(clientData,interp,objc-2,objv+2);
  } else if (!strncmp("add",str,len)) {
    return Mtx_Add(clientData,interp,objc-2,objv+2);
  } else if (!strncmp("normalize",str,len)) {
    return Mtx_Norm(clientData,interp,objc-2,objv+2);
  } else if (!strncmp("write",str,len)) {
    return Mtx_Write(clientData,interp,objc-2,objv+2);
  } else if (!strncmp("print1d",str,len)) {
    return Mtx_Print1D(clientData,interp,objc-2,objv+2);
  } else if (!strncmp("rebin",str,len)) {
    return Mtx_Rebin1D(clientData,interp,objc-2,objv+2);
  } else if (!strncmp("list",str,len)) {
    return Mtx_List(clientData,interp,objc-2,objv+2);
  } else if (!strncmp("version",str,len)) {
    Tcl_AppendResult(interp,rcsid,(char *) 0);
    return TCL_OK;
  } else {
    Tcl_SetResult(interp,"Options: split add normalize write",TCL_STATIC);
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
 *  Tcl Wrapper: automatically called by the package loader
 *               The library name is capitalized.
 */

int Mtxutil_Init(Tcl_Interp *interp) {

  Tcl_CreateObjCommand(interp,"mtx",(Tcl_ObjCmdProc *) Tcl_Mtx, 
		       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  return TCL_OK;
}
