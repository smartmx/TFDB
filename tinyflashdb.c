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
 * 2023-11-07     smartmx      fix bugs, tfdb_get error when flash write in tfdb_set not success.
 * 2026-09-21     smartmx      add tfdb_dual_get_pre function and TFDB_DUAL_SEQ_COUNT option.
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

    TFDB_DEBUG("tfdb_check >\n");
#if (TFDB_WRITE_UNIT_BYTES==8)
    /* flash_size / value_len / end_byte */
    result = tfdb_port_read(index->flash_addr, rw_buffer, 8);
#else
    /* flash_size / value_len / end_byte */
    result = tfdb_port_read(index->flash_addr, rw_buffer, 4);
#endif
    if (result != TFDB_NO_ERR)
    {
        //read err
        TFDB_DEBUG("    read err\n");
        goto end;
    }
    result = TFDB_HDR_ERR;
    /* compare flash_size */
    if ((rw_buffer[0] == ((index->flash_size >> 8) & 0xff)) && (rw_buffer[1] == ((index->flash_size) & 0xff)))
    {
        /* compare value_length and end_byte */
        if ((rw_buffer[2] == index->value_length) && (rw_buffer[3] == index->end_byte))
        {
            /* check hdr success */
            result = TFDB_NO_ERR;
            goto end;
        }
    }
end:
    TFDB_DEBUG("tfdb_check:%d\n", result);
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

    TFDB_DEBUG("tfdb_init >\n");

    result = tfdb_port_erase(index->flash_addr, index->flash_size);
    if (result != TFDB_NO_ERR)
    {
        //erase err
        TFDB_DEBUG("    erase err\n");
        goto end;
    }
    rw_buffer[0] = ((index->flash_size >> 8) & 0xff);
    rw_buffer[1] = ((index->flash_size) & 0xff);
    rw_buffer[2] = index->value_length;
    rw_buffer[3] = index->end_byte;
#if (TFDB_WRITE_UNIT_BYTES==8)
    rw_buffer[4] = index->end_byte;
    rw_buffer[5] = index->end_byte;
    rw_buffer[6] = index->end_byte;
    rw_buffer[7] = index->end_byte;
    /* flash_size / value_len / end_byte */
    result = tfdb_port_write(index->flash_addr, rw_buffer, 8);
#else
    /* flash_size / value_len / end_byte */
    result = tfdb_port_write(index->flash_addr, rw_buffer, 4);
#endif
    if (result != TFDB_NO_ERR)
    {
        //write err
        TFDB_DEBUG("    write err\n");
        goto end;
    }
    result = tfdb_check(index, rw_buffer);
    if (result != TFDB_NO_ERR)
    {
        TFDB_DEBUG("    flash ERR\n");
        result = TFDB_FLASH_ERR;
        goto end;
    }
