#include <stdint.h>
#include <dirent.h>
#include <termios.h>
extern struct fb_var_screeninfo var; //屏幕信息
extern unsigned int line_width;
extern unsigned int pixel_width;
extern int screen_size;
#define maxbmpfile 64
extern unsigned char * framebuffer; //帧缓存区
extern unsigned char * backbuffer; //处理缓存区
extern unsigned char * windowsbuffer; //窗口缓存区
extern unsigned char * BGBbuffer; //背景图片缓存区

extern struct tsdev *ts;
extern struct input_absinfo slot; //插槽结构体，就是同时触摸点的结构体，每个触点一个结构体
extern int max_slots; //最大触摸点的个数
extern struct ts_sample_mt **samp_mt; //触摸点信息结构数组
extern struct ts_sample samp;

extern int touchflag; //游戏是否可触屏交互，0不行，1可以
extern int communicateflag; //两块板子是否通信标准，0没有通信，1通信
extern int fd_uart;//串口文件号
enum type {
    SplitPea,
    shovel,
    zombies
};
struct key{
    int x;
    int y;
    int width;
    int height;
    int select;
};
extern struct key key1; //定义4个按钮
extern struct key key2;
extern struct key key3;
extern struct key key4;
struct object{
    int x;
    int y;
    int health;
    uint8_t framenumber;
    uint8_t maxframenumber;
    uint8_t maxframenumber1;
    int pid;
    int objecttype;//物体类型，1为植物，2为僵尸，3为图标，4为选择拖放的植物,5铲子
    enum type type;
    char* typename;
    DIR *dir; //文件夹地址
    struct object* prev;
    struct object* next;
};
struct bullet{ //给子弹单独定义
    int x;
    int y;
    int state;
    struct bullet* prev;
    struct bullet* next;
};
extern struct bullet* bulletstart;
extern struct object* plantsobject;
extern struct object* zombiesobject;
extern struct object* selectobject;
extern struct object* barobject;

extern int grasswidthpixel; //每块草坪宽像素
extern int grassheigthpixel; //每块草坪高像素
extern int grassstart_x; //草坪开始点像素x
extern int grassstart_y; //草坪开始点像素y

extern char bmppath[32];
extern char barpath[32];
extern char bulletpath[32];
extern char* mapfilepath;
extern char* barfilepath;
extern char barobjectpath[32];
extern char shovelname[32];
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
    int fb; //文件号
    bmp_file_header_t file_header;
    bmp_info_header_t info_header;
};
extern struct bmp_file bmp_files[maxbmpfile];
extern struct graph_block* start_graph_block;
struct graph_block{
    unsigned char* windows_buffer; //串口缓存区
    uint16_t xscale; //缩放倍数，以千为单位，数值为1000代表缩放为1
    uint16_t yscale; //缩放倍数
    //struct bmp_file* file; //对应图片结构
    uint8_t pid; //父图块号
    int x; //起始坐标
    int y;
    uint32_t size; //坐标个数
    int* points; //像素坐标指针
    struct graph_block* prev;
    struct graph_block* next;
};
//线的结构体
struct line{
    // uint16_t len; //线长度
    // uint16_t* points; //像素坐标指针
    int x0;
    int y0;
    int x1;
    int y1;
};
int get_bmp_header(char* pathname,struct bmp_file* file);
int screen_init(int* fb,unsigned char** frame_buffer,unsigned char** back_buffer,unsigned char** windows_buffer);
void lcd_show_line(int x0,int y0,int x1,int y1,unsigned char *frame_buffer);
int ts_init();
int uart_init(const char *device, int baud_rate);
int uart_send_bytes(int fd, char* data,int len);