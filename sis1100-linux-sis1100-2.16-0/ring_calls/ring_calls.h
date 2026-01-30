/*
 * Filename: ring_calls.h
 *
 * Function:
 *
 * Usermode ring calls interfaces
 *
 * Author:          TH/TF
 * Date:            10.03.2017
 * Last modified:   10.10.2023
 *
 * -------------------------------------------
 *
 * Struck Innovative Systeme GmbH
 *
 * Harksheider Straße 102a
 * 22399 Hamburg
 *
 * Tel. +49 (0)40 60 87 305 0
 *
 * http://www.struck.de
 *
 * (c) 2017-2023
 */

#include <sys/cdefs.h>
__BEGIN_DECLS

#define SIS1100_REG_SR 			0x4
#define SIS1100_REG_CR 			0x8 
#define SIS1100_REG_RING 		0x14 

#define SIS1100_REG_T_HDR 		0x80 
#define SIS1100_REG_T_AM 		0x84 
#define SIS1100_REG_T_ADL 		0x88 
#define SIS1100_REG_T_DAL 		0x90 

#define SIS1100_REG_TC_DAL 		0xA0 

#define SIS1100_REG_BALANCE 		0xA8 
#define SIS1100_REG_PROT_ERROR 		0xAC 

#define SIS1100_REG_TP_SPECIAL 		0xE8 
#define SIS1100_REG_TP_DATA 		0xEC 


#define SIS1100_CR_TRANSPARENT_BIT 	0x2 
#define SIS1100_SR_TP_SPECIAL_BIT 	0x100000 
#define SIS1100_SR_TP_DATA_BIT 		0x200000 

#define Stat1100ErrorNoLink 		22 
#define Stat1100ErrorNoRing 		23 
#define Stat1100ErrorRingTimeout 	24 
 
/******************************/
/*                            */
/*    Local Ring control      */
/*                            */
/******************************/

int sis1100_Init_Ring(int p_ctrl, u_int32_t *slaveCount);

/******************************/
/*                            */
/*    Remote control access   */
/*                            */
/******************************/

int sis1100_Remote_Control_Read_Ring(int p, u_int16_t ringAddr, u_int32_t addr, u_int32_t *data);
int sis1100_Remote_Control_Write_Ring(int p, u_int16_t ringAddr, u_int32_t addr, u_int32_t data);
int sis1100_Remote_Dma_Read_Ring(int p,  u_int16_t ringAddr, u_int32_t addr,  u_int32_t fifo_mode,  u_int32_t* dmabufs,
                                 u_int32_t req_num_data, u_int32_t* got_num_data);
int sis1100_Remote_Dma_Write_Ring(int p, u_int16_t ringAddr, u_int32_t addr,  u_int32_t fifo_mode,  u_int32_t* dmabufs,
                      u_int32_t req_num_data, u_int32_t* put_num_data);

__END_DECLS