end:
    TFDB_DEBUG("tfdb_init:%d\n", result);
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
TFDB_Err_Code tfdb_set(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void *value_from)
{
    TFDB_Err_Code result;
    tfdb_addr_t find_addr;
    uint8_t aligned_value_size;
    uint8_t sum_verify_byte;
    uint8_t i;
#if TFDB_WRITE_MAX_RETRY
    uint32_t max_retry = 0;
#endif

    TFDB_DEBUG("tfdb_set >\n");

    aligned_value_size  = index->value_length + 2;/* data + verify + end_byte */

#if (TFDB_WRITE_UNIT_BYTES==2)
    /* aligned with TFDB_WRITE_UNIT_BYTES */
    aligned_value_size = ((aligned_value_size + 1) & 0xfe);
#elif (TFDB_WRITE_UNIT_BYTES==4)
    /* aligned with TFDB_WRITE_UNIT_BYTES */
    aligned_value_size = ((aligned_value_size + 3) & 0xfc);
#elif (TFDB_WRITE_UNIT_BYTES==8)
    /* aligned with TFDB_WRITE_UNIT_BYTES */
    aligned_value_size = ((aligned_value_size + 7) & 0xf8);
#endif
    TFDB_LOG("aigned size:%d\n", aligned_value_size);

    if (addr_cache == NULL)
    {
start:
        /* addr_cache is not init. so check header first. */
        find_addr = 0;
        result = tfdb_get(index, rw_buffer, &find_addr, NULL);
        if(result == TFDB_NO_ERR)
        {
            find_addr = find_addr + aligned_value_size;
#if (TFDB_WRITE_UNIT_BYTES==8)
            if(find_addr > (index->flash_addr + index->flash_size - ((index->flash_size - 8) % aligned_value_size) - aligned_value_size))
#else
            if(find_addr > (index->flash_addr + index->flash_size - ((index->flash_size - 4) % aligned_value_size) - aligned_value_size))
#endif
            {
                /* the flash block is fill */
                TFDB_DEBUG("    the flash is fill\n");
                goto init;
            }

            /* find the addr success */
            TFDB_LOG("    find success\n");
set:
            /* calculate sum verify */
            sum_verify_byte = 0xff;
            for (i = 0; i < index->value_length; i++)
            {
                sum_verify_byte = ((sum_verify_byte + ((uint8_t *)(value_from))[i]) & 0xff);
            }
write:
#if TFDB_WRITE_MAX_RETRY
            max_retry++;
            if (max_retry > TFDB_WRITE_MAX_RETRY)
            {
                result = TFDB_FLASH_ERR;
                goto end;
            }
#endif
            tfdb_memcpy(rw_buffer, value_from, index->value_length);
            rw_buffer[index->value_length] = sum_verify_byte;
            for (i = index->value_length + 1; i < aligned_value_size; i++)
            {
                /* fill aligned data with end_byte */
                rw_buffer[i] = index->end_byte;
            }
            result = tfdb_port_write(find_addr, rw_buffer, aligned_value_size);
            if (result != TFDB_NO_ERR)
            {
                TFDB_DEBUG("    write err\n");
                goto end;
            }
            result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
            if (result != TFDB_NO_ERR)
            {
                TFDB_DEBUG("    read err\n");
                goto end;
            }
            if ((tfdb_memcmp(rw_buffer, value_from, index->value_length) != TFDB_MEMCMP_SAME) \
                    || (rw_buffer[index->value_length] != sum_verify_byte)\
                    || (rw_buffer[aligned_value_size - 1] != index->end_byte))
            {
                /* write verify failed, maybe the flash is error, try next address. */
                TFDB_DEBUG("    Write verify failed, try next address.\n");
                find_addr += aligned_value_size;

#if (TFDB_WRITE_UNIT_BYTES==8)
                if(find_addr > (index->flash_addr + index->flash_size - ((index->flash_size - 8) % aligned_value_size) - aligned_value_size))
#else
                if(find_addr > (index->flash_addr + index->flash_size - ((index->flash_size - 4) % aligned_value_size) - aligned_value_size))
#endif
                {
                    /* the flash is fill */
                    TFDB_DEBUG("    the flash is fill\n");
                    goto init;
                }
                else
                {
                    goto write;
                }
            }
            else
            {
                /* write data to flash success */
                /* save addr to addr_cache */
                if (addr_cache != NULL)
                {
                    *addr_cache = find_addr;
                }
            }
        }
        else if (result == TFDB_HDR_ERR)
        {
            TFDB_DEBUG("    header err\n");
init:
            result = tfdb_init(index, rw_buffer);
            if (result == TFDB_NO_ERR)
            {
after_init:
#if (TFDB_WRITE_UNIT_BYTES==8)
                find_addr = index->flash_addr + 8;
#else
                find_addr = index->flash_addr + 4;
#endif
                goto set;
            }
            goto end;
        }
        else if (result == TFDB_NO_DATA)
        {
            goto after_init;
        }
    }
    else
    {
        if (*addr_cache == 0)
        {
            /* addr_cache is not set */
            goto start;
        }
        else
        {
            /* addr_cache is set */
            TFDB_DEBUG("    addr_cache is set\n");
            find_addr = *addr_cache + aligned_value_size;
            if (find_addr > (index->flash_addr + index->flash_size - aligned_value_size))
            {
                /* the flash is fill */
                TFDB_DEBUG("    the flash is fill\n");
                goto init;
            }
            else
            {
                goto set;
            }
        }
    }
end:
    TFDB_LOG("tfdb_set:%d\n", result);
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
TFDB_Err_Code tfdb_get(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void *value_to)
{
    TFDB_Err_Code result;
    tfdb_addr_t find_addr;
    uint8_t aligned_value_size;
    uint8_t sum_verify_byte;
    uint8_t i;
    TFDB_LOG("tfdb_get >\n");

    aligned_value_size  = index->value_length + 2;/* data + verify + end_byte */

#if (TFDB_WRITE_UNIT_BYTES==2)
    /* aligned with TFDB_WRITE_UNIT_BYTES */
    aligned_value_size = ((aligned_value_size + 1) & 0xfe);
#elif (TFDB_WRITE_UNIT_BYTES==4)
    /* aligned with TFDB_WRITE_UNIT_BYTES */
    aligned_value_size = ((aligned_value_size + 3) & 0xfc);
#elif (TFDB_WRITE_UNIT_BYTES==8)
    /* aligned with TFDB_WRITE_UNIT_BYTES */
    aligned_value_size = ((aligned_value_size + 7) & 0xf8);
#endif
    TFDB_LOG("aigned size:%d\n", aligned_value_size);

    if (addr_cache == NULL)
    {
start:
        /* addr_cache is not init. so check header first. */
        result = tfdb_check(index, rw_buffer);
        if (result == TFDB_NO_ERR)
        {
            /* the header is right. so start to find data location address in flash. */
#if (TFDB_WRITE_UNIT_BYTES==8)
            find_addr = index->flash_addr + index->flash_size - ((index->flash_size - 8) % aligned_value_size) - aligned_value_size;
            while ((find_addr) >= (index->flash_addr + 8))
#else
            find_addr = index->flash_addr + index->flash_size - ((index->flash_size - 4) % aligned_value_size) - aligned_value_size;
            while ((find_addr) >= (index->flash_addr + 4))
#endif
            {
                /* start to find value */
                result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
                if (result != TFDB_NO_ERR)
                {
                    TFDB_DEBUG("    read err\n");
                    goto end;
                }

                if (rw_buffer[aligned_value_size - 1] == index->end_byte)
                {
                    /* find value addr success */
                    break;
                }

                if(find_addr >= aligned_value_size)
                {
                    find_addr -= aligned_value_size;
                }
                else
                {
                    break;
                }
            }

verify:
            if(rw_buffer[aligned_value_size - 1] != index->end_byte)
            {
                TFDB_LOG("end_byte err\n");
                goto read_next;
            }

            sum_verify_byte = 0xff;
            /* calculate sum verify */
            for (i = 0; i < index->value_length; i++)
            {
                sum_verify_byte = ((sum_verify_byte + rw_buffer[i]) & 0xff);
            }
            if (sum_verify_byte != rw_buffer[index->value_length])
            {
                /* not right data, maybe the flash is broken. */
                TFDB_LOG("verify err:%02x,%02x\n", sum_verify_byte, rw_buffer[index->value_length]);
read_next:
#if (TFDB_WRITE_UNIT_BYTES==8)
                if (find_addr >= (index->flash_addr + 8 + aligned_value_size))
#else
                if (find_addr >= (index->flash_addr + 4 + aligned_value_size))
#endif
                {
                    find_addr = find_addr - aligned_value_size;
                    result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
                    if (result != TFDB_NO_ERR)
                    {
                        TFDB_DEBUG("    read err\n");
                        goto end;
                    }
                    goto verify;
                }
                else
                {
                    TFDB_DEBUG("    no data in flash\n");
                    result = TFDB_NO_DATA;
                    goto end;
                }
            }
            else
            {
                TFDB_DEBUG("    find success\n");
                result = TFDB_NO_ERR;
                if(value_to != NULL)
                {
                    tfdb_memcpy(value_to, rw_buffer, index->value_length);
                }
                if (addr_cache != NULL)
                {
                    *addr_cache = find_addr;
                }
            }
        }
        else
        {
            TFDB_DEBUG("    header err\n");
            result = TFDB_HDR_ERR;
            goto end;
        }
    }
    else
    {
        if (*addr_cache == 0)
        {
            /* addr_cache is not set */
            goto start;
        }
        else
        {
            find_addr = *addr_cache;
            result = tfdb_port_read(find_addr, rw_buffer, aligned_value_size);
            if (result != TFDB_NO_ERR)
            {
                TFDB_DEBUG("    read err\n");
                goto end;
            }
            goto verify;
        }
    }
end:
    TFDB_LOG("tfdb_get:%d\n", result);
    return result;
}

/**
 * get the previous data in flash and save the addr of data to addr_cache.
 *
 * @param index the data manage index.
 * @param rw_buffer buffer to store prepared read data or write data.
 * @param addr_cache the pointer to addr which is user offered.
 * @param value_to the pointer to buffer which is user offered to save data.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_get_pre(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, tfdb_addr_t *pre_addr_cache, void *value_to)
{
    TFDB_Err_Code result;
    uint8_t aligned_value_size;
    tfdb_addr_t find_addr;

    TFDB_LOG("tfdb_get_pre >\n");

    if(addr_cache == NULL)
    {
        goto prepare;
    }
    else
    {
        if(*addr_cache != 0)
        {
            find_addr = *addr_cache;
            TFDB_LOG("find_addr:%x\n", find_addr);
find:
            aligned_value_size  = index->value_length + 2;  /* data + verify + end_byte */

#if (TFDB_WRITE_UNIT_BYTES==2)
            /* aligned with TFDB_WRITE_UNIT_BYTES */
            aligned_value_size = ((aligned_value_size + 1) & 0xfe);
#elif (TFDB_WRITE_UNIT_BYTES==4)
            /* aligned with TFDB_WRITE_UNIT_BYTES */
            aligned_value_size = ((aligned_value_size + 3) & 0xfc);
#elif (TFDB_WRITE_UNIT_BYTES==8)
            /* aligned with TFDB_WRITE_UNIT_BYTES */
            aligned_value_size = ((aligned_value_size + 7) & 0xf8);
#endif

            TFDB_LOG("aigned size:%d\n", aligned_value_size);

#if (TFDB_WRITE_UNIT_BYTES==8)
            if(find_addr >= (index->flash_addr + 8 + aligned_value_size))
#else
            if(find_addr >= (index->flash_addr + 4 + aligned_value_size))
#endif
            {
                find_addr = find_addr - aligned_value_size;
                result = tfdb_get(index, rw_buffer, &find_addr, value_to);
                if(result == TFDB_NO_ERR)
                {
                    if(pre_addr_cache != NULL)
                    {
                        *pre_addr_cache = find_addr;
                    }
                }
            }
            else
            {
                /* no old data. */
                result = TFDB_NO_PRE_DATA;
            }
        }
        else
        {
prepare:
            find_addr = 0;
            result = tfdb_get(index, rw_buffer, &find_addr, value_to);
            if(result != TFDB_NO_ERR)
            {
                goto end;
            }
            if(addr_cache != NULL)
            {
                *addr_cache = find_addr;
            }
            goto find;
        }
    }
