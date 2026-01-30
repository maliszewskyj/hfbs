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
u_int32_t* ctl_base; /* pointer for register base (PCI-card) */

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

/* mmap 4 KByte control space (weniger als eine page geht nicht) */
ctl_base=mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, pc, 0);

if (ctl_base==MAP_FAILED) {
    printf("mapping CTL failed\n");
    return -1;
}


/* descriptoren fuellen */
/*
Example:
4 modules with the A32-baseaddresses 0x0, 0x10000000, 0x20000000, .. 0x30000000

*/
u_int32_t descr[16]={
    0xff010800, /* header */
    0x09,       /* A32 non privileged data access */
    0x00000000, /* Window size A23-A22 offset (4 MByte); A21-A01 mapp address: 0x800000 - 0xBFFFFF  (refer to manual 'gigalinkenglish112', page 29) */
    0,
    0xff010800, /* header */
    0x09,       /* A32 non privileged data access */
    0x10000000, /* Window size A23-A22 offset (4 MByte)                */
    0,
    0xff010800, /* header */
    0x09,       /* A32 non privileged data access */
    0x20000000, /* Window size A23-A22 offset (4 MByte)                */
    0,
    0xff010800, /* header */
    0x09,       /* A32 non privileged data access */
    0x30000000, /* Window size A23-A22 offset (4 MByte)                */
    0,
};



    reg.offset=0x400;
    for (i=0; i<16; i++) {
        reg.val=descr[i];
        ioctl(pc, SIS1100_CTRL_WRITE, &reg);
        reg.offset+=4;
    }


/* mapping */
char* start1=vme_base;
char* start2=vme_base+(1*0x400000); /* 4 Mbyte page offset */
char* start3=vme_base+(2*0x400000);
char* start4=vme_base+(3*0x400000);

u_int32_t* base_I_A32_0x00000000    = (u_int32_t *)(start1); /* addresse space 0x00000000 - 0x003FFFFC */
u_int32_t* base_I_A32_0x10000000    = (u_int32_t *)(start2); /* addresse space 0x10000000 - 0x103FFFFC */
u_int32_t* base_I_A32_0x20000000    = (u_int32_t *)(start3); /* addresse space 0x20000000 - 0x203FFFFC */
u_int32_t* base_I_A32_0x30000000    = (u_int32_t *)(start4); /* addresse space 0x30000000 - 0x303FFFFC */
 

do {

/* write all 4 A32 modules */

/* 	A21-A01 mapp address as long	*/
   *(base_I_A32_0x00000000+(0x000000/4))=0x11111111; /* VME A32 address = 0x00000000 */
   usleep(400000);
   *(base_I_A32_0x00000000+(0x100000/4))=0x111111AA; /* VME A32 address = 0x00100000 */
   usleep(400000);

   *(base_I_A32_0x10000000+(0x000000/4))=0x22222222; /* VME A32 address = 0x10000000 */
   usleep(400000);
   *(base_I_A32_0x10000000+(0x200000/4))=0x222222BB; /* VME A32 address = 0x10200000 */
   usleep(400000);

   *(base_I_A32_0x20000000+(0x000000/4))=0x44444444; /* VME A32 address = 0x20000000 */
   usleep(400000);
   *(base_I_A32_0x20000000+(0x300000/4))=0x444444CC; /* VME A32 address = 0x20300000 */
   usleep(400000);

   *(base_I_A32_0x30000000+(0x000000/4))=0x88888888; /* VME A32 address = 0x30000000 */
   usleep(400000);
   *(base_I_A32_0x30000000+(0x100000/4))=0x888888DD; /* VME A32 address = 0x30100000 */
   usleep(400000);
  

} while (1) ;



 return 0;
}
