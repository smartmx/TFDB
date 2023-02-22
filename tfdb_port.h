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
#ifndef _TFDB_PORT_H_
#define _TFDB_PORT_H_

/* add headers of your chips */
#include "stdint.h"

/* tinyflashdb error code */
typedef enum
{
    TFDB_NO_ERR = 0,
    TFDB_ERASE_ERR,
    TFDB_READ_ERR,
    TFDB_WRITE_ERR,
    TFDB_HDR_ERR,
    TFDB_FLASH_ERR,
    TFDB_CACHE_ERR,
    TFDB_SEQ_ERR,
    TFDB_FLASH1_ERR,
    TFDB_FLASH2_ERR,
    TFDB_ERR_MAX,
} TFDB_Err_Code;

/* use string.h or self functions */
#define TFDB_USE_STRING_H               1

#if TFDB_USE_STRING_H
    #include "string.h"
    #define tfdb_memcpy memcpy
    #define tfdb_memcmp memcmp
    #define TFDB_MEMCMP_SAME 0
#else
    #define tfdb_memcpy
    #define tfdb_memcmp
    #define TFDB_MEMCMP_SAME
#endif

#define TFDB_DEBUG                          printf

/* The data value in flash after erased, most are 0xff, some flash maybe different.
 * if it's over 1 byte, please be care of little endian or big endian. */
#define TFDB_VALUE_AFTER_ERASE              0xff

/* The size of value in flash after erased, only support 1/2/4.
 * This value must not bigger than TFDB_WRITE_UNIT_BYTES. */
#define TFDB_VALUE_AFTER_ERASE_SIZE         1

/* the flash write granularity, unit: byte
 * only support 1(stm32f4)/ 2(CH559)/ 4(stm32f1)/ 8(stm32L4) */
#define TFDB_WRITE_UNIT_BYTES               4 /* @note you must define it for a value */

#if TFDB_VALUE_AFTER_ERASE_SIZE > TFDB_WRITE_UNIT_BYTES
    #error "TFDB_VALUE_AFTER_ERASE_SIZE must not bigger than TFDB_WRITE_UNIT_BYTES."
#endif

/* @note the max retry times when flash is error ,set 0 will disable retry count */
#define TFDB_WRITE_MAX_RETRY                32

/* must not use pointer type. Please use uint32_t, uint16_t or uint8_t. */
typedef uint32_t    tfdb_addr_t;

extern TFDB_Err_Code tfdb_port_read(tfdb_addr_t addr, uint8_t *buf, size_t size);

extern TFDB_Err_Code tfdb_port_erase(tfdb_addr_t addr, size_t size);

extern TFDB_Err_Code tfdb_port_write(tfdb_addr_t addr, const uint8_t *buf, size_t size);

#endif

