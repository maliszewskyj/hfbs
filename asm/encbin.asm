/***************************************************************************/
/*                                                                         */
/*   Name:  hfbs.asm                                                       */
/*                                                                         */
/*   RSS SIS9200 DSP code                                                  */
/*                                                                         */
/*   Author: MKI                                                           */
/*                                                                         */
/*   Date: 08.06.2003                                                      */
/*   Date: 21.08.2003                                                      */
/*                                                                         */
/*                                                                         */
/* ----------------------------------------------------------------------- */
/*                                                                         */
/*                                                                         */
/*  Copyright (c) 2003 SIS GmbH 22399 Hamburg.  All rights reserved.       */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

/* header Dateien */	
#include "..\header\def21060.h"
#include "..\header\s3100def.h"

/* */
#define IRQ1_VECTOR_ADDR 0x0002001C
			
/* --- Defines fuer die SHARC-Adressen und Befehle ----------------------- */
#define SYSCON  0x00    
#define WAIT    0x02

// Sharc Local defines
#define LOC_SEQ_STATUS 				        0x01000000


// Sharc VME MAster defines
#define VME_DATA_FIFO_READ 			    	0x40000000
#define VME_ONLY_ARBITRATION_CYCLE	    	        0x40080000
#define VME_ONLY_RELEASE_MASTER_CYCLE		        0x40000000
#define VME_DMA_CYCLE					0x40040000
#define VME_SINGLE_CYCLE				0x40020000
#define INTERNAL_SINGLE_CYCLE				0x40030000

#define READ_FIFO_DISABLE 				0x00800000
#define READ_FIFO_CLR_DISABLE   			0x00400000
#define WORD_COUNT_CLR_DISABLE  			0x00200000    
#define VME ADDR_INC_DISABLE 				0x00100000

#define VME_HOLD_MASTER				        0x00080000
#define VME_WRITE					      0x40
#define VME_AM_F				      	      0x0F
#define VME_AM_B				      	      0x0B
#define VME_AM_9				      	      0x09

#define SEQ_CONTROL				        0x20000000
#define VME_WRITE_DATA_REG				0x20000001
#define VME_DMA_MAX_WORDCOUNT				0x20000002
#define VME_SEQ_FIFO_FLAGS				0x20000004
#define VME_READ_DATA_REG				0x20000008


// Sharc SDRAM defines
#define SDRAM_BLT_BASE		 			0x80000000
#define SDRAM_SGL_BASE		 			0xC0000000
#define SDRAM_RD_ADD_WR_BASE 			   	0xE0000000
#define SDRAM_RING_BASE                                 0xC0800000
	
#define SHARCIO 0x01000004

// SIS3801 defines
# define SIS3801_STAT_REG 			0x0        /* read        */
# define SIS3801_CTRL_REG 			0x0        /* write       */
# define SIS3801_IRQ_REG 			0x4        /* read/write  */
# define SIS3801_COPY_DIS_REG 			0xC        /* write only  */
# define SIS3801_WRITE_TO_FIFO 			0x10       /* write only  */
# define SIS3801_KEY_FIFO_CLR 			0x20       /* write only  */
# define SIS3801_KEY_NEXT_CLK			0x24       /* write only  */
# define SIS3801_KEY_CNT_EN 			0x28       /* write only  */
# define SIS3801_KEY_CNT_DIS 			0x2C       /* write only  */  
# define SIS3801_KEY_REF_EN 			0x50       /* write only  */   
# define SIS3801_KEY_REF_DIS 			0x54       /* write only  */   
# define SIS3801_KEY_RESET 		        0x60       /* write only  */ 
# define SIS3801_KEY_TEST_CLK 		        0x68       /* write only  */  
# define SIS3801_PRESCALER_FACTOR               0x80       /*             */   
# define SIS3801_READ_FIFO	               0x100       /*             */
# define SIS3801_MODULE1                  0x43508000       /*             */
# define SIS3600_MODULE1                  0x43509000
# define SIS3600_CTRL_REG                        0x0
# define SIS3600_READ_FIFO                     0x100
# define SIS3600_CTRL_PULSERON                  0x10

#define INTRAM 0x400000