end:
    TFDB_LOG("tfdb_get_pre:%d\n", result);
    return result;
}

/**
 * get next seq from given seq.
 *
 * @param seq the current seq value.
 *
 * @return uint16_t the next seq value in the cycle,
 *         the unknown value returns the first value of the cycle.
 */
static uint16_t tfdb_dual_get_next_seq(uint16_t seq)
{
#if (TFDB_DUAL_SEQ_COUNT==5)
    /* seq range: 0xff00 -> 0xf0f0 -> 0x0ff0 -> 0x0f0f -> 0x00ff */
    if (seq == 0xff00)
    {
        return 0xf0f0;
    }
    else if (seq == 0xf0f0)
    {
        return 0x0ff0;
    }
    else if (seq == 0x0ff0)
    {
        return 0x0f0f;
    }
    else if (seq == 0x0f0f)
    {
        return 0x00ff;
    }
    else
    {
        return 0xff00;
    }
#else
    /* seq range: 0x00ff -> 0x0ff0 -> 0xff00 */
    if (seq == 0x00ff)
    {
        return 0x0ff0;
    }
    else if (seq == 0x0ff0)
    {
        return 0xff00;
    }
    else
    {
        return 0x00ff;
    }
#endif
}

/**
 * check the seq value is a legal cycle value or not.
 *
 * @param seq the seq value which need to be checked.
 *
 * @return uint8_t 1 is legal, 0 is illegal.
 */
