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
#include <stdint.h>
#include "tcp_io.h"

extern int errno;
struct sockaddr_in tcp_srv_addr;  /* Server's socket address */
struct servent     tcp_serv_info; /* from getservbyname() */
struct hostent     tcp_host_info; /* from gethostbyname() */

/* static int sockfd = -1; */
static int traceflag=0;
static int to_sec=10;
static char mesg[MAXBUF];

/*
 * Recoverable error. Print a message and return to caller.
 */
void err_ret(char * s)
{
  perror(s);
  fflush(stdout);
  fflush(stderr);
  return;
}

/*
 *  Fatal error. Print a message, dump core, and terminate.
 */
void err_dump(char * s)
{
  perror(s);
  fflush(stdout);
  fflush(stderr);
  abort();          /* Dump core and terminate */
  exit(1);          /* Last ditch effort */ 
}

void phex(char * input, int len) 
{
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
int readn (register int fd, register char *ptr, register int nbytes) 
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


int tcp_open(char * host, char * service, int port) 
{
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
  if (traceflag) printf("Connection successful.\n");

  return fd; /* OK */
}

/* 
 * Open a client connection to a remote server 
 */
int net_open (char *host, char *service, int port)
{
  int sockfd;
  if ((sockfd = tcp_open (host, service, port)) < 0)
    return -1;
  return sockfd;
}

/*
 *
 */

int net_server(unsigned short port) 
{
  struct sockaddr_in serv_addr;
  struct linger sl;

  int sockfd;
  int yes = 1;
  
  if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0) {
    printf("net_server: socket\n");
    return -1;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    perror("net_server: setsockopt[SO_REUSEADDR]");
  }

  sl.l_onoff = 1;
  sl.l_linger = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) < 0) {
    perror("net_server: setsockopt[SO_LINGER]");
  }
 
  /* bind to socket */
  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port        = htons(port);
  if (bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0) {
    perror("net_server: bind");
    close(sockfd);
    return -2;
  }  
  /* Now listen at the socket */
  listen(sockfd, 5);

  return sockfd;
}

void net_close (int sockfd)
{
  int retn;
  retn = 0;

  retn = shutdown(sockfd, SHUT_RDWR);
  usleep(100);
  retn = close(sockfd);
  if (retn < 0) {
    perror("net_close: Error closing socket ");
  }
}

int net_listen(int sockfd) 
{
  int newsock;
  socklen_t clilen;
  struct sockaddr cli_addr;

  clilen = sizeof(cli_addr);
  newsock = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);
  if (traceflag) {
    printf("net_listen: new connection\n");
  }
  return newsock;
}

/* Wrapper to turn on debugging */
void net_debug (int debug) 
{
  traceflag = (debug) ? 1 : 0;
}

int sendPKG(int sockfd, int opcode, int prm, void * data, int ldata) 
{
  unsigned char * ptr;
  int retn, n;

  bzero(mesg,sizeof(mesg)); /* Clear out data */
  ptr = (unsigned char *)&mesg[0];
  if (data == NULL) ldata = 0;
  n = 3 *sizeof(uint32_t) + ldata;
  if ((opcode == OP_CMD) || (opcode == OP_ERR)) n += 1;
  *(uint32_t *)ptr = (uint32_t) htonl(n - sizeof(uint32_t)); 
  ptr += sizeof(uint32_t);
  *(uint32_t *)ptr = (uint32_t) htonl(opcode);           
  ptr += sizeof(uint32_t);
  *(uint32_t *)ptr = (uint32_t) htonl(prm);              
  ptr += sizeof(uint32_t);

  /* Write data as-is */
  if (data != NULL) { 
      memcpy(ptr,data,ldata); 
      ptr += ldata;
  }
  if ((opcode == OP_CMD) || (opcode == OP_ERR)) *ptr = EOS; /* End of pkg */

  if (traceflag) {
    printf("SendPKG:"); 
    if (traceflag&2) {
      phex((char *)mesg,n);
    } else {
      printf(" OP = %d PRM = %d len = %d\n",opcode,prm,ldata);
    }
  }
  if ((retn = send(sockfd,mesg,n,0)) != n) {
    if (retn < 0) {
      perror("SendPKG: write");
    } else {
      printf("SendPKG short write");
    }
  }
  return retn;
}

