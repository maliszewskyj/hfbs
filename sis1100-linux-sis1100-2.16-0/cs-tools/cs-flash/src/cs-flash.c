/*
 * Filename: ce-flash.c
 *
 * Function:
 *
 *
 * Author:			CT
 * Date:			22.05.2018
 * Last modified:	13.07.2021
 *
 * -------------------------------------------
 * Mod. 13.07.2021
 *
 *   - Add handling of 'fread' return values
 *     to avoid compiler warnings
 * -------------------------------------------
 *
 * SIS Struck Innovative Systeme GmbH
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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <linux/fs.h>
#include <sys/time.h>

#include "sis1100_var.h"

#define SIS1100_FLASHSIZE 						8388608 // 8MB
#define SIS1100_PAGESIZE 						256 // 256B
#define SIS1100_BLOCKSIZE 						65536 // 64kB
#define SIS1100_READBLOCKSIZE 					2048 // 2kB

#define SIS1100_SPI_FLASH_CONTROL_REG			0x44
#define SPI_FLASH_RD_BLK_FIFO 					9
#define SPI_FLASH_RD_BLK_EN   					10
#define SPI_FLASH_WR_BLK_FILL 					11
#define SPI_FLASH_EXCH        					12
#define SPI_FLASH_WR_BLK_EN   					13   // JK
#define SPI_FLASH_CS          					14   // JK
#define SPI_FLASH_MUX_EN      					15   // JK
#define SPI_FLASH_BUSY        					31

#define TRUE									1
#define FALSE									0

#define GET_TIME

typedef struct {
	uint32_t offset;
	uint32_t data;
	uint32_t error;
}sis1100_reg_t;


int sis1100_control_read(int device, uint32_t offset, uint32_t *data);
int sis1100_control_write(int device, uint32_t offset, uint32_t data);

int sis1100_spiByteExch(int device, uint8_t in, uint8_t *out);
int sis1100_spiFlashPollBusy(int device, int timeout);
int sis1100_spiFlashReadBusy(int device, int *busy);

int sis1100_spiFlashWriteEnable(int device);
int sis1100_spiFlashEraseBlock(int device, uint32_t addr);
int sis1100_spiFlashReadBlock(int device, uint32_t addr, uint8_t *data, uint32_t len);
int sis1100_spiFlashProgramPage(int device, uint32_t addr, uint8_t *data, uint32_t len);

int sis1100_spiFlashUpdateFirmware(int device, char *path, void (*cb)(int percent));
int sis1100_spiFlashVerifyFirmware(int device, char *path, void (*cb)(int percent));

long get_time_ms(void);

void porgressbar(int percent);

#if 1
void porgressbar(int percent)
{
	int i;

	long current_time_ms;
	static long start_time = 0;
	static long final_time = 0;
	static long time_elapsed = 0;

	current_time_ms = get_time_ms();
	if(percent == 0) start_time = current_time_ms; // time in millisecond
	if(percent)      final_time = (current_time_ms - start_time)/(percent*10);
	time_elapsed = (current_time_ms - start_time)/1000;



	printf("\r %3d%% [", percent);
#if 1	// style: progress bar
	for(i=0; i<percent/2;i++)
		printf("#");
	for(i+=1; i<51;i++)
		printf(".");
#endif
#if 0	// style: growing arrow
	for(i=0; i<percent/2;i++)
		printf("=");
	printf(">");
	for(i+=1; i<51;i++)
		printf(" ");
#endif
	printf("]  %02ldm %02lds  ETE %02ldm %02lds", time_elapsed/60, time_elapsed%60, final_time/60, final_time%60);
	fflush(stdout);
}
#endif

/*
 *
 */
int main(int argc, char **argv) {

	int fp;
	char menu;
	uint32_t ret, reg_data;

	if(argc != 3){
		printf("usage: %s [device] [path to *.bin file]\n", argv[0]);
		return -1;
	}

	printf("********************************\n");
	printf("*  SIS1100 e2 Flash Interface  *\n");
	printf("********************************\n\n");


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


	printf("Use flash file: %s\n", argv[2]);
	printf("\n\n");


	printf("Supported operations:\n");
	printf(" - Update firmware:          [ U|u ]\n");
	printf(" - Verify firmware:          [ V|v ]\n");
	printf(" - Exit:                     [ E|e ]\n");
	printf("\n");

	printf("Please select an operation:  ");
	fflush(stdout);
	fflush(stdin);
	menu = getchar();
	fflush(stdin);
	printf("\n");

	switch (menu)
	{
	case 'U':
	case 'u':
		printf("Programing ...\n\n");
		ret = sis1100_spiFlashUpdateFirmware(fp, argv[2], porgressbar);
		printf("\n\n");

		if(ret)	{
			printf( "Program failed: %d\n", ret );
		}
		else
		{
			printf("Programming completed successfully!\n");
			printf("Please restart the PC to activate the new firmware!\n\n");
		}
		break;
	case 'V':
	case 'v':
		printf("Verifying ...\n\n");
		ret = sis1100_spiFlashVerifyFirmware(fp, argv[2], porgressbar);
		printf("\n\n");

		if(ret)
			printf( "Verify failed: %d\n", ret );
		else
			printf("Verifying completed successfully!\n\n");
		break;
	case 'E':
	case 'e':
	case 'Q':
	case 'q':
		printf("Abort\n\n");
		break;
	default:
		printf("Unsupported operation! Abort!\n\n");
		break;
	}


    close(fp);

	return EXIT_SUCCESS;
}

