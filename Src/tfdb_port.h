/*
 * Copyright (c) 2022, smartmx - smartmx@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the Tiny Flash DataBase Library.
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
    TFDB_FILL_ERR,
    TFDB_FLASH_ERR,
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

/* The data value in flash after erased, most are 0xff, some flash maybe different. */
#define TFDB_VALUE_AFTER_ERASE              0xff

/* the flash write granularity, unit: byte
 * only support 1(stm32f4)/ 2(CH559)/ 4(stm32f1)/ 8(stm32L4) */
#define TFDB_WRITE_UNIT_BYTES               8 /* @note you must define it for a value */

/* @note the max retry times when flash is error ,set 0 will disable retry count */
#define TFDB_WRITE_MAX_RETRY                32

/* must not use pointer type. Please use uint32_t, uint16_t or uint8_t. */
typedef uint32_t    tfdb_addr_t;

extern TFDB_Err_Code tfdb_port_read(tfdb_addr_t addr, uint8_t *buf, size_t size);

extern TFDB_Err_Code tfdb_port_erase(tfdb_addr_t addr, size_t size);

extern TFDB_Err_Code tfdb_port_write(tfdb_addr_t addr, const uint8_t *buf, size_t size);

#endif

