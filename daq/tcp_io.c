static char rcsid[] = "$Id: tcp_io.c,v 1.2 2000/04/07 22:26:31 nickm Exp $";
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tcp_io.h"
extern int errno;

int sockfd = -1;
char openhost[MAXHOSTNAMELEN] = {0};
int traceflag=0;
struct sockaddr_in tcp_srv_addr;  /* Server's socket address */
struct servent     tcp_serv_info; /* from getservbyname() */
struct hostent     tcp_host_info; /* from gethostbyname() */

static int mesg[MAXBUF];

/*
 * Recoverable error. Print a message and return to caller.
 */
void
err_ret(char * s)
{
  perror(s);
  fflush(stdout);
  fflush(stderr);
  return;
}

/*
 *  Fatal error. Print a message, dump core, and terminate.
 */
void
err_dump(char * s)
{
  perror(s);
  fflush(stdout);
  fflush(stderr);
  abort();          /* Dump core and terminate */
  exit(1);          /* Last ditch effort */ 
}

void
phex(char * input, int len) {
  unsigned char ch;
  int i;

  for (i=0;i<len;i++) {
    ch = *(input + i);
    if ((i>0) && !(i%4)) printf("- ");
    printf("%02x ",(unsigned int) ch);
  }
  printf("\n");
}

/*
 *  Read "n" bytes from a descriptor. 
 *  Use in place of read() when fd is a stream socket.
 */
int
readn (register int fd, register char *ptr, register int nbytes)
{
  int nleft, nread;

  nleft = nbytes;
  while (nleft > 0) {
    nread = read (fd, ptr, nleft);
    if (nread < 0) {
      if (nleft != nbytes) break;
      return nread;		/* Error, return < 0 */
    } else if (nread == 0) {
      if (nleft != nbytes) break;  /* EOF */
    }
    nleft -= nread;
    ptr += nread;
  }
  return (nbytes - nleft);	/* return >= 0 */
}


int
tcp_open(char * host, char * service, int port) {
  int fd, resvport;
  unsigned long inaddr;
  char *host_err_str();
  struct servent *sp;
  struct hostent *hp;

  /* 
   * Initialize the server's Internet address structure.
   * We'll store the actual 4-byte Internet address and the
   * 2-byte port number below
   */
  bzero((char *) &tcp_srv_addr, sizeof(tcp_srv_addr));
  tcp_srv_addr.sin_family = AF_INET;

  if (service != NULL) {
    if ((sp = getservbyname(service,"tcp")) == NULL) {
      err_ret("tcp_open: unknown service");
      return -1;
    }
    tcp_serv_info = *sp;
    if (port > 0) tcp_srv_addr.sin_port = htons(port); /* Caller's value  */
    else          tcp_srv_addr.sin_port = sp->s_port;  /* Service's value */
  } else {
    if (port <= 0) {
      err_ret("tcp_open: must specify either service or port");
      return -1;
    }
    tcp_srv_addr.sin_port = htons(port);
  }

  /*
   * First try to convert the host hame as a dotted-decimal number.
   * If that fails we'll do gethostbyname().
   */
  if ((inaddr = inet_addr(host)) != INADDR_NONE) { /* It's dotted decimal */
    bcopy((char *) &inaddr, (char *) &tcp_srv_addr.sin_addr, sizeof(inaddr));
    tcp_host_info.h_name = NULL;
  } else {                                        /* It's a named address */
    if ((hp = gethostbyname(host)) == NULL) {
      err_ret("tcp_open: host name error");
      return -1;
    }
    tcp_host_info = *hp;
    bcopy(hp->h_addr, (char *) &tcp_srv_addr.sin_addr, hp->h_length);
  }

  if (port >= 0) {
    if ((fd = socket(AF_INET,SOCK_STREAM, 0)) < 0) {
      err_ret("tcp_open: can't create TCP socket");
      return -1;
    }
  } else {
    resvport = IPPORT_RESERVED - 1;
    if ((fd = rresvport(&resvport)) < 0) {
      err_ret("tcp_open: can't get a reserved TCP port");
      return -1;
    }
  }
   
  /*
   * Connect to the server.
   */
  if (connect(fd,(struct sockaddr *)&tcp_srv_addr,sizeof(tcp_srv_addr)) < 0) {
    err_ret("tcp_open: can't connect to server");
    close(fd);
    return -1;
  }

  return fd; /* OK */
}

