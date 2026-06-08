/* resource_table_empty.h — minimal PRU resource table
 *
 * An empty resource table (zero entries) satisfies the mainline pru_rproc
 * driver, which logs a warning if no resource table is present.
 * The .resource_table section placement is declared in AM335x_PRU.cmd.
 */
#ifndef RESOURCE_TABLE_EMPTY_H
#define RESOURCE_TABLE_EMPTY_H

#include <rsc_types.h>
#include <stdint.h>

struct my_resource_table {
    struct resource_table base;
    uint32_t              offset[1];
};

#pragma DATA_SECTION(resourceTable, ".resource_table")
#pragma RETAIN(resourceTable)
struct my_resource_table resourceTable = {
    { 1, 0, 0, 0 },  /* version=1, num=0, reserved={0,0} */
    { 0 },
};

#endif