int recvPKG(int sockfd, int * len, int * opcode, int * prm, void * data) 
{
  int n,msglen;
  char * ptr;
  fd_set fds;  
  struct timeval timeout;

  FD_ZERO(&fds); FD_SET(sockfd,&fds);
  timeout.tv_sec = to_sec; timeout.tv_usec=0;
  n=select(sockfd+1,&fds,(fd_set*)NULL,(fd_set*)NULL,&timeout);
  if (n<0) {
    fprintf(stderr,"recvPKG: select failed\n");
    return -1;
  }
  if (n==0) {
    *opcode=OP_NOP;
    if (traceflag) { printf("recvPKG: timed out\n"); }
    return -2; /* timeout */
  }

  bzero(mesg,sizeof(mesg));
  n = -1;
  ptr=&mesg[0];
  n = recv(sockfd,(char *)mesg,sizeof(uint32_t),0);
  if (n>0) {
    msglen = ntohl(*(uint32_t *)ptr);
    ptr+=sizeof(uint32_t);
  } else if (n < 0) return -3;
  
  n = recv(sockfd,ptr,msglen,MSG_WAITALL);
  if (n < 0) return -3;
  if (n == 0) return -4; /* Empty message */

  /* Check number of bytes read */
  //ptr = &mesg[0];
  //*len = ntohl(*(uint32_t *)ptr);     ptr += sizeof(uint32_t);
  *len=msglen;
  *opcode = ntohl(*(uint32_t *) ptr); ptr += sizeof(uint32_t);
  *prm    = ntohl(*(uint32_t *) ptr); ptr += sizeof(uint32_t);
  *len   -= 2 * sizeof(uint32_t); /* Return length of data, NOT package */
  if ((*opcode == OP_CMD) || (*opcode == OP_ERR)) *len -= 1;
  if (data != NULL) {
    /* Pass data as-is */
    memcpy(data, ptr, (*len));
  }

  if (traceflag) {
    //printf("recvPKG:"); phex((char *)mesg,*len + 3*sizeof(uint32_t));
    printf("recvPKG: OP = %d PRM = %d len = %d recvlen = %d\n",
	   *opcode, *prm, *len, n);
  }
  
  return *opcode;
}

/*
 * Send histogram data 
 */
int sendHIST(int sockfd, int * hist, int lhist, int blocksize) 
{
  int nfullblks, nperblk, nlastblk;
  int i, j;
  int opcode, prm, dat, len;
  int32_t retn;
  uint32_t idata[2*MAXBLOCK];

  if (blocksize <= 0) { /* Do not send if blocksize zero or negative */
    retn = -1;
    sendPKG(sockfd,OP_ERR,-1, &retn,sizeof(retn));
    return -1;
  }

  /* Should also complain if blocksize is too large */

  nperblk = blocksize / sizeof(uint32_t); 
  nfullblks = lhist / nperblk;
  nlastblk  = lhist % nperblk;

  for (i=0;i<nfullblks;i++) {
    for (j=0;j<nperblk;j++) idata[j] = htonl(hist[i*nperblk + j]);
    sendPKG(sockfd,OP_DAT,i,&idata[0],(nperblk*sizeof(uint32_t)));
   
    retn = recvPKG(sockfd, &len, &opcode, &prm, &dat);
    if ((retn < 0) || (opcode != OP_ACK)) { /* An error occurred */
      fprintf(stderr,"sendHIST: **ERROR** recvPKG\n");
      return -1; /* Turn tail and run */
    } 

    /* Should check that (prm == i), too */
    
  }

  if (nlastblk > 0) {
    for (j=0;j<nlastblk;j++) idata[j] = htonl(hist[nfullblks*nperblk + j]);
    sendPKG(sockfd,OP_DAT,nfullblks,&idata[0],(nlastblk*sizeof(uint32_t)));
    retn = recvPKG(sockfd, &len, &opcode, &prm, &dat);
    if ((retn < 0) || (opcode != OP_ACK)) { /* An error occurred */
      fprintf(stderr,"sendHIST: **ERROR** last packet\n");
      return -1; /* Turn tail and run */
    } 
    /* Should check that (prm == i), too */
  }

  /* Send one last ACK packet to tell the client we're done */

  sendPKG(sockfd, OP_ACK, nfullblks, 0, 0);
  return lhist; /* Return number of elements successfully sent */
}

/* Alternative means of receiving the histogram:
 *    Just treat the matrix as a block of storage
 */
int recvHIST(int sockfd, int * hist, int size) 
{
  int opcode, blockno,len, i, retn, recv_len;
  int * data_ptr;
  uint32_t idata[MAXBLOCK];

  data_ptr = hist; 
  recv_len = 0;
  while((retn = recvPKG(sockfd,&len,&opcode,&blockno,(char *)&idata[0])) 
	== OP_DAT) {
    /* Should set offset from base using blockno & blocksize */
    len /= sizeof(uint32_t);
    for (i = 0; i < len; i++) {
      *data_ptr ++ = (int) ntohl(idata[i]);
      recv_len++;
      /* Check to make sure that we don't overflow storage */
      if (recv_len > size) {
	      sendPKG(sockfd, OP_ERR,-1,0,0);
	      return -1;
      }
    }

    sendPKG(sockfd, OP_ACK,blockno, 0, 0);
  }

  if (opcode == OP_ACK) {
    return recv_len;
    //    return 0;
  }
  sendPKG(sockfd, OP_ERR,-1,0,0);
  return -1;
}
