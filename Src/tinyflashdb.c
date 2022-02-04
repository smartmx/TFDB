/*
 * Copyright (c) 2022, mx1117 - smartmx@qq.com
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
#include "tinyflashdb.h"

/**
 * check header in flash.
 *
 * @param index the data manage index.
 * @param rw_buffer buffer to store prepared read data or write data.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_check(const tfdb_index_t *index, uint8_t *rw_buffer)
{
    TFDB_Err_Code result;

    TFDB_PRINTF("tfdb_check >\n");

    /* flash_size / value_len / end_byte */
    result = tfdb_port_read(index->flash_addr, rw_buffer, 4);
    if(result != TFDB_NO_ERR){
        //read err
        TFDB_PRINTF("    read err\n");
        goto end;
    }
    result = TFDB_HDR_ERR;
    /* compare flash_size */
    if((rw_buffer[0] == ((index->flash_size >> 8) & 0xff)) && (rw_buffer[1] == ((index->flash_size) & 0xff))){
        /* compare value_length and end_byte */
        if((rw_buffer[2] == index->value_length) && (rw_buffer[3] == index->end_byte)){
            /* check hdr success */
            result = TFDB_NO_ERR;
        }
    }
end:
    TFDB_PRINTF("tfdb_check:%d\n",result);
    return result;
}

/**
 * erase the flash block and init header in flash.
 *
 * @param index the data manage index.
 * @param rw_buffer buffer to store prepared read data or write data.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_init(const tfdb_index_t *index, uint8_t *rw_buffer)
{
    TFDB_Err_Code result = TFDB_NO_ERR;

    TFDB_PRINTF("tfdb_init >\n");

    result = tfdb_port_erase(index->flash_addr, index->flash_size);
    if(result != TFDB_NO_ERR){
        //erase err
        TFDB_PRINTF("    erase err\n");
        goto end;
    }
    rw_buffer[0] = ((index->flash_size >> 8) & 0xff);
    rw_buffer[1] = ((index->flash_size) & 0xff);
    rw_buffer[2] = index->value_length;
    rw_buffer[3] = index->end_byte;
    /* flash_size / value_len / end_byte */
    result = tfdb_port_write(index->flash_addr, rw_buffer, 4);
    if(result != TFDB_NO_ERR){
        //write err
        TFDB_PRINTF("    write err\n");
        goto end;
    }
    result = tfdb_check(index, rw_buffer);
    if(result != TFDB_NO_ERR){
        result = TFDB_FLASH_ERR;
        goto end;
    }
end:
    TFDB_PRINTF("tfdb_init:%d\n",result);
    return result;
}

/**
 * set data in flash and save the addr to addr_cache.
 *
 * @param index the data manage index.
 * @param rw_buffer buffer to store prepared read data or write data.
 * @param addr_cache the pointer to addr which is user offered, which will save read addr.
 * @param value_from the pointer to buffer which is user offered that need to save.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_set(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void* value_from)
{
    TFDB_Err_Code result;
    tfdb_addr_t find_addr;
    uint8_t aligned_value_size;
    uint8_t sum_verify_byte;
    TFDB_PRINTF("tfdb_set >\n");

    aligned_value_size  = index->value_length + 2;/* data + verify + end_byte */
#if (TFDB_WRITE_UNIT_BYTES==4)
    /* aligned with TFDB_WRITE_UNIT_BYTES */
    aligned_value_size = ((aligned_value_size + 3) >> 2);
    aligned_value_size = aligned_value_size << 2;
#endif
    if((addr_cache == NULL) || (*addr_cache == 0)){
        /* addr_cache is not init. so check header first. */
        result = tfdb_check(index, rw_buffer);
        if(result == TFDB_NO_ERR){
            /* the header is right. so start to find data location address in flash. */
            find_addr = index->flash_addr + 4;
            result = TFDB_FILL_ERR;
            while((find_addr) <= (index->flash_size + index->flash_addr - aligned_value_size)){
                /* start to find value */
                result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
                if(result != TFDB_NO_ERR){
                    TFDB_PRINTF("    read err\n");
                    goto end;
                }
                if((rw_buffer[aligned_value_size - 1] == TFDB_VALUE_AFTER_ERASE)){
                    /* find value addr success */
                    result = TFDB_NO_ERR;
                    break;
                } else {
                    /* some flash bits maybe bad, can't write. */
                    find_addr += aligned_value_size;
                }
            }
            /* the flash block is fill */
            if(result == TFDB_FILL_ERR){
                goto init;
            }
            /* find the addr success */
            /* calculate sum verify */
            sum_verify_byte = 0;
            for(uint8_t i = 0; i < index->value_length; i++){
                sum_verify_byte = ((sum_verify_byte + rw_buffer[i]) & 0xff);
            }
set:
            tfdb_memcpy(rw_buffer, value_from, index->value_length);
            rw_buffer[index->value_length] = sum_verify_byte;
            for(uint8_t i = index->value_length + 1; i < aligned_value_size; i++){
                /* fill aligned data with end_byte */
                rw_buffer[i] = index->end_byte;
            }
            result = tfdb_port_write(find_addr, rw_buffer, aligned_value_size);
            if(result != TFDB_NO_ERR){
                TFDB_PRINTF("    write err\n");
                goto end;
            }
            result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
            if(result != TFDB_NO_ERR){
                TFDB_PRINTF("    read err\n");
                goto end;
            }
            if((tfdb_memcmp(rw_buffer, value_from, index->value_length) != TFDB_MEMCMP_SAME) \
                || (rw_buffer[index->value_length] != sum_verify_byte)\
                || (rw_buffer[aligned_value_size - 1] != index->end_byte)){
                /* write verify failed, maybe the flash is error, try next address. */
                find_addr += aligned_value_size;
                if((index->flash_size + index->flash_addr - find_addr) >= (aligned_value_size)){
                    goto set;
                } else {
                    /* the flash is fill */
                    goto init;
                }
            } else {
                /* write data to flash success */
                /* save addr to addr_cache */
                if(addr_cache != NULL){
                    *addr_cache = find_addr;
                }
            }
        } else if (result == TFDB_HDR_ERR){
init:
            result = tfdb_init(index, rw_buffer);
            if(result == TFDB_NO_ERR){
                find_addr = index->flash_addr + 4;
                goto set;
            }
        }
    } else {
        /* addr_cache is set */
        find_addr = *addr_cache + aligned_value_size;
        if(find_addr > (index->flash_addr + index->flash_size - aligned_value_size)){
            /* the flash is fill */
            goto init;
        } else {
            goto set;
        }
    }