#define VRESULT      INTRAM+0x800    
#define FLAGIRQB     INTRAM+0x801    
#define NOFIRQ       INTRAM+0x802     
#define IRQSTAT      INTRAM+0x803
#define ASEQSTAT     INTRAM+0x804
#define MONOFF       INTRAM+0x805
#define DETOFF       INTRAM+0x806
#define EXTRA        INTRAM+0x807
#define VADM1        INTRAM+0x810
#define VADM2        INTRAM+0x811
#define LATCHRAW     INTRAM+0x812
#define LATCHVAL     INTRAM+0x813
#define LATCHMSK     INTRAM+0x814
#define MONVAL       INTRAM+0x815
#define LODVAL       INTRAM+0x816
#define HIDVAL       INTRAM+0x817
#define PTROFF1      INTRAM+0x820   /* $2080 monitor write ptr offset   */
#define PTROFF2      INTRAM+0x821   /* $2084 detector write ptr offset  */
#define PTROFF3      INTRAM+0x822   /* $2088  offset between 2 rd ptrs  */
#define PTROFF4      INTRAM+0x823   /*                                  */

#define ERRORFLAG    INTRAM+0x830

/* following buffers have 0x1000 size */
#define LATCH_LOOK   INTRAM+0x1000   /* lookup table for detectors        */
#define RING_BUFF_M  INTRAM+0x2000   /* ring buffer for detector lookup   */
#define RING_BUFF_D  INTRAM+0x3000   /* ring buffer for detector lookup   */
#define VADC_LOOK    INTRAM+0x4000   /* Velocity lookup table             */

/* histogram storage */
#define MON_H_BASE   SDRAM_RD_ADD_WR_BASE+0x20000 /* monitor detector  */
#define LOW_H_BASE   SDRAM_RD_ADD_WR_BASE+0x22000 /* low angle dets    */
#define HIG_H_BASE   SDRAM_RD_ADD_WR_BASE+0x27000 /* high angle dets   */
#define HPV_BASE     SDRAM_RD_ADD_WR_BASE+0x33000	

/* large lookup tables */
#define TIME_LOOK    INTRAM+0x10000 /* time channel lookup table        */
#define PTR_LOOK1    INTRAM+0x14000 /* mon pointer offset lookup table  */
#define PTR_LOOK2    INTRAM+0x24000 /* pointer offset lookup table      */
		
/*********************************************************************/
/* Macros                                                            */	
/*********************************************************************/
#define nimpulse1 \
	BIT CLR ASTAT 0x80000;  \
	BIT SET ASTAT 0x80000 	; 

/* ECL pulse macro */
#define ecl1pulse \
	R15 = 1; \
	DM(LOC_IO_OUT_PULSE) = R15;
	
/* tst macro */
#define tst(TESTREG) \
	R15=0 			; \
	COMP(TESTREG,R15) 	 

// incl macro
#define incl(MEMLOC) \
	R15=DM(MEMLOC) ; \
	R15=R15+1 ; \
	DM(MEMLOC)=R15 ;

// vme single write macro
#define vmew(ADDRESS,DATUM,LABEL) \
	R15=DATUM ; \
	DM(VME_WRITE_DATA_REG)=R15 ; \
	R15=ADDRESS ; \
	DM(VME_SINGLE_CYCLE+VME_WRITE+VME_AM_9)=R15 ; \
LABEL : \
	R15 = DM(LOC_SEQ_STATUS) ;\
	BTST R15 BY 0 ; \
	IF NOT SZ JUMP (PC,LABEL) ;
	
// vme single read macro
#define vmer(ADDRESS,DATUM,LABELR) \
	R15=ADDRESS ; \
	DM(VME_SINGLE_CYCLE+VME_AM_9)=R15 ; \
LABELR : \
	R15 = DM(LOC_SEQ_STATUS) ;\
	BTST R15 BY 0 ; \
	IF NOT SZ JUMP (PC,LABELR) ; \
	DATUM=DM(VME_READ_DATA_REG)	;
	

.SEGMENT/DM     seg_dm1;  // max. 4k x 32
// input Status 
.var INT0ST 			; 
.var D5H			; 
.var SAVER6			; 
.endseg;

/* ------------------------------------------------------------------------ */
.SEGMENT/PM     seg_pm1;
			
