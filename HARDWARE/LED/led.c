#include "led.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32H7������
//LED��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2017/6/8
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

//��ʼ��PB0,PB1Ϊ���.��ʹ���������ڵ�ʱ��		    
//LED IO��ʼ��
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOB_CLK_ENABLE();					//����GPIOBʱ��
		__HAL_RCC_GPIOH_CLK_ENABLE();					//����GPIOBʱ��
	
		__HAL_RCC_GPIOD_CLK_ENABLE();					//����GPIOBʱ��
	 
    GPIO_Initure.Pin=GPIO_PIN_1;			//PB0��1
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  		//�������
    GPIO_Initure.Pull=GPIO_PULLUP;         			//����
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;  	//����
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);     		//��ʼ����GPIOB.1
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);	//PB1��1 
	
	
	 //�ɼ�ָʾ��PD13
		GPIO_Initure.Pin=GPIO_PIN_13;			          
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  		//�������
    GPIO_Initure.Pull=GPIO_PULLUP;         			//����
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;  	//����
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);     		//��ʼ��GPIOH.4
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13,GPIO_PIN_SET);	//PH4��1 
	
	 /*HAL_RCC_MCOConfig(RCC_MCO1,RCC_MCO1SOURCE_HSE,RCC_MCODIV_1);//hsi,PA??????,??
		GPIO_Initure.Pin = GPIO_PIN_8; 
		GPIO_Initure.Speed = GPIO_SPEED_FREQ_VERY_HIGH; 
		GPIO_Initure.Mode = GPIO_MODE_AF_PP ; 
		HAL_GPIO_Init(GPIOA, &GPIO_Initure);*/
		
		//���縴λPD12	
		GPIO_Initure.Pin=GPIO_PIN_12;               //PH11
		GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  		//�������
    GPIO_Initure.Pull=GPIO_PULLUP;         			//����
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;  	//����
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);         //ʼ��
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12,GPIO_PIN_RESET);
}

