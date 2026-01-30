/*
 * Filename: cs-eeprom-write.c
 *
 * Function:
 *
 * demonstrate write accesses to the i2c WR eeprom on the sis1100 e2
 *
 * Author:			CT
 * Date:			22.05.2018
 * Last modified:	22.05.2018
 *
 * -------------------------------------------
 *
 * SIS Struck Innovative Systeme GmbH
 *
 * Harksheider Straße 102a
 * 22399 Hamburg
 *
 * Tel. +49 (0)40 60 87 305 0
 * Fax	+49 (0)40 60 87 305 20
 *
 * http://www.struck.de
 *
 * (c) 2019
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <linux/fs.h>

#include "sis1100_var.h"

#define SIS1100_REG_I2C_EEPROM			0x40
#define SIS1100_EEPROM_I2C_ADR			0x50
//#define SIS1100_EEPROM_SIZE				256
#define SIS1100_EEPROM_WRITABLE_SIZE	128
#define SIS1100_EEPROM_IDENT_OFFS		0xFA
#define SIS1100_EEPROM_IDENT_LEN		0xFF

#define I2C_BUSY				0x80000000UL
#define I2C_READ_BYTE			0x2000UL
#define I2C_WRITE_BYTE			0x1000UL
#define I2C_STOP				0x800UL
#define I2C_REPEATSTART			0x400UL
#define I2C_START				0x200UL
#define I2C_ACK					0x100UL
#define I2C_DATA_MASK			0xFFUL
#define I2C_DATA_POS			0UL

#define I2C_DIR_READ			1
#define I2C_DIR_WRITE			0

#define BUS_FREE_TIME			5000

typedef struct {
	uint32_t offset;
	uint32_t data;
	uint32_t error;
}sis1100_reg_t;


int sis1100_control_read(int device, uint32_t offset, uint32_t *data);
int sis1100_control_write(int device, uint32_t offset, uint32_t data);

int sis1100_i2c_start(int device, uint32_t base);
int sis1100_i2c_busy_wait(int device, uint32_t base);
int sis1100_i2c_stop(int device, uint32_t base);
int sis1100_i2c_read_data(int device, uint32_t base, uint8_t *data, uint8_t ack);
int sis1100_i2c_write_data(int device, uint32_t base, uint8_t data, uint8_t *ack);

int sis1100_i2c_master_transmission(int device, uint32_t base, uint8_t addr, uint8_t *txData, uint8_t txLen, uint8_t *rxData, uint8_t rxLen);


/*
 *
 */
int main(int argc, char **argv) {

	int fp;
	uint32_t ret, reg_data, offset, data;
	uint8_t eeprom_txdata[2], *eeprom_rxdata;

	if(argc != 4){
		printf("usage: %s [device] [offset] [datum]\n", argv[0]);
		return -1;
	}

	printf("***************************************\n");
	printf("*  SIS1100 e2 EEPROM Write Interface  *\n");
	printf("***************************************\n\n");

	fp = open(argv[1], O_RDWR);
		if(fp < 0){
			printf("can't open device %s\n", argv[1]);
			return -2;
	}

	printf("Information about the device:\n");

	ret = sis1100_control_read(fp, 0x0, &reg_data);
	if(ret){
		printf("'sis1100_control_read' failed: %d\n", ret);
	}
	printf(" - Fw version: \t%08X\n", reg_data);

	printf("\n");

	offset 	= (uint32_t)strtoul(argv[2], NULL, 0);
	data 	= (uint32_t)strtoul(argv[3], NULL, 0);


	eeprom_rxdata = (uint8_t*)malloc(2);
	if(eeprom_rxdata == NULL){
		printf("error allocating data buffer of size: %d\n", data);
		return -3;
	}

	printf("trying to write 0x%X(%d) to eeprom at offset: 0x%X(%d) ...\n", data, data, offset, offset);


	eeprom_txdata[0] = (uint8_t)offset;
	eeprom_txdata[1] = (uint8_t)data;

	ret = sis1100_i2c_master_transmission(
			fp,
			SIS1100_REG_I2C_EEPROM,
			SIS1100_EEPROM_I2C_ADR,
			eeprom_txdata, 2,
			NULL, 0
			);
	if(ret){
		printf("error i2c master transmission: %d\n", ret);
		return -3;
	}
	else
		printf("done!\n");


	free(eeprom_rxdata);
	close(fp);

	return EXIT_SUCCESS;
}

/*
 *
 */