/*
 *
 */
int sis1100_spiFlashUpdateFirmware(int device, char *path, void (*cb)(int percent)){
    int ret;
    int percent = 0, percentOld = 0;

    // reset logic
    sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 0xffff0000);


    // read image into buffer
    FILE *fp = fopen(path, "rb");
    if(!fp){
    	printf("ERROR: can't open firmware file!\n");
        return -1;
    }

    if(strstr(path,".bin") == NULL){
        fclose(fp);
        printf("ERROR: firmware file format not supported!\n");
        return -2;
    }

    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);
    rewind(fp);

    if(fileSize > SIS1100_FLASHSIZE){
        fclose(fp);
        printf("ERROR: firmware file format too large!\n");
        return -3;
    }

    uint8_t *image = (uint8_t *)malloc(fileSize * sizeof(uint8_t));
    if(image == NULL){
        fclose(fp);
        printf("ERROR: memory allocation failed!\n");
        return -4;
    }

    ret = fread(image, sizeof(uint8_t), fileSize, fp);
    fclose(fp);

    // invoke callback
    if(cb)
        cb(percent);

    // erase blocks, write pages
    uint8_t *verify = (uint8_t *)malloc(SIS1100_PAGESIZE * sizeof(uint8_t));
    if(verify == NULL){
        free(image);
        printf("ERROR: memory allocation failed!\n");
        return -4;
    }

    uint32_t written = 0;
    uint32_t pageProgramSize = 0;
    while(written < fileSize){
        // erase current block, 64kB
        if((written & (SIS1100_BLOCKSIZE - 1)) == 0){
            ret = sis1100_spiFlashEraseBlock(device, written);
            if(ret){
                return ret;
            }
        }

        // program page, 256B
        if(fileSize >= (written + SIS1100_PAGESIZE)){
            pageProgramSize = SIS1100_PAGESIZE;
        }else{
            pageProgramSize = fileSize - written;
        }

        ret = sis1100_spiFlashProgramPage(device, written, image + written, pageProgramSize);
        if(ret){
            return ret;
        }

        // verify page
        ret = sis1100_spiFlashReadBlock(device, written, verify, pageProgramSize);
        if(ret){
            return ret;
        }

        if(memcmp(image + written, verify, pageProgramSize)){
            int i;
            printf("\nError:  pageProgramSize = %08X \n", pageProgramSize);
            printf("\nsoll\n");
            for(i = 0;i < 256;i++){
                if((i&7)==0) printf("\n[%04X]\t", written + i);

                printf("%02X ", *(image + written + i));
            }

            printf("\nist\n");
            for(i = 0;i < 256;i++){
                if((i&7)==0) printf("\n[%04X]\t", written + i);

                printf("%02X ", *(verify + i));
            }


            free(image);
            free(verify);
            return -1;
        }

        written += SIS1100_PAGESIZE;

        if(cb){
            percent = written * 100 / fileSize;
            if(percent != percentOld){
                (cb)(percent);
                percentOld = percent;
            }
        }
    }

    free(image);
    free(verify);

    return 0;
}

/*
 *
 */
