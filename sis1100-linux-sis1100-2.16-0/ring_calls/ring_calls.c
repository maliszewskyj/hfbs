/*
 * Filename: ring_calls.c
 *
 * Function:
 *
 * Usermode ring calls interfaces
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

#define SIS1100_NEW_CTRL

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "dev/pci/sis1100_var.h" /* pfad im Makefile angeben */

#include "ring_calls.h"

/******************************/
/*                            */
/*    Local Ring control      */
/*                            */
/******************************/

int sis1100_Init_Ring(int p_ctrl, u_int32_t *slaveCount)
{
    int return_code;
    u_int32_t data;
    u_int32_t poll_counter;
    u_int32_t uint_slaveCount;
    struct sis1100_ctrl_reg reg;

    uint_slaveCount = 0;

    /* clear Balance counter, independ of history */
    reg.offset = SIS1100_REG_BALANCE;
    reg.val = 0x0;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
    if (return_code != 0) {
        return return_code;
    }

    /* ring master disable */
    reg.offset = SIS1100_REG_RING;
    reg.val = 0x30000;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
    if (return_code != 0) {
        return return_code;
    }
    usleep(10);

    /* ring master enable */
    reg.offset = SIS1100_REG_RING;
    reg.val = 0x3;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
    if (return_code != 0) {
        return return_code;
    }

    /* check link and ring up */
    poll_counter = 0;
    reg.offset = SIS1100_REG_RING;
    do {
        usleep(10000); /* wait 10ms */
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0);
        if (return_code != 0) {
            return return_code;
        }
        data = (reg.val >> 16) & 0x7; /* bit 18: ring up, bit 17: ring_send_up, bit 16: ring enable */
        poll_counter++;
    } while ((data != 0x7) && (poll_counter < 20)); /* poll max. 200ms */
    if (data != 0x7) {
        if ((data & 0x3) != 0x3) {
            return Stat1100ErrorNoLink;
        }
        else {
            return Stat1100ErrorNoRing;
        }
    }
    /* printf("after link check:   poll_counter = 0x%08x   data = 0x%08x\n", poll_counter, data ); */

    /* use transparent mode for ring address scan */
    reg.offset = SIS1100_REG_CR;
    reg.val = SIS1100_CR_TRANSPARENT_BIT;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
    if (return_code != 0) {
        return return_code;
    }

    /* ring address scan space */
    reg.offset = SIS1100_REG_TP_SPECIAL;
    reg.val = 0x0F22001c;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);

    /* ring address scan: start address 0 */
    reg.offset = SIS1100_REG_TP_DATA;
    reg.val = 0;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);

    /* wait for answer */
    usleep(10000);
    poll_counter = 0;
    reg.offset = SIS1100_REG_SR;
    do {
        usleep(10000); /* wait 10ms */
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0);
        data = (reg.val >> 16) & 0x7; /* bit 18: ring up, bit 17: ring_send_up, bit 16: ring enable */
        poll_counter++;
    } while (((reg.val & SIS1100_SR_TP_SPECIAL_BIT) != SIS1100_SR_TP_SPECIAL_BIT) &&
             (poll_counter < 20)); /* poll max. 200ms */
    if ((reg.val & SIS1100_SR_TP_SPECIAL_BIT) != SIS1100_SR_TP_SPECIAL_BIT) {
        return Stat1100ErrorRingTimeout;
    }

    /* expecting special word */
    reg.offset = SIS1100_REG_SR;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0);

    if ((reg.val & SIS1100_SR_TP_SPECIAL_BIT) != SIS1100_SR_TP_SPECIAL_BIT) {
        /* error: disable transparent mode */
        reg.offset = SIS1100_REG_CR;
        reg.val = (SIS1100_CR_TRANSPARENT_BIT << 16);
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
        if (return_code != 0) {
            return return_code;
        }
        /* error: disable ring mode */
        reg.offset = SIS1100_REG_RING;
        reg.val = 0x30000;
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
        if (return_code != 0) {
            return return_code;
        }
        /* error: clear balance counter */
        reg.offset = SIS1100_REG_BALANCE;
        reg.val = 0x0;
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
        if (return_code != 0) {
            return return_code;
        }
        return Stat1100ErrorRingTimeout;
    }
    else {
        reg.offset = SIS1100_REG_TP_SPECIAL;
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0); /* dummy read */
        if (return_code != 0) {
            return return_code;
        }
    }

    usleep(10);
    /* expecting last ring address + 1 */
    reg.offset = SIS1100_REG_SR;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0);
    if (return_code != 0) {
        return return_code;
    }

    if ((reg.val & SIS1100_SR_TP_DATA_BIT) != SIS1100_SR_TP_DATA_BIT) {
        /* error: disable transparent mode */
        reg.offset = SIS1100_REG_CR;
        reg.val = (SIS1100_CR_TRANSPARENT_BIT << 16);
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
        if (return_code != 0) {
            return return_code;
        }
        /* error: disable ring mode */
        reg.offset = SIS1100_REG_RING;
        reg.val = 0x30000;
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
        if (return_code != 0) {
            return return_code;
        }
        /* error: clear balance counter */
        reg.offset = SIS1100_REG_BALANCE;
        reg.val = 0x0;
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
        if (return_code != 0) {
            return return_code;
        }
        return Stat1100ErrorRingTimeout;
    }
    else {
        reg.offset = SIS1100_REG_TP_DATA;
        return_code = (ioctl(p_ctrl, SIS1100_CTRL_READ, &reg) < 0); /* read */
        if (return_code != 0) {
            return return_code;
        }
        /* reorder endianness */
        uint_slaveCount = ((reg.val & 0xFF000000) >> 24) | ((reg.val & 0x00FF0000) >> 8) |
                          ((reg.val & 0x0000FF00) << 8) | ((reg.val & 0x000000FF) << 24);
    }

    /* disable transparent mode for ring address scan */
    reg.offset = SIS1100_REG_CR;
    reg.val = (SIS1100_CR_TRANSPARENT_BIT << 16);
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
    if (return_code != 0) {
        return return_code;
    }

    /* clear Balance counter */
    reg.offset = SIS1100_REG_BALANCE;
    reg.val = 0x0;
    return_code = (ioctl(p_ctrl, SIS1100_CTRL_WRITE, &reg) < 0);
    if (return_code != 0) {
        return return_code;
    }

    *slaveCount = uint_slaveCount;
    return 0;
}

