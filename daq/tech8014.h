#ifndef _tech8014_h
#define _tech8014_h


/* Defines for IndustryPack carrier */
#define IP_A_IO_BASE 0x0000
#define IP_A_ID_BASE 0x0080
#define IP_B_IO_BASE 0x0100
#define IP_B_ID_BASE 0x0180
#define IP_C_IO_BASE 0x0200
#define IP_C_ID_BASE 0x0280
#define IP_D_IO_BASE 0x0300
#define IP_D_ID_BASE 0x0080

/* Tech80 Model 14 encoder defines */
#define GLOB_CSR     0x0000
#define GLOB_ISR     0x0002
#define GLOB_IOV     0x0004
#define GLOB_IIV     0x0006
#define GLOB_MULTI   0x0010
#define GLOB_TSMODE  0x0012
#define GLOB_TCA     0x0014
#define GLOB_TCB     0x0016

/* Axis base addresses */
#define AXIS0_OFFSET 0x0020
#define AXIS1_OFFSET 0x0030
#define AXIS2_OFFSET 0x0040
#define AXIS3_OFFSET 0x0050
#define AXIS4_OFFSET 0x0060
#define AXIS5_OFFSET 0x0070

/* Axis register addresses */
#define AXIS_IC      0x0000
#define AXIS_CC      0x0002
#define AXIS_CSR     0x0004
#define AXIS_REG_HI  0x0008
#define AXIS_REG_LO  0x000A
#define AXIS_OUT_HI  0x000C
#define AXIS_OUT_LO  0x000E


#endif /* _tech8014_h */