static uint8_t tfdb_dual_seq_valid(uint16_t seq)
{
    uint16_t cycle_seq;
    uint8_t i;

    /* all the legal values can be found by walking the cycle from the first value. */
    cycle_seq = tfdb_dual_get_next_seq(0);
    for (i = 0; i < TFDB_DUAL_SEQ_COUNT; i++)
    {
        if (cycle_seq == seq)
        {
            return 1;
        }
        cycle_seq = tfdb_dual_get_next_seq(cycle_seq);
    }
    return 0;
}

/**
 * judge which seq is new.
 *
 * @param seq the pointer to seq[2] buffer.
 *
 * @return uint8_t which seq is new, 0xff means all seq is illegal.
 */
static uint8_t tfdb_dual_judge(uint16_t *seq)
{
    /* the seq of the newer block is the next of the older one in the cycle. */
    if (tfdb_dual_seq_valid(seq[0]) != 0)
    {
        return (tfdb_dual_get_next_seq(seq[0]) == seq[1]) ? 1 : 0;
    }
    if (tfdb_dual_seq_valid(seq[1]) != 0)
    {
        return 1;
    }
    return 0xff;
}

/**
 * rank the seq by the backward distance to the current seq in the seq cycle.
 *
 * @param current_seq the seq value of the current latest data.
 * @param seq the seq value which need to be ranked.
 *
 * @return uint8_t 1 ~ (TFDB_DUAL_SEQ_COUNT - 1): the seq value is some steps before the current seq,
 *         TFDB_DUAL_SEQ_COUNT: the same seq value to the current seq,
 *         (TFDB_DUAL_SEQ_COUNT + 1): illegal seq value.
 */
