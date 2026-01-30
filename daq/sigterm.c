#include <stdio.h>
#include <signal.h>

main(argc, argv) 
int argc;
char **argv;
{
  int pid;
  if (argc < 2) {
    printf(stderr,"Usage: sigterm pid\n");
    exit(1);
  }
  sscanf(argv[1],"%d",&pid);
  printf("I would be sending a signal to pid = %d\n",pid);
  exit(0);

  if (kill(pid,36) < 0) {
    printf(stderr,"Error: unable to signal process %d\n",pid);
    exit(2);
  }
}
