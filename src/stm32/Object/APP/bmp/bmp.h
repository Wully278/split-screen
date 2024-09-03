#ifndef _BMP_H
#define _BMP_H
#include "system.h"
#include "ff.h"
#include <string.h>
typedef struct
{
    uint16_t type;  //位图文件的类型，必须为BM(1-2字节）
    uint32_t size;  //位图文件的大小，以字节为单位（3-6字节，低位在前）
    uint16_t reserved1;  //位图文件保留字，必须为0(7-8字节）
    uint16_t reserved2;  //位图文件保留字，必须为0(9-10字节）
    uint32_t off_bits;  //位图数据位置的地址偏移，即起始位置，以相对于位图（11-14字节，低位在前）
}__attribute__ ((packed)) bmp_file_header_t;

typedef struct
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image;
    uint32_t x_pels_permeter;
    uint32_t y_pels_permeter;
    uint32_t clr_used;
    uint32_t clr_important;
} bmp_info_header_t;

//bmp图片信息
struct bmp_file{
    FIL fp; //文件号
    bmp_file_header_t file_header;
    bmp_info_header_t info_header;
};
extern struct bmp_file bmp_files;
extern uint16_t *bullet_buffer1;
extern uint16_t *bullet_buffer2;
extern uint16_t *zombies_buffer;
extern int bullet_width;
extern int bullet_height;
extern int zombies_width;
extern int zombies_height;
int get_bmp_header(char* pathname,struct bmp_file* file,int argv);
void bmp_show(int x,int y,char* src_path);
void bmp_show_flush(int x0,int y0,char* src_path,char* map_path,int flag,int type);
#endif