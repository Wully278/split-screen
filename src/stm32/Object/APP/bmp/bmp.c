
#include "bmp.h"
#include "usart.h"
#include "malloc.h"
#include "tftlcd.h"
struct bmp_file bmp_files;

int get_bmp_header(char* pathname,struct bmp_file* file,int argv){
	u8 res=0;
	char buff[32];
	UINT br =0;
	res = f_open(&file->fp, pathname, FA_OPEN_ALWAYS | FA_READ);
	if(res != FR_OK) {
      printf("Open Error: %d\r\n", res);
			return 1;
    }else{
        // 读取文件内容
        if(f_read(&file->fp, &file->file_header, 14, &br)){
					printf("error:get file_header\r\n");
					printf("error:get file_header %d\r\n",br);
					return 1;
				};
				if(f_read(&file->fp,&file->info_header,40,&br)){
					printf("error:get info_heade\r\n");
					return 1;
				}
        // 根据参数决定是否关闭文件
				if(argv==1)
        f_close(&file->fp);
    }
		return 0;
}

//uint16_t convert_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
//    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
//}

//int create_directories(char *path) {
//    char temp_path[256];
//		char *sep;
//		FRESULT res;
//    strncpy(temp_path, path, sizeof(temp_path) - 1);
//    temp_path[sizeof(temp_path) - 1] = '\0'; // 确保字符串以NULL结尾

//    sep = strchr(temp_path, '/');
//    

//    while (sep) {
//        *sep = '\0';
//        res = f_mkdir(temp_path);
//        if (res != FR_OK && res != FR_EXIST) {
//            return res;
//        }
//        *sep = '/';
//        sep = strchr(sep + 1, '/');
//    }
//    return FR_OK;
//}

//int convert_bmp24_to_rgb565(char* src_path, char* dst_path) {
//    FRESULT res;
//    FIL src_file, dst_file;
//    UINT br, bw;
//    uint8_t *buffer24;
//    uint16_t *buffer565;
//    int row_size, padding;
//    int i, j;
//		u32 buffer24_offset;
//    u32 buffer565_offset;
//		uint8_t b;
//    uint8_t g;
//    uint8_t r;
//	
//	FILINFO fno;
//    // 打开源文件
//    res = f_open(&src_file, src_path, FA_READ);
//    if (res != FR_OK) {
//        printf("Failed to open source file %s: %d\r\n", src_path, res);
//        return res;
//    }
//		
//		// 创建目标文件的目录
//		res = create_directories(dst_path);
//    if (res != FR_OK && res != FR_EXIST) {
//        printf("Failed to create directories for %s: %d\r\n", dst_path, res);
//        f_close(&src_file);
//        return res;
//    }
//		
//    // 打开目标文件
//    res = f_open(&dst_file, dst_path, FA_WRITE | FA_CREATE_ALWAYS);
//    if (res != FR_OK) {
//        printf("Failed to open destination file %s: %d\r\n", dst_path, res);
//        f_close(&src_file);
//        return res;
//    }
//		
//    // 读取BMP头文件
//    res = get_bmp_header(src_path, &bmp_files,0);
//    if (res != 0) {
//        f_close(&src_file);
//        f_close(&dst_file);
//        return res;
//    }
//		
//    // 写入修改后的头文件到目标文件
//    bmp_files.file_header.size = bmp_files.file_header.size - bmp_files.info_header.size_image + (bmp_files.info_header.width * bmp_files.info_header.height * 2);
//    bmp_files.info_header.bit_count = 16;
//    bmp_files.info_header.size_image = bmp_files.info_header.width * bmp_files.info_header.height * 2;
//    f_write(&dst_file, &bmp_files.file_header, sizeof(bmp_file_header_t), &bw);
//		printf("bw is %d\r\n",bw);
//    f_write(&dst_file, &bmp_files.info_header, sizeof(bmp_info_header_t), &bw);
//		printf("bw is %d\r\n",bw);
//    // 分配缓冲区
//    row_size = (bmp_files.info_header.width * 3 + 3) & ~3;
//    padding = row_size - bmp_files.info_header.width * 3;
//    buffer24_offset = my_mem_malloc(0, row_size);
//		buffer565_offset = my_mem_malloc(0, bmp_files.info_header.width * sizeof(uint16_t));
//    if (buffer24_offset == 0xFFFFFFFF || buffer565_offset == 0xFFFFFFFF) {
//        printf("Failed to allocate memory\n");
//        f_close(&src_file);
//        f_close(&dst_file);
//        if (buffer24_offset != 0xFFFFFFFF) my_mem_free(0, buffer24_offset);
//        if (buffer565_offset != 0xFFFFFFFF) my_mem_free(0, buffer565_offset);
//        return FR_NOT_ENOUGH_CORE;
//    }
//		buffer24 = malloc_cortol.membase[0] + buffer24_offset;
//		printf("buffer24 is %x\r\n",buffer24);
//		buffer565 = (u16 *)(malloc_cortol.membase[0] + buffer565_offset);
//		printf("buffer565 is %x\r\n",buffer565);
//    // 将每一行从24位转换为16位RGB565
//		f_lseek (&src_file, 54); //把文件头跳过
//    for (i = 0; i < bmp_files.info_header.height; i++) {
//        f_read(&src_file, buffer24, row_size, &br);
//			//printf("br is %d\r\n",br);
//        for (j = 0; j < bmp_files.info_header.width; j++) {
//            b = buffer24[j * 3];
//            g = buffer24[j * 3 + 1];
//            r = buffer24[j * 3 + 2];
//            buffer565[j] = convert_to_rgb565(r, g, b);
//        }
//        f_write(&dst_file, buffer565, bmp_files.info_header.width * sizeof(uint16_t), &bw);
//    }
//    // 释放缓冲区
//    my_mem_free(0, buffer24_offset);
//    my_mem_free(0, buffer565_offset);
//		printf("here is ok\r\n");
//    f_close(&src_file);
//    f_close(&dst_file);
//		printf("here1 is ok\r\n");
//    return FR_OK;
//}

