#include "sis1100linux_vme_class.h"
#include <iostream>


//#include "sis1100_var.h"


//#include <stdio.h>
//#include <unistd.h>

//#include <fcntl.h>



#include <stdio.h>
//#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
//#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>




int call_vme_A32D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=4; /* driver does not change any field except data */
  req.am=0x9; /* "" */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;    /* NEW */
  return 0 ;
}




int call_vme_A32D32_write(int p, u_int32_t vme_adr, u_int32_t vme_data )
{
struct sis1100_vme_req req;
  req.size=4;
  req.am=0x9;
  req.addr= vme_adr;
  req.data= vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}



int call_vme_A32BLT32FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=1;
   block_req.size=4;
   block_req.am=0xb;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}

int call_vme_A32MBLT64FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=1;
   block_req.size=4;
   block_req.am=0x8;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}






using namespace std;

sis1100::sis1100( unsigned int use_device_no)
{

    this->used_device_no    = use_device_no ;
    this->nof_devices_found = 0 ;
	strcpy( this->char_messages, "sis1100 device not open");
}





int sis1100::vmeopen(){

	err = 0;
	if ((this->p=open("/dev/sis1100_00remote", O_RDWR, 0))<0) {
		strcpy( this->char_messages, "sis1100 device open failed");
		this->nof_devices_found = 0 ;
	    err = -1;
	}
	else {
		strcpy( this->char_messages, "sis1100 device open OK");
		this->nof_devices_found = 1 ;
	    err = 0;
	}

	return err;
}

int sis1100::vmeclose(){
	return -1;
}


int sis1100::get_vmeopen_messages(CHAR* messages, UINT* nof_found_devices){
	strcpy(messages, this->char_messages);
	*nof_found_devices = this->nof_devices_found ;
	return -1;
}






// local side register: only for sis1100
int sis1100::localRegisterRead(UINT addr, PUINT data)
{
	return -1;
}

int sis1100::localRegisterWrite(UINT addr, UINT data)
{
	return -1;
}



 

// vme master side register
int sis1100::controlRegisterRead(UINT addr, PUINT data)
{
struct sis1100_ctrl_reg reg;
  reg.val = 0 ;
  reg.offset = addr;
  if (ioctl(p, SIS1100_CTRL_READ, &reg)<0)   return -1;        /* NEW */
  if (reg.error) return reg.error ;
  *data = reg.val;    /* NEW */
  return 0 ;
}

int sis1100::controlRegisterWrite(UINT addr, UINT data)
{
struct sis1100_ctrl_reg reg;
int error ;
  reg.offset = addr; 
  reg.val  = data; 
  error = (ioctl(p, SIS1100_CTRL_WRITE, &reg)<0)  ;
  return error;
}

// vme read
int sis1100::vme_A32D32_read(UINT addr, UINT* data)
{
	unsigned int return_code ;

 	return_code =  call_vme_A32D32_read(p, addr, data);
	return return_code ;
}


int sis1100::vme_A32DMA_D32_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	//return vme_A32DMA_D32_read(p, addr, data, request_nof_words, got_nof_words);
	return -1 ;
}
int sis1100::vme_A32BLT32_read(UINT addr, UINT* data, UINT req_num_data, UINT* got_num_data)
{
	//return vme_A32BLT32_read(p, addr, data, req_num_data, got_num_data);
	return -1 ;
}
int sis1100::vme_A32MBLT64_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	//return vme_A32MBLT64_read(p, addr, data, request_nof_words, got_nof_words);
	return -1 ;
}


int sis1100::vme_A32_2EVME_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	//return vme_A32MBLT64_read(p, addr, data, request_nof_words, got_nof_words);
	return -1 ;
}
int sis1100::vme_A32_2ESST160_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	//return vme_A32MBLT64_read(p, addr, data, request_nof_words, got_nof_words);
	return -1 ;
}
int sis1100::vme_A32_2ESST267_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	//return vme_A32MBLT64_read(p, addr, data, request_nof_words, got_nof_words);
	return -1 ;
}
int sis1100::vme_A32_2ESST320_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	//return vme_A32MBLT64_read(p, addr, data, request_nof_words, got_nof_words);
	return -1 ;
}








int sis1100::vme_A32DMA_D32FIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	//return vme_A32DMA_D32FIFO_read(p, addr, data, request_nof_words, got_nof_words);
	return -1 ;
}
int sis1100::vme_A32BLT32FIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return call_vme_A32BLT32FIFO_read(p, addr, data, request_nof_words, got_nof_words);
	//return -1 ;

}
int sis1100::vme_A32MBLT64FIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return call_vme_A32MBLT64FIFO_read(p, addr, data, request_nof_words, got_nof_words);
}


int sis1100::vme_A32_2EVMEFIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return -1 ;
}

int sis1100::vme_A32_2ESST160FIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return -1 ;
}

int sis1100::vme_A32_2ESST267FIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return -1 ;
}

int sis1100::vme_A32_2ESST320FIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return -1 ;
}



// vme write
int sis1100::vme_A32D32_write(UINT addr, UINT data)
{
	return call_vme_A32D32_write(p, addr, data);
}
int sis1100::vme_A32DMA_D32_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
	//return vme_A32DMA_D32_write(p, addr, data, request_nof_words, written_nof_words);
	return -1 ;

}
int sis1100::vme_A32BLT32_write(UINT addr, PUINT data, UINT request_nof_words, PUINT written_nof_words)
{
	//return vme_A32BLT32_write(p, addr, data, request_nof_words, written_nof_words);
	return -1 ;

}
int sis1100::vme_A32MBLT64_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
	//return vme_A32MBLT64_write(p, addr, data, request_nof_words, written_nof_words);
	return -1 ;
}

