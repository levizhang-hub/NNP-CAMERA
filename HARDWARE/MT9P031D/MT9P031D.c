#include "sys.h"
#include "MT9P031D.h"
#include "timer.h"	  
#include "delay.h"
#include "usart.h"			 
#include "sccb.h"	
#include "pcf8574.h"  
#include "ltdc.h" 
#include "udp_demo.h"
#include "dcmi.h"
#include "ltdc.h"
#include "lcd.h"
#include "lwip/api.h"
#include "malloc.h"
#include "24CXX.h"
volatile u32 jpeg_data_len=0; 		      //buf�е�JPEG��Ч���ݳ��� 
volatile u8 jpeg_data_ok=0;				      //JPEG���ݲɼ���ɱ�־ 
vu8         jpeg_data_send=0;           //�Ƿ���Ҫͨ�����緢��ͼ������
vu8         jpeg_data_pcsend_flag=0;    //PC��ͼ��־
volatile u8 DirectShow=0;               //ֱ������ʾ������ʾͼ��
//�ɼ������ջ	
OS_STK CAPCAMERA_TASK_STK[CAPCAMERA_STK_SIZE];	
//���������ջ
OS_STK IMGCAMERA_TASK_STK[IMGCAMERA_STK_SIZE];
u8  cmos_rset( u8 regnum, u16 regval )
{
	u8 res=0;
	SCCB_Start(); 														//����SCCB����
	if(SCCB_WR_Byte(MT9P031_I2C_ADDR))res=1;	//д����ID	  
   	if(SCCB_WR_Byte(regnum))res=1;					//д�Ĵ�����8λ��ַ
   	if(SCCB_WR_Byte(regval>>8))res=1;				//д�Ĵ�����8λ��ַ		  
   	if(SCCB_WR_Byte(regval))res=1; 					//д����	 
  	SCCB_Stop();	  
  return	res;
}

u16 cmos_rget( u16 regnum )
{
		u16 val=0;
	  u16 val1=0;
		SCCB_Start(); 				//����SCCB����
		SCCB_WR_Byte(MT9P031_I2C_ADDR);	//д����ID
		//SCCB_No_Ack();
   	//SCCB_WR_Byte(regnum>>8);	    //д�Ĵ�����8λ��ַ  
		//SCCB_No_Ack();	
  	SCCB_WR_Byte(regnum);			//д�Ĵ�����8λ��ַ	  
		//SCCB_No_Ack();	   
		//���üĴ�����ַ�󣬲��Ƕ�
		SCCB_Start();
		SCCB_WR_Byte(MT9P031_I2C_ADDR| 0x01);//���Ͷ�����	 
		//SCCB_No_Ack();	
   	val=SCCB_RD_Byte();		 	//��ȡ����
  	//SCCB_No_Ack();
		SCCB_Ack();
		//SCCB_Stop(); 
		//SCCB_Start();
	  val1=SCCB_RD_Byte();
		val=(val1+(val<<8));	
		SCCB_No_Ack();
  	SCCB_Stop();
  	return (val);
}

//������������ṹ���ʼ��MT9P031D������
u16 cmos_init(void)
{
#if 1
	cmos_rset( MT9P031_OUTPUT_CTRL, MT9P031_OUTPUT_CTRL_DEFAULT );
	cmos_rset( MT9P031_PIXEL_CLK_CTRL, MT9P031_PIXEL_CLK_CTRL_DEFAULT );
	cmos_rset( MT9P031_RESTART, MT9P031_RESTART_DEFAULT );
	cmos_rset( MT9P031_SHUTTER_DELAY, MT9P031_SHUTTER_DELAY_DEFAULT );
	cmos_rset( MT9P031_READ_MODE1, MT9P031_READ_MODE1_DEFAULT );
	cmos_rset( MT9P031_READ_MODE2, MT9P031_READ_MODE2_DEFAULT );
	cmos_rset( MT9P031_READ_MODE3, MT9P031_READ_MODE3_DEFAULT );
	cmos_rset( MT9P031_GREEN1_GAIN, MT9P031_GREEN1_GAIN_DEFAULT );
	cmos_rset( MT9P031_BLUE_GAIN, MT9P031_BLUE_GAIN_DEFAULT );
	cmos_rset( MT9P031_RED_GAIN, MT9P031_RED_GAIN_DEFAULT );
	cmos_rset( MT9P031_GREEN2_GAIN, MT9P031_GREEN2_GAIN_DEFAULT );
	cmos_rset( MT9P031_GLOBAL_GAIN, MT9P031_GLOBAL_GAIN_DEFAULT );
	cmos_rset( MT9P031_BLACK_LEVEL, MT9P031_BLACK_LEVEL_DEFAULT );
	cmos_rset( MT9P031_CHIP_ENABLE_SYNC, MT9P031_CHIP_ENABLE_SYNC_DEFAULT );
	cmos_rset( MT9P031_RESET, MT9P031_RESET_ENABLE );
	cmos_rset( MT9P031_RESET, MT9P031_RESET_DISABLE );
	cmos_rset( MT9P031_OUTPUT_CTRL, MT9P031_HALT_MODE );
	cmos_rset( MT9P031_OUTPUT_CTRL, MT9P031_NORMAL_OPERATION_MODE );
	//cmos_rset( 0x10, 0x03 );
#endif
	return 1;
}

