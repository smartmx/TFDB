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
#ifndef _TINY_FLASH_DB_H_
#define _TINY_FLASH_DB_H_

#include "tfdb_port.h"

#define TFDB_VERSION    "0.0.7"

#define TFDB_ALIGNED_RW_BUFFER_SIZE(VALUE_LENGTH, ALIGNED_SIZE)             ((VALUE_LENGTH + 1 + ALIGNED_SIZE) / (ALIGNED_SIZE))

#define TFDB_DUAL_ALIGNED_RW_BUFFER_SIZE(VALUE_LENGTH, ALIGNED_SIZE)        ((VALUE_LENGTH + 3 + ALIGNED_SIZE) / (ALIGNED_SIZE))

#define TFDB_DUAL_VALUE_LENGTH(VALUE_LENGTH)                                (VALUE_LENGTH + 2)

typedef struct _tfdb_index_struct
{
    tfdb_addr_t     flash_addr;     /* the start address of the flash block */
    uint16_t        flash_size;     /* the size of the flash block */
    uint8_t         value_length;   /* the length of value that saved in this flash block */
    uint8_t         end_byte;       /* must different to TFDB_VALUE_AFTER_ERASE */
    /* 0x00 is recommended for end_byte, because almost all flash is 0xff after erase. */
} tfdb_index_t;

extern TFDB_Err_Code tfdb_get(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void *value_to);

extern TFDB_Err_Code tfdb_set(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void *value_from);

typedef struct _tfdb_dual_index_struct
{
    tfdb_index_t indexes[2];
} tfdb_dual_index_t;

typedef struct _tfdb_dual_cache_struct
{
    tfdb_addr_t     addr_cache[2];
    uint16_t        seq[2];
} tfdb_dual_cache_t;

extern TFDB_Err_Code tfdb_dual_get(const tfdb_dual_index_t *index, uint8_t *rw_buffer, uint8_t *rw_buffer_bak, tfdb_dual_cache_t *cache, void *value_to);

extern TFDB_Err_Code tfdb_dual_set(const tfdb_dual_index_t *index, uint8_t *rw_buffer, uint8_t *rw_buffer_bak, tfdb_dual_cache_t *cache, void *value_from);

#endif
