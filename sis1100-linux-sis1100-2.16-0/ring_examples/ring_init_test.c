/*
 * Filename: ring_init_test.c
 *
 * Function:
 *
 * Ringmode initialization example
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
    int return_code;
    int loop_counter;
    unsigned int uint_slaveCount;
    struct sis1100_ctrl_reg reg;

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

    loop_counter = 0;
    do {
        return_code = sis1100_Init_Ring(p_ctrl, &uint_slaveCount);
        printf("sis1100_Init_Ring:  loop_counter = %d   \treturn_code = 0x%08x   uint_slaveCount = %d  \n",
               loop_counter, return_code, uint_slaveCount);
        usleep(200000);
        loop_counter++;
    } while (return_code == 0);

    printf("\n");

    reg.offset = SIS1100_REG_BALANCE;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0);
    printf("SIS1100_CTRL_READ SIS1100_REG_BALANCE:  offset = 0x%08x   data = 0x%08x\n", reg.offset, reg.val);

    reg.offset = SIS1100_REG_T_HDR;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0);
    printf("SIS1100_CTRL_READ SIS1100_REG_T_HDR:    offset = 0x%08x   data = 0x%08x\n", reg.offset, reg.val);

    reg.offset = SIS1100_REG_T_AM;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0);
    printf("SIS1100_CTRL_READ SIS1100_REG_T_AM:     offset = 0x%08x   data = 0x%08x\n", reg.offset, reg.val);

    reg.offset = SIS1100_REG_SR;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0);
    printf("SIS1100_CTRL_READ SIS1100_REG_SR:       offset = 0x%08x   data = 0x%08x\n", reg.offset, reg.val);

    reg.offset = SIS1100_REG_CR;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0);
    printf("SIS1100_CTRL_READ SIS1100_REG_CR:       offset = 0x%08x   data = 0x%08x\n", reg.offset, reg.val);

    close(p_ctrl);
    close(p_ring);
    return 0;
}
