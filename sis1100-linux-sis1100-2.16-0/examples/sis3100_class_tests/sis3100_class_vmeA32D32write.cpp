#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include "dev/pci/sis1100_var.h"

#include "vme_interface_class.h"
#include "sis1100linux_vme_class.h"
 


/****************************************************************************/
int main(int argc, char* argv[])
{


u_int32_t offset ; 
u_int32_t data ;
int return_code ;

	char char_messages[128] ;
	unsigned int nof_found_devices ;




	if (argc<3)
  	{
  		fprintf(stderr, "usage: address data \n", argv[0]);
  		return 1;
  	}
	offset     = strtoul(argv[1],NULL,0) ;
 	data       = strtoul(argv[2],NULL,0) ;
 

	sis1100 *vme_crate = new sis1100(0);  // open first SIS1100 (/dev/sis1100_00remote) 
	return_code = vme_crate->vmeopen ();  // open Vme interface
	vme_crate->get_vmeopen_messages (char_messages, &nof_found_devices);  // open Vme interface	
    	printf("get_vmeopen_messages = %s , nof_found_devices %d \n",char_messages, nof_found_devices);


	return_code = vme_crate->vme_A32D32_write(offset, data);

   	printf("vme_A32D32_write:   return_code = 0x%08x\n", return_code );         
	vme_crate->vmeclose ();  //  
	return 0;
}





 

























