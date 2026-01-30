/* ctrl.h */
/* Dr. Matthias Kirsch, STRUCK, 1.8.97      */
/* header file for offsets of control words */
/* and command codes                        */
/* $Id: ctrl.h,v 1.1.1.1 1999/09/20 14:32:07 nickm Exp $ */
#define RUN_OFF       0x0
#define MODE_OFF      0x1
#define SAVE_TIME_OFF 0x2
#define NEW_RUN_OFF   0x3
#define COMMAND_LEN    20
#define COMMAND         1
#define ERROR_LEN      20
#define DAQRUN          0
#define MODE            1
#define SAVE_TIME       2
#define TEST_PULSE      3
#define GET_TIMER       4
#define LOAD_LOOKUP     5
#define SEND_SCALERS    6
#define SEND_HISTOS     7
#define GET_TIMBIN      8