//�����������
u16 cmos_set_Gain(u16 GainVal)
{
	u16 val;
	val=cmos_rget(MT9P031_GLOBAL_GAIN);
	cmos_rset( MT9P031_GLOBAL_GAIN,GainVal );
	delay_ms(5);
	val=cmos_rget(MT9P031_GLOBAL_GAIN);
	while(val!=GainVal)
	{
		cmos_rset( MT9P031_GLOBAL_GAIN,GainVal );
		delay_ms(5);
		val=cmos_rget(MT9P031_GLOBAL_GAIN);
	}

	return val;
}
//���ÿ���ʱ��
u16 cmos_set_Shutter(u32 ShutterVal)
{
	u16 val=0;
	cmos_rset( MT9P031_SHUTTER_WIDTH, (u16)((ShutterVal) & MT9P031_SHUTTER_WIDTH_LOWER_MASK ));
	delay_ms(5);
	val=cmos_rget( MT9P031_SHUTTER_WIDTH );
	while(val!=(u16)(ShutterVal))
	{
		cmos_rset( MT9P031_SHUTTER_WIDTH, (u16)((ShutterVal) & MT9P031_SHUTTER_WIDTH_LOWER_MASK ));
		delay_ms(5);
		val=cmos_rget( MT9P031_SHUTTER_WIDTH );
	}

	cmos_rset( MT9P031_SHUTTER_WIDTH_UPPER, (u16)((ShutterVal)>> MT9P031_SHUTTER_WIDTH_UPPER_SHIFT));
	delay_ms(5);
	val=cmos_rget( MT9P031_SHUTTER_WIDTH_UPPER );
	while(val!=(u16)((ShutterVal)>>16))
	{
		cmos_rset( MT9P031_SHUTTER_WIDTH_UPPER, (u16)((ShutterVal)>> MT9P031_SHUTTER_WIDTH_UPPER_SHIFT));
		delay_ms(5);
		val=cmos_rget( MT9P031_SHUTTER_WIDTH_UPPER );
	}

	return 1;
}

//����aoi
//700,480,800,800
u16 cmos_set_Aoi(u16 Y_start,u16 Y_size,u16 X_Start,u16 X_Size)
{
	u16 val=0;
	cmos_rset(MT9P031_ROW_START,(Y_start+54));
	delay_ms(5);
	val=cmos_rget(MT9P031_ROW_START);
	while(val!=(Y_start+54))
	{
		cmos_rset(MT9P031_ROW_START,(Y_start+54));
		delay_ms(5);
		val=cmos_rget(MT9P031_ROW_START);
	}
	
	cmos_rset(MT9P031_HEIGHT,Y_size);
	delay_ms(5);
	val=cmos_rget(MT9P031_HEIGHT);
	while(val!=(Y_size))
	{
		cmos_rset(MT9P031_HEIGHT,Y_size);
		delay_ms(5);
		val=cmos_rget(MT9P031_HEIGHT);
	}
	
	//x
	cmos_rset(MT9P031_COL_START,(X_Start+16));
	delay_ms(5);
	val=cmos_rget(MT9P031_COL_START);
	while(val!=(X_Start+16))
	{
		cmos_rset(MT9P031_COL_START,(X_Start+16));
		delay_ms(5);
		val=cmos_rget(MT9P031_COL_START);
	}
	
	cmos_rset(MT9P031_WIDTH,X_Size);
	delay_ms(5);
	val=cmos_rget(MT9P031_WIDTH);
	while(val!=(X_Size))
	{
		cmos_rset(MT9P031_WIDTH,X_Size);
		delay_ms(5);
		val=cmos_rget(MT9P031_WIDTH);
	}
	return val;
}