int sis1100_spiFlashVerifyFirmware(int device, char *path, void (*cb)(int percent)){
    int ret;
    int percent = 0, percentOld = 0;

    // reset logic
    sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 0xffff0000);


    // read image into buffer
    FILE *fp = fopen(path, "rb");
    if(!fp){
    	printf("ERROR: can't open firmware file!\n");
        return -1;
    }

    if(strstr(path,".bin") == NULL){
        fclose(fp);
        printf("ERROR: firmware file format not supported!\n");
        return -2;
    }

    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);
    rewind(fp);

    if(fileSize > SIS1100_FLASHSIZE){
        fclose(fp);
        printf("ERROR: firmware file format too large!\n");
        return -3;
    }

    uint8_t *image = (uint8_t *)malloc(fileSize * sizeof(uint8_t));
    if(image == NULL){
        fclose(fp);
        printf("ERROR: memory allocation failed!\n");
        return -4;
    }

    ret = fread(image, sizeof(uint8_t), fileSize, fp);
    fclose(fp);

    // invoke callback
    if(cb)
        cb(percent);

    // erase blocks, write pages
    uint8_t *verify = (uint8_t *)malloc(SIS1100_PAGESIZE * sizeof(uint8_t));
    if(verify == NULL){
        free(image);
        printf("ERROR: memory allocation failed!\n");
        return -4;
    }

    uint32_t written = 0;
    uint32_t pageProgramSize = 0;
    while(written < fileSize){
#if 0
        // erase current block, 64kB
        if((written & (SIS1100_BLOCKSIZE - 1)) == 0){
            ret = sis1100_spiFlashEraseBlock(device, written);
            if(ret){
                return ret;
            }
        }
#endif
        // program page, 256B
        if(fileSize >= (written + SIS1100_PAGESIZE)){
            pageProgramSize = SIS1100_PAGESIZE;
        }else{
            pageProgramSize = fileSize - written;
        }
#if 0
        ret = sis1100_spiFlashProgramPage(device, written, image + written, pageProgramSize);
        if(ret){
            return ret;
        }
#endif
        // verify page
        ret = sis1100_spiFlashReadBlock(device, written, verify, pageProgramSize);
        if(ret){
            return ret;
        }

        if(memcmp(image + written, verify, pageProgramSize)){
            int i;
            printf("\nError:  pageProgramSize = %08X \n", pageProgramSize);
            printf("\nsoll\n");
            for(i = 0;i < 256;i++){
                if((i&7)==0) printf("\n[%04X]\t", written + i);

                printf("%02X ", *(image + written + i));
            }

            printf("\nist\n");
            for(i = 0;i < 256;i++){
                if((i&7)==0) printf("\n[%04X]\t", written + i);

                printf("%02X ", *(verify + i));
            }


            free(image);
            free(verify);
            return -1;
        }

        written += SIS1100_PAGESIZE;

        if(cb){
            percent = written * 100 / fileSize;
            if(percent != percentOld){
                (cb)(percent);
                percentOld = percent;
            }
        }
    }

    free(image);
    free(verify);

    return 0;
}

/*
 *
 */
int sis1100_spiFlashProgramPage(int device, uint32_t addr, uint8_t *data, uint32_t len)
{
    int ret;
    int i;

    ret = sis1100_spiFlashWriteEnable(device);
    if(ret){
        return ret;
    }

    // gain control of spi lines
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_MUX_EN);
    if(ret){
        return ret;
    }

    // select flash
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_CS);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // load command, PROGRAM PAGE = 0x02
    ret = sis1100_spiByteExch(device, 0x02, NULL);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // load address, 24bit
    for(i = 2;i >= 0;i--){
        ret = sis1100_spiByteExch(device, (addr >> (8 * i)) & 0xFF, NULL);
        if(ret){
            // try to disable flash mux back to mmc access
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
            return ret;
        }
    }

    // enable block mode
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_WR_BLK_EN);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }


    // shift data in
    for(i = 0;i < len;i++){
        ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_WR_BLK_FILL) | *(data+i));
        if(ret){
            // try to disable flash mux back to mmc access
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_WR_BLK_EN)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
            return ret;
        }
    }

    // poll fsm busy
    uint32_t reg;
    do{
        ret = sis1100_control_read(device, SIS1100_SPI_FLASH_CONTROL_REG, &reg);
        if(ret){
            // try to disable flash mux back to mmc access
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_WR_BLK_EN)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
            return ret;
        }
    }while(reg & (1<<SPI_FLASH_BUSY));

    // disable block mode
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_WR_BLK_EN)<<16);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // deselect flash and return mux to mmc access
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
    if(ret){
        return ret;
    }

    // poll busy, for a maximum of 5 ms
    // datasheet: page program max: 3ms
    ret = sis1100_spiFlashPollBusy(device, 5);
    if(ret){
        return ret;
    }

    return 0;
}

/*
 *
 */
int sis1100_spiFlashReadBlock(int device, uint32_t addr, uint8_t *data, uint32_t len)
{
    int ret;
    int i;

    // gain control of spi lines
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_MUX_EN);
    if(ret){
        return ret;
    }

    // select flash
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_CS);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // load command, READ DATA = 0x03
    ret = sis1100_spiByteExch(device, 0x03, NULL);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // load address, 24bit
    for(i = 2;i >= 0;i--){
        ret = sis1100_spiByteExch(device, (addr >> (8 * i)) & 0xFF, NULL);
        if(ret){
            // try to disable flash mux back to mmc access
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
            return ret;
        }
    }

    // read data out, select blockmode
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_RD_BLK_EN);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }
    usleep(1);

    // readout fifo
    uint32_t reg;
    for(i = 0;i < len;i++){
        ret = sis1100_control_read(device, SIS1100_SPI_FLASH_CONTROL_REG, &reg);
        if(ret){
            // try to disable flash mux back to mmc access
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_RD_BLK_EN)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
            return ret;
        }

        *(data+i) = (uint8_t)reg;
        // advance fifo
        ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_RD_BLK_FIFO));
        if(ret){
            // try to disable flash mux back to mmc access
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_RD_BLK_EN)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
            return ret;
        }
    }

    // read data out, deselect blockmode
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_RD_BLK_EN)<<16);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // deselect flash and return mux to mmc access
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
    if(ret){
        return ret;
    }

    return 0;
}

