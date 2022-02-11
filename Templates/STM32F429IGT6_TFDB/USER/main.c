#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "sdram.h"
#include "usmart.h"
#include "stmflash.h"
#include "tinyflashdb.h"
/************************************************
 ALIENTEK 阿波罗STM32F429开发板实验36
 FLASH模拟EEPROM实验-HAL库函数版
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com  
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

//要写入到STM32 FLASH的字符串数组
const u8 TEXT_Buffer[]={"STM32 FLASH TEST"};
#define TEXT_LENTH sizeof(TEXT_Buffer)	 		  	//数组长度	
#define SIZE TEXT_LENTH/4+((TEXT_LENTH%4)?1:0)

#define FLASH_SAVE_ADDR  0x08010000 	//设置FLASH 保存地址(必须为4的倍数，且所在扇区,要大于本代码所占用到的扇区.
										//否则,写操作的时候,可能会导致擦除整个扇区,从而引起部分程序丢失.引起死机.

const tfdb_index_t test_index = {
		.end_byte = 0x00,
		.flash_addr = FLASH_SAVE_ADDR,
		.flash_size = 16*1024,
		.value_length = 4,
};

tfdb_addr_t addr = 0; /*addr cache*/

uint32_t test_buf[2]; /*aligned_value_size*/

uint8_t test_value[4]={"hi0\0"};


int main(void)
{
	u8 key=0;
	u16 i=0;
	TFDB_Err_Code result;
	u8 datatemp[SIZE];
	HAL_Init();                     //初始化HAL库   
	Stm32_Clock_Init(360,25,2,8);   //设置时钟,180Mhz
	delay_init(180);                //初始化延时函数
	uart_init(115200);              //初始化USART
	usmart_dev.init(90); 		    //初始化USMART	
	LED_Init();                     //初始化LED 
	KEY_Init();                     //初始化按键
	SDRAM_Init();                   //初始化SDRAM
	LCD_Init();                     //初始化LCD
	POINT_COLOR=RED;
	LCD_ShowString(30,50,200,16,16,"Apollo STM32F4/F7"); 
	LCD_ShowString(30,70,200,16,16,"FLASH EEPROM TEST");	
	LCD_ShowString(30,90,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,110,200,16,16,"2016/1/16");	 		
	LCD_ShowString(30,130,200,16,16,"KEY1:Write  KEY0:Read");
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY1_PRES)	//KEY1按下,写入STM32 FLASH
		{
			LCD_Fill(0,170,239,319,WHITE);//清除半屏    
 			LCD_ShowString(30,170,200,16,16,"Start Write FLASH....");
			test_value[2]++;
			if(test_value[2]>('9')){ 
				test_value[2] = '0';
			}
			result = tfdb_set(&test_index, (uint8_t *)test_buf, &addr, &test_value);
			if(result == TFDB_NO_ERR){
					printf("set ok, addr:%x\n", addr);
					LCD_ShowString(30,170,200,16,16,"FLASH Write Finished!");//提示传送完成
			} else {
					LCD_ShowString(30,170,200,16,16,"FLASH Write error!");//提示传送完成
			}
			
		}
		if(key==KEY0_PRES)	//KEY0按下,读取字符串并显示
		{
 			LCD_ShowString(30,170,200,16,16,"Start Read FLASH.... ");
			result = tfdb_get(&test_index, (uint8_t *)test_buf, NULL, &test_value);
			if(result == TFDB_NO_ERR){
					printf("get ok\n");
			}
			LCD_ShowString(30,170,200,16,16,"The Data Readed Is:  ");//提示传送完成
			LCD_ShowString(30,190,200,16,16,test_value);//显示读到的字符串
		}
		i++;
		delay_ms(10);  
		if(i==20)
		{
			LED0=!LED0;//提示系统正在运行	
			i=0;
		}		   
	}    
}