//��ʼ��OV5640 
//�������Ժ�,Ĭ�������1600*1200�ߴ��ͼƬ!! 
//����ֵ:0,�ɹ�
//����,�������
u8 MT9P031_Init(void)
{ 
	u16 reg;
	//����IO     	
	GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_GPIOI_CLK_ENABLE();			//����GPIOIʱ��
  GPIO_Initure.Pin=GPIO_PIN_1;           //PI1
  GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //�������
  GPIO_Initure.Pull=GPIO_PULLUP;          //����
  GPIO_Initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;     //����
  HAL_GPIO_Init(GPIOI,&GPIO_Initure);     //��ʼ��
	
	//PCF8574_Init();												//��ʼ��PCF8574
	MT9P031D_RST(1);													//����������OV5640��RST��,���ϵ�
	delay_ms(5);  
	MT9P031D_RST(0);													//������λ 
	delay_ms(20);      
	SCCB_Init();														//��ʼ��SCCB ��IO�� 
	delay_ms(5); 
	
	while(reg!=MT9P031_ID)
	{
		reg=cmos_rget(0X00);
		delay_ms(10);
	}
	if(reg!=MT9P031_ID)
	{
		printf("ID:%d\r\n",reg);
		return 1;
	}  
	MT9P031D_RST(1);			//����������OV5640��RST��,���ϵ�
	delay_ms(20); 
	MT9P031D_RST(0);			//������λ 
	delay_ms(10);		
	cmos_init();
	//��ȱ���Ϊ�������������ݶ���32���ֽ�
	cmos_set_Aoi((CurrentCamPara->aoistartyh<<8)+CurrentCamPara->aoistartyl-240,479,0,2591);
	cmos_set_Gain(9);  
  cmos_set_Shutter((CurrentCamPara->exposureh<<8)+CurrentCamPara->exposurel);
	
	/*delay_ms(5);	
	cmos_rset(0xa0,0x21);
	delay_ms(5);	
	cmos_rset(0xa1,1);
	delay_ms(5);	
	cmos_rset(0xa2,1);
	delay_ms(5);	
	cmos_rset(0xa3,1);
	delay_ms(5);	
	
	cmos_rset(0xa4,1001);
	*/
//GRRS mode
#if 0
	cmos_rset(0x30,1);
	delay_ms(20); 
	Mode1Reg=cmos_rget(0x01E);
	Mode1Reg=(Mode1Reg |(1<<7));
	Mode1Reg=(Mode1Reg | (1<<8));
	delay_ms(20); 
	cmos_rset(0x01E,Mode1Reg);
	delay_ms(20); 
	Mode1Reg=cmos_rget(0x30);
#endif
	return 0x00; 	//ok
} 

u16 rgb_24_2_565(int r, int g, int b)  
{  
    return (u16)((((unsigned)(r) << 8) & 0xF800) |   (((unsigned)(g) << 3) & 0x7E0)  |  (((unsigned)(b) >> 3)));  
}  

