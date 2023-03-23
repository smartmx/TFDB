# TFDB

 Tiny Flash Database for MCU.

## TinyFlashDB设计前言

在单片机日常开发中，总会需要存储一些信息，这时就需要使用单片机FLASH存储的方案，目前单片机存储的方案有很多，比如：EASYFLASH、FLASHDB、OSAL_NV等等方案，他们程序都非常大，在存储不多的变量时不值得。而且很少有考虑到flash写入出错的情况。  
在实际产品中，嵌入式产品flash写入可能会受各种因素影响（电池供电、意外断电、气温等）从而并不是很稳定，一旦出现错误，会导致产品一系列问题。  

## TinyFlashDB设计理念

不同于其他很多的KV型数据库，TinyFlashDB每一个需要存储的变量都会分配一个单独的单片机flash扇区，变量长度不可变。  
所以TinyFlashDB仅适用于存储几个关键性变量（例如：IAP跳转标志、系统断电时间等等），不适合大规模数据存储（大规模数据存储可使用EASYFLASH等）。  
TinyFlashDB在设计时就考虑了写入错误的影响，追求力所能及的安全保障、资源占用方面尽可能的缩小（不到1kb代码占用）、尽可能的通用性（可以移植到51等8位机，无法逆序写入的stm32L4系列，某些flash加密的单片机和其他普通32位机上）。  

## TinyFlashDB使用示例

```c
const tfdb_index_t test_index = {
    .end_byte = 0x00,
    .flash_addr = 0x4000,
    .flash_size = 256,
    .value_length = 2,
};/* c99写法，如果编译器不支持，可自行改为c89写法 */
tfdb_addr_t addr = 0; /*addr cache*/

uint8_t test_buf[TFDB_ALIGNED_RW_BUFFER_SIZE(2,1)]; /*aligned_value_size*/

uint16_t test_value;

void main()
{
    TFDB_Err_Code result;
    result = tfdb_set(&test_index, test_buf, &addr, &test_value);
    if(result == TFDB_NO_ERR)
    {
        printf("set ok, addr:%x\n", addr);
    }

    addr = 0;   /* reset addr cache, to see tfdb_get. */

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

函数功能：从`index`指向的扇区中获取一个index中指定变量长度的变量，flash头部数据校验出错不会重新初始化flash。  

参数 `index`：tfdb操作的index指针。

参数 `rw_buffer`：写入和读取的缓存，所有flash的操作最后都会将整理后的数据拷贝到该buffer中，再调用`tfdb_port_write`或者`tfdb_port_read`进行读取写入。当芯片对于写入的数据区缓存有特殊要求（例如4字节对齐，256字节对齐等），可以通过该参数将符合要求的变量指针传递给函数使用。至少为4字节长度。  

参数 `addr_cache`：可以是`NULL`，或者是地址缓存变量的指针，当`addr_cache`不为`NULL`，并且也不为0时，则认为`addr_cache`已经初始化成功，不再校验flash头部，直接从该`addr_cache`的地址读取数据。  

参数 `value_to`：要存储数据内容的地址。  

返回值：`TFDB_NO_ERR`成功，其他失败。  

```c
TFDB_Err_Code tfdb_set(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void* value_from);
```

函数功能：在`index`指向的扇区中写入一个index中指定变量长度的变量，flash头部数据校验出错重新初始化flash。  

参数 `index`：tfdb操作的index指针。  

参数 `rw_buffer`：写入和读取的缓存，所有flash的操作最后都会将整理后的数据拷贝到该buffer中，再调用`tfdb_port_write`或者`tfdb_port_read`进行读取写入。当芯片对于写入的数据区缓存有特殊要求（例如4字节对齐，256字节对齐等），可以通过该参数将符合要求的变量指针传递给函数使用。至少为4字节长度。  

参数 `addr_cache`：可以是`NULL`，或者是地址缓存变量的指针，当`addr_cache`不为`NULL`，并且也不为0时，则认为`addr_cache`已经初始化成功，不再校验flash头部，直接从该`addr_cache`的地址读取数据。  

参数 `value_from`：要存储的数据内容。  

返回值：`TFDB_NO_ERR`成功，其他失败。  

## TinyFlashDB dual使用示例

tfdb dual api是基于`tfdb_set`和`tfdb_get`封装而成的。`tfdb dual`会调用`tfdb_set`和`tfdb_get`，并且在数据前部添加两个字节的seq，所以在tfdb dual中，最长支持的存储变量长度为253字节。  
同时，tfdb dual api需要提供两个缓冲区，并且需要是增加两字节变量长度再重新计算的`aligned_value_size`。

```c
typedef struct _my_test_params_struct
{
    uint32_t    aa[2];
    uint8_t     bb[16];
} my_test_params_t;

