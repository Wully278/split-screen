#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "malloc.h" 
#include "sd_sdio.h"
#include "flash.h"
#include "mass_mal.h"
#include "usb_lib.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "memory.h"	    
#include "usb_bot.h" 



extern u8 Max_Lun;	//支持的磁盘个数,0表示1个,1表示2个

int main()
{
	u8 offline_cnt=0;
	u8 tct=0;
	u8 USB_STA;
	u8 Divece_STA;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(115200);
	TFTLCD_Init();			//LCD初始化
	EN25QXX_Init();
	my_mem_init(SRAMIN);	//初始化内部内存池
	
	FRONT_COLOR=RED;//设置字体为红色 
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN-STM32F1");	
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"USB Card Reader TEST");	
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");  
	
	if(SD_Init()!=0)
	{	
		Max_Lun=0;	//SD卡错误,则仅只有一个磁盘
		LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"SD Card Error!");	//检测SD卡错误
	}
	else //SD 卡正常
	{   															  
		LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"SD Card Size:     MB"); 
 		Mass_Memory_Size[1]=SDCardInfo.CardCapacity;//得到SD卡容量（字节），当SD卡容量超过4G的时候,需要用到两个u32来表示
	    Mass_Block_Size[1] =512;//因为我们在Init里面设置了SD卡的操作字节为512个,所以这里一定是512个字节.
	    Mass_Block_Count[1]=Mass_Memory_Size[1]/Mass_Block_Size[1];
		LCD_ShowNum(10+13*8,50,Mass_Memory_Size[1]>>20,5,16);	//显示SD卡容量 
 	}
	if(EN25QXX_ReadID()!=EN25Q128)
	{
		LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"EN25QXX Error!          ");	//检测EN25QXX错误
	}
	else //SPI FLASH 正常
	{   														 	 
		Mass_Memory_Size[0]=1024*1024*12;//前12M字节
	    Mass_Block_Size[0] =512;//设置SPI FLASH的操作扇区大小为512
	    Mass_Block_Count[0]=Mass_Memory_Size[0]/Mass_Block_Size[0];
		LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"EN25QXX FLASH Size:12MB");
	} 
	
	delay_ms(1800);
	USB_Port_Set(0); 	//USB先断开
	delay_ms(700);
	USB_Port_Set(1);	//USB再次连接 
	LCD_ShowString(10,170,200,16,16,"USB Connecting...");	//提示USB开始连接	 
  	Data_Buffer=mymalloc(SRAMIN,BULK_MAX_PACKET_SIZE*2*4);	//为USB数据缓存区申请内存
	Bulk_Data_Buff=mymalloc(SRAMIN,BULK_MAX_PACKET_SIZE);	//申请内存
 	//USB配置
 	USB_Interrupts_Config();    
 	Set_USBClock();   
 	USB_Init();	    
	delay_ms(1800);	   	    
	while(1)
	{	
		delay_ms(1);				  
		if(USB_STA!=USB_STATUS_REG)//状态改变了 
		{	 						   
			//LCD_Fill(10,190,240,190+16,WHITE);//清除显示			  	   
			if(USB_STATUS_REG&0x01)//正在写		  
			{
				//LCD_ShowString(10,190,200,16,16,"USB Writing...");//提示USB正在写入数据	 
			}
			if(USB_STATUS_REG&0x02)//正在读
			{
				//LCD_ShowString(10,190,200,16,16,"USB Reading...");//提示USB正在读出数据  		 
			}	 										  
			if(USB_STATUS_REG&0x04)LCD_ShowString(10,210,200,16,16,"USB Write Err ");//提示写入错误
			else LCD_Fill(10,210,240,210+16,WHITE);//清除显示	  
			if(USB_STATUS_REG&0x08)LCD_ShowString(30,230,200,16,16,"USB Read  Err ");//提示读出错误
			else LCD_Fill(10,230,240,230+16,WHITE);//清除显示    
			USB_STA=USB_STATUS_REG;//记录最后的状态
		}
		if(Divece_STA!=bDeviceState) 
		{
			if(bDeviceState==CONFIGURED)LCD_ShowString(10,170,200,16,16,"USB Connected    ");//提示USB连接已经建立
			else LCD_ShowString(10,170,200,16,16,"USB DisConnected ");//提示USB被拔出了
			Divece_STA=bDeviceState;
		}
		tct++;
		if(tct==200)
		{
			tct=0;
			LED1=!LED1;//提示系统在运行
			if(USB_STATUS_REG&0x10)
			{
				offline_cnt=0;//USB连接了,则清除offline计数器
				bDeviceState=CONFIGURED;
			}else//没有得到轮询 
			{
				offline_cnt++;  
				if(offline_cnt>10)bDeviceState=UNCONNECTED;//2s内没收到在线标记,代表USB被拔出了
			}
			USB_STATUS_REG=0;
		}
	}
}
