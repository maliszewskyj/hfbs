/*
 * Filename: ring_dma_write_read.c
 *
 * Function:
 *
 * Ringmode read/write example for block transfer access
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
    int i;

    unsigned int uint_slaveCount;
    u_int32_t req_num_data;
    u_int32_t got_num_data;
    u_int32_t dma_read_data_buf[0x100];
    u_int32_t put_num_data;
    u_int32_t dma_write_data_buf[0x100];

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

    /* read version registers */
    for (i_ring_addr = 0; i_ring_addr < uint_slaveCount; i_ring_addr++) {
        return_code = sis1100_Remote_Control_Read_Ring(p_ring, i_ring_addr, addr, &data);
        if (return_code != 0) {
            printf("sis1100_Remote_Control_Read_Ring failed:    return_code = 0x%08x   return_code = %d\n", return_code,
                   return_code);
            return return_code;
        }
        printf("sis1100_Remote_Control_Read_Ring:  ring_addr = 0x%04x  addr = 0x%08x   data = 0x%08x  return_code = "
               "0x%08x  \n",
               i_ring_addr, addr, data, return_code);
    }

    i_ring_addr = 0x9035;
    addr = 0x200204;
    req_num_data = 0x10;

    loop_counter = 0;
    do {
        return_code = sis1100_Remote_Dma_Read_Ring(p_ring, i_ring_addr, addr, 0 /*fifo mode*/, dma_read_data_buf,
                                                   req_num_data, &got_num_data);

        if (return_code != 0) {
            printf("sis1100_Remote_Dma_Read_Ring failed:    return_code = 0x%08x   return_code = %d\n", return_code,
                   return_code);
        }

        printf("\n");
        printf("loop_counter = %d    \n", loop_counter);
        usleep(100000);
        loop_counter++;
    } while (1);

    loop_counter = 0;
    do {
        for (i_ring_addr = 0; i_ring_addr < uint_slaveCount; i_ring_addr++) {
            addr = 0;
            req_num_data = 0x10;
            return_code = sis1100_Remote_Dma_Read_Ring(p_ring, i_ring_addr, addr, 0 /*fifo mode*/, dma_read_data_buf,
                                                       req_num_data, &got_num_data);
            if (return_code != 0) {
                printf("sis1100_Remote_Dma_Read_Ring failed:    return_code = 0x%08x   return_code = %d\n", return_code,
                       return_code);
                return return_code;
            }
            for (i = 0; i < got_num_data; i++) {
                printf("ring_addr = 0x%04x    i = 0x%02x   data = 0x%08x  \n", i_ring_addr, i, dma_read_data_buf[i]);
            }

            printf("\n");
            addr = 0x0;
            for (i = 0; i < req_num_data; i++) {
                dma_write_data_buf[i] = loop_counter + i;
            }
            return_code = sis1100_Remote_Dma_Write_Ring(p_ring, i_ring_addr, addr, 0 /*fifo mode*/, dma_write_data_buf,
                                                        req_num_data, &put_num_data);
            if (return_code != 0) {
                printf("sis1100_Remote_Dma_Write_Ring failed:    return_code = 0x%08x   return_code = %d\n",
                       return_code, return_code);
                return return_code;
            }
            if (req_num_data != put_num_data) {
                printf("sis1100_Remote_Dma_Write_Ring failed:    req_num_data = 0x%08x   put_num_data = %d\n",
                       req_num_data, put_num_data);
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