my_test_params_t my_test_params = {
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18
};

tfdb_dual_index_t my_test_tfdb_dual = {
        .indexes[0] = {
                .end_byte     = 0x00,
                .flash_addr   = 0x08077000,
                .flash_size   = 256,
                .value_length = TFDB_DUAL_VALUE_LENGTH(sizeof(my_test_params_t)),
        },
        .indexes[1] = {
                .end_byte     = 0x00,
                .flash_addr   = 0x08077100,
                .flash_size   = 256,
                .value_length = TFDB_DUAL_VALUE_LENGTH(sizeof(my_test_params_t)),
        },
};

tfdb_dual_cache_t my_test_tfdb_dual_cache = {0};

void my_test_tfdb_dual_func()
{
    uint32_t rw_buffer[TFDB_DUAL_ALIGNED_RW_BUFFER_SIZE(TFDB_DUAL_VALUE_LENGTH(sizeof(my_test_params_t)), 4)];
    uint32_t rw_buffer_bak[TFDB_DUAL_ALIGNED_RW_BUFFER_SIZE(TFDB_DUAL_VALUE_LENGTH(sizeof(my_test_params_t)), 4)];
    TFDB_Err_Code err;
    for(uint8_t i = 0; i < 36; i++)
    {
        err = tfdb_dual_get(&my_test_tfdb_dual, (uint8_t *)rw_buffer, (uint8_t *)rw_buffer_bak, &my_test_tfdb_dual_cache, &my_test_params);
        if(err == TFDB_NO_ERR)
        {
            printf("read ok\ncache seq1:0x%04x, seq2:0x%04x\naddr1:0x%08x, addr2:0x%08x\n", my_test_tfdb_dual_cache.seq[0], my_test_tfdb_dual_cache.seq[1], my_test_tfdb_dual_cache.addr_cache[0], my_test_tfdb_dual_cache.addr_cache[1]);
        }
        else
        {
            printf("read err:%d\n", err);
        }

        my_test_params.aa[0]++;
        my_test_params.aa[1]++;
        for(uint8_t i = 0; i < 16; i++)
        {
            my_test_params.bb[i]++;
        }

        memset(&my_test_tfdb_dual_cache, 0, sizeof(my_test_tfdb_dual_cache));   /* 测试无地址缓存写入 */

        err = tfdb_dual_set(&my_test_tfdb_dual, (uint8_t *)rw_buffer, (uint8_t *)rw_buffer_bak, &my_test_tfdb_dual_cache, &my_test_params);
        if(err == TFDB_NO_ERR)
        {
            printf("write ok\ncache seq1:0x%04x, seq2:0x%04x\naddr1:0x%08x, addr2:0x%08x\n", my_test_tfdb_dual_cache.seq[0], my_test_tfdb_dual_cache.seq[1], my_test_tfdb_dual_cache.addr_cache[0], my_test_tfdb_dual_cache.addr_cache[1]);
        }
        else
        {
            printf("write err:%d\n", err);
        }

        memset(&my_test_tfdb_dual_cache, 0, sizeof(my_test_tfdb_dual_cache));   /* 测试无地址缓存读取 */
    }
}