int
sis1100_i2c_master_transmission(int device,
		uint32_t base, uint8_t addr,
		uint8_t *txData, uint8_t txLen,
		uint8_t *rxData, uint8_t rxLen
) {

	int rc;
	uint32_t i;
	uint8_t ack, dir;

	// argument sanity check
	if ((txLen > 0 && txData == NULL) || (rxLen > 0 && rxData == NULL) ) {
		return -1;
	}

	// start condition
	rc = sis1100_i2c_start(device, base);
	if (rc) {
		sis1100_i2c_stop(device, base);
		return rc;
	}

	// slave address cycle
	// setup the direction bit based on the following data phase
	// if an out-phase follows, set WRITE direction, else set READ direction
	dir = (txLen > 0) ? I2C_DIR_WRITE : I2C_DIR_READ;
	sis1100_i2c_write_data(device, base, ((addr & 0x7F) << 1) | dir, &ack);
	if (rc) {
		sis1100_i2c_stop(device, base);
		return rc;
	}
	// abort if slave is missing (NACK)
	if (!ack) {
		printf("abort if slave is missing (NACK)\n");
		sis1100_i2c_stop(device, base);
		return -10;
	}

	// data write cycle (if needed)
	if (txLen > 0) {
		for (i = 0; i < txLen; i++) {
			rc = sis1100_i2c_write_data(device, base, txData[i], &ack);
			if (rc) {
				sis1100_i2c_stop(device, base);
				return rc;
			}

			if (!ack) {
				sis1100_i2c_stop(device, base);
				return -10;
			}
		}
	}

	// data read cycle (if needed)
	if (rxLen > 0) {
		// generate a REPEATSTART cycle if an out-phase preceeded the in-phase
		if (dir == I2C_DIR_WRITE) {
			// the hardware currently does not implement a repeatstart cycle.
			// simulate by using a stop/start cycle.
			// this should not create any problems, since there is only one
			// master on the bus anyways.
			/*
			rc = i2c_repeatstart(dev, base);
			if (API_ERR(rc)) {
				i2c_stop(dev, base);
				return rc;
			}
			*/
			rc = sis1100_i2c_stop(device, base);
			if (rc) {
				return rc;
			}
			rc = sis1100_i2c_start(device, base);
			if (rc) {
				sis1100_i2c_stop(device, base);
				return rc;
			}
			// re-address slave
			rc = sis1100_i2c_write_data(device, base, ((addr & 0x7F) << 1) | I2C_DIR_READ, &ack);
			if (rc) {
				sis1100_i2c_stop(device, base);
				return rc;
			}
		}

		for (i = 0; i < rxLen; i++) {
			// read the data, NACK the last byte
			ack = i == (rxLen - 1) ? 0 : 1;
			//rc = sis1100_i2c_read_data(device, base, &rxData[i], ack);
			rc = sis1100_i2c_read_data(device, base, &rxData[i], ack);
			if (rc) {
				sis1100_i2c_stop(device, base);
				return rc;
			}
		}
	}

	// free bus
	sis1100_i2c_stop(device, base);
	usleep(BUS_FREE_TIME);

	return 0;
}

/*
 *
 */
int sis1100_i2c_start(int device, uint32_t base){

	if(sis1100_control_write(device, base, I2C_START)){
		return -1;
	}

	return sis1100_i2c_busy_wait(device, base);
}

/*
 *
 */
int sis1100_i2c_read_data(int device, uint32_t base, uint8_t *data, uint8_t ack){

	uint32_t reg;

	// integrate ack bit into read command
	reg = I2C_READ_BYTE | (ack ? I2C_ACK : 0);
	if(sis1100_control_write(device, base, reg)){
		return -1;
	}

	// wait for logic to finish
	if(sis1100_i2c_busy_wait(device, base)){
		return -1;
	}

	// read data from register
	if(sis1100_control_read(device, base, &reg)){
		return -1;
	}

	*data = reg & I2C_DATA_MASK;

	return 0;
}

/*
 *
 */
int sis1100_i2c_write_data(int device, uint32_t base, uint8_t data, uint8_t *ack){

	uint32_t reg;

	// start byte transmission
	reg = I2C_WRITE_BYTE | data;
	if(sis1100_control_write(device, base, reg)){
		return -1;
	}

	// wait for logic to finish
	if(sis1100_i2c_busy_wait(device, base)){
		return -1;
	}

	// read ACK/NACK from ip
	if(sis1100_control_read(device, base, &reg)){
		return -1;
	}

	// return the ACK/NACK
	*ack = reg & I2C_ACK ? 1 : 0;

	return 0;
}

/*
 *
 */
int sis1100_i2c_busy_wait(int device, uint32_t base){

	uint32_t reg;

	do{
		if(sis1100_control_read(device, base, &reg)){
			return -1;
		}

		if(reg & I2C_BUSY){
			usleep(1);
		}

	}while(reg & I2C_BUSY);

	return 0;
}

/*
 *
 */
int sis1100_i2c_stop(int device, uint32_t base){

	if(sis1100_control_write(device, base, I2C_STOP)){
		return -1;
	}

	return sis1100_i2c_busy_wait(device, base);
}



/*
 *
 */
int sis1100_control_read(int device, uint32_t offset, uint32_t *data){

	sis1100_reg_t reg;
	reg.offset = offset;

	ioctl(device, SIS1100_CTRL_READ, &reg);

	*data = reg.data;

	return reg.error;
}

/*
 *
 */
int sis1100_control_write(int device, uint32_t offset, uint32_t data){

	sis1100_reg_t reg;
	reg.offset 	= offset;
	reg.data	= data;

	ioctl(device, SIS1100_CTRL_WRITE, &reg);

	return reg.error;
}
