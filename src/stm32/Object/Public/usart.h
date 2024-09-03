#ifndef _usart_H
#define _usart_H

#include "system.h" 
#include "stdio.h" 

#define USART1_REC_LEN		200  	//�����������ֽ��� 200

extern u8  USART1_RX_BUF[USART1_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u16 USART1_RX_STA;         		//����״̬���


void USART1_Init(u32 bound);

struct receive_info{
	//uint16_t *buffer565;
	int x;
	int y;
	char filename[32];
	int flag;
	int type;
};
extern struct receive_info* object[32];

#endif