```

## TinyFlashDB dual API介绍

```c
typedef struct _tfdb_dual_index_struct
{
    tfdb_index_t indexes[2];
} tfdb_dual_index_t;

typedef struct _tfdb_dual_cache_struct
{
    tfdb_addr_t     addr_cache[2];
    uint16_t        seq[2];
} tfdb_dual_cache_t;
```

结构体功能：在TinyFlashDB dual中，API的操作都需要指定的参数`index`，该`index`结构体中存储了两个`tfdb_index_t`。

```c
TFDB_Err_Code tfdb_dual_get(const tfdb_dual_index_t *index, uint8_t *rw_buffer, uint8_t *rw_buffer_bak, tfdb_dual_cache_t *cache, void *value_to);
```

函数功能：从index指向的扇区中获取一个index中指定变量长度的变量，flash头部数据校验出错不会重新初始化flash。  

参数 `index`：tfdb操作的index指针。

参数 `rw_buffer`：写入和读取的缓存，所有flash的操作最后都会将整理后的数据拷贝到该buffer中，再调用`tfdb_port_write`或者`tfdb_port_read`进行读取写入。当芯片对于写入的数据区缓存有特殊要求（例如4字节对齐，256字节对齐等），可以通过该参数将符合要求的变量指针传递给函数使用。至少为4字节长度。  

参数 `rw_buffer_bak`：写入和读取的缓存，所有flash的操作最后都会将整理后的数据拷贝到该buffer中，再调用`tfdb_port_write`或者`tfdb_port_read`进行读取写入。当芯片对于写入的数据区缓存有特殊要求（例如4字节对齐，256字节对齐等），可以通过该参数将符合要求的变量指针传递给函数使用。至少为4字节长度。  

参数 `cache`：不可以是`NULL`，必须是`tfdb_dual_cache_t`定义的缓存的指针，当`cache`中数据合法时，则认为`cache`已经初始化成功，直接从该`cache`的flash块和地址读取数据。  

参数 `value_to`：要存储数据内容的地址。  

返回值：`TFDB_NO_ERR`成功，其他失败。  

```c
TFDB_Err_Code tfdb_dual_set(const tfdb_dual_index_t *index, uint8_t *rw_buffer, uint8_t *rw_buffer_bak, tfdb_dual_cache_t *cache, void *value_from);
```

函数功能：在index指向的扇区中写入一个index中指定变量长度的变量，flash头部数据校验出错重新初始化flash。  

参数 `index`：tfdb操作的index指针。  

参数 `rw_buffer`：写入和读取的缓存，所有flash的操作最后都会将整理后的数据拷贝到该buffer中，再调用`tfdb_port_write`或者`tfdb_port_read`进行读取写入。当芯片对于写入的数据区缓存有特殊要求（例如4字节对齐，256字节对齐等），可以通过该参数将符合要求的变量指针传递给函数使用。至少为4字节长度。  

参数 `rw_buffer_bak`：写入和读取的缓存，所有flash的操作最后都会将整理后的数据拷贝到该buffer中，再调用`tfdb_port_write`或者`tfdb_port_read`进行读取写入。当芯片对于写入的数据区缓存有特殊要求（例如4字节对齐，256字节对齐等），可以通过该参数将符合要求的变量指针传递给函数使用。至少为4字节长度。  

参数 `cache`：不可以是`NULL`，必须是`tfdb_dual_cache_t`定义的缓存的指针，当`cache`中数据合法时，则认为`cache`已经初始化成功，直接从该`cache`的flash块和地址读取数据。  

参数 `value_from`：要存储的数据内容。  

返回值：`TFDB_NO_ERR`成功，其他失败。  

## TinyFlashDB设计原理

观察上方代码，可以发现TinyFlashDB的操作都需要`tfdb_index_t`定义的`index`参数。  
Flash初始化后头部信息为4字节，所以只支持1、2、4、8字节操作的flash：  
头部初始化时会读取头部，所以函数中`rw_buffer`指向的数据第一要求至少为4字节，如果最小写入单位是8字节，则为第一要求最少为8字节。  

|第一字节|第二字节|第三字节|第四字节和其他对齐字节|
-|-|-|-
|flash_size高8位字节|flash_size低8位字节|value_length|end_byte|

数据存储时，会根据flash支持的字节操作进行对齐，所以函数中`rw_buffer`指向的数据第二要求至少为下面函数中计算得出的`aligned_value_size`个字节：

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

每次写入后都会再读取出来进行校验，如果校验不通过，就会继续在下一个地址继续尝试写入。直到达到最大写入次数（TFDB_WRITE_MAX_RETRY）或者头部校验错误。  

读取数据时也会计算和校验，不通过的话继续读取，直到返回校验通过的最新数据，或者读取失败。  

## TinyFlashDB dual设计原理

数据前部两字节seq只有3种合法值，0x00ff->0x0ff0->0xff00。  
如此循环往复，通过读取两个block中最新变量的seq来判断哪个flash扇区中存储的是最新值。  
当最新值存储在第一扇区时，下次写入则会在第二扇区写入，反之亦然。

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

/* The data value in flash after erased, most are 0xff, some flash maybe different.
 * if it's over 1 byte, please be care of little endian or big endian. */
#define TFDB_VALUE_AFTER_ERASE              0xff

/* The size of TFDB_VALUE_AFTER_ERASE, only support 1 / 2 / 4.
 * This value must not bigger than TFDB_WRITE_UNIT_BYTES. */
#define TFDB_VALUE_AFTER_ERASE_SIZE         1

/* the flash write granularity, unit: byte
 * only support 1(stm32f4)/ 2(CH559)/ 4(stm32f1)/ 8(stm32L4) */
#define TFDB_WRITE_UNIT_BYTES               8 /* @note you must define it for a value */

/* @note the max retry times when flash is error ,set 0 will disable retry count */
#define TFDB_WRITE_MAX_RETRY                32

/* must not use pointer type. Please use uint32_t, uint16_t or uint8_t. */
typedef uint32_t    tfdb_addr_t;
```

