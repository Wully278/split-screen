#ifndef _BMP_H
#define _BMP_H
#include "system.h"
#include "ff.h"
#include <string.h>
typedef struct
{
    uint16_t type;  //λͼ�ļ������ͣ�����ΪBM(1-2�ֽڣ�
    uint32_t size;  //λͼ�ļ��Ĵ�С�����ֽ�Ϊ��λ��3-6�ֽڣ���λ��ǰ��
    uint16_t reserved1;  //λͼ�ļ������֣�����Ϊ0(7-8�ֽڣ�
    uint16_t reserved2;  //λͼ�ļ������֣�����Ϊ0(9-10�ֽڣ�
    uint32_t off_bits;  //λͼ����λ�õĵ�ַƫ�ƣ�����ʼλ�ã��������λͼ��11-14�ֽڣ���λ��ǰ��
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

//bmpͼƬ��Ϣ
struct bmp_file{
    FIL fp; //�ļ���
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