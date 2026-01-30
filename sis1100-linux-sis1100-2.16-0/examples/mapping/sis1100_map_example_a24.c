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
char*      vme_base; /* hier landen die VME-Adressen */
u_int32_t* ctl_base; /* das sind die Register der PCI-Karte */

/* open */
pv=open("/dev/sis1100_00remote", O_RDWR, 0);
pc=open("/dev/sis1100_00ctrl", O_RDWR, 0);
if ((pv<0) || (pc<0)) {
    printf("error on opening\n");
    return -1;
}


/* mmap ? MByte VME space (koennte auch weniger sein) */
/*vme_base=mmap(0, 0x100000, PROT_READ|PROT_WRITE, MAP_SHARED, pv, 0);*/
vme_base=mmap(0, 0x1000000, PROT_READ|PROT_WRITE, MAP_SHARED, pv, 0);
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
16 modules with the A24-baseaddresses 0x0, 0x100000, 0x200000, .. 0xF00000

*/
u_int32_t descr[16]={
    0xff010800, /* header */
    0x39,        /* A24 */
    0x000000,   /* Window size A23-A22 offset (4 MByte); A21-A01 mapp address: 0x800000 - 0xBFFFFF  (refer to manual 'gigalinkenglish112', page 29) */
    0,
    0xff010800, /* header */
    0x39,        /* A24 */
    0x400000,   /* Window size A23-A22 offset (4 MByte); */
    0,
    0xff010800, /* header */
    0x39,        /* A24 */
    0x800000,   /* Window size A23-A22 offset (4 MByte);  */
    0,
    0xff010800, /* header */
    0x39,        /* A24 */
    0xC00000,   /* Window size A23-A22 offset (4 MByte);  */
    0,
};



    reg.offset=0x400;
    for (i=0; i<16; i++) {
        reg.val=descr[i];
        ioctl(pc, SIS1100_CTRL_WRITE, &reg);
        reg.offset+=4;
    }


/* ein paar Hilfspointer */
char* start1=vme_base;
char* start2=vme_base+(1*0x400000);
char* start3=vme_base+(2*0x400000);
char* start4=vme_base+(3*0x400000);

u_int32_t* base_I_A24_0x000000    = (u_int32_t *)(start1);
u_int32_t* base_I_A24_0x100000    = (u_int32_t *)(start1+(0x100000));
u_int32_t* base_I_A24_0x200000    = (u_int32_t *)(start1+(0x200000));
u_int32_t* base_I_A24_0x300000    = (u_int32_t *)(start1+(0x300000));

u_int32_t* base_II_A24_0x400000   = (u_int32_t *)(start2);
u_int32_t* base_II_A24_0x500000   = (u_int32_t *)(start2+(0x100000));
u_int32_t* base_II_A24_0x600000   = (u_int32_t *)(start2+(0x200000));
u_int32_t* base_II_A24_0x700000   = (u_int32_t *)(start2+(0x300000));

u_int32_t* base_III_A24_0x800000  = (u_int32_t *)(start3) ;
u_int32_t* base_III_A24_0x900000  = (u_int32_t *)(start3+(0x100000));
u_int32_t* base_III_A24_0xA00000  = (u_int32_t *)(start3+(0x200000));
u_int32_t* base_III_A24_0xB00000  = (u_int32_t *)(start3+(0x300000));

u_int32_t* base_IV_A24_0xC00000   = (u_int32_t *)(start4);
u_int32_t* base_IV_A24_0xD00000   = (u_int32_t *)(start4+(0x100000));
u_int32_t* base_IV_A24_0xE00000   = (u_int32_t *)(start4+(0x200000));
u_int32_t* base_IV_A24_0xF00000   = (u_int32_t *)(start4+(0x300000));
 

