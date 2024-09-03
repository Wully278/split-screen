#include "usart.h"		 
#include "tftlcd.h"
#include "bmp.h"
int fputc(int ch,FILE *p)  //����Ĭ�ϵģ���ʹ��printf����ʱ�Զ�����
{
	USART_SendData(USART1,(u8)ch);	
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)==RESET);
	return ch;
}

//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART1_RX_BUF[USART1_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART1_RX_STA=0;       //����״̬���


/*******************************************************************************
* �� �� ��         : USART1_Init
* ��������		   : USART1��ʼ������
* ��    ��         : bound:������
* ��    ��         : ��
*******************************************************************************/ 
void USART1_Init(u32 bound)
{
   //GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
 
	
	/*  ����GPIO��ģʽ��IO�� */
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;//TX			   //�������PA9
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;	    //�����������
	GPIO_Init(GPIOA,&GPIO_InitStructure);  /* ��ʼ����������IO */
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;//RX			 //��������PA10
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;		  //ģ������
	GPIO_Init(GPIOA,&GPIO_InitStructure); /* ��ʼ��GPIO */
	

	//USART1 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART1, &USART_InitStructure); //��ʼ������1
	
	USART_Cmd(USART1, ENABLE);  //ʹ�ܴ���1 
	
	USART_ClearFlag(USART1, USART_FLAG_TC);
		
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//��������ж�

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//����1�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����	
}

/*******************************************************************************
* �� �� ��         : USART1_IRQHandler
* ��������		   : USART1�жϺ���
* ��    ��         : ��
* ��    ��         : ��
*******************************************************************************/ 
int temp_count=1; //��0������������־���ӵ�һ����ʼʹ��
int temp_i=0;
int temp_j=0;
int x_uart,y_uart;
int restart = 0;
void USART1_IRQHandler(void)                	//����1�жϷ������
{
	u8 r;	
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //�����ж�
	{
		r =USART_ReceiveData(USART1);//(USART1->DR);	//��ȡ���յ�������
		if((USART1_RX_STA&0x8000)==0)//����δ���
		{
			if(USART1_RX_STA&0x4000)//���յ���0x0d
			{
				if(r!=0x0a)USART1_RX_STA=0;//���մ���,���¿�ʼ
				else USART1_RX_STA|=0x8000;	//��������� 
				if(USART1_RX_BUF[0]=='0' && USART1_RX_BUF[1]=='0' && USART1_RX_BUF[2]=='0' && USART1_RX_BUF[3]=='0'){
					restart =1;
				}else if(USART1_RX_BUF[0]=='1' && USART1_RX_BUF[1]=='1' && USART1_RX_BUF[2]=='1' && USART1_RX_BUF[3]=='1'){
					object[temp_count]->filename[0] = 0;
					temp_count=0;
				}else{
					x_uart = ((int)USART1_RX_BUF[0]+(int)USART1_RX_BUF[1]*256-624)*0.8;
					y_uart = ((int)USART1_RX_BUF[2]+(int)USART1_RX_BUF[3]*256)*0.8;
					object[temp_count]->x = x_uart;
					object[temp_count]->y = y_uart;
					object[temp_count]->flag = USART1_RX_BUF[4];
					object[temp_count]->type = USART1_RX_BUF[5];
					temp_i = 0;
					do{
						object[temp_count]->filename[temp_i] = *(USART1_RX_BUF+8+temp_i);
						temp_i++;
					}while(*(USART1_RX_BUF+8 + temp_i));
					object[temp_count]->filename[temp_i] = *(USART1_RX_BUF+8+temp_i);
					temp_count++;
				}
				for(temp_i=0;temp_i<200;temp_i++){USART1_RX_BUF[temp_i]=0;}
				USART_SendData(USART1,1);
					
				
			}
			else //��û�յ�0X0D
			{	
				if(r==0x0d)USART1_RX_STA|=0x4000;
				else
				{
					USART1_RX_BUF[USART1_RX_STA&0X3FFF]=r;
					USART1_RX_STA++;
					if(USART1_RX_STA>(USART1_REC_LEN-1))USART1_RX_STA=0;//�������ݴ���,���¿�ʼ����	  
				}		 
			}
		}else{
			USART1_RX_STA=0;
			USART1_RX_BUF[USART1_RX_STA&0X3FFF]=r;
			USART1_RX_STA++;
		}
	}
}