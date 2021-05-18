#include "sccb.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F429开发板
//OV系列摄像头 SCCB 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2016/1/16
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

void SCCB_Delay(void)
{
	delay_us(1);
}

//初始化SCCB接口
void SCCB_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOB_CLK_ENABLE();           //使能GPIOB时钟
    
    //PB3.4初始化设置
    GPIO_Initure.Pin=GPIO_PIN_3|GPIO_PIN_4;
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;     //快速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure); 
}

//SCCB起始信号
//当时钟为高的时候,数据线的高到低,为SCCB起始信号
//在激活状态下,SDA和SCL均为低电平
void SCCB_Start(void)
{
    SCCB_SDA(1);     //数据线高电平	   
    SCCB_SCL(1);	    //在时钟线高的时候数据线由高至低
    SCCB_Delay();  
    SCCB_SDA(0);
    SCCB_Delay();	 
    SCCB_SCL(0);	    //数据线恢复低电平，单操作函数必要	    
}

//SCCB停止信号
//当时钟为高的时候,数据线的低到高,为SCCB停止信号
//空闲状况下,SDA,SCL均为高电平
void SCCB_Stop(void)
{
    SCCB_SDA(0);
    SCCB_Delay();	 
    SCCB_SCL(1);	
    SCCB_Delay(); 
    SCCB_SDA(1);	
    SCCB_Delay();
}  
//产生NA信号
void SCCB_No_Ack(void)
{
	SCCB_Delay();
	SCCB_SDA(1);	
	SCCB_SCL(1);	
	SCCB_Delay();
	SCCB_SCL(0);	
	SCCB_Delay();
	SCCB_SDA(0);	
	SCCB_Delay();
}

void SCCB_Ack(void)
{
		SCCB_Delay();
    SCCB_SDA(0);
    SCCB_SCL(1);
    SCCB_Delay();
    SCCB_SCL(0);
    SCCB_Delay(); 
    SCCB_SDA(1);
		SCCB_Delay();
}
//SCCB,写入一个字节
//返回值:0,成功;1,失败. 
u8 SCCB_WR_Byte(u8 dat)
{
	u8 j,res;	 
	for(j=0;j<8;j++) //循环8次发送数据
	{
		if(dat&0x80)SCCB_SDA(1);	
		else SCCB_SDA(0);
		dat<<=1;
		SCCB_Delay();
		SCCB_SCL(1);	
		SCCB_Delay();
		SCCB_SCL(0);		   
	}			 
	SCCB_SDA_IN();		//设置SDA为输入 
	SCCB_Delay();
	SCCB_SCL(1);			//接收第九位,以判断是否发送成功
	SCCB_Delay();
	if(SCCB_READ_SDA)res=1;  //SDA=1发送失败，返回1
	else res=0;         //SDA=0发送成功，返回0
	SCCB_SCL(0);		 
	SCCB_SDA_OUT();		//设置SDA为输出    
	return res;  
}	 
//SCCB 读取一个字节
//在SCL的上升沿,数据锁存
//返回值:读到的数据
u8 SCCB_RD_Byte(void)
{
	u8 temp=0,j;    
	SCCB_SDA_IN();		//设置SDA为输入  
	for(j=8;j>0;j--) 	//循环8次接收数据
	{		     	  
		SCCB_Delay();
		SCCB_SCL(1);
		temp=temp<<1;
		if(SCCB_READ_SDA)temp++;   
		SCCB_Delay();
		SCCB_SCL(0);
	}	
	SCCB_SDA_OUT();		//设置SDA为输出    
	return temp;
} 

u16 SCCB_RD_Byte2(void)
{
	u16 temp=0,j;    
	SCCB_SDA_IN();		//设置SDA为输入  
	for(j=16;j>0;j--) 	//循环8次接收数据
	{		     	  
		SCCB_Delay();
		SCCB_SCL(1);
		temp=temp<<1;
		if(SCCB_READ_SDA)temp++;   
		SCCB_Delay();
		SCCB_SCL(0);
	}	
	SCCB_SDA_OUT();		//设置SDA为输出    
	return temp;
} 				