end:
    TFDB_PRINTF("tfdb_set:%d\n",result);
    return result;
}

/**
 * get the data in flash and save the addr of data to addr_cache.
 *
 * @param index the data manage index.
 * @param rw_buffer buffer to store prepared read data or write data.
 * @param addr_cache the pointer to addr which is user offered.
 * @param value_to the pointer to buffer which is user offered to save data.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_get(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void* value_to)
{
    TFDB_Err_Code result;
    tfdb_addr_t find_addr;
    uint8_t aligned_value_size;
    uint8_t sum_verify_byte;
    TFDB_PRINTF("tfdb_get >\n");

    aligned_value_size  = index->value_length + 2;/* data + verify + end_byte */
#if (TFDB_WRITE_UNIT_BYTES==4)
    /* aligned with TFDB_WRITE_UNIT_BYTES */
    aligned_value_size = ((aligned_value_size + 3) >> 2);
    aligned_value_size = aligned_value_size << 2;
#endif
    if((addr_cache == NULL) || (*addr_cache == 0)){
        /* addr_cache is not init. so check header first. */
        result = tfdb_check(index, rw_buffer);
        if(result == TFDB_NO_ERR){
            /* the header is right. so start to find data location address in flash. */
            find_addr = index->flash_addr + 4;
            result = TFDB_FILL_ERR;
            while((find_addr) <= (index->flash_size + index->flash_addr - aligned_value_size)){
                /* start to find value */
                result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
                if(result != TFDB_NO_ERR){
                    TFDB_PRINTF("    read err\n");
                    goto end;
                }
                if((rw_buffer[aligned_value_size - 1] == TFDB_VALUE_AFTER_ERASE)){
                    /* find value addr success */
                    result = TFDB_NO_ERR;
                    break;
                } else {
                    /* some flash bits maybe bad, can't write. */
                    find_addr += aligned_value_size;
                }
            }
            if(result == TFDB_NO_ERR){
                /* TFDB_NO_ERR */
                find_addr = find_addr - aligned_value_size;
                result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
                if(result != TFDB_NO_ERR){
                    TFDB_PRINTF("    read err\n");
                    goto end;
                }
                goto verify;
            } else {
                /* TFDB_FILL_ERR */
verify:
                sum_verify_byte = 0;
                /* calculate sum verify */
                for(uint8_t i = 0; i < index->value_length; i++){
                    sum_verify_byte = ((sum_verify_byte + rw_buffer[i]) & 0xff);
                }
                if((sum_verify_byte != rw_buffer[index->value_length])\
                    || (rw_buffer[aligned_value_size - 1] != index->end_byte)){
                    /* not right data, maybe the flash is broken. */
                    find_addr = find_addr - aligned_value_size;
                    if(find_addr >= (index->flash_addr + 4)){
                        result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
                        if(result != TFDB_NO_ERR){
                            TFDB_PRINTF("    read err\n");
                            goto end;
                        }
                        goto verify;
                    } else {
                        result = TFDB_FLASH_ERR;
                        goto end;
                    }
                } else {
                    result = TFDB_NO_ERR;
                    tfdb_memcpy(value_to, rw_buffer, index->value_length);
                    if(addr_cache != NULL){
                        *addr_cache = find_addr;
                    }
                }
            }
        } else {
            result = TFDB_HDR_ERR;
            goto end;
        }
    } else {
        find_addr = *addr_cache;
        result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
        if(result != TFDB_NO_ERR){
            TFDB_PRINTF("    read err\n");
            goto end;
        }
        goto verify;
    }
end:
    TFDB_PRINTF("tfdb_get:%d\n",result);
    return result;
}