static uint8_t tfdb_dual_seq_rank(uint16_t current_seq, uint16_t seq)
{
    uint8_t i;

    if (tfdb_dual_seq_valid(seq) == 0)
    {
        /* illegal seq value, keep the lowest priority. */
        return TFDB_DUAL_SEQ_COUNT + 1;
    }
    for (i = 1; i <= TFDB_DUAL_SEQ_COUNT; i++)
    {
        seq = tfdb_dual_get_next_seq(seq);
        if (seq == current_seq)
        {
            return i;
        }
    }
    return TFDB_DUAL_SEQ_COUNT + 1;
}

/**
 * get the data in flash and save the addr and seq to cache.
 *
 * @param index the data manage index.
 * @param rw_buffer buffer to store prepared read data or write data.
 * @param rw_buffer_bak buffer to store prepared read data or write data.
 * @param cache the pointer to addr which is user offered, which will save read addr and seq.
 * @param value_from the pointer to buffer which is user offered that need to save.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_dual_get(const tfdb_dual_index_t *index, uint8_t *rw_buffer, uint8_t *rw_buffer_bak, tfdb_dual_cache_t *cache, void *value_to)
{
    TFDB_Err_Code rresult = TFDB_NO_ERR;
    TFDB_Err_Code result[2];
    uint8_t judge_state;

    if (cache != NULL)
    {
        judge_state = tfdb_dual_judge(cache->seq);

        TFDB_DEBUG("tfdb_dual_judge:%d\n", judge_state);

        /* usually, we just read value once during the initializing. */
        if (judge_state == 0xff)
        {
            result[0] = tfdb_get(&index->indexes[0], rw_buffer, &(cache->addr_cache[0]), rw_buffer_bak);
            if (result[0] == TFDB_NO_ERR)
            {
                cache->seq[0] = (rw_buffer_bak[0] << 8) | (rw_buffer_bak[1]);
                tfdb_memcpy(value_to, &(rw_buffer_bak[2]), index->indexes[0].value_length - 2);
            }
            else
            {
                cache->seq[0] = 0;
            }

            result[1] = tfdb_get(&index->indexes[1], rw_buffer, &(cache->addr_cache[1]), rw_buffer_bak);
            if (result[1] == TFDB_NO_ERR)
            {
                cache->seq[1] = (rw_buffer_bak[0] << 8) | (rw_buffer_bak[1]);
            }
            else
            {
                cache->seq[1] = 0;
            }

            judge_state = tfdb_dual_judge(cache->seq);
            if (judge_state == 1)
            {
                tfdb_memcpy(value_to, &(rw_buffer_bak[2]), index->indexes[1].value_length - 2);
            }
            else if (judge_state == 0xff)
            {
                rresult = TFDB_SEQ_ERR;
            }
        }
        else
        {
            rresult = tfdb_get(&index->indexes[judge_state], rw_buffer, &(cache->addr_cache[judge_state]), rw_buffer_bak);
            if (rresult == TFDB_NO_ERR)
            {
                tfdb_memcpy(value_to, &(rw_buffer_bak[2]), index->indexes[judge_state].value_length - 2);
            }
            else
            {
                /* block not right, don't read another block. */
            }
        }
    }
    else
    {
        rresult = TFDB_CACHE_ERR;
    }

    return rresult;
}

