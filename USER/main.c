#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "sdram.h"
#include "malloc.h"
#include "usmart.h"
#include "pcf8574.h"
#include "mpu.h"
#include "lwip_comm.h"
#include "lwip/netif.h"
#include "ethernetif.h"
#include "lwipopts.h"
#include "includes.h"
#include "udp_demo.h"
#include "dcmi.h"
#include "MT9P031D.h"
#include "24CXX.h"
/************************************************
 ALIENTEK ������STM32H7������ ����ʵ��9
 ����NETCONN API��UDPʵ��(UCOSII�汾)-HAL�⺯����
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/
//START����
//�������ȼ�
#define START_TASK_PRIO		11
//�����ջ��С
#define START_STK_SIZE		128
//�����ջ
OS_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *pdata); 

//��LCD����ʾ��ַ��Ϣ
//mode:1 ��ʾDHCP��ȡ���ĵ�ַ
//	  ���� ��ʾ��̬��ַ
void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"DHCP IP :%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"DHCP GW :%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,170,210,16,16,buf); 
	}
	else 
	{
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK :%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,170,210,16,16,buf); 
	}	
}

int main(void)
{
  Write_Through();                //����ǿ��͸д��
  MPU_Memory_Protection();        //������ش洢����
  Cache_Enable();                 //��L1-Cache
	HAL_Init();				        			//��ʼ��HAL��
	Stm32_Clock_Init(180,5,2,4);    //����ʱ��,400Mhz 
	delay_init(450);		        		//��ʱ��ʼ��
	uart_init(115200);			    		//���ڳ�ʼ��
	usmart_dev.init(200); 		    	//��ʼ��USMART	
	LED_Init();					    				//��ʼ��LED
	KEY_Init();					    				//��ʼ������
	SDRAM_Init();                   //��ʼ��SDRAM
	AT24CXX_Init();                 //��ʼ��IIC
	//LCD_Init();				        		//��ʼ��LCD
	//PCF8574_Init();               //��ʼ��PCF8574
  my_mem_init(SRAMIN);		    		//��ʼ���ڲ��ڴ��
	my_mem_init(SRAMEX);		    		//��ʼ���ⲿ�ڴ��
	my_mem_init(SRAMDTCM);		    	//��ʼ��DTCM�ڴ��
	POINT_COLOR = RED; 		          //��ɫ����
	//LCD_ShowString(30,50,200,20,16,"UDP NETCONN Test");  
	InitCamParaPacket();
	//�����ڴ�
  netmem_malloc();
  OSInit(); 					    //UCOS��ʼ��
	while(lwip_comm_init()) 	    //lwip��ʼ��
	{
		//LCD_ShowString(30,110,200,20,16,"Lwip Init failed!"); 	//lwip��ʼ��ʧ��
		delay_ms(500);
		LCD_Fill(30,110,230,150,WHITE);
		delay_ms(500);
	}
	
  //LCD_ShowString(30,130,200,20,16,"Lwip Init Success!"); 		//lwip��ʼ���ɹ�
	//show_address(0);
	//���������������ź���
	msg_netrecvok=OSSemCreate(0);	
	//��������׼�����ź���
	msg_netsendready=OSSemCreate(0);	
	udp_demo_init(); 																						//��ʼ��udp_demo ֻ����UDP�������
	//LCD_ShowString(30,130,200,20,16,"UDP Success!       "); 		//UDP��ʼ���ɹ�
	
	OSTaskCreate(start_task,(void*)0,(OS_STK*)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO);
	OSStart(); //����UCOS	
}

//start����
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;	
	OSStatInit();  			//��ʼ��ͳ������
	OS_ENTER_CRITICAL();  	//���ж�
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //����DHCP����
#endif
	OSTaskCreate(capcamera_task,(void*)0,(OS_STK*)&CAPCAMERA_TASK_STK[CAPCAMERA_STK_SIZE-1],CAPCAMERA_TASK_PRIO); //ͼ��ɼ�����
	OSTaskCreate(imgcamera_task,(void*)0,(OS_STK*)&IMGCAMERA_TASK_STK[IMGCAMERA_STK_SIZE-1],IMGCAMERA_TASK_PRIO); //�ɼ������ݴ�������																							
	OSTaskCreate(udp_send_task,(void*)0,(OS_STK*)&UDP_SEND_TASK_STK[UDP_SEND_STK_SIZE-1],UDP_SEND_TASK_PRIO);		//UDP���ݷ�������
	OSTaskCreate(udp_dataprocess_task,(void*)0,(OS_STK*)&UDP_DATAPROCESS_TASK_STK[UDP_DATAPROCESS_STK_SIZE-1],UDP_DATAPROCESS_TASK_PRIO);	//UDP�������ݴ�������
	OSTaskSuspend(OS_PRIO_SELF); //����start_task����
	OS_EXIT_CRITICAL();  		//���ж�
}

//����JPEG����
//���ɼ���һ֡JPEG���ݺ�,���ô˺���,�л�JPEG BUF.��ʼ��һ֡�ɼ�.
void jpeg_data_process(void)
{
	u16 i;
	u16 rlen;			                                                    //ʣ�����ݳ���
	u32 *pbuf;
	if(jpeg_data_ok==0)	                                              //jpeg���ݻ�δ�ɼ���?
	{
      __HAL_DMA_DISABLE(&DMADMCI_Handler);//�ر�DMA
			rlen=NETCAM_LINE_SIZE-__HAL_DMA_GET_COUNTER(&DMADMCI_Handler);//�õ�ʣ�����ݳ���	   
			pbuf=imgbuf2592x480x32[currentcatch]+jpeg_data_len;//ƫ�Ƶ���Ч����ĩβ,�������
			//if(DMADMCI_Handler.Instance->CR&(1<<19))for(i=0;i<rlen;i++)pbuf[i]=dcmi_line_buf[1][i];//��ȡbuf1�����ʣ������
		  SCB_CleanInvalidateDCache();        //�����Ч��DCache
			//SCB_CleanDCache();
			//SCB_InvalidateDCache();
			if(DMA1_Stream1->CR&(1<<19))
			{
				for(i=0;i<rlen;i++)pbuf[i]=netcam_line_buf0[i];         //��ȡbuf1�����ʣ������
				//mymemcpy(pbuf,netcam_line_buf0,rlen<<2);
			}
			else 
			{
				for(i=0;i<rlen;i++)pbuf[i]=netcam_line_buf1[i];         //��ȡbuf0�����ʣ������ 
				//mymemcpy(pbuf,netcam_line_buf1,rlen<<2);
			}
			jpeg_data_len+=rlen;			                                //����ʣ�೤��
			currentcatch++;
		  currentcatch%=3;
			jpeg_data_ok=1; 				                                  //���JPEG���ݲɼ����,�ȴ�������������
	}
	if(jpeg_data_ok==2)	                                          //��һ�ε�jpeg�����Ѿ���������
	{
			//u8 buf[30];
			//sprintf((char*)buf,"Recv%d BYTE",(jpeg_data_len<<2));						//��ӡ��̬IP��ַ
			//LCD_ShowString(30,250,210,16,16,buf); 
      __HAL_DMA_SET_COUNTER(&DMADMCI_Handler,NETCAM_LINE_SIZE);	//���䳤��Ϊjpeg_buf_size*4�ֽ�
			__HAL_DMA_ENABLE(&DMADMCI_Handler);                       //��DMA
			jpeg_data_ok=0;					                                  //�������δ�ɼ�
			jpeg_data_len=0;				                                  //�������¿�ʼ		
	}
	//SCB_CleanInvalidateDCache();        //�����Ч��DCache
}
