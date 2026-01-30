/*
 * Filename: ring_ctrl_write_read.c
 *
 * Function:
 *
 * Ringmode read/write example for single cycle control register access
 *
 * Author:        TH/TF
 * Date:          10.03.2017
 * Last modified: 10.10.2023
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

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "dev/pci/sis1100_var.h"

#include "ring_calls.h"

/****************************************************************************/
int main(int argc, char *argv[])
{
    int p_ctrl, p_ring;
    u_int16_t i_ring_addr;
    u_int32_t addr;
    u_int32_t data;
    int return_code;
    int loop_counter;

    unsigned int uint_slaveCount;

    if (argc < 2) {
        fprintf(stderr, "usage: %s  path_ctrl  path_ring \n", argv[0]);
        return 1;
    }

    if ((p_ctrl = open(argv[1], O_RDWR, 0)) < 0) {
        perror("open");
        return 1;
    }

    if ((p_ring = open(argv[2], O_RDWR, 0)) < 0) {
        perror("open");
        return 1;
    }

    printf("\n");
    return_code = sis1100_Init_Ring(p_ctrl, &uint_slaveCount);
    if (return_code != 0) {
        printf("sis1100_Init_Ring failed:    return_code = 0x%08x   return_code = %d\n", return_code, return_code);
        return return_code;
    }
    printf("sis1100_Init_Ring:  found %d slave(s)  \n", uint_slaveCount);
    printf("\n");

    loop_counter = 0;
    do {
        for (i_ring_addr = 0; i_ring_addr < uint_slaveCount; i_ring_addr++) {
            for (addr = 0; addr < 4; addr++) {
                return_code = sis1100_Remote_Control_Read_Ring(p_ring, i_ring_addr, addr, &data);
                if (return_code != 0) {
                    printf("sis1100_Remote_Control_Read_Ring failed:    return_code = 0x%08x   return_code = %d\n",
                           return_code, return_code);
                    return return_code;
                }
                printf("sis1100_Remote_Control_Read_Ring:  ring_addr = 0x%04x  addr = 0x%08x   data = 0x%08x  "
                       "return_code = 0x%08x  \n",
                       i_ring_addr, addr, data, return_code);
            }
            printf("\n");
            addr = 0x0;
            data = loop_counter;
            return_code = sis1100_Remote_Control_Write_Ring(p_ring, i_ring_addr, addr, data);
            if (return_code != 0) {
                printf("sis1100_Remote_Control_Read_Ring failed:    return_code = 0x%08x   return_code = %d\n",
                       return_code, return_code);
                return return_code;
            }
        }
        printf("\n");
        printf("loop_counter = %d    \n", loop_counter);
        usleep(100000);
        loop_counter++;
    } while (return_code == 0);

    printf("\n");
    close(p_ctrl);
    close(p_ring);
    return 0;
}