/*
 *
 */
int sis1100_spiFlashEraseBlock(int device, uint32_t addr)
{
    int ret;
    int i;

    ret = sis1100_spiFlashWriteEnable(device);
    if(ret){
        return ret;
    }

    // gain control of spi lines
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_MUX_EN);
    if(ret){
        return ret;
    }

    // select flash
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_CS);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // load command, BLOCK ERASE 64kB = 0xD8
    ret = sis1100_spiByteExch(device, 0xD8, NULL);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // load address, 24bit
    for(i = 2;i >= 0;i--){
        ret = sis1100_spiByteExch(device, (addr >> (8 * i)) & 0xFF, NULL);
        if(ret){
            // try to disable flash mux back to mmc access
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
        	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
            return ret;
        }
    }

    // deselect flash and return mux to mmc access
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
    if(ret){
        return ret;
    }

    // poll busy, for a maximum of 2 seconds
    // datasheet: blockerase max: 1000ms
    ret = sis1100_spiFlashPollBusy(device, 2000);
    if(ret){
        return ret;
    }

    return 0;
}

/*
 *
 */
int sis1100_spiFlashWriteEnable(int device)
{
    int ret;

    // gain control of spi lines
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_MUX_EN);
    if(ret){
        return ret;
    }

    // select flash
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_CS);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // load command, WRITE ENABLE = 0x06
    ret = sis1100_spiByteExch(device, 0x06, NULL);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // deselect flash and return mux to mmc access
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
    if(ret){
        return ret;
    }

    return 0;
}

/*
 *
 */
int sis1100_spiFlashPollBusy(int device, int timeout)
{
    int ret;
    int busy = TRUE;
    int wait100ms = timeout > 100 ? TRUE : FALSE;
    int maxWaitLoops = timeout;

    if(wait100ms){
        maxWaitLoops = (timeout + 50) / 100; // roundup to nearest 100ms
    }

    while((busy == TRUE) && (maxWaitLoops > 0)){
        ret = sis1100_spiFlashReadBusy(device, &busy);
        if(ret){
            return ret;
        }

        if(busy){
            if(wait100ms){
                usleep(100000); // 100 msec
            }else{
                usleep(1000); // 1 msec
            }

            maxWaitLoops--;
        }

    }
    if(maxWaitLoops == 0){
        return -1;
    }

    return 0;
}

/*
 *
 */
int sis1100_spiFlashReadBusy(int device, int *busy)
{
    int ret;

    // gain control of spi lines
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_MUX_EN);
    if(ret){
        return ret;
    }

    // select flash
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, 1<<SPI_FLASH_CS);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // load command, READ STATUS REGISTER-1 = 0x05
    ret = sis1100_spiByteExch(device, 0x05, NULL);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    // exchange byte
    uint8_t reg;
    ret = sis1100_spiByteExch(device, 0, &reg);
    if(ret){
        // try to disable flash mux back to mmc access
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    	sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
        return ret;
    }

    *busy = FALSE;
    if(reg & 1){
        *busy = TRUE;
    }

    // deselect flash and return mux to mmc access
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_CS)<<16);
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_MUX_EN)<<16);
    if(ret){
        return ret;
    }

    return 0;
}


/*
 *
 */
int sis1100_spiByteExch(int device, uint8_t in, uint8_t *out)
{
    int ret;
    int maxBusyLoops = 1000;

    // start transfer
    ret = sis1100_control_write(device, SIS1100_SPI_FLASH_CONTROL_REG, (1<<SPI_FLASH_EXCH) | in);
    if(ret){
        return ret;
    }

    // poll fsm busy
    uint32_t reg;
    int busy = TRUE;
    while(busy == TRUE && maxBusyLoops > 0){
        ret = sis1100_control_read(device, SIS1100_SPI_FLASH_CONTROL_REG, &reg);
        if(ret){
            return ret;
        }

        if(!(reg & (1<<SPI_FLASH_BUSY))){
            busy = FALSE;
        }

        usleep(1);
        maxBusyLoops--;
    }
    if(maxBusyLoops == 0){
        return -1;
    }

    if(out){
        *out = (uint8_t)reg;
    }

    return 0;
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
long get_time_ms(void){
#ifdef __Win
	return timeGetTime();
#endif
#ifdef __linux
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
#endif


}
