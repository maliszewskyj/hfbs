/* ICS 110B ADC header file */

/* Register definitions */
#define ICS_DATA     0x0000
#define ICS_STATUS   0xFE00
#define ICS_CONTROL  0xFE02
#define ICS_IVEC     0xFE04
#define ICS_NCHANS   0xFE06
#define ICS_DECIM    0xFE08
#define ICS_ADC_CONF 0xFE0A
#define ICS_CALIB    0xFE0C
#define ICS_FPDPADDR 0xFE0E
#define ICS_FPDPNCHN 0xFE10
#define ICS_FPDPNBLK 0xFE12
#define ICS_VSB_BASE 0xFE14
#define ICS_VSB_SC   0xFE16
#define ICS_SYNCWORD 0xFE18
#define ICS_CLKFREQ  0xFE1C

/* Status register */
#define ICS_BRDRST   0x0080
#define ICS_FIFORST  0x0040
#define ICS_ACQENBL  0x0008
#define ICS_FIFOF    0x0004
#define ICS_FIFOHF   0x0002
#define ICS_FIFOE    0x0001

/* Control register */
#define ICS_VME_INT_ENB 0x8000
#define ICS_VSB_INT_ENB 0x4000
#define ICS_128_SAMPLE  0x1000
#define ICS_CLK_EXTERN  0x0800
#define ICS_OUT_VME_P   0x0000
#define ICS_OUT_VSB_P   0x0008
#define ICS_OUT_VME_U   0x0010
#define ICS_OUT_VSB_U   0x0018
#define ICS_OUT_FP_M    0x0020
#define ICS_OUT_FP_S    0x0028
#define ICS_TRG_EXTERN  0x0004
#define ICS_SMP_MASTER  0x0002
#define ICS_ACQUIRE     0x0001


/* Some "short hand" definitions for programming internal freq generator
 * Found in ICS manual E10534 Rev.C, p.62
 */
#define ICS_CR_ENPW     0x38001E05  /* Enable loading of Prog word reg */
#define ICS_PW_HIHZ     0x10381375  /* Programming word for 25.6 MHz */
#define ICS_CR_LDPW     0x38001E04  /* Load Prog word */
#define ICS_CR_ENFR     0x38001E00  /* Enable output frequency */ 
