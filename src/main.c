#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

#include <tslib.h>
#include <linux/input.h>
#include "event.h"
// #define maxbmpfile 64

struct fb_var_screeninfo var; //屏幕信息
unsigned int line_width;
unsigned int pixel_width;
int screen_size;

unsigned char * framebuffer; //帧缓存区
unsigned char * backbuffer; //处理缓存区
unsigned char * windowsbuffer; //窗口缓存区

struct tsdev *ts;
struct input_absinfo slot; //插槽结构体，就是同时触摸点的结构体，每个触点一个结构体
int max_slots; //最大触摸点的个数
struct ts_sample_mt **samp_mt; //触摸点信息结构数组
struct ts_sample samp;

struct key key1 = {40,100,80,50,0}; //定义4个按钮
struct key key2 = {40,200,80,50,0};
struct key key3 = {40,300,80,50,0};
struct key key4 = {40,400,80,50,0};
struct key key5 = {40,500,80,50,0};
int touchflag = 0; //触摸模式标志位
int communicateflag = 0; //主板与stm32串口通信标志位
struct bmp_file bmp_files[maxbmpfile];
struct graph_block* start_graph_block = NULL;//初始块地址
//获取bmp图片的头文件，pathname是文件路径例如"./2.bmp"，file_header和info_header是头文件内容指针
int get_bmp_header(char* pathname,struct bmp_file* file){
	file->fb = open(pathname,O_RDWR);
	if(file->fb < 0){
		printf("can't open the %s\n",pathname);
		return 0;
	};
	if(!read(file->fb,&file->file_header,14)){
		printf("error:get file_header\n");
		return 0;
	}
	if(!read(file->fb,&file->info_header,40)){
		printf("error:get info_heade\n");
		return 0;
	}
    //printf("get_bmp_header sucessfully\n");
    return 1;
}
//打开屏幕文件，将屏幕映射到内存，参数var存放屏幕参数，fb存放屏幕文件号，frame_buffer映射的内存首地址,back_buffer为缓冲地址
int screen_init(int* fb,unsigned char** frame_buffer,unsigned char** back_buffer,unsigned char** windows_buffer){
    *fb = open("/dev/fb0", O_RDWR); //这里屏幕设备为fb0
    
    //printf("screen_size %d\n",screen_size);
    
    if (*fb < 0){
		printf("can't open /dev/fb0\n");
		return 0;
	}
    if (ioctl(*fb, FBIOGET_VSCREENINFO, &var)){
		printf("can't get var\n");
		return 0;
	}
    screen_size = var.xres * var.yres * var.bits_per_pixel / 8;//printf("screen_size %d\n",screen_size);
    line_width  = var.xres * var.bits_per_pixel / 8;
	pixel_width = var.bits_per_pixel / 8;
    *frame_buffer = (unsigned char *)mmap(NULL , screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, *fb, 0);
    if (*frame_buffer == (unsigned char *)-1){
		printf("can't mmap\n");
		return 0;
	}
    *back_buffer = malloc(screen_size);
    *windows_buffer = malloc(screen_size);
    memset(*frame_buffer, 0xff, screen_size);
    //printf("now screen_size %d\n",screen_size);
    
    return 1;
}
//在x,y坐标处显示一张图片，file是该图片的信息。frame_buffer和back_buffer分别是缓冲帧和后台缓存,参数argv选择在显示图片时是否全屏刷为白色
void lcd_show_image(int x, int y, struct bmp_file* file, unsigned char *frame_buffer, unsigned char *back_buffer, int argv){
    char buff[4];
    unsigned char *temp_buffer;
    int ii=0;
    unsigned int *temp;
	temp_buffer = malloc(file->file_header.size - file->file_header.off_bits);
    read(file->fb,temp_buffer,file->file_header.size - file->file_header.off_bits);
    lseek(file->fb,54,SEEK_SET);
    if(argv == 1){ //是否刷屏
        memset(back_buffer, 0xff, screen_size);
    }
    for(int i = file->info_header.height;i>0;i--){
		for(int j=0;j<file->info_header.width;j++){
			buff[0] = *(temp_buffer+ii);
			buff[1] = *(temp_buffer+ii+1);
			buff[2] = *(temp_buffer+ii+2);
			temp = back_buffer+(y+i)*line_width+(x+j)*pixel_width;
			*temp = ((*(buff+2))<<16 | (*(buff+1))<<8 | (*(buff)));
			ii+=3;
		}
		if(file->info_header.width%4!=0){
			ii+= 4 - file->info_header.width*3%4;
		}
	}
    memcpy(frame_buffer, back_buffer, screen_size);
    free(temp_buffer);
}
//展示一条线段，两点表示一条线段
void lcd_show_line(int x0,int y0,int x1,int y1,unsigned char *frame_buffer){
    unsigned int *temp;
    int x;
    int y;
    for(int i=0;i<var.yres;i++){
        x = (i-y0)*(x1-x0)/(y1-y0)+x0;
        if(x>=line_width/4)continue;
        //printf("x is %d\n",x);
        y = i;
        temp = frame_buffer+(y)*line_width+(x)*pixel_width;
        *temp = 0xff0000;
    }
}
void show_key(struct key* key,unsigned char *back_buffer){ //显示一个按钮
    unsigned int *temp;
    int xend = key->x + key->width/2;
    int yend = key->y + key->height/2;
    int color;
    if(key->select ==0){
        color = 0xff0000;
    }else{
        color = 0x00ff00;
    }
    for(int i= key->y - key->height/2;i<yend;i++){
        for(int j= key->x - key->width/2;j<xend;j++){
            temp = back_buffer+i*line_width+j*pixel_width;
            *temp = color; //红色
        }
    }
}
void show_keys(unsigned char *back_buffer){ //显示一组按钮
    show_key(&key1,back_buffer);
    show_key(&key2,back_buffer);
    show_key(&key3,back_buffer);
    show_key(&key4,back_buffer);
    show_key(&key5,back_buffer);
}
//改进后的显示图像块函数graph_block是图形块，back_buffer是图形缓存区，其余参数实际未用到
void lcd_show_block_plus(struct graph_block* graph_block, unsigned char *frame_buffer, unsigned char *back_buffer, int argv){
    unsigned int *temp;
    unsigned int *temp1;
    int ystart = *(graph_block->points);
    int yspan = *(graph_block->points + 1);
    int ycount = ystart * 2 + 2;
    int xstart, xspan, x, y;
    int ytemp = graph_block->y + graph_block->yscale * ystart / 1000;
    int var_yres = var.yres;
    int var_xres = var.xres;
    int line_width1 = line_width; // set this value to the appropriate width of a line in your buffer
    int pixel_width1 = pixel_width; // set this value to the appropriate width of a pixel in your buffer
    int xspan_dest,xspan_dest_count;
    int remain0,remain1;
    int compare;
    for (int i = 0; i < yspan; i++) {
        xstart = *(graph_block->points + ycount);
        xspan = *(graph_block->points + ycount + 1);
        xspan_dest = xspan*graph_block->xscale/1000;
        y = graph_block->y + graph_block->yscale * (ycount / 2 - 1) / 1000;
        if(y<0){ytemp = y;ycount += 2;continue;}
        if(y>=var_yres){break;}
        temp = graph_block->windows_buffer + (ycount / 2 - 1) * line_width1 + xstart*pixel_width1;
        x = graph_block->x + graph_block->xscale * xstart / 1000;
        for (int k1 = 0; k1 < y - ytemp; k1++) {
            temp1 = (unsigned int *)(back_buffer + (y - k1) * line_width1 + x * pixel_width1);
            if(x+xspan_dest>=var_xres){
                xspan_dest_count = var_xres - x;
            }else{
                xspan_dest_count = xspan_dest;
            }
            remain0 = xspan_dest*1000;
            remain1 = remain0;
            for (int j = 0; j < xspan_dest_count; j++) {
                compare = remain0 - remain1;
                if(compare>=0 && compare <1){
                    *(temp1 + xspan_dest - remain0/1000)=*(temp + xspan - remain1/graph_block->xscale);
                    remain0-=1000;
                    remain1-=graph_block->xscale;
                }else if(compare>=1){
                    *(temp1 + xspan_dest - remain0/1000)=*(temp + xspan - remain1/graph_block->xscale);
                    remain0-=1000;
                }else{
                    remain1-=graph_block->xscale;
                    j--;
                }
            }
        }
        ytemp = y;
        ycount += 2;
    }
}
//创建一个图形块
struct graph_block* make_graph(unsigned char* windows_buffer,int x,int y,uint16_t xscale,uint16_t yscale,uint8_t pid,struct graph_block* prev){
    struct graph_block* graph_block = (struct graph_block*)malloc(sizeof(struct graph_block));
    struct graph_block* temp_graph_block;
    int* points = malloc(((int)(var.yres)*2+4)*4);
    for(int i=0;i<var.yres*2+4;i++){
        *(points+i) = -1;
    }
    graph_block->windows_buffer = windows_buffer;
    graph_block->xscale = xscale;
    graph_block->yscale = yscale;
    graph_block->x = x;
    graph_block->y = y;
    graph_block->pid = pid;
    //graph_block->size = (int)var.xres * (int)var.yres;
    graph_block->points = points;
    //printf("make here is break%d %d\n",*(graph_block->points),*(graph_block->points+1));
    if(start_graph_block == NULL){
        start_graph_block = graph_block;
        //object->pid = 1;
        graph_block->prev = NULL;
    }else{
        temp_graph_block = start_graph_block;
        while(temp_graph_block->next){
            temp_graph_block = temp_graph_block->next;
        }
        temp_graph_block->next = graph_block;
        graph_block->prev = temp_graph_block;
    }
    graph_block->next = NULL;
    return graph_block;
}
//销毁一个图形块
int free_graph(struct graph_block* graph_block){
    if(graph_block->prev == NULL){
        if(graph_block->next ==NULL){
            start_graph_block = NULL;
            free(graph_block->points);
            free(graph_block);
            return;
        }else{
            start_graph_block = graph_block->next;
            start_graph_block->prev =NULL;
            free(graph_block->points);
            free(graph_block);
            return;
        }
    }else{
        if(graph_block->next == NULL){
            graph_block->prev->next = NULL;
            free(graph_block->points);
            free(graph_block);
            return;
        }else{
            graph_block->prev->next = graph_block->next;
            graph_block->next->prev = graph_block->prev;
            free(graph_block->points);
            free(graph_block);
            return;
        }
    }
    return 1;
}
//注册一个线
struct line* make_line(int x0,int y0,int x1,int y1){
    struct line* line = (struct line*)malloc(sizeof(struct line));
    int temp;
    if(y1>=y0){}else{temp = y1;y1 = y0;y0 = temp;}
    line->x0 = x0;
    line->y0 = y0;
    line->x1 = x1;
    line->y1 = y1;
    return line;
}
//销毁一个线
int free_line(struct line* line){
    free(line);
    return 1;
}
//线将图形块分割
int line_chap_block(struct graph_block* graph, struct line* line){
    int flag=1;
    struct graph_block* graph1;
    struct graph_block* graph2;
    int count_line=0;
    int count_graph=0;
    int count_graph1=0;
    int count_graph2=0;
    int temp;
    int xtemp;
    int yspan;
    int xspan;
    int xstart;
    int flag1,flag2;
    int location_x,location_y;
    int x0 = (line->x0 - graph->x)*1000/graph->xscale; //将线段的两个点坐标转换为windowsbuffer相应点位
    int x1 = (line->x1 - graph->x)*1000/graph->xscale;
    int y0 = (line->y0 - graph->y)*1000/graph->yscale;
    int y1 = (line->y1 - graph->y)*1000/graph->yscale;
    int x,y;
    
    if(y1<*(graph->points) || y0>(*(graph->points)+*(graph->points+1)))return 0;
    for(int i=0;i<y1-y0 && flag;i++){ //辨别线段和图形块是否相交
        x = i*(x1-x0)/(y1-y0)+x0;
        y = i + y0;
        xtemp = x - *(graph->points + 2 + 2*y);
        if(xtemp>0 && *(graph->points+2+2*y+1)>xtemp){
            flag =0;
            break;
        }
    }
    //printf("here is break%d %d\n",*(graph->points),*(graph->points+1));
    if(flag==1){ //如果没有相交
        return 0;
    }
    graph1 = make_graph(windowsbuffer,0,0,1000,1000,0,NULL);
    graph2 = make_graph(windowsbuffer,0,0,1000,1000,0,NULL);
    graph1->x = graph->x;
    graph2->x = graph->x;
    graph1->y = graph->y;
    graph2->y = graph->y;
    graph1->xscale = graph->xscale;
    graph1->yscale = graph->yscale;
    graph2->xscale = graph->xscale;
    graph2->yscale = graph->yscale;
    y = *(graph->points);
    yspan = *(graph->points+1);
    for(int i=0;i<yspan;i++){
        x = (y - y0)*(x1-x0)/(y1-y0) + x0;
        xstart = *(graph->points + 2 + 2*y);
        xspan = *(graph->points + 2 + 2*y +1);
        xtemp = x - xstart;
        //printf("now xtemp is %d\n",xtemp);
        if(xtemp<=0){
            *(graph2->points +2+2*y) = xstart;
            *(graph2->points +2+2*y +1) = xspan;
        }else if(xtemp>0&&xtemp<xspan){
            *(graph1->points +2+2*y) = xstart;//printf("now y is %d\n",y);
            *(graph1->points +2+2*y +1) = xtemp;
            *(graph2->points +2+2*y) = x;
            *(graph2->points +2+2*y +1) = xspan - xtemp;
        }else{
            *(graph1->points +2+2*y) = xstart;
            *(graph1->points +2+2*y +1) = xspan;//printf("now y is \n",y);
        }
        y++;
    }
    temp =2;
    while(*(graph1->points +temp)==-1){
        temp+=2;
    }
    *(graph1->points) = (temp-2)/2;
    //printf("the *(graph1->points) is %d\n",*(graph1->points));
    while(*(graph1->points +temp)!=-1){
        temp+=2;
    }
    *(graph1->points +1) = (temp-2)/2 - *(graph1->points);
    temp =2;
    while(*(graph2->points +temp)==-1){
        temp+=2;
    }
    *(graph2->points) = (temp-2)/2;
    while(*(graph2->points +temp)!=-1){
        temp+=2;
    }
    *(graph2->points +1) = (temp-2)/2 - *(graph2->points);
}
int ts_init(){
    ts = ts_setup(NULL, 1);
    if (!ts)
	{
		printf("ts_setup err\n");
		return -1;
	}
    if (ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
		perror("ioctl EVIOGABS");
		ts_close(ts);
		return -1;
	}
    max_slots = slot.maximum + 1 - slot.minimum;

	samp_mt = malloc(sizeof(struct ts_sample_mt *));
	if (!samp_mt) {
		ts_close(ts);
		return -1;
	}
	samp_mt[0] = calloc(max_slots, sizeof(struct ts_sample_mt)); //到这里相当于用samp_mt[0][i]可以寻址到全部的max_slots个触点结构体
	if (!samp_mt[0]) {
		free(samp_mt);
		ts_close(ts);
		return -1;
	}
}
int x_screen_to_game; //全局变量用于储存触摸点的位置
int y_screen_to_game;
int presure_screen_to_game;
//运行一帧游戏
void game_run_once(){
    static int x;
    static int y;
    static int valid;
    static struct object* tempobject;
    static struct bullet* tempbullet;
    static int flag;
    static int xoffeset,yoffeset;
    static int xrecord,yrecord;
    static int count=0;
        change_perframe(bulletstart,plantsobject,zombiesobject);
        memcpy(windowsbuffer, BGBbuffer, screen_size);

        if(plantsobject!=NULL){
            tempobject = plantsobject;
            while (tempobject!=NULL){
                show_object(tempobject);
                tempobject = tempobject->next;
            }
        }
        if(zombiesobject!=NULL){
            tempobject = zombiesobject;
            while (tempobject!=NULL){
                show_object(tempobject);
                tempobject = tempobject->next;
            }
        }
        if(bulletstart!=NULL){
            tempbullet = bulletstart;
            while (tempbullet!=NULL){
                show_bullet(tempbullet);
                tempbullet = tempbullet->next;
            }
        }
        if(touchflag == 1){
            x = x_screen_to_game;
            y = y_screen_to_game;
            valid = presure_screen_to_game;
        }else{
            valid = 0;
        }
        if(valid && !selectobject){//有触点但还没有块被选择
            flag =1;
            tempobject=barobject;
            while(tempobject){ //检查每个图标是否被选择
                if((x>tempobject->x) && (x<tempobject->x + 75) && (y>tempobject->y) && (y<tempobject->y + 95)){ //这里的偏移需要调整。是图标的宽高
                    flag = 0;
                }
                if(flag == 0){
                    selectobject = create_object(tempobject->x,tempobject->y,4,tempobject->type,NULL);
                    xoffeset = x - selectobject->x; //记录下偏移
                    yoffeset = y - selectobject->y;
                    break;}
                tempobject = tempobject->next;
            }
        }else if(valid && selectobject){ //有新触点，但还是原来的图形块被选择
            selectobject->x=x -xoffeset; 
            selectobject->y=y -yoffeset;
            xrecord = selectobject->x ;
            yrecord = selectobject->y ;
        }else{//没有图形块被选择
            if(selectobject!=NULL){
                xrecord = xrecord + grasswidthpixel/2 -grassstart_x; //这里加grasswidthpixel/2是为了让选择的点更偏块中央，可调整或去掉
                yrecord = yrecord + grassheigthpixel/2 -grassstart_y;
                
                if(xrecord >0 && yrecord >0){
                    xrecord = xrecord/grasswidthpixel;
                    yrecord = yrecord/grassheigthpixel;
                    xrecord = grassstart_x + xrecord*grasswidthpixel;
                    yrecord = grassstart_y + yrecord*grassheigthpixel;
                    tempobject = plantsobject;
                    flag = 1;
                    while(tempobject){
                        if(tempobject->x == xrecord && tempobject->y == yrecord){
                            flag = 0;
                            break;
                        }
                        tempobject = tempobject->next;
                    }
                    if(flag){ //放置植物
                        count = 0; //这里和下面一样，都是判断selectobject的名字是否是铲子，是的话把flag标记为0
                        flag = 0;
                        while (*(selectobject->typename +count)){
                            if(shovelname[count]  != *(selectobject->typename +count)){
                                flag=1;
                            }
                            count++;
                        }
                        if(flag)create_object(xrecord,yrecord,1,selectobject->type,NULL);
                    }else{
                        count = 0;
                        flag = 0;
                        while (*(selectobject->typename +count)){
                            if(shovelname[count]  != *(selectobject->typename +count)){
                                flag=1;
                            }
                            count++;
                        }
                        if(flag ==0){
                            delete_object(tempobject);
                        }
                    }
                }
                delete_object(selectobject);
                selectobject = NULL;
            }
        }
        if(selectobject!=NULL)show_object(selectobject);
}
//主运行程序
void event_run(int mode){ //mode模式，1平移块，2缩放块，3与次板通信，4分割块，5游戏交互
    struct graph_block* graph_block = make_graph(windowsbuffer,0,0,1000,1000,0,NULL); //先对最初块进行初始化
    struct graph_block* graph_temp;
    struct graph_block* graph_select = NULL; //切换模式需重置
    struct line* line;

    int block_offeset_x,block_offeset_y;
    int x0,y0,x1,y1;
    int count;
    int abs_x0,abs_x1;
    int temp_x0=0;
    int temp_x1=0; //用来储存上一次两个触点的x坐标，避免相邻很近的两个点被当作两个输入

    int gap=0;

    int used = 0;
    *(graph_block->points) = 0;
    *(graph_block->points +1) = 600;
    for(int i=0;i<600;i++){
        *(graph_block->points +i*2 +2) = 0;
        *(graph_block->points +1 +i*2 +2) = 1024;
    }

    while(1){
        
        if(mode==1){
            touchflag = 0; //除了模式3和5之外，这两个标识都要清零
            communicateflag = 0;
            game_run_once();
            for(int i=0;i<5;i++)ts_read_mt(ts, samp_mt, max_slots, 1);
            if(samp_mt[0][0].pressure && !graph_select){//有触点但还没有块被选择
                graph_temp =start_graph_block;
                while(graph_temp){
                    x0 = (samp_mt[0][0].x - graph_temp->x)*1000/graph_temp->xscale; //将触点坐标映射到原windowsbuffer的坐标
                    y0 = (samp_mt[0][0].y - graph_temp->y)*1000/graph_temp->yscale;
                    if(*(graph_temp->points + 2+y0*2)!=-1){ //判断触点是否在该图形块内
                        if(*(graph_temp->points + 2+y0*2)<=x0&&*(graph_temp->points + 2+y0*2+1)>=(x0-*(graph_temp->points + 2+y0*2))){
                            graph_select = graph_temp;
                            block_offeset_x = samp_mt[0][0].x - graph_select->x; //记录下偏移
                            block_offeset_y = samp_mt[0][0].y - graph_select->y;
                            break;
                        }
                    }
                    graph_temp = graph_temp->next;
                }
            }else if(samp_mt[0][0].pressure && graph_select){//有新触点，但还是原来的图形块被选择
                graph_select->x=samp_mt[0][0].x -block_offeset_x;
                graph_select->y=samp_mt[0][0].y -block_offeset_y;
            }else{//没有图形块被选择
                graph_select = NULL;
            }
            graph_temp =start_graph_block; //每次重回项首
            memset(backbuffer, 0xff, screen_size); //刷屏白色
            while(graph_temp){
                lcd_show_block_plus(graph_temp, framebuffer, backbuffer,1); //把每一个图形块的内容写进backbuffer
                graph_temp = graph_temp->next;
            }
            show_keys(backbuffer);
            memcpy(framebuffer, backbuffer, screen_size); //刷屏
        }else if(mode==2){//printf("mode is 2\n");
            touchflag = 0;
            communicateflag = 0;
            game_run_once();
            for(int i=0;i<6;i++)ts_read_mt(ts, samp_mt, max_slots, 1);//printf("samp_mt[0][0].pressure is %d samp_mt[0][1].pressure is %d\n",samp_mt[0][0].pressure,samp_mt[0][1].pressure);
            if(samp_mt[0][0].pressure&&samp_mt[0][1].pressure&&!graph_select){//printf("2 now is click\n");
                graph_temp =start_graph_block;
                while(graph_temp){
                    x0 = (samp_mt[0][0].x - graph_temp->x)*1000/graph_temp->xscale; //将触点坐标映射到原windowsbuffer的坐标
                    y0 = (samp_mt[0][0].y - graph_temp->y)*1000/graph_temp->yscale;
                    x1 = (samp_mt[0][1].x - graph_temp->x)*1000/graph_temp->xscale; //将触点坐标映射到原windowsbuffer的坐标
                    y1 = (samp_mt[0][1].y - graph_temp->y)*1000/graph_temp->yscale;
                    if((*(graph_temp->points + 2+y0*2)!=-1)&&(*(graph_temp->points + 2+y1*2)!=-1)){
                        if(*(graph_temp->points + 2+y0*2)<=x0 && *(graph_temp->points + 2+y0*2+1)>=(x0-*(graph_temp->points + 2+y0*2)) \
                        && *(graph_temp->points + 2+y1*2)<=x1 && *(graph_temp->points + 2+y1*2+1)>=(x1-*(graph_temp->points + 2+y1*2))){
                            graph_select = graph_temp;
                            break;
                        }
                    }
                    graph_temp = graph_temp->next;
                }
            }else if(samp_mt[0][0].pressure&&samp_mt[0][1].pressure&&graph_select){
                graph_select->x = (x1*samp_mt[0][0].x - x0*samp_mt[0][1].x)/(x1 - x0);
                graph_select->y = (y1*samp_mt[0][0].y - y0*samp_mt[0][1].y)/(y1 - y0);
                graph_select->xscale = (samp_mt[0][1].x - samp_mt[0][0].x)*1000/(x1 - x0);
                graph_select->yscale = (samp_mt[0][1].y - samp_mt[0][0].y)*1000/(y1 - y0);
            }else{
                graph_select = NULL;
            }
            graph_temp =start_graph_block; //每次重回项首
            memset(backbuffer, 0xff, screen_size); //刷屏白色
            while(graph_temp){
                lcd_show_block_plus(graph_temp, framebuffer, backbuffer,1); //把每一个图形块的内容写进backbuffer
                graph_temp = graph_temp->next;
            }
            show_keys(backbuffer);
            memcpy(framebuffer, backbuffer, screen_size); //刷屏
        }else if(mode==3){
            communicateflag = 1;
            touchflag = 1;
            char file = 0;

            for(int i=0;i<6;i++)ts_read_mt(ts, samp_mt, max_slots, 1);
            if(samp_mt[0][0].pressure){//printf("2 now is click\n");
                graph_temp =start_graph_block;
                while(graph_temp){
                    x0 = (samp_mt[0][0].x - graph_temp->x)*1000/graph_temp->xscale; //将触点坐标映射到原windowsbuffer的坐标
                    y0 = (samp_mt[0][0].y - graph_temp->y)*1000/graph_temp->yscale;
                    if((*(graph_temp->points + 2+y0*2)!=-1)&&(*(graph_temp->points + 2+y1*2)!=-1)){
                        if(*(graph_temp->points + 2+y0*2)<=x0 && *(graph_temp->points + 2+y0*2+1)>=(x0-*(graph_temp->points + 2+y0*2)) \
                        && *(graph_temp->points + 2+y1*2)<=x1 && *(graph_temp->points + 2+y1*2+1)>=(x1-*(graph_temp->points + 2+y1*2))){
                            graph_select = graph_temp;
                            break;
                        }
                    }
                    graph_temp = graph_temp->next;
                }
            }
            x_screen_to_game = x0;
            y_screen_to_game = y0;
            presure_screen_to_game = samp_mt[0][0].pressure;

            if(used==0){
                used = 1;
                uart_send_bytes(fd_uart, "00000000000\r\n",14); //重新刷屏一次
                uart_read_bytes(fd_uart,&file, 1);
            }else{
                uart_read_bytes(fd_uart,&file, 1);
            }
            game_run_once();
            uart_send_bytes(fd_uart, "111111111111\r\n",14); //一帧发送完成
            uart_read_bytes(fd_uart,&file, 1);
            count = 0;
            graph_temp =start_graph_block;
            while(graph_temp){graph_temp = graph_temp->next;count++;} //只有一个块就不用操作了
            if(count>1){
                graph_temp =start_graph_block;
                while(graph_temp){ //删掉所有的块
                    graph_block = graph_temp;
                    graph_temp = graph_temp->next;
                    free_graph(graph_block);
                }
                graph_block = make_graph(windowsbuffer,0,0,1000,1000,0,NULL); //创建唯一块
                *(graph_block->points) = 0;
                *(graph_block->points +1) = 600;
                for(int i=0;i<600;i++){
                    *(graph_block->points +i*2 +2) = 0;
                    *(graph_block->points +1 +i*2 +2) = 1024;
                }
            }
            lcd_show_block_plus(graph_block, framebuffer, backbuffer,1);
            show_keys(backbuffer);
            memcpy(framebuffer, backbuffer, screen_size); //刷屏
        }else if(mode==4){
            touchflag = 0;
            communicateflag = 0;
            for(int i=0;i<6;i++)ts_read_mt(ts, samp_mt, max_slots, 1);
            if(samp_mt[0][0].pressure&&samp_mt[0][1].pressure){
                //这部分进行过滤，避免两个很近的点被当作两次输入
                if(temp_x0==0&&temp_x1==0){}else{
                    if(samp_mt[0][0].x>temp_x0){abs_x0 = samp_mt[0][0].x - temp_x0;}else{abs_x0 = temp_x0 - samp_mt[0][0].x;}
                    if(samp_mt[0][1].x>temp_x1){abs_x1 = samp_mt[0][1].x - temp_x1;}else{abs_x1 = temp_x1 - samp_mt[0][1].x;}
                    temp_x0 = samp_mt[0][0].x;
                    temp_x1 = samp_mt[0][1].x;
                    if(abs_x0 <25||abs_x1<50){continue;}
                }
                temp_x0 = samp_mt[0][0].x;
                temp_x1 = samp_mt[0][1].x;
                line = make_line(samp_mt[0][0].x,samp_mt[0][0].y,samp_mt[0][1].x,samp_mt[0][1].y);
                lcd_show_line(samp_mt[0][0].x,samp_mt[0][0].y,samp_mt[0][1].x,samp_mt[0][1].y,framebuffer);
                graph_temp =start_graph_block;
                count = 0;
                while(graph_temp){graph_temp = graph_temp->next;count++;} //这里避免新创建的块又被重复判断切割，所以只判断已有的块，记录下次数
                graph_temp =start_graph_block;
                for(int i=0;i<count;i++){
                    if(!line_chap_block(graph_temp, line)){ //如果没有相交
                    }else{
                        graph_temp = graph_temp->next;
                        free_graph(graph_temp->prev);
                        continue;
                    };
                    graph_temp = graph_temp->next;
                }
                free_line(line);
            }
            game_run_once();
            graph_temp =start_graph_block; //每次重回项首
            memset(backbuffer, 0xff, screen_size); //刷屏白色
            while(graph_temp){
                lcd_show_block_plus(graph_temp, framebuffer, backbuffer,1); //把每一个图形块的内容写进backbuffer
                graph_temp = graph_temp->next;
            }
            show_keys(backbuffer);
            memcpy(framebuffer, backbuffer, screen_size); //刷屏
        }else if(mode == 5){
            touchflag = 1;
            communicateflag = 0;
            for(int i=0;i<6;i++)ts_read_mt(ts, samp_mt, max_slots, 1);
            if(samp_mt[0][0].pressure){
                graph_temp =start_graph_block;
                while(graph_temp){
                    x0 = (samp_mt[0][0].x - graph_temp->x)*1000/graph_temp->xscale; //将触点坐标映射到原windowsbuffer的坐标
                    y0 = (samp_mt[0][0].y - graph_temp->y)*1000/graph_temp->yscale;
                    if((*(graph_temp->points + 2+y0*2)!=-1)&&(*(graph_temp->points + 2+y1*2)!=-1)){
                        if(*(graph_temp->points + 2+y0*2)<=x0 && *(graph_temp->points + 2+y0*2+1)>=(x0-*(graph_temp->points + 2+y0*2)) \
                        && *(graph_temp->points + 2+y1*2)<=x1 && *(graph_temp->points + 2+y1*2+1)>=(x1-*(graph_temp->points + 2+y1*2))){
                            graph_select = graph_temp;
                            break;
                        }
                    }
                    graph_temp = graph_temp->next;
                }
            }
            x_screen_to_game = x0;
            y_screen_to_game = y0;
            presure_screen_to_game = samp_mt[0][0].pressure;
            game_run_once();
            graph_temp =start_graph_block; //每次重回项首
            memset(backbuffer, 0xff, screen_size); //刷屏白色
            while(graph_temp){
                lcd_show_block_plus(graph_temp, framebuffer, backbuffer,1); //把每一个图形块的内容写进backbuffer
                graph_temp = graph_temp->next;
            }
            show_keys(backbuffer);
            memcpy(framebuffer, backbuffer, screen_size); //刷屏
        }
        if(gap < 5){gap++;continue;}
        for(int i=0;i<5;i++)ts_read_mt(ts, samp_mt, max_slots, 1);
        if(samp_mt[0][0].pressure){
            if(samp_mt[0][0].x < key1.width/2 +key1.x && samp_mt[0][0].x >  key1.x-key1.width/2){
                if(samp_mt[0][0].y < key1.height/2 +key1.y && samp_mt[0][0].y >  key1.y-key1.height/2) {
                    key1.select = 1;
                    mode = 1;
                    key2.select = 0;
                    key3.select = 0;
                    key4.select = 0;
                    key5.select = 0;
                    graph_select = NULL; //每一次模式切换都把之前的选择块给清除，以免冲突
                    used = 0;
                }
            }
            if(samp_mt[0][0].x < key2.width/2 +key2.x && samp_mt[0][0].x > key2.x -key2.width/2){
                if(samp_mt[0][0].y < key2.height/2 +key2.y && samp_mt[0][0].y > key2.y -key2.height/2) {
                    key2.select = 1;
                    mode = 2;
                    key1.select = 0;
                    key3.select = 0;
                    key4.select = 0;
                    key5.select = 0;
                    graph_select = NULL;
                    used = 0;
                }
            }
            if(samp_mt[0][0].x < key3.width/2 +key3.x && samp_mt[0][0].x > key3.x -key3.width/2){
                if(samp_mt[0][0].y < key3.height/2 +key3.y && samp_mt[0][0].y > key3.y -key3.height/2) {
                    key3.select = 1;
                    mode = 4;
                    key1.select = 0;
                    key2.select = 0;
                    key4.select = 0;
                    key5.select = 0;
                    graph_select = NULL;
                    used = 0;
                }
            }
            if(samp_mt[0][0].x < key4.width/2 +key4.x && samp_mt[0][0].x > key4.x -key4.width/2){
                if(samp_mt[0][0].y < key4.height/2 +key4.y && samp_mt[0][0].y > key4.y -key4.height/2) {
                    key4.select = 1;
                    mode = 5;
                    key1.select = 0;
                    key2.select = 0;
                    key3.select = 0;
                    key5.select = 0;
                    graph_select = NULL;
                    used = 0;
                }
            }
            if(samp_mt[0][0].x < key5.width/2 +key5.x && samp_mt[0][0].x > key5.x -key5.width/2){
                if(samp_mt[0][0].y < key5.height/2 +key5.y && samp_mt[0][0].y > key5.y -key5.height/2) {
                    key5.select = 1;
                    mode = 3;
                    key1.select = 0;
                    key2.select = 0;
                    key3.select = 0;
                    key4.select = 0;
                    graph_select = NULL;
                    used = 0;
                }
            }
        }
        gap = 0;
    }
}
int main(){
    char* filename = "./2.bmp";
    int fb1; //屏幕文件号
    int fb2;//串口文件号

    for(int i=0;i<maxbmpfile;i++){ //将每一个bmp文件号初始化为0
        bmp_files[i].fb =0;
    }

    screen_init(&fb1,&framebuffer,&backbuffer,&windowsbuffer);
    get_bmp_header(filename,&bmp_files[0]);
    if(ts_init()==-1){
        printf("error ts_init fail");
    }
    game_init(&BGBbuffer);
    fd_uart  = uart_init("/dev/ttymxc5", B115200);
    event_run(1);
}