void bayer2rgb24_bingo(unsigned short *dst, unsigned char *src, long width, long height)
{
	unsigned char	 *bayer;
	unsigned short *image;
	int y=0;
	int x=0;
	unsigned char	red 	= 0,
					green	= 0,
					blue 	= 0;

	bayer = src;					//raw
	image = dst;					//rgb24
	
	for(y = 0; y < height; y++ )
	{ 
	
		for(x = 0; x < width; x++ )
		{	
			if( x == 0 )	
			{
				//������
				if( y == 0 )				//(0,0)?
				{
					//������
					red 	=  bayer[x + 1];
					green 	= ((bayer[x] + bayer[x+1 + (y+1)*width])>>1);
					blue 	=  bayer[(y+1)*width];
				}
				else if( y == height - 1 )		//(width,0)?
				{
					//���һ��
					red 	= bayer[x+1 + (y-1)*width];
					green 	= ((bayer[(x+1)+ y*width]+bayer[x+ (y-1)*width])>>1);
					blue 	= bayer[x 	+ y*width];
				}
				else if( (y % 2) == 1 )		//?????
				{
					//������
					red 	=  bayer[x+1+ (y+1)*width];
					green 	=  ((bayer[x+1 	+ y*width]+ bayer[x 	+ (y+1)*width])>>1);
					blue 	= bayer[x 	+ (y)*width];
				}
				else if( (y % 2) == 0 )		//?????
				{
					//ż����
					red 	= (bayer[x+1+ (y)*width]);
					green 	= ((bayer[x+1 	+ (y+1)*width] + bayer[x 	+ (y)*width] )>>1);
					blue 	=  bayer[x 	+  (y+1) *width];
				}
			}
///////////////////////////////////////////////////////////////////////////////////////////////////////
			else if( x == (width-1) )
			{
				//���һ��
				if( y == 0 )				//(0,0)?
				{
					//��һ��
					red 	=  bayer[x 	+ (y)*width];
					green 	= ((bayer[x	+ (y+1)*width]+bayer[x-1+y*width])>>1) ;
					blue 	=  bayer[x-1 + (y+1)*width];
				}
				else if( y == height - 1)		//(height,0)?
				{
					//���һ��
					red 	= bayer[x	+ (y-1)*width];
					green 	=((bayer[x 	+ (y)*width] + bayer[x-1 	+ (y-1)*width])>>1);
					blue 	= bayer[x-1 + (y-1)*width];
				}
				else if( (y % 2) == 1 )		//????
				{
					//������
					red 	=  bayer[x 	+ 	(y+1)*width];
					green 	= ((bayer[x 	+ 	(y)*width] + bayer[x-1 	+ 	(y+1)*width])>>1);
					blue 	= (bayer[x-1 	+ (y)*width]);
				}
				else if( (y % 2) == 0 )		//????
				{
					//ż����
					red 	= (bayer[x 	+ (y)*width]);
					green 	= (( bayer[x 	+  (y+1)*width]+bayer[x-1	+  (y)*width])>>1);
					blue 	=  bayer[x-1 	+  (y+1) *width];
				}
			}

////////////////////////////////////////////////////////////////////////////////////////////
			else if( x % 2 == 1)				//???
			{
				//������
				if( y == 0 )				//?????
				{
					//��һ��
					red		=  bayer[x + (y)*width];
					green 	=  ((bayer[x + (y+1)*width]+bayer[x-1+ y*width])>>1);
					blue 	= (bayer[x-1 + (y+1)*width]);
				}
				else if( y == height - 1)		//(height,0)?
				{
					//���һ��
					red 	=  bayer[x  + (y-1)*width];
					green 	= ((bayer[x 	+ (y)*width] + bayer[x-1 + (y-1)*width])>>1);
					blue 	= (bayer[x-1 + (y)*width]);
				}

				else if( (y % 2) == 1 )		//???
				{
					//������
					red 	=  bayer[x  + (y+1)*width];
					green 	= ((bayer[x 	+ (y)*width] + bayer[x-1 + (y+1)*width])>>1);
					blue 	= (bayer[x-1 + (y)*width]);	
				}
				else if( (y % 2) == 0 )		//???
				{
					//ż����
					red 	= (bayer[x	 + (y)*width]);
					green 	= ((bayer[x 	+ (y+1)*width] + bayer[x-1 + (y)*width])>>1);
					blue 	= (bayer[x-1 +  (y+1) *width]);
				}
			}
////////////////////////////////////////////////////////////////////////////////////////////
			else if ((x % 2) == 0)			//???
			{
				//ż����
				if( y == 0 )				//?????
				{
					//��һ��
					red 	=  bayer[x-1  + (y)*width];
					green 	= ((bayer[x 	+ (y)*width] + bayer[x-1 + (y+1)*width])>>1);
					blue 	= (bayer[x + (y+1)*width]);	
				}
				else if( y == height -1 )		//(width,0)?
				{
					//���һ��
					red 	=  bayer[x-1  + (y-1)*width];
					green 	= ((bayer[x 	+ (y-1)*width] + bayer[x-1 + (y)*width])>>1);
					blue 	= (bayer[x + (y)*width]);	
				}

				else if( (y % 2) == 0 )		//???
				{
					//ż����
					red 	=  bayer[x-1  + (y)*width];
					green 	= ((bayer[x 	+ (y)*width] + bayer[x-1 + (y+1)*width])>>1);
					blue 	= (bayer[x + (y+1)*width]);						
				}
				else if( (y % 2) == 1 )		//???
				{
					//������
					red 	=  bayer[x-1  + (y+1)*width];
					green 	= ((bayer[x 	+ (y+1)*width] + bayer[x-1 + (y)*width])>>1);
					blue 	= (bayer[x + (y)*width]);	
				}
			}
			///////////////////////////////////////////////////////////////////////////////////////////////////////
			image[y*width+x]=rgb_24_2_565(red,green,blue);

		}		
	}
}

