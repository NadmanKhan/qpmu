/*
 * Copyright (C) 2016-2018 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *	* Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 *
 *	* Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the
 *	  distribution.
 *
 *	* Neither the name of Texas Instruments Incorporated nor the names of
 *	  its contributors may be used to endorse or promote products derived
 *	  from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RSC_TABLE_PRU_H_
#define _RSC_TABLE_PRU_H_

#include <stddef.h>
#include <rsc_types.h>
#include "pru_virtio_ids.h"

/*
 * Sizes of the virtqueues (expressed in number of buffers supported,
 * and must be power of 2)
 */
#define PRU_RPMSG_VQ0_SIZE	16
#define PRU_RPMSG_VQ1_SIZE	16

/*
 * The feature bitmap for virtio rpmsg
 */
#define VIRTIO_RPMSG_F_NS	0		//name service notifications

/* This firmware supports name service notifications as one of its features */
#define RPMSG_PRU_C0_FEATURES	(1 << VIRTIO_RPMSG_F_NS)

/*
 * Interrupt map for mainline kernel pru_rproc driver.
 *
 * The old resource_table TYPE_POSTLOAD_VENDOR (type 5) custom entry is not
 * supported by mainline kernels (6.x). The mainline pru_rproc driver instead
 * reads interrupt routing from a ".pru_irq_map" ELF section whose layout is:
 *
 *   struct { uint8_t type; uint8_t num_evts; struct { uint8_t event; uint8_t chnl; uint8_t host; } maps[]; }
 *
 * For RPMsg on PRU1 (sysevt 18 = PRU1->ARM, sysevt 19 = ARM->PRU1):
 *   sysevt 18 -> channel 3 -> host 3  (TO_ARM_HOST)
 *   sysevt 19 -> channel 1 -> host 1  (FROM_ARM_HOST)
 *
 * type=0 selects the standard (non-K3) interrupt map format.
 */
struct pru_irq_rsc {
	uint8_t  type;
	uint8_t  num_evts;
	struct {
		uint8_t event;
		uint8_t chnl;
		uint8_t host;
	} maps[2];
};

#pragma DATA_SECTION(pru_irq_map, ".pru_irq_map")
#pragma RETAIN(pru_irq_map)
struct pru_irq_rsc pru_irq_map = {
	0,  /* type: 0 = standard (non-K3) */
	2,  /* num_evts */
	{
		{ 18, 3, 3 },  /* sysevt 18 -> channel 3 -> host 3 (PRU1->ARM, TO_ARM_HOST) */
		{ 19, 1, 1 },  /* sysevt 19 -> channel 1 -> host 1 (ARM->PRU1, FROM_ARM_HOST) */
	},
};

struct my_resource_table {
	struct resource_table base;

	uint32_t offset[1]; /* Should match 'num' in actual definition */

	/* rpmsg vdev entry */
	struct fw_rsc_vdev rpmsg_vdev;
	struct fw_rsc_vdev_vring rpmsg_vring0;
	struct fw_rsc_vdev_vring rpmsg_vring1;
};

#pragma DATA_SECTION(resourceTable, ".resource_table")
#pragma RETAIN(resourceTable)
struct my_resource_table resourceTable = {
	1,  /* Resource table version */
	1,  /* number of entries: vdev only (no custom PRU_INTS entry - not supported
	     * by mainline kernels; interrupt map is in .pru_irq_map section instead) */
	0, 0,  /* reserved, must be zero */
	/* offsets to entries */
	{
		offsetof(struct my_resource_table, rpmsg_vdev),
	},

	/* rpmsg vdev entry */
	{
		(uint32_t)TYPE_VDEV,                    //type
		(uint32_t)VIRTIO_ID_RPMSG,              //id
		(uint32_t)0,                            //notifyid
		(uint32_t)RPMSG_PRU_C0_FEATURES,	//dfeatures
		(uint32_t)0,                            //gfeatures
		(uint32_t)0,                            //config_len
		(uint8_t)0,                             //status
		(uint8_t)2,                             //num_of_vrings, only two is supported
		{ (uint8_t)0, (uint8_t)0 },             //reserved
		/* no config data */
	},
	/* the two vrings */
	{
		FW_RSC_ADDR_ANY,        //da, will be populated by host, can't pass it in
		16,                     //align (bytes),
		PRU_RPMSG_VQ0_SIZE,     //num of descriptors
		0,                      //notifyid, will be populated, can't pass right now
		0                       //reserved
	},
	{
		FW_RSC_ADDR_ANY,        //da, will be populated by host, can't pass it in
		16,                     //align (bytes),
		PRU_RPMSG_VQ1_SIZE,     //num of descriptors
		0,                      //notifyid, will be populated, can't pass right now
		0                       //reserved
	},
};

#endif /* _RSC_TABLE_PRU_H_ */