Start:
	JUMP (PC,IRQSETUP)	; 
Mirrow_IRQ1_Vector:
	JUMP (Irq_Service_IRQ1);
	RTI; 
IRQSETUP:	
	M13=1		;
	I13=Mirrow_IRQ1_Vector;
	I14=IRQ1_VECTOR_ADDR	;
	/* copy 2 lines */
	LCNTR=0x2,DO(PC,2) UNTIL LCE;
	PX=PM(I13,M13)	;
	PM(I14,M13)=PX	;

	R6=0x2004EAA5 ;    
	nop ;
	DM(WAIT)=R6  ;  
	nop ;
	BIT SET MODE2 0x78000; /* Flag 3 .. Flag 0 sind outputs */ 
	BIT CLR ASTAT 0x80000 ; /* clr Flag 0 ; Led an */

	I1=0x2		;
	DM(INTERNAL_SINGLE_CYCLE_WR,I1)=R0;
	R15=0xff00		;
	DM (LOC_IO_IN_REG)=R15;  /* clear latched input bits */
	BIT SET IMASK IRQ1I	;    /* enable IRQ1 on SHARC (inputs) */
	BIT SET MODE2 IRQ1E	;    /* set IRQ1 to edge sensitive */
	BIT SET MODE1 IRPTEN;    /* enable IRQs on SHARC */
	I1=0x0102           ;    /* FLAT 1 */
	DM(INTERNAL_SINGLE_CYCLE_WR,I1)=R0;
	
LOOP_START:

// clear SDRAM

	I1 = SDRAM_SGL_BASE ; /* SDRAM Single Cycle or LAST BLT Cycle */
	M1 = 0x1 ;      
	R1 = 0x0 ;
	R2 = 0x1000000	;  /* lowest 16 MByte */

/* preload all ring buffers with base         */
/* (presets corresponding I register as well) */	
        B3=RING_BUFF_M  ;
        B5=RING_BUFF_D  ;
        B6=RING_BUFF_M  ;
        B7=RING_BUFF_D  ;
/******************************************************/	
	M0=4096		;
	M1=1		; 
	M2=0		; 
	M3=1		;
	M4=0		; 
	M5=1		;
	M6=1		;
	M7=1		;
/******************************************************/	
	L0=0		;
	L1=0		;
	L2=0		;
	L3=4096		;
	L4=0		;
	L5=4096		;
	L6=4096		;
	L7=4096         ;
	L8=0		;
	L9=0		;
	L10=0		;
	L11=0		;
	L12=0		;
	L12=0		;
	L14=0		;
	L15=0		;
/*********************************************************************/
/*   	Register Constants                                           */
/*********************************************************************/
	R8  =  SIS3801_MODULE1 + SIS3801_STAT_REG  ;  // address
	R9  =  SIS3600_MODULE1 + SIS3600_READ_FIFO ;  // address
 	R10 =  SIS3801_MODULE1 + SIS3801_READ_FIFO ;  // address	
/******************************************************/	
/* clear ring buffers                                 */
/******************************************************/	
	LCNTR=4096, DO (PC,2) UNTIL LCE ; 
	DM(I3,M3)=0	; 
	DM(I5,M5)=0	; 
/******************************************************/	
/* position pointers                                  */
/******************************************************/
/* distance between 2nd and 3r read pointer           */
	// M7=1			; 
	M7=DM(PTROFF3)	; 
/* position write pointer for monitor                 */
	// M3=114		; 
	M3=DM(PTROFF1)	;
	R1=DM(I3,M3) 	; /* dummy read */
	M3=1		; 

/* position write pointer for detector                */
	// M5=340		; 
	M5=DM(PTROFF2)	;
	R1=DM(I5,M5) 	; /* dummy read */
	M5=1		; 

	
/* preload shadow debugging locations with zero */
	R15=0;
	DM(MONOFF)=R15  ;
	DM(DETOFF)=R15  ;
	DM(EXTRA)=R15   ;	
	DM(VADM1)=R15   ;
	DM(VADM2)=R15   ;
	DM(LATCHRAW)=R15;
	DM(LATCHVAL)=R15;
	DM(LATCHMSK)=R15;
	DM(MONVAL)=R15  ;
	DM(LODVAL)=R15  ;
	DM(HIDVAL)=R15  ;
	DM(NOFIRQ)=R15  ; // Initialize heartbeat counter
	DM(FLAGIRQB)=R15	;
	DM(ERRORFLAG)=R15	;

