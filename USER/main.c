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
 ALIENTEK 阿波罗STM32H7开发板 网络实验9
 基于NETCONN API的UDP实验(UCOSII版本)-HAL库函数版
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/
//START任务
//任务优先级
#define START_TASK_PRIO		11
//任务堆栈大小
#define START_STK_SIZE		128
//任务堆栈
OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata); 

//在LCD上显示地址信息
//mode:1 显示DHCP获取到的地址
//	  其他 显示静态地址
void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"DHCP IP :%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"DHCP GW :%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,170,210,16,16,buf); 
	}
	else 
	{
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK :%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,170,210,16,16,buf); 
	}	
}

int main(void)
{
  Write_Through();                //开启强制透写！
  MPU_Memory_Protection();        //保护相关存储区域
  Cache_Enable();                 //打开L1-Cache
	HAL_Init();				        			//初始化HAL库
	Stm32_Clock_Init(180,5,2,4);    //设置时钟,400Mhz 
	delay_init(450);		        		//延时初始化
	uart_init(115200);			    		//串口初始化
	usmart_dev.init(200); 		    	//初始化USMART	
	LED_Init();					    				//初始化LED
	KEY_Init();					    				//初始化按键
	SDRAM_Init();                   //初始化SDRAM
	AT24CXX_Init();                 //初始化IIC
	//LCD_Init();				        		//初始化LCD
	//PCF8574_Init();               //初始化PCF8574
  my_mem_init(SRAMIN);		    		//初始化内部内存池
	my_mem_init(SRAMEX);		    		//初始化外部内存池
	my_mem_init(SRAMDTCM);		    	//初始化DTCM内存池
	POINT_COLOR = RED; 		          //红色字体
	//LCD_ShowString(30,50,200,20,16,"UDP NETCONN Test");  
	InitCamParaPacket();
	//申请内存
  netmem_malloc();
  OSInit(); 					    //UCOS初始化
	while(lwip_comm_init()) 	    //lwip初始化
	{
		//LCD_ShowString(30,110,200,20,16,"Lwip Init failed!"); 	//lwip初始化失败
		delay_ms(500);
		LCD_Fill(30,110,230,150,WHITE);
		delay_ms(500);
	}
	
  //LCD_ShowString(30,130,200,20,16,"Lwip Init Success!"); 		//lwip初始化成功
	//show_address(0);
	//创建网络接收完成信号量
	msg_netrecvok=OSSemCreate(0);	
	//创建发送准备好信号量
	msg_netsendready=OSSemCreate(0);	
	udp_demo_init(); 																						//初始化udp_demo 只负责UDP网络接收
	//LCD_ShowString(30,130,200,20,16,"UDP Success!       "); 		//UDP初始化成功
	
	OSTaskCreate(start_task,(void*)0,(OS_STK*)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO);
	OSStart(); //开启UCOS	
}

//start任务
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;	
	OSStatInit();  			//初始化统计任务
	OS_ENTER_CRITICAL();  	//关中断
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //创建DHCP任务
#endif
	OSTaskCreate(capcamera_task,(void*)0,(OS_STK*)&CAPCAMERA_TASK_STK[CAPCAMERA_STK_SIZE-1],CAPCAMERA_TASK_PRIO); //图像采集任务
	OSTaskCreate(imgcamera_task,(void*)0,(OS_STK*)&IMGCAMERA_TASK_STK[IMGCAMERA_STK_SIZE-1],IMGCAMERA_TASK_PRIO); //采集到数据处理任务																							
	OSTaskCreate(udp_send_task,(void*)0,(OS_STK*)&UDP_SEND_TASK_STK[UDP_SEND_STK_SIZE-1],UDP_SEND_TASK_PRIO);		//UDP数据发送任务
	OSTaskCreate(udp_dataprocess_task,(void*)0,(OS_STK*)&UDP_DATAPROCESS_TASK_STK[UDP_DATAPROCESS_STK_SIZE-1],UDP_DATAPROCESS_TASK_PRIO);	//UDP接收数据处理任务
	OSTaskSuspend(OS_PRIO_SELF); //挂起start_task任务
	OS_EXIT_CRITICAL();  		//开中断
}

//处理JPEG数据
//当采集完一帧JPEG数据后,调用此函数,切换JPEG BUF.开始下一帧采集.
void jpeg_data_process(void)
{
	u16 i;
	u16 rlen;			                                                    //剩余数据长度
	u32 *pbuf;
	if(jpeg_data_ok==0)	                                              //jpeg数据还未采集完?
	{
      __HAL_DMA_DISABLE(&DMADMCI_Handler);//关闭DMA
			rlen=NETCAM_LINE_SIZE-__HAL_DMA_GET_COUNTER(&DMADMCI_Handler);//得到剩余数据长度	   
			pbuf=imgbuf2592x480x32[currentcatch]+jpeg_data_len;//偏移到有效数据末尾,继续添加
			//if(DMADMCI_Handler.Instance->CR&(1<<19))for(i=0;i<rlen;i++)pbuf[i]=dcmi_line_buf[1][i];//读取buf1里面的剩余数据
		  SCB_CleanInvalidateDCache();        //清除无效化DCache
			//SCB_CleanDCache();
			//SCB_InvalidateDCache();
			if(DMA1_Stream1->CR&(1<<19))
			{
				for(i=0;i<rlen;i++)pbuf[i]=netcam_line_buf0[i];         //读取buf1里面的剩余数据
				//mymemcpy(pbuf,netcam_line_buf0,rlen<<2);
			}
			else 
			{
				for(i=0;i<rlen;i++)pbuf[i]=netcam_line_buf1[i];         //读取buf0里面的剩余数据 
				//mymemcpy(pbuf,netcam_line_buf1,rlen<<2);
			}
			jpeg_data_len+=rlen;			                                //加上剩余长度
			currentcatch++;
		  currentcatch%=3;
			jpeg_data_ok=1; 				                                  //标记JPEG数据采集完成,等待其他函数处理
	}
	if(jpeg_data_ok==2)	                                          //上一次的jpeg数据已经被处理了
	{
			//u8 buf[30];
			//sprintf((char*)buf,"Recv%d BYTE",(jpeg_data_len<<2));						//打印动态IP地址
			//LCD_ShowString(30,250,210,16,16,buf); 
      __HAL_DMA_SET_COUNTER(&DMADMCI_Handler,NETCAM_LINE_SIZE);	//传输长度为jpeg_buf_size*4字节
			__HAL_DMA_ENABLE(&DMADMCI_Handler);                       //打开DMA
			jpeg_data_ok=0;					                                  //标记数据未采集
			jpeg_data_len=0;				                                  //数据重新开始		
	}
	//SCB_CleanInvalidateDCache();        //清除无效化DCache
}
