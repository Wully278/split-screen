#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "malloc.h" 
#include "sd_sdio.h"
#include "flash.h"
#include "ff.h" 
#include "fatfs_app.h"
#include "bmp.h"

struct receive_info* object[32];
uint16_t *bullet_buffer1;
uint16_t *bullet_buffer2;
uint16_t *zombies_buffer;
int bullet_width;
int bullet_height;
int zombies_width;
int zombies_height;
extern temp_count;
extern int restart;
int main()
{
	int i,j;		
	u8 res=0;
	u32 total,free;
	FIL fp;
	FATFS fs;
	u32 offset;
	int ii = 0;
	int jj = 0;
	u16 buff[320];
	int buffer_count;
	UINT br =0;
	UINT bw;
	char* aa = "game/type/zombies/1.bmp";
	char* bb = "game/type/zombies/2.bmp";
	int base = 512*2048*12;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(115200);
	TFTLCD_Init();			//LCD初始化
	EN25QXX_Init();				//初始化EN25Q128	  
	my_mem_init(SRAMIN);		//初始化内部内存池
	
	
	for(i=0;i<32;i++){
		offset = my_mem_malloc(0, sizeof(struct receive_info));
		if(offset == 0XFFFFFFFF){printf("malloc error\r\n");}
		object[i] = (struct receive_info*)(malloc_cortol.membase[0] + offset);
		object[i]->x = 0;
		object[i]->y = 0;
		object[i]->flag = 0;
	}
	printf("here is ok\r\n");
	FRONT_COLOR=RED;//设置字体为红色 
//	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN STM32F1");	
//	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"Fatfs Test");	
//	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	  res = f_mount(&fs, "0:", 1);  
		//LCD_ShowxNum(100,190,res,6,16,0);
    if(res==0X0D){
			printf("create a new: %d\r\n", res);
			LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"create a new");
			res = f_mkfs("0:", 1, 4096); // 第一个参数是逻辑驱动器号，第二个参数是分区规则，第三个参数是分配单元大小
			if (res != FR_OK) {
					printf("Format Error: %d\r\n", res);
			} else {
					printf("Format completed successfully.\r\n");
			}
		}
		//将地图图片从flash的前12M范围重新写到12M开始地址，这样就可以不通过fatfs来访问了
		res = f_open(&bmp_files.fp, "game/Map/map0.bmp", FA_OPEN_ALWAYS | FA_READ);
		if(res != FR_OK) {
      printf("Open Error: %d\r\n", res);
			return 1;
    }else{
				if(f_read(&bmp_files.fp, &bmp_files.file_header, 14, &br)){
					printf("error:get file_header\r\n");
					printf("error:get file_header %d\r\n",br);
					return 1;
				};
				if(f_read(&bmp_files.fp,&bmp_files.info_header,40,&br)){
					printf("error:get info_heade\r\n");
					return 1;
				}
				printf("bmp_files.info_header.height is %d\r\n",bmp_files.info_header.height);
				printf("bmp_files.info_header.width is %d\r\n",bmp_files.info_header.width);
        // 读取文件内容
				for(i=0;i<bmp_files.info_header.height;i++){//printf("i is %d\r\n",i);
					f_read(&bmp_files.fp, buff, bmp_files.info_header.width*2, &br);
					EN25QXX_Write((u8*)buff,base,bmp_files.info_header.width*2);
					base+=bmp_files.info_header.width*2;
				}
    }
		jj=0;
		
		//为子弹申请内存
		get_bmp_header("game/bullet/1.bmp",&bmp_files,0);
		offset = my_mem_malloc(0, (bmp_files.info_header.width) * bmp_files.info_header.height * sizeof(uint16_t));
		bullet_buffer1 = (u16 *)(malloc_cortol.membase[0] + offset);
		f_lseek (&bmp_files.fp, 54);
		buffer_count = 0;
		for(i=0;i<bmp_files.info_header.height;i++){ //读取子弹图片放入缓存
			f_read(&bmp_files.fp, buff, bmp_files.info_header.width * sizeof(uint16_t), &br);
			for(j=0;j<bmp_files.info_header.width;j++){
				//if(buff[j]==0xFFFF){continue;}
				*(bullet_buffer1 + buffer_count + j) = buff[j];
			}
			buffer_count+=bmp_files.info_header.width;
		}
		//再申请一个
		offset = my_mem_malloc(0, (bmp_files.info_header.width +10) * bmp_files.info_header.height * sizeof(uint16_t));
		bullet_buffer2 = (u16 *)(malloc_cortol.membase[0] + offset);
		//保存子弹的宽和高
		bullet_width = bmp_files.info_header.width+10;
		bullet_height = bmp_files.info_header.height;
		//为僵尸图片申请内存
		get_bmp_header("game/type/zombies/1.bmp",&bmp_files,0);
		offset = my_mem_malloc(0, (bmp_files.info_header.width+2) * bmp_files.info_header.height * sizeof(uint16_t));
		zombies_buffer = (u16 *)(malloc_cortol.membase[0] + offset);
		zombies_width = bmp_files.info_header.width +2; //这里2根据实际调整
		zombies_height = bmp_files.info_header.height;
	while(1)
	{
		i++;
		if(i%10==0)
		{
			LED1=!LED1;
		}
		delay_ms(1);	
		
		if(restart){
			restart =0;
			bmp_show(0,0,"game/Map/map0.bmp");
		}
	if(!temp_count){
		ii = 1;
		while(object[ii]->filename[0]){
			bmp_show_flush(object[ii]->x,object[ii]->y-36,object[ii]->filename,"game/Map/map0.bmp",object[ii]->flag-48,object[ii]->type-48);
			ii++;
		}
		temp_count=1;
		USART_SendData(USART1,9);
		}
	}
}