/**
 * get the previous data in dual flash blocks and save the addr and seq of it to pre_cache.
 *
 * The previous data is the newest valid record older than the current latest one.
 * It is usually the latest record in another flash block, when that record is broken,
 * the previous valid record in the same block of the latest one will be used.
 *
 * @param index the data manage index.
 * @param rw_buffer buffer to store prepared read data or write data.
 * @param rw_buffer_bak buffer to store prepared read data or write data.
 * @param cache the pointer to cache which is user offered, which saves the read addr and seq.
 * @param pre_cache the pointer to cache which is user offered, which will save the previous data addr and seq, can be NULL.
 * @param value_to the pointer to buffer which is user offered to save data.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_dual_get_pre(const tfdb_dual_index_t *index, uint8_t *rw_buffer, uint8_t *rw_buffer_bak, tfdb_dual_cache_t *cache, tfdb_dual_cache_t *pre_cache, void *value_to)
{
    TFDB_Err_Code rresult;
    TFDB_Err_Code result[2];
    uint8_t judge_state;
    uint8_t current_block;
    uint8_t pre_block;
    uint16_t current_seq;
    uint16_t seq[2];
    tfdb_addr_t pre_addr;
    uint8_t rank[2];
    uint8_t i;

    TFDB_LOG("tfdb_dual_get_pre >\n");

    if (cache == NULL)
    {
        return TFDB_CACHE_ERR;
    }

    judge_state = tfdb_dual_judge(cache->seq);

    TFDB_DEBUG("tfdb_dual_judge:%d\n", judge_state);

    /* usually, we just read value once during the initializing. */
    if (judge_state == 0xff)
    {
        result[0] = tfdb_get(&index->indexes[0], rw_buffer, &(cache->addr_cache[0]), rw_buffer_bak);
        if (result[0] == TFDB_NO_ERR)
        {
            cache->seq[0] = (rw_buffer_bak[0] << 8) | (rw_buffer_bak[1]);
            if (value_to != NULL)
            {
                tfdb_memcpy(value_to, &(rw_buffer_bak[2]), index->indexes[0].value_length - 2);
            }
        }
        else
        {
            cache->seq[0] = 0;
        }

        result[1] = tfdb_get(&index->indexes[1], rw_buffer, &(cache->addr_cache[1]), rw_buffer_bak);
        if (result[1] == TFDB_NO_ERR)
        {
            cache->seq[1] = (rw_buffer_bak[0] << 8) | (rw_buffer_bak[1]);
        }
        else
        {
            cache->seq[1] = 0;
        }

        judge_state = tfdb_dual_judge(cache->seq);
        if (judge_state == 1)
        {
            if (value_to != NULL)
            {
                tfdb_memcpy(value_to, &(rw_buffer_bak[2]), index->indexes[1].value_length - 2);
            }
        }
        else if (judge_state == 0xff)
        {
            rresult = TFDB_SEQ_ERR;
            goto end;
        }
    }

    /* the latest data is in the current block, find the two candidates of the previous data:
     * candidate 1: the previous valid record in the current block.
     * candidate 0: the latest valid record in another block. */
    current_block = judge_state;
    current_seq = cache->seq[current_block];

    pre_addr = 0;
    result[1] = tfdb_get_pre(&index->indexes[current_block], rw_buffer, &(cache->addr_cache[current_block]), &pre_addr, rw_buffer_bak);
    if (result[1] == TFDB_NO_ERR)
    {
        seq[1] = (rw_buffer_bak[0] << 8) | (rw_buffer_bak[1]);
        if (value_to != NULL)
        {
            /* save candidate 1 to value_to first, it will be covered if candidate 0 is selected. */
            tfdb_memcpy(value_to, &(rw_buffer_bak[2]), index->indexes[current_block].value_length - 2);
        }
    }

    result[0] = tfdb_get(&index->indexes[1 - current_block], rw_buffer, &(cache->addr_cache[1 - current_block]), rw_buffer_bak);
    if (result[0] == TFDB_NO_ERR)
    {
        seq[0] = (rw_buffer_bak[0] << 8) | (rw_buffer_bak[1]);
    }

    /* rank the two candidates by the backward distance to the current seq in the seq cycle,
     * the smaller rank is the newer one:
     * rank 1 ~ (TFDB_DUAL_SEQ_COUNT - 1): the seq value is some steps before the current seq.
     * rank TFDB_DUAL_SEQ_COUNT: the same seq value to the current seq, at least a full cycle before.
     * rank (TFDB_DUAL_SEQ_COUNT + 1): illegal seq value, keep the lowest priority.
     * rank 0xff: the candidate does not exist. */
    for (i = 0; i < 2; i++)
    {
        if (result[i] != TFDB_NO_ERR)
        {
            rank[i] = 0xff;
        }
        else
        {
            rank[i] = tfdb_dual_seq_rank(current_seq, seq[i]);
        }
    }

    if ((rank[0] == 0xff) && (rank[1] == 0xff))
    {
        if ((result[1] != TFDB_NO_PRE_DATA) && (result[1] != TFDB_NO_DATA))
        {
            /* the flash block of the latest data is broken. */
            rresult = result[1];
        }
        else if ((result[0] != TFDB_NO_DATA) && (result[0] != TFDB_HDR_ERR))
        {
            rresult = result[0];
        }
        else
        {
            /* no older valid record in both blocks. */
            rresult = TFDB_NO_PRE_DATA;
        }
        goto end;
    }

    if (rank[0] <= rank[1])
    {
        /* the latest record in another block is the previous data, and it wins when the
         * ranks are the same, because the two blocks are written alternately, the record
         * in another block is newer when both candidates have the same seq value. */
        pre_block = 1 - current_block;
        pre_addr = cache->addr_cache[pre_block];
        if (value_to != NULL)
        {
            tfdb_memcpy(value_to, &(rw_buffer_bak[2]), index->indexes[pre_block].value_length - 2);
        }
    }
    else
    {
        /* the record before the latest one in the current block is the previous data,
         * which has been saved to value_to. */
        pre_block = current_block;
    }

    TFDB_DEBUG("    pre block:%d\n", pre_block);

    if (pre_cache != NULL)
    {
        pre_cache->addr_cache[0] = 0;
        pre_cache->addr_cache[1] = 0;
        pre_cache->seq[0] = 0;
        pre_cache->seq[1] = 0;
        pre_cache->addr_cache[pre_block] = pre_addr;
        pre_cache->seq[pre_block] = (pre_block == current_block) ? seq[1] : seq[0];
    }

    rresult = TFDB_NO_ERR;
