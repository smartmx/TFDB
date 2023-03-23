/*
 * Copyright (c) 2022-2023, smartmx - smartmx@qq.com
 *
 * SPDX-License-Identifier: MIT
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-02-03     smartmx      the first version
 * 2022-02-08     smartmx      fix bugs
 * 2022-02-12     smartmx      fix bugs, add support for 2 byte write flash
 * 2022-03-15     smartmx      fix bugs, add support for stm32l4 flash
 * 2022-08-02     smartmx      add TFDB_VALUE_AFTER_ERASE_SIZE option
 * 2023-02-22     smartmx      add dual flash index function
 *
 */
#include "tfdb_port.h"

/**
 * Read data from flash.
 * @note This operation's units is refer to TFDB_WRITE_UNIT_BYTES.
 *
 * @param addr flash address.
 * @param buf buffer to store read data.
 * @param size read bytes size.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_port_read(tfdb_addr_t addr, uint8_t *buf, size_t size)
{
    TFDB_Err_Code result = TFDB_NO_ERR;
    /* You can add your code under here. */

    return result;
}

/**
 * Erase flash.
 * @param addr flash address.
 * @param size erase bytes size.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_port_erase(tfdb_addr_t addr, size_t size)
{
    TFDB_Err_Code result = TFDB_NO_ERR;
    /* You can add your code under here. */

    return result;
}

/**
 * Write data to flash.
 * @note This operation's units is refer to TFDB_WRITE_UNIT_BYTES.
 * if you're using some flash like stm32L4xx, please add flash check
 * operations before write flash to ensure the write area is erased.
 * if the write area is not erased, please just return TFDB_NO_ERR.
 * TFDB will check data and retry at next address.
 *
 * @param addr flash address.
 * @param buf the write data buffer.
 * @param size write bytes size.
 *
 * @return result
 */
TFDB_Err_Code tfdb_port_write(tfdb_addr_t addr, const uint8_t *buf, size_t size)
{
    TFDB_Err_Code result = TFDB_NO_ERR;
    /* You can add your code under here. */

    return result;
}