void gray2rgb565_bingo(unsigned short *dst, unsigned char *src, long width, long height)
{
	u8	 *bayer;
	u8   *p;
	u16  *q;
	u16 *image;
	int y=0;
	int x=0;
	//u8	red 	= 0;
	//u8	green	= 0;
	//u8	blue 	= 0;
	bayer = src;					//raw
	image = dst;					//rgb565
	
	for(y = 0; y < height; y++)
	{ 
		p=(u8*)(&bayer[y*width]);
		q=(u16*)(&image[y*width]);
		
		for(x = 0; x < width; x++ )
		{	
			//red=(bayer[x + y*width]>>3);
			//green=(bayer[x + y*width]>>2);
			//blue=(bayer[x + y*width]>>3);
			//image[y*width+x]=rgb_24_2_565(red,green,blue);
			*(q+x)=GRAYTORGB16(*(p+x));
		}
	}
}


//��������ͷ�ɼ�������
void capcamera_task(void *pdata)
{
	camera_init();
  delay_ms(1000);              //����ʱһ��Ҫ�ӣ��
	DCMI_Start(); 		           //��������
	OSTaskSuspend(OS_PRIO_SELF); //����start_task����
}

//��������ͷ����������
void imgcamera_task(void *pdata)
{
	err_t     send_err; 
	u8 *ImageBuf;
	static struct netbuf  *sentbuf;
	u8 *p;
	vu8 GetCatchPos=0;
	while(1)
	{	
			if(jpeg_data_ok==1)	//�Ѿ��ɼ���һ֡ͼ����
			{ 
				jpeg_data_ok=2;		
			}			
			//ͼͼ����
			if(jpeg_data_send==1)
			{
				//ͼ����
				if(currentcatch==0)
					GetCatchPos=2;
				else if(currentcatch==1)
					GetCatchPos=0;
				else if(currentcatch==2)
					GetCatchPos=1;
					
				p=(u8*)imgbuf2592x480x32[GetCatchPos]+32+StartY240*2592;
				if(jpeg_data_pcsend_flag==0)
				{
					//��λ��Ҫ�������ͼ
					ImageBuf=(u8*)p;
					if(udp_flag==1)
					{
						u16 i=0;
						sentbuf = netbuf_new();
						netbuf_alloc(sentbuf,UDPIMAGESIZE);
						for(i=0;i<UDPIMAGEDC;i++)
						{
							mymemcpy(sentbuf->p->payload,ImageBuf+i*UDPIMAGESIZE,UDPIMAGESIZE);
							send_err = netconn_send(udpconn,sentbuf);  	//��netbuf�е����ݷ��ͳ�ȥ
							if((send_err==ERR_CLSD)||(send_err==ERR_RST))//�ر�����,������������ 
							{
								udp_flag=0;
							}	 
						}
						netbuf_delete(sentbuf);      	//ɾ��sentbuf	
						//����һ���̰�����һ֡����
						sentbuf = netbuf_new();
						netbuf_alloc(sentbuf,23);
						//�̰��Ļ�����80���ֽڷֱ�Ϊ5a,5a,a5,a5��W���ֽڣ�W���ֽڣ�H���ֽڣ�H���ֽ�
						//1024�����Դ��룩 0x01 0x02 0x03 0x04(��ǰ֡��)  ����汾�� ����ʱ���  ����ʱ���   cmos��ַ  ��˾����  c3 c2 c1 c0
						//u8          UdpShort[23]={0x5a,0x5a,0xa5,0xa5,(u8)(2592>>8),(u8)(2592),(u8)(480>>8),(u8)(480),(u8)(1024>>8),\
													(u8)(1024),0x01,0x02,0x03,0x04,10,(u8)(300>>8),(u8)(300),0xBA,01,\
                          0xc3,0xc2,0xc1,0xc0};                   //udp ����̰�
						UdpShort[10]=(u8)(ov_frame>>24);
						UdpShort[11]=(u8)(ov_frame>>16);
						UdpShort[12]=(u8)(ov_frame>>8);
						UdpShort[13]=(u8)(ov_frame);
						UdpShort[15]=CurrentCamPara->exposureh;
						UdpShort[16]=CurrentCamPara->exposurel;
						
						mymemcpy(sentbuf->p->payload,UdpShort,23);
						send_err = netconn_send(udpconn,sentbuf);  	//��netbuf�е����ݷ��ͳ�ȥ
						if((send_err==ERR_CLSD)||(send_err==ERR_RST))//�ر�����,������������ 
						{
							udp_flag=0;
						}	 
						netbuf_delete(sentbuf);      	//ɾ��sentbuf		
					}
					jpeg_data_send=0;
				}
				else if(jpeg_data_pcsend_flag==1)
				{
					//PCҪ�������ͼ,�ֳ�һ֡ͼ�����һ֡�̰����̰��Ļ�����80���ֽڷֱ�Ϊ5a,5a,a5,a5��W���ֽڣ�W���ֽڣ�H���ֽڣ�H���ֽ�
					//1024�����Դ��룩 0x01 0x02 0x03 0x04  c3 c2 c1 c0
					u16 pPos=0;
					ImageBuf=(u8*)imgbuf2592x480x32[GetCatchPos]+32;
					
					if(udp_flag==1)
					{
						u16 i=0;	
						for(i=0;i<UDPIAMGEVC;i++)
						{
							pPos=i;
							sentbuf = netbuf_new();
						  netbuf_alloc(sentbuf,UDPIMAGESIZE+4);
							//SCB_InvalidateDCache();
							mymemcpy((u8*)(sentbuf->p->payload)+4,ImageBuf+i*UDPIMAGESIZE,UDPIMAGESIZE);
							*((u8*)sentbuf->p->payload)=(u8)(i>>8);
							*((u8*)(sentbuf->p->payload)+1)=(u8)(i);
							
							*((u8*)(sentbuf->p->payload)+2)=(u8)(CurrentCamPara->id>>8);
							*((u8*)(sentbuf->p->payload)+3)=(u8)(CurrentCamPara->id);
							
							send_err = netconn_send(udpconn,sentbuf);  	//��netbuf�е����ݷ��ͳ�ȥ
							if((send_err==ERR_CLSD)||(send_err==ERR_RST))//�ر�����,������������ 
							{
								udp_flag=0;
							 }	
							 netbuf_delete(sentbuf);      	//ɾ��sentbuf	
						   delay_us(50);
						 }
						
						delay_ms(5);
						//�ٷ���һ���̰�����һ֡����
						sentbuf = netbuf_new();
						netbuf_alloc(sentbuf,23);
						//�̰��Ļ�����80���ֽڷֱ�Ϊ5a,5a,a5,a5��W���ֽڣ�W���ֽڣ�H���ֽڣ�H���ֽ�
						//1024�����Դ��룩 0x01 0x02 0x03 0x04(��ǰ֡��)  ����汾�� ����ʱ���  ����ʱ���  ����  cmos��ַ  ��˾����  c3 c2 c1 c0
						//u8          UdpShort[23]={0x5a,0x5a,0xa5,0xa5,(u8)(2592>>8),(u8)(2592),(u8)(480>>8),(u8)(480),(u8)(1024>>8),\
													(u8)(1024),0x01,0x02,0x03,0x04,10,(u8)(300>>8),(u8)(300),0xBA,01,\
                          0xc3,0xc2,0xc1,0xc0};                   //udp ����̰�
						UdpShort[10]=(u8)(ov_frame>>24);
						UdpShort[11]=(u8)(ov_frame>>16);
						UdpShort[12]=(u8)(ov_frame>>8);
						UdpShort[13]=(u8)(ov_frame);
						UdpShort[15]=CurrentCamPara->exposureh;
						UdpShort[16]=CurrentCamPara->exposurel;
						mymemcpy(sentbuf->p->payload,UdpShort,23);
						send_err = netconn_send(udpconn,sentbuf);  	//��netbuf�е����ݷ��ͳ�ȥ
						if((send_err==ERR_CLSD)||(send_err==ERR_RST))//�ر�����,������������ 
						{
							udp_flag=0;
						}	 
						netbuf_delete(sentbuf);      	//ɾ��sentbuf						  
					}
					jpeg_data_send=0;					
				}else if(jpeg_data_pcsend_flag==3)
				{
					//ֻ�Ǵ�����AOI�������ڵ�ͼ������
					u16 pPos=0;
					ImageBuf=(u8*)imgbuf2592x480x32[GetCatchPos]+32;
					
					if(udp_flag==1)
					{
						u16 i=0;	
						u16 UdpPacketCount=(CurrentCamPara->aoishowheight<<1);
						u16 UdpDetectYStartPacketIndex=(((CurrentCamPara->aoishowstartyh<<8)+(CurrentCamPara->aoishowstartyl))<<1);
						for(i=0;i<UdpPacketCount;i++)
						{
							pPos=i;
							sentbuf = netbuf_new();
						  netbuf_alloc(sentbuf,UDPIMAGESIZE+4);
							//SCB_InvalidateDCache();
							mymemcpy((u8*)(sentbuf->p->payload)+4,ImageBuf+(UdpDetectYStartPacketIndex+i)*UDPIMAGESIZE,UDPIMAGESIZE);
							*((u8*)sentbuf->p->payload)=(u8)((i+UdpDetectYStartPacketIndex)>>8);
							*((u8*)(sentbuf->p->payload)+1)=(u8)(i+UdpDetectYStartPacketIndex);
							
							*((u8*)(sentbuf->p->payload)+2)=(u8)(CurrentCamPara->id>>8);
							*((u8*)(sentbuf->p->payload)+3)=(u8)(CurrentCamPara->id);
							
							send_err = netconn_send(udpconn,sentbuf);  	 //��netbuf�е����ݷ��ͳ�ȥ
							if((send_err==ERR_CLSD)||(send_err==ERR_RST))//�ر�����,������������ 
							{
								udp_flag=0;
							 }	
							 netbuf_delete(sentbuf);      	             //ɾ��sentbuf	
						   delay_us(50);
						 }
						
						delay_ms(5);
						//�ٷ���һ���̰�����һ֡����
						sentbuf = netbuf_new();
						netbuf_alloc(sentbuf,23);
						//�̰��Ļ�����80���ֽڷֱ�Ϊ5a,5a,a5,a5��W���ֽڣ�W���ֽڣ�H���ֽڣ�H���ֽ�
						//1024�����Դ��룩 0x01 0x02 0x03 0x04(��ǰ֡��)  ����汾�� ����ʱ���  ����ʱ���  ����  cmos��ַ  ��˾����  c3 c2 c1 c0
						//u8          UdpShort[23]={0x5a,0x5a,0xa5,0xa5,(u8)(2592>>8),(u8)(2592),(u8)(480>>8),(u8)(480),(u8)(1024>>8),\
													(u8)(1024),0x01,0x02,0x03,0x04,10,(u8)(300>>8),(u8)(300),0xBA,01,\
                          0xc3,0xc2,0xc1,0xc0};                   //udp ����̰�
						UdpShort[10]=(u8)(ov_frame>>24);
						UdpShort[11]=(u8)(ov_frame>>16);
						UdpShort[12]=(u8)(ov_frame>>8);
						UdpShort[13]=(u8)(ov_frame);
						UdpShort[15]=CurrentCamPara->exposureh;
						UdpShort[16]=CurrentCamPara->exposurel;
						mymemcpy(sentbuf->p->payload,UdpShort,23);
						send_err = netconn_send(udpconn,sentbuf);  	//��netbuf�е����ݷ��ͳ�ȥ
						if((send_err==ERR_CLSD)||(send_err==ERR_RST))//�ر�����,������������ 
						{
							udp_flag=0;
						}	 
						netbuf_delete(sentbuf);      	//ɾ��sentbuf						  
					}
					jpeg_data_send=0;					
				}
			}	
 	}
}