int
net_open (char *host, char *service, int port)
{
  if (sockfd >= 0) net_close(); /* Cover ourselves in case we have one open */
  if ((sockfd = tcp_open (host, service, port)) < 0)
    return -1;
  strcpy (openhost, host);
  return 0;
}

void
net_close (void)
{
  int retn, try;
  retn = 0;
  try = 0;
  while ((retn = close (sockfd)) < 0) {
    fprintf(stderr,"Error closing socket, retrying\n");
    try++;
    usleep(100);
    if (try > 5) {
      fprintf(stderr,"Unable to close socket; giving up\n");
      break;
    }
  }
  sockfd = -1;
}

void
send_PKG(int opcode, int flag, int prm) 
{
  int n;
  bzero(mesg,sizeof(mesg));
  mesg[0] = htonl(4 * sizeof(int));
  mesg[1] = htonl(opcode);
  mesg[2] = htonl(flag);
  mesg[3] = htonl(prm);
  mesg[4] = htonl(EOS);

  n = 5 * sizeof(int);

  if (traceflag) {
    printf("Send:");
    phex((char *)mesg,n);
  }
  if (write(sockfd,mesg,n) != n) err_dump("send_PKG:write");
}

int
recv_MSG(int * len, int * opcode, int * prm1, int * prm2, 
	 unsigned int to_sec) {
  int n;
  fd_set fds;  
  struct timeval timeout;

  FD_ZERO(&fds); FD_SET(sockfd,&fds);
  timeout.tv_sec = to_sec; timeout.tv_usec=0;
  n=select(sockfd+1,&fds,(fd_set*)NULL,(fd_set*)NULL,&timeout);
  if (n<0) {
    fprintf(stderr,"recv_MSG: select failed\n");
    exit(1);
  }

  if (n==0) {
    if (traceflag) { printf("recv_MSG: timed out\n"); }
    return -1; /* timeout */
  }

  bzero(mesg,sizeof(mesg));
  n = -1;
  while(n<0){
    n = read(sockfd, (char *)mesg, sizeof(mesg));
  }
  if (n == 0) return -2;
  if ((*len = ntohl(mesg[0])) <= 0) return -3;
  /* if (*len != (n - 4) / 4) return -4; */
  if (traceflag) {
    printf("Received: n = %d len = %d\n",n,*len);
    printf("Recv:");
    phex((char *)mesg, 5 * sizeof(int));
  }
  *opcode = ntohl(mesg[1]);
  *prm1   = ntohl(mesg[2]);
  if (prm2 != NULL) {
    *prm2 = ntohl(mesg[3]);
  }
  if (traceflag) {
    printf("Op = %d prm1 = %d len = %d recvlen = %d\n", 
	   *opcode,*prm1,*len,n);
  }

  *len /= sizeof(int);                /* Return message length in longwords */
  return *opcode;
}

/* Alternative means of receiving the histogram:
 *    Just treat the matrix as a block of storage
 */
int
recv_HIST(float * hist, int size) {
  int opcode, blockno,len, i, retn, recv_len;
  unsigned int to_sec = 10;
  float * data_ptr;

  data_ptr = hist;
  recv_len = 0;
  while((retn = recv_MSG(&len,&opcode,&blockno,0,to_sec)) == OP_DAT) {
    /* Should set offset from base using blockno & blocksize */
    len -= 2;
    for (i = 0; i < len; i++) {
      *data_ptr ++ = (float) ntohl(mesg[3+i]);
      recv_len++;
      /* Check to make sure that we don't overflow storage */
      if (recv_len > size) {
	send_PKG(OP_ERR,-1,-1);
	return -1;
      }
    }

    send_PKG(OP_ACK,blockno, 0);
  }

  if (opcode == OP_ACK) {
    return 0;
  }
  send_PKG(OP_ERR,-1,-1);
  return -1;
}
