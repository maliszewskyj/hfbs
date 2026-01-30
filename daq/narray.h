#ifndef narray_h
#define narray_h

/* $Id: narray.h,v 1.1.1.1 1999/12/16 21:50:49 nickm Exp $ */

/*
 * This software is copyright (C) 1994 by the Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "tcl.h"

#define NARRAY_VERSION "0.81"

typedef float NArrayFloat;

typedef struct _NArray NArray;

#define NARRAY_MAX_BOUND_ARRAYS 8  /* maximum number of array's that
                                    * can be bound to names
                                    */


/* Vars and arguments to functions can be numbers or strings */
typedef enum {
    NARRAY_TYPE_FLOAT = 0, NARRAY_TYPE_STRING
} NArrayOperandType;

typedef NArrayFloat NArrayVar;

typedef struct {
    NArrayOperandType type;
    union {
	NArrayFloat n;
	char* s;
    } value;
} NArrayOperand;

typedef struct {
    char* id;			/* the name of the id */
    int flags;			/* NARRAY_SLOT_* flags */
} NArrayIdSlot;

#define NARRAY_SLOT_IN_USE   1	/* is the slot in use? */
#define NARRAY_SLOT_VARIABLE 2	/* is it holding a variable? */
#define NARRAY_SLOT_STRING   4	/* is it holding a string? */

/* Closure's hold information about the execution context of a Code
 * sequence.  Part of this information is copied from CompileState
 * when the compiler finishes.
 */

typedef struct {
    Tcl_Interp* interp;		/* Tcl interpreter */
    NArrayVar* vars;		/* the values of the id's in id_table
				 * this array is kept in sync with id_table
				 */
    NArrayIdSlot* id_table;	/* identifiers and strings referenced in
				 * code
				 */
    int alloced_ids;		/* the length of id_table */
    NArrayFloat* double_table;	/* floats that are referenced in the code
				 * are stored here
				 */
    int n_doubles;		/* the # of floats stored */
    int alloced_doubles;	/* the length of double_table */
    struct {			/* names of arrays in current context */
	char* name;
	NArray* array;
    } array_table[NARRAY_MAX_BOUND_ARRAYS];
    int n_arrays;		/* # of names */
} NArray_Closure;

struct _NArray {
    int n_dims;			/* # of dimensions */
    int length;			/* length of storage */
    NArrayFloat* storage;
    int debug;			/* DEBUG_* flags */
    char* errmsg;		/* error messages from code eval's */
    NArray_Closure closure;
    int dim_length[1];		/* this struct is extended so dim_length
				 * works
				 */
};


/* NArray_Fn's return 0 on success, otherwise a descriptive error
 * message.
 */


typedef char* (*NArray_Fn)(int argc, int* p_sp, NArrayOperand* stack,
			   Tcl_Interp* interp);

void NArray_CreateFn(char* name, NArray_Fn fn);
extern int NArray_Init(Tcl_Interp* interp);

#endif