/******************************/
/*                            */
/*    Remote control access   */
/*                            */
/******************************/

int sis1100_Remote_Control_Read_Ring(int p, u_int16_t ringAddr, u_int32_t addr, u_int32_t *data)
{
    struct sis1100_vme_req req;

    req.size = 4; /* driver does not change any field except data */
    /*req.am   = (((u_int32_t)ringAddr) << 16 ) + 1;  "" */
    req.am = (((u_int32_t)ringAddr) << 16); /* "" */
    req.addr = addr;
    if (ioctl(p, SIS1100_RING_READ, &req) < 0)
        return -1; /* NEW */
    if (req.error)
        return req.error;
    *data = req.data;
    return 0;
}

int sis1100_Remote_Control_Write_Ring(int p, u_int16_t ringAddr, u_int32_t addr, u_int32_t data)
{
    struct sis1100_vme_req req;

    req.size = 4; /* driver does not change any field except data */
    /*req.am   = (((u_int32_t)ringAddr) << 16 ) + 1;  "" */
    req.am = (((u_int32_t)ringAddr) << 16); /* "" */
    req.addr = addr;
    req.data = data;

    if (ioctl(p, SIS1100_RING_WRITE, &req) < 0)
        return -1; /* NEW */
    if (req.error)
        return req.error;
    return 0;
}

int sis1100_Remote_Dma_Read_Ring(int p, u_int16_t ringAddr, u_int32_t addr, u_int32_t fifo_mode, u_int32_t *dmabufs,
                                 u_int32_t req_num_data, u_int32_t *got_num_data)
{
    struct sis1100_vme_block_req block_req;

    *got_num_data = 0;

    block_req.num = req_num_data; /*  */
    block_req.fifo = fifo_mode;
    block_req.size = 4;
    block_req.am = (((u_int32_t)ringAddr) << 16);
    block_req.addr = addr;
    block_req.data = (u_int8_t *)dmabufs;
    if (ioctl(p, SIS1100_RING_BLOCK_READ, &block_req) < 0)
        return -1;
    *got_num_data = block_req.num;
    return block_req.error;
}

int sis1100_Remote_Dma_Write_Ring(int p, u_int16_t ringAddr, u_int32_t addr, u_int32_t fifo_mode, u_int32_t *dmabufs,
                                  u_int32_t req_num_data, u_int32_t *put_num_data)
{
    struct sis1100_vme_block_req block_req;

    *put_num_data = 0;

    block_req.num = req_num_data; /*  */
    block_req.fifo = fifo_mode;
    block_req.size = 4;
    block_req.am = (((u_int32_t)ringAddr) << 16);
    block_req.addr = addr;
    block_req.data = (u_int8_t *)dmabufs;
    if (ioctl(p, SIS1100_RING_BLOCK_WRITE, &block_req) < 0)
        return -1;
    *put_num_data = block_req.num;
    return block_req.error;
}