## TFDB资源占用

在去除DEBUG打印信息后，资源占用如下：

### Cortex M4平台

keil -o2编译优化选项

```c
      Code (inc. data)   RO Data    RW Data    ZI Data      Debug   Object Name

       154          0          0          0          0       2621   tfdb_port.o
       682          0          0          0          0       4595   tinyflashdb.o
```

### RISC-V平台

gcc -os编译优化选项

```c
 .text.tfdb_port_read
                0x00000000000039b4       0x1a ./Drivers/TFDB/tfdb_port.o
                0x00000000000039b4                tfdb_port_read
 .text.tfdb_port_erase
                0x00000000000039ce       0x46 ./Drivers/TFDB/tfdb_port.o
                0x00000000000039ce                tfdb_port_erase
 .text.tfdb_port_write
                0x0000000000003a14       0x5c ./Drivers/TFDB/tfdb_port.o
                0x0000000000003a14                tfdb_port_write
 .text.tfdb_check
                0x0000000000003a70       0x56 ./Drivers/TFDB/tinyflashdb.o
                0x0000000000003a70                tfdb_check
 .text.tfdb_init
                0x0000000000003ac6       0x56 ./Drivers/TFDB/tinyflashdb.o
                0x0000000000003ac6                tfdb_init
 .text.tfdb_set
                0x0000000000003b1c      0x186 ./Drivers/TFDB/tinyflashdb.o
                0x0000000000003b1c                tfdb_set
 .text.tfdb_get
                0x0000000000003ca2      0x11c ./Drivers/TFDB/tinyflashdb.o
                0x0000000000003ca2                tfdb_get
```

## Demo

裸机移植例程，RT-Thread可以参考使用：  
[STM32F429IGT6](https://github.com/smartmx/TFDB/tree/raw/Templates/STM32F429IGT6_TFDB)  
[CH583](https://github.com/smartmx/TFDB/tree/raw/Templates/CH583_TFDB)  

## [博客主页](https://blog.maxiang.vip/)