int sis1100::vme_A32DMA_D32FIFO_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
unsigned int i;
int return_code = 0 ;
unsigned int write_value ;
	for (i=0;i<request_nof_words;i++) {
		write_value =   data[i] ;//
		return_code = return_code | call_vme_A32D32_write(p, addr, write_value);
	}
	*written_nof_words = 0;
	if (return_code == 0) {
		*written_nof_words = request_nof_words ;
	}
	//return vme_A32DMA_D32FIFO_write(p, addr, data, request_nof_words, written_nof_words);
	return return_code ;
}
int sis1100::vme_A32BLT32FIFO_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
	//return vme_A32BLT32FIFO_write(p, addr, data, request_nof_words, written_nof_words);
	return -1 ;
}
int sis1100::vme_A32MBLT64FIFO_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
//	return vme_A32MBLT64FIFO_write(p, addr, data, request_nof_words, written_nof_words);
	return -1 ;
}


int sis1100::vme_IRQ_Status_read( UINT* data )
{
	return -1 ;
}



#ifdef SIS3150USB
#include "SisVmeInterface.h"
#include "sis3150usb_vme.h"

struct SIS3150USB_Device_Struct gl_dev_struct[1];

sis1100::sis1100()
{
    unsigned int numDev;
    int rc;
    err = -1;

    rc = FindAll_SIS3150USB_Devices(gl_dev_struct, &numDev, 1);
    std::cout << "FindAll_SIS3150USB_Devices': " << numDev  << std::endl;
    if(rc){
        std::cout << "Error during 'FindAll_SIS3150USB_Devices': " << rc << std::endl;
    }

    rc = Sis3150usb_OpenDriver_And_Download_FX2_Setup((char *)gl_dev_struct[0].cDName, &gl_dev_struct[0]);
    this->usbdevice_handle = gl_dev_struct[0].hDev ;

   // rc = Sis3150usb_OpenDriver_And_Download_FX2_Setup((char *)gl_dev_struct[0].cDName, &this->usbdevice_handle);

    if(rc){
        std::cout << "Error during 'Sis3150usb_OpenDriver_And_Download_FX2_Setup': " << rc << std::endl;
    }
    err = 0;
}



int sis1100::vmeopen(){
	return err;
}

int sis1100::vmeclose(){
	return -1;
}

// local side register: only for sis1100
int sis1100::localRegisterRead(UINT addr, PUINT data)
{
	return -1;
}

int sis1100::localRegisterWrite(UINT addr, UINT data)
{
	return -1;
}

// vme master side register
int sis1100::controlRegisterRead(UINT addr, PUINT data)
{
	return sis3150Usb_Register_Single_Read(this->usbdevice_handle, addr, data);
}

int sis1100::controlRegisterWrite(UINT addr, UINT data)
{
	return sis3150Usb_Register_Single_Write(this->usbdevice_handle, addr, data);
}

// vme read
int sis1100::_vme_A32D32_read(UINT addr, UINT* data)
{
    return vme_A32D32_read(this->usbdevice_handle, addr, data);
}
int sis1100::_vme_A32DMA_D32_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return vme_A32DMA_D32_read(this->usbdevice_handle, addr, data, request_nof_words, got_nof_words);
}
int sis1100::_vme_A32BLT32_read(UINT addr, UINT* data, UINT req_num_data, UINT* got_num_data)
{
	return vme_A32BLT32_read(this->usbdevice_handle, addr, data, req_num_data, got_num_data);
}
int sis1100::_vme_A32MBLT64_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return vme_A32MBLT64_read(this->usbdevice_handle, addr, data, request_nof_words, got_nof_words);
}
int sis1100::_vme_A32DMA_D32FIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return vme_A32DMA_D32FIFO_read(this->usbdevice_handle, addr, data, request_nof_words, got_nof_words);
}
int sis1100::_vme_A32BLT32FIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return vme_A32BLT32FIFO_read(this->usbdevice_handle, addr, data, request_nof_words, got_nof_words);
}
int sis1100::_vme_A32MBLT64FIFO_read(UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words)
{
	return vme_A32MBLT64FIFO_read(this->usbdevice_handle, addr, data, request_nof_words, got_nof_words);
}
// vme write
int sis1100::_vme_A32D32_write(UINT addr, UINT data)
{
	return vme_A32D32_write(this->usbdevice_handle, addr, data);
}
int sis1100::_vme_A32DMA_D32_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
	return vme_A32DMA_D32_write(this->usbdevice_handle, addr, data, request_nof_words, written_nof_words);
}
int sis1100::_vme_A32BLT32_write(UINT addr, PUINT data, UINT request_nof_words, PUINT written_nof_words)
{
	return vme_A32BLT32_write(this->usbdevice_handle, addr, data, request_nof_words, written_nof_words);
}
int sis1100::_vme_A32MBLT64_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
	return vme_A32MBLT64_write(this->usbdevice_handle, addr, data, request_nof_words, written_nof_words);
}
int sis1100::_vme_A32DMA_D32FIFO_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
	return vme_A32DMA_D32FIFO_write(this->usbdevice_handle, addr, data, request_nof_words, written_nof_words);
}
int sis1100::_vme_A32BLT32FIFO_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
	return vme_A32BLT32FIFO_write(this->usbdevice_handle, addr, data, request_nof_words, written_nof_words);
}
int sis1100::_vme_A32MBLT64FIFO_write(UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
	return vme_A32MBLT64FIFO_write(this->usbdevice_handle, addr, data, request_nof_words, written_nof_words);
}



{
	return -1; // not implemented
}


#endif

//sis1100::~sis1100(void)
//{
//}