do {

/* write all 16 A24 modules */


   *base_I_A24_0x000000=0x1;
   usleep(400000);
   *(base_I_A24_0x000000+(0x100000/4))=0x2; /* A21-A01 mapp address as long */
   usleep(400000);
   *(base_I_A24_0x000000+(0x200000/4))=0x3;
   usleep(400000);
   *(base_I_A24_0x000000+(0x300000/4))=0x4;
   usleep(400000);

   *(base_II_A24_0x400000+(0x000000/4))=0x5;
   usleep(400000);
   *(base_II_A24_0x400000+(0x100000/4))=0x6;
   usleep(400000);
   *(base_II_A24_0x400000+(0x200000/4))=0x7;
   usleep(400000);
   *(base_II_A24_0x400000+(0x300000/4))=0x8;
   usleep(400000);

   *(base_III_A24_0x800000+(0x000000/4))=0x9;
   usleep(400000);
   *(base_III_A24_0x800000+(0x100000/4))=0xA;
   usleep(400000);
   *(base_III_A24_0x800000+(0x200000/4))=0xB;
   usleep(400000);
   *(base_III_A24_0x800000+(0x300000/4))=0xC;
   usleep(400000);

   *(base_IV_A24_0xC00000+(0x000000/4))=0xD;
   usleep(400000);
   *(base_IV_A24_0xC00000+(0x100000/4))=0xE;
   usleep(400000);
   *(base_IV_A24_0xC00000+(0x200000/4))=0xF;
   usleep(400000);
   *(base_IV_A24_0xC00000+(0x300000/4))=0x10;
   usleep(400000);


/* or in this manner */

   *base_I_A24_0x000000=0x101;
   usleep(400000);
   *(base_I_A24_0x100000)=0x102;
   usleep(400000);
   *(base_I_A24_0x200000)=0x103;
   usleep(400000);
   *(base_I_A24_0x300000)=0x104;
   usleep(400000);

   *(base_II_A24_0x400000)=0x105;
   usleep(400000);
   *(base_II_A24_0x500000)=0x106;
   usleep(400000);
   *(base_II_A24_0x600000)=0x107;
   usleep(400000);
   *(base_II_A24_0x700000)=0x108;
   usleep(400000);

   *(base_III_A24_0x800000)=0x109;
   usleep(400000);
   *(base_III_A24_0x900000)=0x10A;
   usleep(400000);
   *(base_III_A24_0xA00000)=0x10B;
   usleep(400000);
   *(base_III_A24_0xB00000)=0x10C;
   usleep(400000);

   *(base_IV_A24_0xC00000)=0x10D;
   usleep(400000);
   *(base_IV_A24_0xD00000)=0x10E;
   usleep(400000);
   *(base_IV_A24_0xE00000)=0x10F;
   usleep(400000);
   *(base_IV_A24_0xF00000)=0x110;
   usleep(400000);


} while (1) ;

 for (i=0;i<100;i++) {
   *base_I_A24_0x000000=0x0;
   usleep(400000);
   *base_I_A24_0x100000=0x1;
   usleep(400000);
   *base_I_A24_0x200000=0x2;
   usleep(400000);
   *base_I_A24_0x300000=0x3;
   usleep(400000);
 
#ifdef raus
   *base_II_A24_0x400000=0x4;
   usleep(400000);
   *base_II_A24_0x500000=0x5;
   usleep(400000);
   *base_II_A24_0x600000=0x6;
   usleep(400000);
   *base_II_A24_0x700000=0x7;
   usleep(400000);

   *base_III_A24_0x800000=0x8;
   usleep(400000);
   *base_III_A24_0x900000=0x9;
   usleep(400000);
   *base_III_A24_0xA00000=0xA;
   usleep(400000);
   *base_III_A24_0xB00000=0xB;
   usleep(400000);

   *base_IV_A24_0xC00000=0xC;
   usleep(400000);
   *base_IV_A24_0xD00000=0xD;
   usleep(400000);
   *base_IV_A24_0xE00000=0xE;
   usleep(400000);
   *base_IV_A24_0xF00000=0xF;
   usleep(400000);
#endif

   sleep(1);
  }



 return 0;
}