end:
    TFDB_LOG("tfdb_dual_get_pre:%d\n", rresult);
    return rresult;
}

/**
 * set data in flash and save the addr and seq to cache.
 *
 * @param index the data manage index.
 * @param rw_buffer buffer to store prepared read data or write data.
 * @param rw_buffer_bak buffer to store prepared read data or write data.
 * @param cache the pointer to addr which is user offered, which will save read addr and seq.
 * @param value_from the pointer to buffer which is user offered that need to save.
 *
 * @return TFDB_Err_Code
 */
TFDB_Err_Code tfdb_dual_set(const tfdb_dual_index_t *index, uint8_t *rw_buffer, uint8_t *rw_buffer_bak, tfdb_dual_cache_t *cache, void *value_from)
{
    TFDB_Err_Code rresult = TFDB_NO_ERR;
    TFDB_Err_Code result[2];
    uint8_t judge_state;
    uint16_t write_seq;

    if (cache != NULL)
    {
        judge_state = tfdb_dual_judge(cache->seq);

        TFDB_DEBUG("tfdb_dual_judge:%d\n", judge_state);

        /* usually, we just read value once during the initializing. */
        if (judge_state != 0xff)
        {
write:
            write_seq = tfdb_dual_get_next_seq(cache->seq[judge_state]);
            judge_state = 1 - judge_state;  /* we need to write in another flash block. */

            rw_buffer_bak[0] = (uint8_t)(write_seq >> 8);
            rw_buffer_bak[1] = (uint8_t)write_seq;

            tfdb_memcpy(&(rw_buffer_bak[2]), value_from, index->indexes[judge_state].value_length - 2);

            result[judge_state] = tfdb_set(&index->indexes[judge_state], rw_buffer, &(cache->addr_cache[judge_state]), rw_buffer_bak);
            if (result[judge_state] == TFDB_NO_ERR)
            {
                cache->seq[judge_state] = write_seq;
            }
            else
            {
                /* block is error, do not write to another block, for keeping the old data safe.
                 * if you want to write in another block, please use tfdb_set directly. */
                rresult = judge_state + TFDB_FLASH1_ERR;
            }
        }
        else
        {
            result[0] = tfdb_get(&index->indexes[0], rw_buffer, &(cache->addr_cache[0]), rw_buffer_bak);
            if (result[0] == TFDB_NO_ERR)
            {
                cache->seq[0] = (rw_buffer_bak[0] << 8) | (rw_buffer_bak[1]);
            }
            else
            {
                cache->seq[0] = 0;
            }

            result[1] = tfdb_get(&index->indexes[1], rw_buffer, &(cache->addr_cache[1]), rw_buffer_bak);
            if (result[1] == TFDB_NO_ERR)
            {
                cache->seq[1] = (rw_buffer_bak[0] << 8) | (rw_buffer_bak[1]);
            }
            else
            {
                cache->seq[1] = 0;
            }

            judge_state = tfdb_dual_judge(cache->seq);
            if (judge_state == 0xff)
            {
                judge_state = 0;    /* first write 0 block. */
            }

            goto write;
        }
    }
    else
    {
        return TFDB_CACHE_ERR;
    }

    return rresult;
}