// preload D5.L in case first latch read is invalid 
	R5=0;

/* switch on user led (red) */
	BIT CLR ASTAT 0x80000 ; /* clr Flag 0 ; Led an */
	
#ifdef STATLATCH
/* start SIS3600 pulse generator */
	R0 = SIS3600_CTRL_PULSERON ; 
	DM(VME_WRITE_DATA_REG)=R0 ; 
	R0 =  SIS3600_MODULE1 +  SIS3600_CTRL_REG ;  // address
	DM (VME_SINGLE_CYCLE + VME_WRITE + VME_AM_9) = R0 ;
/* clear R0 for use as IRQ counter */
	R0=0				; 
	CALL Wait_On_VME_Busy ;   /* R15 is used */
#endif

/* first IRQ, scaler start */
WORKLOOP1:
	tst(R0)                  ;  /* IRQA set? */
	IF EQ JUMP(PC,WORKLOOP1) ;
	R0=R0-1                  ;  /* decrement IRQ flag */

/* second IRQ, first CIP */        
WORKLOOP2:
	tst(R0)                  ;  /* IRQA set? */
	IF EQ JUMP(PC,WORKLOOP2) ;
	R0=R0-1                  ;  /*decrement number of pending IRQs */

/* third IRQ first full read of all VME components            */
/* data are coming into the COM_FIFO and evaluated from the   */
/* fourth IRQ on                                              */
PRELOOP:
	tst(R0)                  ;  /* IRQA set? */
	IF EQ JUMP(PC,PRELOOP)   ;
/* start after IRQ */
	incl(NOFIRQ)             ;
	R0=R0-1                  ;  /* decrement number of pending IRQs */

/* read SIS3600/STR7137 latch */
	R15=SIS3600_MODULE1+SIS3600_READ_FIFO ;
	DM(VME_SINGLE_CYCLE+VME_AM_9)=R15; 
	CALL Wait_On_VME_Busy       ;

/* read SIS3801/STR7201 scaler */
	R15=19;                                  /* number of channels to read */
	DM(VME_DMA_MAX_WORDCOUNT)=R15;           /* remains unchanged on BLT   */

	R15=SIS3801_MODULE1+SIS3801_READ_FIFO ;
	DM(VME_DMA_CYCLE+READ_FIFO_CLR_DISABLE+VME_AM_B)=R15; 
	CALL Wait_On_VME_Busy       ;
/*  fourth and higher IRQs -> standard readout from now on */
/*  VME and evaluation of COM_FIFO data                    */
	
WORKLOOP:
	tst(R0)                 ; /* IRQA set? */
	IF EQ JUMP(PC,WORKLOOP) ;

/* start after IRQ */
	incl(NOFIRQ) ; /* increment IRQ counter            */
	R0=R0-1      ; /* decrement number of pending IRQs */

//	R12=DM(VME_SEQ_FIFO_FLAGS)      ;
//	DM(SDRAM_SGL_BASE+1)=R12		;

//      ecl1pulse ;
//      vmer(SIS3600_MODULE1,R10,FIFOLVL) ;
//	DM(SDRAM_SGL_BASE+3)=R10 ;

/* read SIS3600/STR7137 latch */
	R15=SIS3600_MODULE1+SIS3600_READ_FIFO ;
	DM(VME_SINGLE_CYCLE+READ_FIFO_CLR_DISABLE+VME_AM_9)=R15; 
	
/* get data from last read */	
	R3=DM(VME_DATA_FIFO_READ); /* read Data from FIFO */
	DM(LATCHRAW)=R3;
	BTST R3 BY 15 ; 
	IF NOT SZ JUMP (PC,NOUPDATE) ; 
	R7=0xFFF;
	R3=R3 AND R7;    /* pass only bits 0-11 */
	DM(LATCHVAL)=R3;
	M3=R3;
	I0=LATCH_LOOK		; 
	R5=DM(M3,I0);
	DM(LATCHMSK)=R5;
