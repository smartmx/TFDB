# TFDB

 Tiny Flash Database for MCU.

## TinyFlashDB设计前言

在单片机日常开发中，总会需要存储一些信息，这时就需要使用单片机FLASH存储的方案，目前单片机存储的方案有很多，比如：EASYFLASH、FLASHDB、OSAL_NV等等方案，他们程序都非常大，在存储不多的变量时不值得。而且现有方案的代码中很少有考虑到flash写入出错的情况。  
在实际产品中，嵌入式产品flash写入可能会受各种因素影响（电池供电、意外断电、气温等）从而并不是很稳定，一旦出现错误，会导致产品一系列问题。  

## TinyFlashDB设计理念

不同于其他很多的KV型数据库，TinyFlashDB每一个需要存储的变量都会分配一个单独的单片机flash扇区，变量长度不可变。  
TinyFlashDB在设计时就考虑了写入错误的影响，追求力所能及的安全保障、资源占用方面尽可能的缩小（不到1kb代码占用）、尽可能的通用性（可以移植到51等8位机，无法逆序写入的stm32L4系列，某些flash加密的单片机和其他普通32位机上）。  

## TinyFlashDB使用示例

```c
const tfdb_index_t test_index = {
    .end_byte = 0x00,
    .flash_addr = 0x4000,
    .flash_size = 256,
    .value_length = 2,

};
tfdb_addr_t addr = 0; /*addr cache*/

uint8_t test_buf[4]; /*aligned_value_size*/

uint16_t test_value;

void main()
{
    TFDB_Err_Code result;
    result = tfdb_set(&test_index, test_buf, &addr, &test_value);
    if(result == TFDB_NO_ERR)
    {
        printf("set ok, addr:%x\n", addr);
    }

    result = tfdb_get(&test_index, test_buf, &addr, &test_value);
    if(result == TFDB_NO_ERR)
    {
        printf("get ok, addr:%x, value:%x\n", addr, test_value);
    }
}
```

## TinyFlashDB API介绍

```c
typedef struct _tfdb_index_struct{
    tfdb_addr_t     flash_addr;/* the start address of the flash block */
    uint16_t        flash_size;/* the size of the flash block */
    uint8_t         value_length;/* the length of value that saved in this flash block */
    uint8_t         end_byte; /* must different to TFDB_VALUE_AFTER_ERASE */
    /* 0x00 is recommended for end_byte, because almost all flash is 0xff after erase. */
}tfdb_index_t;
```

结构体功能：在TinyFlashDB中，API的操作都需要指定的参数index，该index结构体中存储了flash的地址，flash的大小，存储的变量的长度，结束标志位。 在读取flash扇区时会去校验此信息。  

```c
TFDB_Err_Code tfdb_get(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void* value_to);
```

函数功能：从index指向的扇区中获取一个index中指定变量长度的变量，flash头部数据校验出错不会重新初始化flash。  

参数 index：tfdb操作的index指针。

参数 rw_buffer：写入和读取的缓存，所有flash的操作最后都会将整理后的数据拷贝到该buffer中，再调用tfdb_port_write或者tfdb_port_read进行写入。当芯片对于写入的数据区缓存有特殊要求（例如4字节对齐，256字节对齐等），可以通过该参数将符合要求的变量指针传递给函数使用。至少为4字节长度。  

参数 addr_cache：可以是NULL，或者是地址缓存变量的指针，当addr_cache不为NULL，并且也不为0时，则认为addr_cache已经初始化成功，不再校验flash头部，直接从该addr_cache的地址读取数据。  

参数 value_to：要存储数据内容的地址。  

返回值：TFDB_NO_ERR成功，其他失败。  

```c
TFDB_Err_Code tfdb_set(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void* value_from);
```

函数功能：在index指向的扇区中写入一个index中指定变量长度的变量，flash头部数据校验出错重新初始化flash。  

参数 index：tfdb操作的index指针。  

参数 rw_buffer：写入和读取的缓存，所有flash的操作最后都会将整理后的数据拷贝到该buffer中，再调用tfdb_port_write或者tfdb_port_read进行写入。当芯片对于写入的数据区缓存有特殊要求（例如4字节对齐，256字节对齐等），可以通过该参数将符合要求的变量指针传递给函数使用。至少为4字节长度。  

参数 addr_cache：可以是NULL，或者是地址缓存变量的指针，当addr_cache不为NULL，并且也不为0时，则认为addr_cache已经初始化成功，不再校验flash头部，直接从该addr_cache的地址读取数据。  

参数 value_from：要存储的数据内容。  

返回值：TFDB_NO_ERR成功，其他失败。  

## TinyFlashDB设计原理

观察上方代码，可以发现TinyFlashDB的操作都需要tfdb_index_t定义的index参数。  
Flash初始化后头部信息为4字节，所以只支持1、2、4、8字节操作的flash：  
头部初始化时会读取头部，所以函数中rw_buffer指向的数据第一要求至少为4字节，如果最小写入单位是8字节，则为第一要求最少为8字节。  

|第一字节|第二字节|第三字节|第四字节和其他对齐字节|
-|-|-|-
|flash_size高8位字节|flash_size低8位字节|value_length|end_byte|

数据存储时，会根据flash支持的字节操作进行对齐，所以函数中rw_buffer指向的数据第二要求至少为下面函数中计算得出的aligned_value_size个字节：

```c
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
```

|前value_length个字节|第value_length+1字节|第value_length+2字节|其他对齐字节|
-|-|-|-
|value_from数据内容|value_from的和校验|end_byte|end_byte|  

每次写入后都会再读取出来进行校验，如果校验不通过，就会继续在下一个地址写入。指导达到最大写入次数（TFDB_WRITE_MAX_RETRY）或者头部校验错误。  

读取数据时也会计算和校验，不通过的话继续读取，直到返回校验通过的最新数据，或者读取失败。  

## TinyFlashDB移植和配置

### 移植使用只需要在tfdb_port.c中，编写完成三个接口函数，也要在tfdb_port.h中添加相应的头文件和根据不同芯片修改宏定义

```c
TFDB_Err_Code tfdb_port_read(tfdb_addr_t addr, uint8_t *buf, size_t size);

TFDB_Err_Code tfdb_port_erase(tfdb_addr_t addr, size_t size);

TFDB_Err_Code tfdb_port_write(tfdb_addr_t addr, const uint8_t *buf, size_t size);
```

### 所有的配置项都在tfdb_port.h中

```c
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
```

## Demo

裸机移植例程：  
[STM32F429IGT6](https://github.com/smartmx/TFDB/tree/raw/Templates/STM32F429IGT6_TFDB)  
[CH583](https://github.com/smartmx/TFDB/tree/raw/Templates/CH583_TFDB)  

## [博客主页](https://blog.maxiang.vip/)
