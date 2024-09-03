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
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //�ж����ȼ����� ��2��
	LED_Init();
	USART1_Init(115200);
	TFTLCD_Init();			//LCD��ʼ��
	EN25QXX_Init();				//��ʼ��EN25Q128	  
	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
	
	
	for(i=0;i<32;i++){
		offset = my_mem_malloc(0, sizeof(struct receive_info));
		if(offset == 0XFFFFFFFF){printf("malloc error\r\n");}
		object[i] = (struct receive_info*)(malloc_cortol.membase[0] + offset);
		object[i]->x = 0;
		object[i]->y = 0;
		object[i]->flag = 0;
	}
	printf("here is ok\r\n");
	FRONT_COLOR=RED;//��������Ϊ��ɫ 
//	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN STM32F1");	
//	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"Fatfs Test");	
//	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	  res = f_mount(&fs, "0:", 1);  
		//LCD_ShowxNum(100,190,res,6,16,0);
    if(res==0X0D){
			printf("create a new: %d\r\n", res);
			LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"create a new");
			res = f_mkfs("0:", 1, 4096); // ��һ���������߼��������ţ��ڶ��������Ƿ������򣬵����������Ƿ��䵥Ԫ��С
			if (res != FR_OK) {
					printf("Format Error: %d\r\n", res);
			} else {
					printf("Format completed successfully.\r\n");
			}
		}
		//����ͼͼƬ��flash��ǰ12M��Χ����д��12M��ʼ��ַ�������Ϳ��Բ�ͨ��fatfs��������
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
        // ��ȡ�ļ�����
				for(i=0;i<bmp_files.info_header.height;i++){//printf("i is %d\r\n",i);
					f_read(&bmp_files.fp, buff, bmp_files.info_header.width*2, &br);
					EN25QXX_Write((u8*)buff,base,bmp_files.info_header.width*2);
					base+=bmp_files.info_header.width*2;
				}
    }
		jj=0;
		
		//Ϊ�ӵ������ڴ�
		get_bmp_header("game/bullet/1.bmp",&bmp_files,0);
		offset = my_mem_malloc(0, (bmp_files.info_header.width) * bmp_files.info_header.height * sizeof(uint16_t));
		bullet_buffer1 = (u16 *)(malloc_cortol.membase[0] + offset);
		f_lseek (&bmp_files.fp, 54);
		buffer_count = 0;
		for(i=0;i<bmp_files.info_header.height;i++){ //��ȡ�ӵ�ͼƬ���뻺��
			f_read(&bmp_files.fp, buff, bmp_files.info_header.width * sizeof(uint16_t), &br);
			for(j=0;j<bmp_files.info_header.width;j++){
				//if(buff[j]==0xFFFF){continue;}
				*(bullet_buffer1 + buffer_count + j) = buff[j];
			}
			buffer_count+=bmp_files.info_header.width;
		}
		//������һ��
		offset = my_mem_malloc(0, (bmp_files.info_header.width +10) * bmp_files.info_header.height * sizeof(uint16_t));
		bullet_buffer2 = (u16 *)(malloc_cortol.membase[0] + offset);
		//�����ӵ��Ŀ�͸�
		bullet_width = bmp_files.info_header.width+10;
		bullet_height = bmp_files.info_header.height;
		//Ϊ��ʬͼƬ�����ڴ�
		get_bmp_header("game/type/zombies/1.bmp",&bmp_files,0);
		offset = my_mem_malloc(0, (bmp_files.info_header.width+2) * bmp_files.info_header.height * sizeof(uint16_t));
		zombies_buffer = (u16 *)(malloc_cortol.membase[0] + offset);
		zombies_width = bmp_files.info_header.width +2; //����2����ʵ�ʵ���
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
