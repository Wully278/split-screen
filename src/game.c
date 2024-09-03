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


#include <unistd.h>

unsigned char * BGBbuffer; 
int grasswidthpixel = 80; //每块草坪宽像素
int grassheigthpixel = 95; //每块草坪高像素
int grassstart_x = 250; //草坪开始点像素x
int grassstart_y = 80; //草坪开始点像素y

int randomcount =0;//随机数计时
//一些游戏图片路径
char bmppath[32] = "./game/type/";
char barpath[32] = "./game/bar/";
char bulletpath[32] = "./game/bullet/";
char* mapfilepath = "./game/Map/map0.bmp";
char* barfilepath = "./game/barglobal/ChooserBackground.bmp";
char barobjectpath[32] = "./game/bar/";
char shovelname[32] = "shovel";
//几个游戏对象起始链表
struct bullet* bulletstart = NULL;
struct object* plantsobject = NULL;
struct object* zombiesobject = NULL;
struct object* selectobject = NULL;
struct object* barobject = NULL;
//创建一个游戏对象
struct object* create_object(int x,int y,int objecttype,enum type type,struct object* prev){
    struct object* tempobject;
    struct object* object = (struct object*)malloc(sizeof(struct object));
    int flag =0;
    object->type = type;
    switch (object->type) {
        case SplitPea: object->typename = "SplitPea";object->maxframenumber = 14;break;
        case shovel: object->typename = "shovel";object->maxframenumber = 1;break;
        case zombies: object->typename = "zombies";object->maxframenumber = 22;object->maxframenumber1 = 43;break;
    }
    object->dir = opendir(object->typename);
    object->x = x;
    object->y = y;
    object->health = 100;
    object->framenumber = 1;
    object->objecttype = objecttype;
    if(object->objecttype == 3){ //图标
        if(barobject!=NULL){
            tempobject = barobject;
            while(tempobject->next){
                tempobject = tempobject->next;
            }
            object->prev = tempobject;
            tempobject->next = object;
            //object->pid = tempobject->pid +1;
            object->next = NULL;
        }else{
                //object->pid = 1;
                barobject = object;
        }
    }else if(object->objecttype == 1){ //植物
        //object->pid = 0;
        if(plantsobject == NULL){
            plantsobject = object;
            //object->pid = 1;
            object->prev = NULL;
            object->next = NULL;
        }else{
            tempobject = plantsobject;
            while(tempobject){
                if(object->y>tempobject->y){
                    tempobject = tempobject->next;
                    continue;
                }else if(object->y == tempobject->y){
                    if(object->x>=tempobject->x){ //这里应该是没有等于的情况
                        tempobject = tempobject->next;
                        continue;
                    }else{
                        object->prev = tempobject->prev;
                        object->next = tempobject;
                        if(tempobject->prev !=NULL){tempobject->prev->next = object;}else{plantsobject = object;}
                        tempobject->prev = object;
                        //object->pid = tempobject->prev->pid + 1;
                        flag =1;
                        break;
                    }
                }else{
                    object->prev = tempobject->prev;
                    object->next = tempobject;
                    if(tempobject->prev !=NULL){tempobject->prev->next = object;}else{plantsobject = object;}
                    tempobject->prev = object;
                    //object->pid = tempobject->prev->pid + 1;
                    flag =1;
                    break;
                }
            }
            while(tempobject){//这是将插入点后面的pid全都加1，但如果不需要pid，可以把这个循环注释
                //tempobject->pid++;
                tempobject = tempobject->next;
            }
            if(flag == 0){
                tempobject = plantsobject;
                while(tempobject->next){
                    tempobject = tempobject->next;
                }
                object->prev = tempobject;
                tempobject->next = object;
                object->next = NULL;
                //object->pid = tempobject->pid + 1;
            }
        }
    }else if(object->objecttype == 2){ //僵尸的情况和植物相似
        if(zombiesobject == NULL){
            zombiesobject = object;
            //object->pid = 1;
            object->prev = NULL;
            object->next = NULL;
        }else{
            tempobject = zombiesobject;
            while(tempobject){
                if(object->y>tempobject->y){
                    tempobject = tempobject->next;
                    continue;
                }else if(object->y == tempobject->y){
                    if(object->x>=tempobject->x){ //这里应该是没有等于的情况
                        tempobject = tempobject->next;
                        continue;
                    }else{
                        object->prev = tempobject->prev;
                        object->next = tempobject;
                        if(tempobject->prev !=NULL){tempobject->prev->next = object;}else{zombiesobject = object;}
                        tempobject->prev = object;
                        //object->pid = tempobject->prev->pid + 1;
                        flag =1;
                        break;
                    }
                }else{
                    object->prev = tempobject->prev;
                    object->next = tempobject;
                    if(tempobject->prev !=NULL){tempobject->prev->next = object;}else{zombiesobject = object;}
                    tempobject->prev = object;
                    //object->pid = tempobject->prev->pid + 1;
                    flag =1;
                    break;
                }
            }
            while(tempobject){//这是将插入点后面的pid全都加1，但如果不需要pid，可以把这个循环注释
                //tempobject->pid++;
                tempobject = tempobject->next;
            }
            if(flag == 0){
                tempobject = zombiesobject;
                while(tempobject->next){
                    tempobject = tempobject->next;
                }
                object->prev = tempobject;
                tempobject->next = object;
                object->next = NULL;
                //object->pid = tempobject->pid + 1;
            }
        }
    }else if(object->objecttype == 4){ //选择拖放植物

    }
    return object;
}
//删除对象
void delete_object(struct object* object){
    int pid =2;
    struct object* tempobject;
    if(object->objecttype == 3){ //图标
        if(object->prev == NULL){
            barobject = NULL;
            free(object);
            return;
        }else{
            if(object->next == NULL){
                object->prev->next = NULL;
                free(object);
                return;
            }else{
                object->prev->next = object->next;
                object->next->prev = object->prev;
                free(object);
                return;
            }
        }
    }else if(object->objecttype == 1){ //植物
        if(object->prev == NULL){
            if(object->next ==NULL){
                plantsobject = NULL;
                free(object);
                return;
            }else{
                plantsobject = object->next;
                plantsobject->prev =NULL;
                free(object);
                return;
            }
        }else{
            if(object->next == NULL){
                object->prev->next = NULL;
                free(object);
                return;
            }else{
                object->prev->next = object->next;
                object->next->prev = object->prev;
                free(object);
                return;
            }
        }
    }else if(object->objecttype == 2){ //僵尸和植物相似

        if(object->prev == NULL){
            if(object->next ==NULL){
                zombiesobject = NULL;
                free(object);
                return;
            }else{
                zombiesobject = object->next;
                zombiesobject->prev =NULL;
                free(object);
                return;
            }
        }else{
            if(object->next == NULL){
                object->prev->next = NULL;
                free(object);
                return;
            }else{
                object->prev->next = object->next;
                object->next->prev = object->prev;
                free(object);
                return;
            }
        }
    }else if(object->objecttype == 4){ //选择拖放植物

    }
}
//创建子弹
struct bullet* creat_bullet(int x,int y){
    struct bullet* tempbullet;
    struct bullet* bullet = (struct bullet*)malloc(sizeof(struct bullet));

