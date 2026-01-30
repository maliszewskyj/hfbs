/* Standard IO */
#include <stdio.h>

/* networking/socket communication */
#include <sgstat.h>
#include <types.h>
#include <machine/types.h>
#include <time.h>


void main()
{
  time_t t1,t2;
  
  time(&t1);
  tsleep(1000);
  time(&t2);
  printf("t1 = %ld\n",(long) t1);
  printf("t2 = %ld\n",(long) t2);
  printf("t12 = %ld\n",(long) (t2 - t1));
}
