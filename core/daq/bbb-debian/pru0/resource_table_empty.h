/* Minimal resource table (zero entries) for mainline pru_rproc driver. */
#ifndef PRU_RESOURCE_TABLE_EMPTY_H
#define PRU_RESOURCE_TABLE_EMPTY_H

#include <rsc_types.h>
#include <stdint.h>

struct PRU_Resource_Table
{
    struct resource_table base;
    uint32_t offset[1];
};

#pragma DATA_SECTION(resource_table, ".resource_table")
#pragma RETAIN(resource_table)

struct PRU_Resource_Table resource_table = {
    /* ver, num, reserved[2] */
    { 1, 0, 0, 0 },
    { 0 },
};

#endif