NOUPDATE:	
/* wait for completion of new read	*/
MCMDLATCH:
	R15 = DM(LOC_SEQ_STATUS) ;
	BTST R15 BY 0 ; 
	IF NOT SZ JUMP (PC,MCMDLATCH) ; 

/* read SIS3801/STR7201 scaler */
	R15=19;                              /* number of channels to read */
	DM(VME_DMA_MAX_WORDCOUNT)=R15;       /* remains unchanged on BLT   */
	
	R15=SIS3801_MODULE1+SIS3801_READ_FIFO ;
	DM(VME_DMA_CYCLE+READ_FIFO_CLR_DISABLE+VME_AM_B)=R15; 
		
	M0=4096;  /* reload M0 */

/* monitor counter ======================================================== */

	
	R6=DM(I6,M6)		; 
	DM(MONOFF)=R6		; 

	R3=DM(VME_DATA_FIFO_READ) ; 
	DM(MONVAL)=R3		; 
	R4=MON_H_BASE		; /* Get base address */
	R6=R6+R4		; /* Add base + pointer */
	I0=R6			; /* Set pointer */
	
	// Test PST masking
	R5=DM(LATCHMSK)		; 
	BTST R5 BY 0		; 
	IF SZ JUMP (PC,NOMONHIST) ; 

	DM(I0,M0)=R3 		; /* Increment histogram */
	// Increment monitor interval counter
	R4=1			; 
	DM(I0,0)=R4		; 
NOMONHIST:

/* low angle detectors ==================================================== */
	R2=LOW_H_BASE		;
	NOP			;
	M7=DM(PTROFF3)          ;
	NOP			; 
	R6=DM(I7,M7)		; /* load next ptr, increment by ptr diff */
	NOP			; 
	R6=R2+R6		; /* Add base + pointer */
	I0=R6                   ; /* Set pointer */
	M0=4096			; /* Reload M0 */

	// Check to see whether low angle detectors are masked	
	R5=DM(LATCHMSK)		; 
	BTST R5 BY 1            ; 
	IF SZ JUMP (PC,NOLOWHIST);

	LCNTR=0x4,DO(PC,3) UNTIL LCE;
	R3=DM(VME_DATA_FIFO_READ)       ; /* get scaler contents */
	DM(LODVAL)=R3		        ; 
	DM(I0,M0)=R3 		        ; /* update, postinc          */

	/* Increment low angle interval counter */	
	R4=1			;
	DM(I0,0)=R4		; 
	JUMP(PC,LOWHIST)	; 		
NOLOWHIST:
	LCNTR=0x4,DO(PC,3) UNTIL LCE;
	R3=DM(VME_DATA_FIFO_READ)       ; /* get scaler contents */
	DM(LODVAL)=R3		        ;
	NOP			; 
LOWHIST:			

/* high angle detectors ===================================================*/	
	R2=HIG_H_BASE		;
	NOP			;
	R6=DM(PTROFF3)		;
	R4=0			; 
	R4=R4-R6		; 
	M7=R4			; 
	R6=DM(I7,M7)		; /* Get pointer, decrement back */
	NOP			;
	R6=R2+R6		;
	I0=R6			; /* Set pointer */
	M0=4096			; /* Reload M0 */
	DM(DETOFF)=R6		; 

	// Check to see whether high angle detectors are masked	
	R5=DM(LATCHMSK)		; 
	BTST R5 BY 2		; 
	IF SZ JUMP (PC,NOHIGHIST) ;

	LCNTR=12,DO(PC,3) UNTIL LCE ;
	R3=DM(VME_DATA_FIFO_READ)       ; /* get scaler contents */
	DM(HIDVAL)=R3		        ; /* store */
	DM(I0,M0)=R3 		        ; /* read, add, store, postinc */ 

	/* Increment high angle interval counter */
	R4=1			; 
	DM(I0,0)=R4		; 
	
	JUMP(PC,HIGHIST)	; 
NOHIGHIST:
	LCNTR=12,DO(PC,3) UNTIL LCE ;
	R3=DM(VME_DATA_FIFO_READ) ; /* get scaler contents */
	DM(HIDVAL)=R3		  ; /* store */
	NOP			; 
