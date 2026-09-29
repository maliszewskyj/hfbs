
/* $Id: tcp_io.h,v 1.6 2004/04/30 15:12:34 pheiffer Exp $ */
#ifndef _TCP_IO_H
#define _TCP_IO_H

/*
 * Define opcodes
 */
#define OP_NOP  0 /* Do nothing */
#define OP_CMD  1 /* Plaintext command */
#define OP_ACK  2 /* Acknowledgement */
#define OP_DAT  3 /* Data */
#define OP_ERR  4 /* Error, see error codes */
#define OP_SET  5 /* Set parameters */
#define OP_GET  6 /* Get parameters */
#define OP_EXIT 7 /* Kill communications thread on server */

/*
 * Define error codes
 */
#define ERR_UNDEF   0 /* Undefined error */
#define ERR_ACCESS  1 /* */
#define ERR_BADOP   2 /* Bad opcode */
#define ERR_BADID   3 /* Bad parameter */
#define ERR_STATE   4 /* Run state not appropriate for action */
#define ERR_ARGS    5 /* Wrong number of arguments */
#define ERR_INVAL   6 /* Bad value for parameter */
#define ERR_BOUNDS  7 /* Boundary error */

/*
 * Command Syntax defines
 */
#define CMD_NOP      0 /* No operation */
#define CMD_ARM      1 /* Arm */
#define CMD_DISARM   2 /* Disarm */
#define CMD_CLEAR    3 /* Clear remote histogram */
#define CMD_XFER     4 /* Transfer histogram data */
#define CMD_SET      5 /* Get status */
#define CMD_GET      6 /* Get time of last transfer */
#define CMD_EXIT     7 

#define PRM_STATUS    0
#define PRM_RANK      1 /* Rank of histogram */
#define PRM_DIMS      2 /* Dimensions of histogram */
#define PRM_XFERMODE  3 /* Mode of data transfer */
#define PRM_XFERSEL   4 /* Selection of histogram to transfer */
#define PRM_ROI       5 /* Bounds of region of interest */
#define PRM_ROI_PRE   6 /* Preset counts in region of interest */
#define PRM_ROI_CTS   7 /* Total events binned in region of interest */
#define PRM_TOT_CTS   8 /* Total events binned over detector */
#define PRM_BIN_TIME  9 /* Histogram with time slicing (true/false) */
#define PRM_NTIM     11 /* Number of time bins (default 1) */
#define PRM_TBINW    12 /* Constant time bin width (in us) */
#define PRM_TBINS    13 /* Array of time bin widths (in us) */
#define PRM_SHUTSTAT 16 /* State of quick shutter */
#define PRM_SHUTTHRE 17 /* CPS at which quick shutter will be closed */
#define PRM_SHUTPIX  18 /* Number of pixels overcounting for shutter to close */
#define PRM_HV_STAT  20 /* State of HV bias */
#define PRM_HV_THRE  21 /* CPS at which HV bias will be turned off */
#define PRM_EVT_GEN  30 /* Generate Event File */
#define PRM_EVT_FILE 31 /* Event File Name */
#define PRM_EVT_PRE  32 /* Event File Prefix */
#define PRM_TPRESET  33 /* Time Preset */
#define PRM_CCD_SAFE 34 /* CCD Safe to Proceed */
#define PRM_CCD_GAIN 35 /* CCD Gain */
#define PRM_HST_MODE 36 /* Histogram mode */
#define PRM_NFRAMES  37 /* Number of acquisition frames */
#define PRM_SCALERS  38 /* Accumulated scaler counts */

/*
 * Status word defines
 */
#define STATUS_ACCUM  0x0001 /* System is accumulating events */
#define STATUS_TIMBIN 0x0002 /* System is set to bin events in time */
#define STATUS_QSHUT  0x0010 /* Quick shutter is open */
#define STATUS_HVBIAS 0x0020 /* HV Bias is on */

/*
 * "Boundary" defines
 */

//#define MAXBUF     2048        /* Xmit and recv buffer length (in longwords)*/
//#define MAXBLOCK     2048
#define MAXBLOCK     327680
#define MAXBUF       MAXBLOCK
#define EOS           0

/*
 * Function prototypes
 */
void err_ret(char *);
void err_dump(char *);
void phex(char *, int);
int  net_open (char *, char *, int);
int  net_server (unsigned short);
int  net_listen(int );
void net_close(int);
void net_debug(int );
int  sendPKG(int, int, int, void *, int);
int  recvPKG(int, int *, int *, int *, void *);
int  sendHIST(int, int *, int, int);
int  recvHIST(int, int *, int);
#endif /* _TCP_IO_H */
