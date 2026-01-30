/*
 * Filename: cs-eeprom-configurator.c
 *
 * Function:
 *
 * Author:			CT/TF
 * Date:			23.05.2018
 * Last modified:	23.07.2021
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
 * (c) 2021
 */

//-----------------------------------------------------------------------------
#define APP_VERSION		"0.0.3"
#define APP_DATE		"23. JUL 2021"
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
#define MEM_OFFSET_BS			0
#define MEM_OFFSET_SN			MEM_OFFSET_BS + 0
#define MEM_OFFSET_LS			MEM_OFFSET_BS + 3
#define BUS_FREE_TIME			5000
//-----------------------------------------------------------------------------
typedef struct {
	uint32_t offset;
	uint32_t data;
	uint32_t error;
}sis1100_reg_t;
//-----------------------------------------------------------------------------
int sis1100_control_read(int device, uint32_t offset, uint32_t *data);
int sis1100_control_write(int device, uint32_t offset, uint32_t data);

int sis1100_i2c_start(int device, uint32_t base);
int sis1100_i2c_busy_wait(int device, uint32_t base);
int sis1100_i2c_stop(int device, uint32_t base);
int sis1100_i2c_read_data(int device, uint32_t base, uint8_t *data, uint8_t ack);
int sis1100_i2c_write_data(int device, uint32_t base, uint8_t data, uint8_t *ack);

int sis1100_i2c_master_transmission(int device, uint32_t base, uint8_t addr, uint8_t *txData, uint8_t txLen, uint8_t *rxData, uint8_t rxLen);

void print_command_line_help(char* command) ;
//-----------------------------------------------------------------------------
int main(int argc, char **argv) {

	int fp;
	char ch;

	char	 device_path[50] = "/dev/sis1100_00ctrl";
	uint32_t cmd_speed_in;

	uint8_t	 cmd_flag_set_speed = 0;
	uint8_t	 cmd_flag_read_back = 0;

	uint32_t ret, reg_data;
	uint8_t  eeprom_txdata[20], eeprom_rxdata[20];



	// parameter
	while ((ch = getopt(argc, argv, "?rd:s:")) != -1){
	switch(ch){
		case 'd':
			memset(device_path, 0, 50);
			sscanf(optarg, "%s", device_path);
			break;
		case 's':
			sscanf(optarg, "%d", &cmd_speed_in);
			cmd_flag_set_speed = 1;
			break;
		case 'r':
			cmd_flag_read_back = 1;
			break;
		case '?':
		default:
			print_command_line_help(argv[0]);
			return EXIT_FAILURE;
			break;
	}
	}


	if(argc < 2 ){
		print_command_line_help(argv[0]);
		return EXIT_FAILURE;
	}



	fp = open(device_path, O_RDWR);
		if(fp < 0){
			printf("can't open device %s\n", device_path);
			return EXIT_FAILURE;
	}
		

	// setup link speed
	if(cmd_flag_set_speed){

//		printf("cmd_flag_set_speed: %d\n", cmd_speed_in);

		eeprom_txdata[0] =   MEM_OFFSET_LS;
		eeprom_txdata[1] =   (uint8_t)cmd_speed_in;

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

		// force the card to update the linkspeed configuration at runtime.
		ret = sis1100_control_write(fp, 0x48, 0);
		if (ret) {
			printf("error in 'sis1100_control_write': %d\n", ret);
			return -3;
		}
	}

	// read back
	if(cmd_flag_read_back){
		printf("************************************\n");
		printf("*  SIS1100 e2 EEPROM Configurator  *\n");
		printf("************************************\n\n");

		eeprom_txdata[0] = MEM_OFFSET_BS; // offset

		ret = sis1100_i2c_master_transmission(
				fp,
				SIS1100_REG_I2C_EEPROM,
				SIS1100_EEPROM_I2C_ADR,
				eeprom_txdata, 1,
				eeprom_rxdata, 4
				);
		if(ret){
			printf("error i2c master transmission: %d\n", ret);
			return -3;
		}

		printf("Information about the device:\n");

		ret = sis1100_control_read(fp, 0x0, &reg_data);
		if(ret){
			printf("'sis1100_control_read' failed: %d\n", ret);
			return -3;
		}
		else
			printf(" - Fw version: \t%08X\n", reg_data);

		printf(" - SIS SN: \t%4d\n",*(uint16_t*)eeprom_rxdata);

		printf(" - Link speed: \t%s\n",eeprom_rxdata[3]&0x1?"2.5 GHz":"1.25 GHz");

		printf("\n");
	}



	close(fp);

	return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
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
		sis1100_i2c_stop(device, base);
		printf("abort if slave is missing (NACK)\n");
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

/*
 *
 */
void print_command_line_help(char* command) {

	printf("%s (Version: %s - %s)\n", command, APP_VERSION, APP_DATE);
	printf("Command line interface to setup the configuration EEPROM.\n");
	printf("\n");
	printf("usage: %s [-?] [-d device] [-s link speed] [-r]\n", command);
	printf("\n");
	printf("    -d path     Path to device (default: /dev/sis1100_00ctrl)\n");
	printf("    -s num      Configure the link speed (default: num = 0, 0: 1.25 GHz, 1: 2.5 GHz)\n");
	printf("\n");
	printf("    -r          Read the current setup from the EEPROM (optional after programming)\n");
	printf("\n");
	printf("    -?          Print this message\n");
	printf("\n");
	printf("Example: ./cs-eeprom-configurator -d /dev/sis1100_00ctrl -s 1\n");
	printf("\n");
}