HIGHIST:			

	R6=DM(I7,M1)		; /* Advance detector ring buffer */

/* wait for new read to be completed */
        CALL Wait_On_VME_Busy       ;


/*  Work out ring buffer pointers ========================================== */

	
	
/* We have encoder signals fed to the scaler */
/* Read index and quadrature signal */
	R4=DM(VME_DATA_FIFO_READ)       ; /* get scaler contents */
	tst(R4)			; 
	IF GT JUMP(PC,ENCZERO)	; 
	R4=DM(VADM1)		; 
	JUMP(PC,ENCINC)		; 
ENCZERO:	
	R4=0			; 
ENCINC:	
	R3=DM(VME_DATA_FIFO_READ)    ; /* get scaler contents */
	R4=R4+R3		     ; /* add new counts to old */ 
	R2=4095			     ; 
	R2=R2-R4		     ;
	tst(R2)			     ; 
	IF LT JUMP(PC,ENCCEILING)   ; 
	JUMP(PC,ENCFINISH)	     ; 
ENCCEILING:
	R4=4095			; 
	incl(EXTRA)		; /* Keep track of number of revolutions */ 
ENCFINISH:	
	DM(VADM1)=R4		; 

/* Now we have the current encoder count in R4 */

/******************************************************************* */
/* This is where we do two lookups: flight time offset correction    */
/*                                  energy/velocity channel          */
/******************************************************************* */

	I0=TIME_LOOK		; 
	M0=R4;
	
//	R15=DM(I0,M0);       /* dummy read to position pointer */
//	R6=DM(I0,M2); 		/* Now have energy value */
	R6=DM(M0,I0)		;
	NOP			; 

//	R6=R4			    ; /* Short circuit the lookup */
	DM(VADM2)=R6	            ; /* Store it */
	I0=PTR_LOOK1		        ; 
	R15=DM(I0,M0)               ; /* dummy read, position pointer */
	R4=DM(I0,M2)                ;
	I0= PTR_LOOK2		        ; 
	R4=0	                    ; /* Set offset to zero for now */
	M3=R4                       ;
	R15=DM(I0,M0)               ; /* dummy read, position pointer */
	R4=DM(I0,M2)                ;
	R4=0	                    ; /* Set offset to zero for now */
	M5=R4                       ;
	R15=DM(I3,M3)               ; /* dummy read, position pointer */
	DM(I3,M1)=R6                ; /* write */
	DM(I3,M2)=R6                ; /* write to next position also */
	R15=DM(I5,M5)               ; /* dummy read, position pointer */
	DM(I5,M1)=R6                ; /* write */
	DM(I5,M2)=R6                ; /* write to next position also */


/* error checking */	
	R15 = DM(LOC_SEQ_STATUS) ; 
	BTST R15 BY 1 ; 
	IF SZ JUMP (PC,NOERRORLABEL) ; 
	DM(ERRORFLAG)=R15		; 
	R15=2			; 
	nimpulse1 		; 
	DM(SEQ_CONTROL)=R15 	; 
NOERRORLABEL:  

	ecl1pulse		;
	

	R15=DM(NOFIRQ)		;
	R12=1000	;
	COMP(R12,R15)		;

	JUMP(PC,WORKLOOP)	; 

/* IRQ service routine */
Irq_Service_IRQ1:	
	R0=R0+1	                ; /* increment IRQ counter     */
	R14=DM(LOC_IO_IN_REG)	; /* get latched signals       */
	DM(LOC_IO_IN_REG)=R14	; /* clear all latched signals */
	BIT CLR ASTAT 0x100000  ;  
	BIT SET ASTAT 0x100000 	; 
	RTI			;
	
	
	.ENDSEG;
/* --------------------------------------------------------------------------
        SUBROUTINES
------------------------------------------------------------------------- */
.SEGMENT/PM     seg_pm2;

/* subroutine */
Wait_On_VME_Busy:
	R15 = DM (LOC_SEQ_STATUS)  ;     	 
	BTST R15 by 0	;	
	IF NOT SZ JUMP (PC,Wait_On_VME_Busy) ;  
	RTS ;

	.ENDSEG;
	
























