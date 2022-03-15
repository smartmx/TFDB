/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        : 外设从机应用主函数及任务系统初始化
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "CH58x_common.h"
#include "HAL.h"
#include "tinyflashdb.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) u32 MEM_BUF[BLE_MEMHEAP_SIZE / 4];
__attribute__((aligned(4))) uint8_t tfkvtest[128];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
u8C MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*******************************************************************************
 * Function Name  : Main_Circulation
 * Description    : 主循环
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/

tfdb_index_t tetttt =
{
    .end_byte = 0x00,
    .flash_addr = 0x4000,
    .flash_size = 256,
    .value_length = 1,
};

uint8_t buf[256];
/*******************************************************************************
 * Function Name  : main
 * Description    : 主函数
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
int main(void)
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
    PRINT("%s\n", VER_LIB);
    tfdb_addr_t maddr = 0; //地址缓存
    uint8_t likii[8] = {'3', '2', '1', '0', '7', '6', '5', '4'}; //测试写入错误数据后能否找到正确地址
    uint16_t tests = 16; //数据缓存
    TFDB_Err_Code result;
    result = tfdb_set(&tetttt, tfkvtest, &maddr, &tests);
    if (result == TFDB_NO_ERR)
    {
        PRINT("set ok\n");
    }
    PRINT("addr cache:%x\n", maddr); //打印缓存地址

    EEPROM_READ(tetttt.flash_addr, buf, 256);//读取flash所有数据
    PRINT("DATA:");//将数据打印出来
    for (uint16_t i = 0; i < 256; i++)
    {
        PRINT(" %02x", buf[i]);
    }
    PRINT("\n");
#if 1
    /* test read */
    EEPROM_WRITE(maddr + 8, likii, 8);//写入错误数据
    maddr = 0;//重置地址，看接下来能否找到地址
#endif
    tests = 0;//将数据清零，重新从flash读取，看是否写入成功
    result = tfdb_get(&tetttt, tfkvtest, &maddr, &tests);
    if (result == TFDB_NO_ERR)
    {
        PRINT("addr cache:%x\n", maddr); //打印缓存地址
        PRINT("tests:%d\n", tests);
    }
    while (1) {}//hold

}

/******************************** endfile @ main ******************************/
