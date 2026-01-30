/* $Id: tcp_io.h,v 1.1.1.1 1999/09/20 14:32:07 nickm Exp $ */
#ifndef _TCP_IO_H
#define _TCP_IO_H

/*
 * Define our TFTP opcodes
 */
#define OP_CMD  1 /* Plaintext command */
#define OP_ACK  2 /* Acknowledgement */
#define OP_DAT  3 /* Data */
#define OP_ERR  4 /* Error, see error codes */

/*
 * Define TFTP error codes
 */
#define ERR_UNDEF  0
#define ERR_ACCESS 1
#define ERR_BADOP  2
#define ERR_BADID  3

/*
 * Syntax defines
 */
#define CMD_RUN      0
#define CMD_RUN_STOP 0
#define CMD_RUN_STRT 1
#define CMD_RUN_RSM  2

#define CMD_MODE     1
#define CMD_MODE_PBN 0
#define CMD_MODE_TBN 1
#define CMD_MODE_VBN 2
#define CMD_MODE_SCN 3

#define CMD_PULS     3
#define CMD_PULS_ON  1
#define CMD_PULS_OFF 0

#define CMD_TIMER    4
#define CMD_LDLK     5
#define CMD_SXFER    6
#define CMD_HXFER    7
#define CMD_TWID     8

#define MAXMSG      2048
#define SERVERPORT 20000
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif
/*
 * "Boundary" defines
 */

#define MAXBUF     2048        /* Xmit and recv buffer length (in longwords)*/
#define BLKSIZE     256
#define EOS        0xefefefef

void err_ret(char *);
void err_dump(char *);
void phex(char *, int);
int  net_open (char *, char *, int);
void net_close(void);
void send_PKG(int, int, int);
int  recv_MSG(int *, int *, int *, int *, unsigned int);
int  recv_HIST(float *, int);
#endif /* _TCP_IO_H */
