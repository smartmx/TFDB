# TFDB
 tiny flash database for MCU.

每次项目中使用flash存储一个特定变量时，因为芯片、数据长度等不同因素都会重新写一遍代码。而直接擦写一个flash扇区，却只用其中一点字节存储信息，会影响产品寿命，均衡使用一个flash扇区会更好。而使用大型的flash数据管理（EasyFlash、FlashDB等）又没有必要，而且也速度也慢。

所以就此整理出TFDB，tiny flash database：
原则就是小，不但flash占用小，ram占用也一定要小，要和直接使用差不了多少。
同时也考虑了偶发的flash错误，有简单的纠错机制，可以使产品寿命更长。
代码Flash占用不到1kb,RAM占用全由用户根据存储的大小决定。

flash配置支持1字节和4字节操作的flash，不支持其他flash。通过修改tfdb_port.h中宏定义：TFDB_WRITE_UNIT_BYTES，完成配置。

```
flash错误一般都是无法从0写到1，即无法擦除。
tfdb_set写操作会去寻找到最后一个擦除后的位置进行写入。
tfdb_get读操作会先找最后一个擦除未写入的位置，从后往前读取，并校验校验位和停止位。直到找到一个正确的值。

该库中，函数申请局部变量很少，需要使用到的读写缓存空间由用户申请后，作为参数传递给函数使用，函数不使用栈内存。因为有的芯片的flash加密，只可以调用api函数，对读写缓存有特殊要求。所以将其整理成函数参数有助于更合理的编写flash接口函数。
rw_buffer指针指向的数组长度必须是aligned_value_size（可以查看代码看计算方式）大小，使用需要根据自己存储的变量长度申请局部变量进行使用。
```

# 该库的主要理念就是：一个分配过来的flash空间只存一个变量，无备份等操作。擦除也是将该分配的空间大小直接全部擦除，并不会按照flash的最小擦除大小擦除。

## 使用只需要调用两个函数：

### TFDB_Err_Code tfdb_get(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void* value_to);

### TFDB_Err_Code tfdb_set(const tfdb_index_t *index, uint8_t *rw_buffer, tfdb_addr_t *addr_cache, void* value_from);

## 移植使用只需要在tfdb_port.c中，编写完成三个接口函数，也要在tfdb_port.h中添加相应的头文件和根据不同芯片修改宏定义：

### TFDB_Err_Code tfdb_port_read(tfdb_addr_t addr, uint8_t *buf, size_t size);

### TFDB_Err_Code tfdb_port_erase(tfdb_addr_t addr, size_t size);

### TFDB_Err_Code tfdb_port_write(tfdb_addr_t addr, const uint8_t *buf, size_t size);

# 使用样例：
```
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
    if(result == TFDB_NO_ERR){
        PRINT("set ok, addr:%x\n", addr);
    }

    result = tfdb_get(&test_index, test_buf, &addr, &test_value);
    if(result == TFDB_NO_ERR){
        PRINT("get ok, addr:%x, value:%x\n", addr, test_value);
    }
}
```

