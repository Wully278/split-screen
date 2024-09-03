#include <fcntl.h>
#include <stdio.h>
#include "event.h"
int fd_uart;
int uart_init(const char *device, int baud_rate){ //device是设备的名字"/dev/ttymxc5"，baud_rate波特率，可以为B115200
    int fd;
    struct termios newtio, oldtio;

    // 打开设备
    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    
    // 设置串口为阻塞状态
    if (fcntl(fd, F_SETFL, 0) < 0) {
        printf("fcntl failed!\n");
        close(fd);
        return -1;
    }

    // 获取当前串口配置
    if (tcgetattr(fd, &oldtio) != 0) { 
        perror("SetupSerial 1");
        close(fd);
        return -1;
    }
    bzero(&newtio, sizeof(newtio)); // 清零结构体
    
    // 配置串口参数
    newtio.c_cflag |= (CLOCAL | CREAD); 
    newtio.c_cflag &= ~CSIZE; 
    newtio.c_cflag |= CS8; // 8位
    newtio.c_cflag &= ~PARENB; // 不校验
    newtio.c_cflag &= ~CSTOPB; // 1个停止位

    // 设置波特率
    cfsetispeed(&newtio, B115200); // 设置输入波特率
    cfsetospeed(&newtio, B115200); // 设置输出波特率
    
    // 配置输入输出模式
    newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Input
    newtio.c_oflag &= ~OPOST; // Output
    
    // 设置读取数据的最小字节数和等待时间
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;

    // 刷新输入输出缓冲区
    tcflush(fd, TCIFLUSH);

    // 设置新的串口配置
    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
        perror("com set error");
        close(fd);
        return -1;
    }
    return fd; // 返回串口文件描述符
}

int uart_send_bytes(int fd, char* data,int len) { //写一个字节
    int iRet = write(fd, data, len);
    if (iRet == -1) {
        perror("write");
        return -1;
    }
    return 0;
}
int uart_read_bytes(int fd, char* buffer, int len) {
    int iRet = read(fd, buffer, len);
    if (iRet == -1) {
        perror("read");
        return -1;
    } else if (iRet == 0) {
        fprintf(stderr, "No data available\n");
        return -1;
    }
    return iRet;
}
int main3(){
    int fd;
    char c;
    fd  = uart_init("/dev/ttymxc5", B115200);
    close(fd);
    return 0;
}
