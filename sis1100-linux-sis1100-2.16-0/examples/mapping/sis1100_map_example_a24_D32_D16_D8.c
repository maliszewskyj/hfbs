#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "../../dev/pci/sis1100_map.h"
#include "../../dev/pci/sis1100_var.h"

int main(void) {

int pv;              /* path for VME calls */
int pc;              /* control (errorhandling) */
int i;

struct sis1100_ctrl_reg reg;
char*      vme_base; /* pointer for VME mappining */
/*u_int32_t* ctl_base;*/ /* pointer for register base (PCI-card) */

/* open */
pv=open("/dev/sis1100_00remote", O_RDWR, 0);
pc=open("/dev/sis1100_00ctrl", O_RDWR, 0);
if ((pv<0) || (pc<0)) {
    printf("error on opening\n");
    return -1;
}


vme_base=mmap(0, 0x1000000, PROT_READ|PROT_WRITE, MAP_SHARED, pv, 0); 	/* 16 MByte page offset */
if (vme_base==MAP_FAILED) {  
    printf("mapping VME failed\n");
    return -1;
}


/* fill descriptor */
/*
Example:
1 module (memory) with the A24-baseaddresses 0x0

*/
u_int32_t descr[4]={
    0xff010800, /* header */
    0x39,        /* A24 */
    0x000000,   /* Window size A23-A22 offset (4 MByte); A21-A01 mapp address: 0x800000 - 0xBFFFFF  (refer to manual 'gigalinkenglish112', page 29) */
    0,
};

reg.offset=0x400;
for (i=0; i<4; i++) {
    reg.val=descr[i];
    ioctl(pc, SIS1100_CTRL_WRITE, &reg);
    reg.offset+=4;
}


/* help spointer */
char* start1=vme_base;

u_int32_t* base_I_A24D32_memory_ptr    = (u_int32_t *)(start1); /* D32 cycle pointer */
u_int16_t* base_I_A24D16_memory_ptr    = (u_int16_t *)(start1); /* D16 cycle pointer */
u_int8_t*  base_I_A24D8_memory_ptr     = (u_int8_t *)(start1);  /* D8 cycle pointer */

unsigned int write_data_d32 = 0 ;
unsigned int read_data_d32  ;
unsigned short write_data_d16 = 0 ;
unsigned short read_data_d16  ;
unsigned char write_data_d8 = 0 ;
unsigned char read_data_d8  ;

do {


#ifdef not_used
   for (i=0;i<10; i++) {
   	*base_I_A24D32_memory_ptr = write_data_d32 ;     /* write to Memory A24/D32 */
  	usleep(100000);
   	read_data_d32 =  *base_I_A24D32_memory_ptr ;
   	printf("vme read_d32 0x%08x\n", read_data_d32); /* write from Memory A24/D32 */
  	usleep(200000);
	write_data_d32++; 
   }
#endif

/*******************************************************************************/
/*  write with A24/D32, read with A24/D32, A24/D16, A24/D8                     */
   printf("\n");
   printf("write with A24/D32, read with A24/D32, A24/D16, A24/D8 \n");
   printf("\n");

   write_data_d32 = 0x12345678 ;
   *(base_I_A24D32_memory_ptr + 0) = write_data_d32 ;     /* write to Memory addr 0x0 with A24/D32; A21-A01 mapp address as long  */
   printf("vme write_d32 0x%08x\n", write_data_d32);
   write_data_d32 = 0x11223344 ;
   *(base_I_A24D32_memory_ptr + 1) = write_data_d32 ;     /* write to Memory addr 0x4 with A24/D32  */
   printf("vme write_d32 0x%08x\n", write_data_d32);  
   write_data_d32 = 0x55667788 ;
   *(base_I_A24D32_memory_ptr + 2) = write_data_d32 ;     /* write to Memory addr 0x8 with A24/D32  */
   printf("vme write_d32 0x%08x\n", write_data_d32); 
   write_data_d32 = 0x99AABBCC ;
   *(base_I_A24D32_memory_ptr + 3) = write_data_d32 ;     /* write to Memory addr 0xC with A24/D32  */
   printf("vme write_d32 0x%08x\n", write_data_d32); 
   printf("\n");

   for (i=0;i<4; i++) {
   	read_data_d32 =  *(base_I_A24D32_memory_ptr + i) ;
   	printf("vme read_d32 0x%08x\n", read_data_d32); /* read from Memory A24/D32 */
   }
   printf("\n");
   for (i=0;i<8; i++) {
   	read_data_d16 =  *(base_I_A24D16_memory_ptr + i) ;
   	printf("vme read_d16 0x%04x\n", read_data_d16); /* read from Memory A24/D16 */
   }
   printf("\n");
   for (i=0;i<16; i++) {
   	read_data_d8 =  *(base_I_A24D8_memory_ptr + i) ;
   	printf("vme read_d8 0x%02x\n", read_data_d8); /* read from Memory A24/D8 */
   }
   printf("\n");
   printf("\n");

/*******************************************************************************/
/*  write with A24/D16, read with A24/D32                                      */
   printf("\n");
   printf("write with A24/D16, read with A24/D32 \n");
   printf("\n");
   write_data_d16 = 0x0011 ;
   for (i=0;i<8; i++) {
   	*(base_I_A24D16_memory_ptr + i) = write_data_d16 ;     /* write to Memory addr (i * 0x2) with A24/D16  */
   	printf("vme write_d16 0x%04x\n", write_data_d16);  
	write_data_d16 = write_data_d16 + 0x2222 ;
   }
   printf("\n");


   for (i=0;i<4; i++) {
   	read_data_d32 =  *(base_I_A24D32_memory_ptr + i) ;
   	printf("vme read_d32 0x%08x\n", read_data_d32); /* read from Memory A24/D32 */
   }
   printf("\n");

/*******************************************************************************/

/*  write with A24/D8, read with A24/D32                                      */
   printf("\n");
   printf("write with A24/D8, read with A24/D32 \n");
   printf("\n");
   write_data_d8 = 0x00 ;
   for (i=0;i<16; i++) {
   	*(base_I_A24D8_memory_ptr + i) = write_data_d8 ;     /* write to Memory addr (i * 0x1) with A24/D8  */
   	printf("vme write_d8 0x%02x\n", write_data_d8);  
	write_data_d8 = write_data_d8 + 0x01 ;
   }
   printf("\n");


   for (i=0;i<4; i++) {
   	read_data_d32 =  *(base_I_A24D32_memory_ptr + i) ;
   	printf("vme read_d32 0x%08x\n", read_data_d32); /* read from Memory A24/D32 */
   }
   printf("\n");

/*******************************************************************************/


} while (0) ;



 return 0;
}