void bmp_show(int x0,int y0,char* src_path){
		u32 offset;
		uint16_t *buffer565;
		UINT br =0;
		u16 temp = 0;
		long tmp=0;
		u16 i,j;
		int x,y;
		x = 320 -x0;
		y = 480 -y0;
		get_bmp_header(src_path,&bmp_files,0);
		offset = my_mem_malloc(0, bmp_files.info_header.width * sizeof(uint16_t));
		buffer565 = (u16 *)(malloc_cortol.membase[0] + offset);
		f_lseek (&bmp_files.fp, 54);
		//f_read(&bmp_files.fp, buffer565, bmp_files.info_header.width * bmp_files.info_header.height * sizeof(uint16_t), &br);
		
	
	
	//LCD_Set_Window(x, y, x+bmp_files.info_header.width-1, y+bmp_files.info_header.height-1);
	for(i=bmp_files.info_header.height;i>0;i--)
	{
		f_read(&bmp_files.fp, buffer565, bmp_files.info_header.width * sizeof(uint16_t), &br);
		for(j=bmp_files.info_header.width;j>0;j--)
		{
			temp = buffer565[bmp_files.info_header.width - tmp-1];
			if(temp==0xFFFF){tmp ++;continue;}
			LCD_DrawFRONT_COLOR(x-j,y-i,temp);
			tmp ++;
		}
		tmp = 0;
	}
		my_mem_free(0, offset);
	f_close(&bmp_files.fp);
}

void bmp_show_flush(int x0,int y0,char* src_path,char* map_path,int flag,int type){
		u16 i,j;
		u16 buff[320];
		u16* buff1;
		int base;
		UINT br =0;
		int lseek_temp=0;
		int span;
		int tempx;
		int buffer_count=0;
		
		if(type==1){//僵尸
			get_bmp_header(map_path,&bmp_files,0);
			f_close(&bmp_files.fp);
			lseek_temp = (bmp_files.info_header.height-y0 -zombies_height)*bmp_files.info_header.width*2;
			span = bmp_files.info_header.width*2;
			lseek_temp -= (span -2*x0);
			buffer_count = 0;
			base = 512*2048*12;
			for(i=0;i<zombies_height;i++){
				lseek_temp+=span;
				EN25QXX_Read(zombies_buffer + buffer_count,base+lseek_temp,zombies_width * sizeof(uint16_t));
				buffer_count+=zombies_width;
			}
			if(x0<10){flag=0;}//僵尸到了边界，手动删除
			get_bmp_header(src_path,&bmp_files,0);
			if(flag==1){ //如果不止显示地图，还要显示僵尸
				f_lseek (&bmp_files.fp, 54);
				buffer_count = 0;
				for(i=0;i<bmp_files.info_header.height;i++){ //读取僵尸图片放入缓存
					f_read(&bmp_files.fp, buff, bmp_files.info_header.width * sizeof(uint16_t), &br);
					for(j=0;j<bmp_files.info_header.width;j++){
						tempx = buff[j];
						if(tempx==0xFFFF){continue;}
						*(zombies_buffer + buffer_count + j) = tempx;
					}
					buffer_count+=zombies_width;
				}
				
			}
			f_close(&bmp_files.fp);
			buffer_count = (bmp_files.info_header.height -1)*zombies_width;
			for(i=0;i<bmp_files.info_header.height;i++){
				LCD_Set_Window(320 - x0-zombies_width, 480 -y0-i-1,320 - x0-1, 480 -y0-i-1);
				for(j=zombies_width-1;j>0;j--){
					LCD_WriteData_Color(zombies_buffer[buffer_count + j]);
				}
				buffer_count -= zombies_width;
			}
		}else if(type==0){//子弹
			y0+=36;
			x0-=16;
			get_bmp_header(map_path,&bmp_files,0);
			f_close(&bmp_files.fp);
			lseek_temp = (bmp_files.info_header.height-y0 -bullet_height)*bmp_files.info_header.width*2;
			span = bmp_files.info_header.width*2;
			lseek_temp -= (span -2*x0);
			buffer_count = 0;
			base = 512*2048*12;
			for(i=0;i<bullet_height;i++){
				lseek_temp+=span;
				EN25QXX_Read(bullet_buffer2 + buffer_count,base+lseek_temp,bullet_width * sizeof(uint16_t));
				buffer_count+=bullet_width;
			}
			if(flag==1){ //如果不止显示地图，还要显示子弹
				buffer_count = 0;
				for(i=0;i<bullet_height;i++){ //读取子弹图片放入缓存
					buff1 = bullet_buffer1 + i*(bullet_width-10);
					for(j=10;j<bullet_width;j++){ //这里2和前面对应
						tempx = *(buff1+j-10);
						if(tempx==0xFFFF){continue;}
						*(bullet_buffer2 + buffer_count + j) = tempx;
					}
					buffer_count+=bullet_width;
				}
				//f_close(&bmp_files.fp);
			}
			buffer_count = (bullet_height -1)*bullet_width;
			for(i=0;i<bullet_height;i++){
				LCD_Set_Window(320 - x0-bullet_width, 480 -y0-i-1,320 - x0-1, 480 -y0-i-1);
				for(j=bullet_width-1;j>0;j--){
					LCD_WriteData_Color(bullet_buffer2[buffer_count + j]);
				}
				buffer_count -= bullet_width;
			}
		}
}