    bullet->x = x;
    bullet->y = y;
    bullet->state = 0;
    if(bulletstart==NULL){
        bulletstart = bullet;
        bullet->prev = NULL;
    }else{
        tempbullet = bulletstart;
        while(tempbullet->next){
            tempbullet = tempbullet->next;
        }
        tempbullet->next = bullet;
        bullet->prev = tempbullet;
    }
    bullet->next = NULL;
}
//删除子弹
void delete_bullet(struct bullet* bullet){
    if(bullet->prev == NULL){
        if(bullet->next ==NULL){
            bulletstart = NULL;
            free(bullet);
            return;
        }else{
            bulletstart = bullet->next;
            bulletstart->prev =NULL;
            free(bullet);
            return;
        }
    }else{
        if(bullet->next == NULL){
            bullet->prev->next = NULL;
            free(bullet);
            return;
        }else{
            bullet->prev->next = bullet->next;
            bullet->next->prev = bullet->prev;
            free(bullet);
            return;
        }
    }
}

//直接用了之前的函数，只加了判断某个像素是否是白点
int write_to_windowsbuffer(int x_start, int y_start,int x_end, int y_end, struct bmp_file* file, unsigned char *windows_buffer){
    int x_len,y_len;
    int ytemp;
    int xtemp;
    int x,y;
    int gapx,gapy;
    char buff[4];
    unsigned char *temp_buffer;
    int ii=0;
    int jj=0;
    int kk=0;
    int pcount;
    unsigned int *temp;
    int xscale = 1000*(x_end - x_start)/file->info_header.width;
    int yscale = 1000*(y_end - y_start)/file->info_header.height;
    int bmprow = file->info_header.height;
    int bmpline = 3*file->info_header.width;
    
    if(file->info_header.width%4!=0){
			bmpline+= 4 - file->info_header.width*3%4;
		}
    if(x_start>(int)var.xres || x_end>(int)var.xres || y_start>(int)var.yres || y_end>(int)var.yres){ //超界
        return 0;
    }
    temp_buffer = malloc(file->file_header.size - file->file_header.off_bits);
    read(file->fb,temp_buffer,file->file_header.size - file->file_header.off_bits);
    
    lseek(file->fb,54,SEEK_SET);
    ytemp = y_start + yscale*(file->info_header.height-1)/1000;//printf("file->info_header.height is %d\n",y_start);
    xtemp = x_start;
    for(int i = 0;i<file->info_header.height;i++){
        y = y_start + yscale*(file->info_header.height -i-1)/1000;//printf("x i is %d x i is %d\n",ytemp,y);
        for(int ii =0;ii<ytemp-y;ii++){
            for(int j=0;j<file->info_header.width;j++){
                x = x_start + xscale*j/1000;
                pcount = i*bmpline + j*3;
                if(*(temp_buffer+pcount)==0xff){xtemp = x;continue;}
                for(int jj=0;jj<x-xtemp;jj++){
                    if(y +ii >var.yres || y +ii <0 ||x -jj>var.xres || x -jj <0){continue;}
                    buff[0] = *(temp_buffer+pcount);
			        buff[1] = *(temp_buffer+pcount+1);
			        buff[2] = *(temp_buffer+pcount+2);
                    temp = windows_buffer+(y +ii)*line_width+(x -jj)*pixel_width;
                    *temp = ((*(buff+2))<<16 | (*(buff+1))<<8 | (*(buff)));
                }
                xtemp = x;
		    }
        }
        ytemp = y;
    }
    free(temp_buffer);
}
//根据对象属性找到它的文件路径
int find_object_name(char* pathname,struct object* object){
    int ptr1 =0;
    int ptr2 =0;
    while (bmppath[ptr1]){
        pathname[ptr1] = bmppath[ptr1];
        ptr1++;
    }
    while (*(object->typename +ptr2)){
        pathname[ptr1++] = *(object->typename +ptr2++);
    }
    pathname[ptr1++] = '/';
    if(object->framenumber>9){ //这里只考虑到编号最多两位数
        pathname[ptr1++] = object->framenumber/10 + 48;
        pathname[ptr1++] = object->framenumber%10 + 48;
    }else{
        pathname[ptr1++] = object->framenumber + 48;
    }
    pathname[ptr1++] = '.';
    pathname[ptr1++] = 'b';
    pathname[ptr1++] = 'm';
    pathname[ptr1++] = 'p';
    pathname[ptr1] = '\0';
    return ptr1;
}
//显示一个对象
void show_object(struct object* object){
    struct dirent *entry;
    char file[64];
    int ptr1 =0;
    int ptr2 =0;
    if(object->objecttype == 1 || object->objecttype == 2 || object->objecttype == 4){
        ptr1 = find_object_name(file,object);
        if(object->framenumber<object->maxframenumber){
            object->framenumber++;
        }else if(object->framenumber == object->maxframenumber){
            object->framenumber=1;
        }else if(object->framenumber<object->maxframenumber1){
            object->framenumber++;
        }else{
            object->framenumber= object->maxframenumber +1;
        }
    }else if(object->objecttype == 3){ //显示图标，暂时未用
        while (barpath[ptr1]){
            file[ptr1] = bmppath[ptr1];
            ptr1++;
        }
        while (*(object->typename +ptr2)){
            file[ptr1++] = *(object->typename +ptr2++);
        }
        file[ptr1++] = '.';
        file[ptr1++] = 'b';
        file[ptr1++] = 'm';
        file[ptr1++] = 'p';
        file[ptr1] = '\0';
    }
    
    get_bmp_header(file,&bmp_files[0]);
    if(object->objecttype == 1 || object->objecttype == 4){
        write_to_windowsbuffer(object->x, object->y,object->x+80, object->y+95, &bmp_files[0], windowsbuffer);
    }else if(object->objecttype == 2){
        if(communicateflag==0){
            write_to_windowsbuffer(object->x, object->y -45,object->x+80, object->y+95, &bmp_files[0], windowsbuffer);
        }else{ //如果需要向次板发送信息
            if(object->x>624){
                uart_send_bytes(fd_uart, &object->x,2);
                uart_send_bytes(fd_uart, &object->y,2);
                uart_send_bytes(fd_uart, "1",1);
                uart_send_bytes(fd_uart, "1",1);
                uart_send_bytes(fd_uart, file,ptr1);
                uart_send_bytes(fd_uart, "\r\n",2);
                uart_read_bytes(fd_uart, file, 1);
                write_to_windowsbuffer(object->x, object->y -45,object->x+80, object->y+95, &bmp_files[0], windowsbuffer);
            }else{write_to_windowsbuffer(object->x, object->y -45,object->x+80, object->y+95, &bmp_files[0], windowsbuffer);}
        }
    }
    close(bmp_files[0].fb);
}
void show_bullet(struct bullet* bullet){
    char file[64];
    int ptr1 =0;
    while (bulletpath[ptr1]){
        file[ptr1] = bulletpath[ptr1];
        ptr1++;
    }
    if(bullet->state == 0){
        file[ptr1++] = '1';
    }else{
        file[ptr1++] = '2';
    }
    file[ptr1++] = '.';
    file[ptr1++] = 'b';
    file[ptr1++] = 'm';
    file[ptr1++] = 'p';
    file[ptr1] = '\0';
    get_bmp_header(file,&bmp_files[0]);
    if(communicateflag==0){
        write_to_windowsbuffer(bullet->x, bullet->y,bullet->x+30, bullet->y+30, &bmp_files[0], windowsbuffer);
    }else{
        if(bullet->x>624){
            uart_send_bytes(fd_uart, &bullet->x,2);
            uart_send_bytes(fd_uart, &bullet->y,2);
            uart_send_bytes(fd_uart, "1",1);
            uart_send_bytes(fd_uart, "0",1);
            uart_send_bytes(fd_uart, file,ptr1);
            uart_send_bytes(fd_uart, "\r\n",2);
            uart_read_bytes(fd_uart, file, 1);
                
            write_to_windowsbuffer(bullet->x, bullet->y,bullet->x+30, bullet->y+30, &bmp_files[0], windowsbuffer);
        }else{write_to_windowsbuffer(bullet->x, bullet->y,bullet->x+30, bullet->y+30, &bmp_files[0], windowsbuffer);}
    }
    close(bmp_files[0].fb);
}
void game_init(unsigned char ** buffer){
    struct object* tempobject;
    char file[64];
    int ptr1 =0;
    int ptr2 =0;
    *buffer = malloc(screen_size);
    get_bmp_header(mapfilepath,&bmp_files[0]);
    
    write_to_windowsbuffer(0, 0,1024, 600, &bmp_files[0], *buffer);
    close(bmp_files[0].fb);
    get_bmp_header(barfilepath,&bmp_files[0]);
    write_to_windowsbuffer(250, 0,750, 75, &bmp_files[0], *buffer);
    close(bmp_files[0].fb);
    barobject = create_object(250+75,0,3,SplitPea,NULL);
    create_object(850+75,0,3,shovel,NULL);
    tempobject = barobject;
    while(tempobject!=NULL){
        ptr1 =0;
        ptr2 =0;
        while (barobjectpath[ptr1]){
            file[ptr1] = barobjectpath[ptr1];
            ptr1++;
        }
        while (*(tempobject->typename +ptr2)){
            file[ptr1++] = *(tempobject->typename +ptr2++);
        } 
        file[ptr1++] = '.';
        file[ptr1++] = 'b';
        file[ptr1++] = 'm';
        file[ptr1++] = 'p';
        file[ptr1] = '\0';
        
        get_bmp_header(file,&bmp_files[0]);
        write_to_windowsbuffer(tempobject->x,tempobject->y,tempobject->x +50, tempobject->y +75, &bmp_files[0], *buffer);
        tempobject = tempobject->next;
    }
}
//一帧的改变
void change_perframe(struct bullet* bulletstart,struct object* plantsobject,struct object* zombiesobject){
    struct bullet* bullets;
    struct object* plants;
    struct object* zombie;
    int eatflag = 0;
    char file[64];
    int file_name_long;
    plants = plantsobject;
    while(plants){
        if(plants->health<=0){
            if(plants->next){
                plants = plants->next;
                delete_object(plants->prev);
            }else{
                delete_object(plants);
                break;
            }            
        }else{
            if(plants->framenumber == plants->maxframenumber){
                creat_bullet(plants->x + grasswidthpixel*3/4,plants->y + grassheigthpixel/8); //y根据实际调整,这里改变对应bullets->y - grassheigthpixel/8)!=zombie->y也要改
            }
            plants = plants->next;
        }
    }
    bullets = bulletstart;
    while(bullets){
        if(bullets->state || bullets->x>980){
            if(communicateflag!=0){
                if(bullets->x>624){
                uart_send_bytes(fd_uart, &bullets->x,2);
                uart_send_bytes(fd_uart, &bullets->y,2);
                uart_send_bytes(fd_uart, "0",1);
                uart_send_bytes(fd_uart, "0",1);
                uart_send_bytes(fd_uart, file,17); //这里发送几个字节其实无所谓，上面的"0"指明了它是子弹
                uart_send_bytes(fd_uart, "\r\n",2);
                uart_read_bytes(fd_uart, file, 1);
                }
            }
            
            if(bullets->next){
                bullets = bullets->next;
                delete_bullet(bullets->prev);
            }else{
                delete_bullet(bullets);
                break;
            } 
        }else{
            bullets->x+=10;
            bullets = bullets->next;
        }
    }
    zombie = zombiesobject;
    while(zombie){
        if(zombie->health<=0 || zombie->x<250){
            if(communicateflag!=0){
                if(zombie->x>624){
                    file_name_long = find_object_name(file,zombie);
                    uart_send_bytes(fd_uart, &zombie->x,2);
                    uart_send_bytes(fd_uart, &zombie->y,2);
                    uart_send_bytes(fd_uart, "0",1);
                    uart_send_bytes(fd_uart, "1",1);
                    uart_send_bytes(fd_uart, file,file_name_long);//这里需要确切文件名的长度，因为单片机对子弹和僵尸文件的处理不同
                    uart_send_bytes(fd_uart, "\r\n",2);
                    uart_read_bytes(fd_uart, file, 1);
                }
            }
            if(zombie->next){
                zombie = zombie->next;
                delete_object(zombie->prev);
            }else{
                delete_object(zombie);
                break;
            }            
        }else{
            bullets = bulletstart;
            while(bullets){
                if((bullets->y - grassheigthpixel/8)!=zombie->y){ //如果不在同一行，这里草坪行的判断根据实际调整
                    bullets = bullets->next;
                    continue;
                }else{
                    if(bullets->x > zombie->x +10){ //这里判断范围根据实际调整
                        bullets->state = 1; //子弹击中后需要的变化
                        zombie->health -= 2;
                        zombie->x++;
                    }
                    bullets = bullets->next;
                }
            }
            plants = plantsobject;
            while(plants){
                if((plants->y)!=zombie->y){
                    plants = plants->next;
                    continue;
                }else{
                    if(plants->x > zombie->x -grasswidthpixel/2){ //遇到了植物
                        eatflag = 1;
                        plants->health -= 1;
                        zombie->x++;
                        if(zombie->framenumber<=zombie->maxframenumber){
                            zombie->framenumber = zombie->maxframenumber+1;
                        }
                    }
                    plants = plants->next;
                }
            }
            if(eatflag!=1){
                if(zombie->framenumber > zombie->maxframenumber){
                    zombie->framenumber = 1;
                }
            }
            zombie->x--;
            zombie = zombie->next;
        }
    }
    randomcount++;
    if(randomcount%300<5){
        create_object(900,grassstart_y + (randomcount/300%5)*grassheigthpixel,2,zombies,NULL);
        randomcount+=5;
    }
    if(randomcount>1000000){
        randomcount = 0;
    }

}
//读取触屏点
void get_touch_point(int* x,int* y,int* valid){
    for(int i=0;i<5;i++)ts_read_mt(ts, samp_mt, max_slots, 1);
    *x = samp_mt[0][0].x;
    *y = samp_mt[0][0].y;
    *valid = samp_mt[0][0].pressure;
}
//单独运行游戏
void game_run(){
    int x;
    int y;
    int valid;
    struct object* tempobject;
    struct bullet* tempbullet;
    int flag;
    int xoffeset,yoffeset;
    int xrecord,yrecord;
    int count=0;
    while(1){
        change_perframe(bulletstart,plantsobject,zombiesobject);
        for(int i=0;i<1000000;i++);
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
        get_touch_point(&x,&y,&valid);
        if(valid && !selectobject){printf("now is click\n"); //有触点但还没有块被选择
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
        }else if(valid && selectobject){//有新触点，但还是原来的图形块被选择
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
                        if(flag ==0){printf("it is deleted\n");
                            delete_object(tempobject);
                        }
                    }
                }
                delete_object(selectobject);
                selectobject = NULL;
            }
        }
        if(selectobject!=NULL)show_object(selectobject);
        memcpy(framebuffer, windowsbuffer, screen_size);
    }

}

// int main(){
//     char* filename = "./22.bmp";
//     int fb1; //屏幕文件号

//     for(int i=0;i<maxbmpfile;i++){ //将每一个bmp文件号初始化为0
//         bmp_files[i].fb =0;
//     }
//     screen_init(&fb1,&framebuffer,&backbuffer,&windowsbuffer);
//     if(ts_init()==-1){
//         printf("error ts_init fail");
//     }

//     game_init(&BGBbuffer);
//     game_run();
// }