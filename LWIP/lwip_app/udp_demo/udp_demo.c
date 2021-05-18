#include "udp_demo.h"
#include "lwip_comm.h"
#include "usart.h"
#include "led.h"
#include "includes.h"
#include "lwip/api.h"
#include "lwip/lwip_sys.h"
#include "string.h"
#include "malloc.h"
#include "MT9P031D.h"
#include "dcmi.h"
#include "24CXX.h"
#include "delay.h"
#include "led.h"
#include "lwip/tcp.h"
#include "lwip/ip.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F4&F7&H7开发板
//NETCONN API编程方式的UDP测试代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2016/8/5
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//*******************************************************************************
//修改信息
//无
////////////////////////////////////////////////////////////////////////////////// 	   
 
//UDP客户端任务
#define UDP_PRIO		6
//任务堆栈大小
#define UDP_STK_SIZE	300
//任务堆栈
OS_STK UDP_TASK_STK[UDP_STK_SIZE];


//UDP发送任务
OS_STK UDP_SEND_TASK_STK[UDP_SEND_STK_SIZE];	

//UDP接收数据处理任务
OS_STK UDP_DATAPROCESS_TASK_STK[UDP_DATAPROCESS_STK_SIZE];	
u8 udp_flag;															//UDP数据发送标志位
u32 *netcam_line_buf0;					    			//定义行缓存0  
u32 *netcam_line_buf1;					    			//定义行缓存1   

//NET SEND FIFO
u8*  netsend_line_buf;
vu16 netsendfifordpos=0;					    		//FIFO读位置
vu16 netsendfifowrpos=0;					    		//FIFO写位置
u8 *netsendfifobuf[NETSEND_FIFO_NUM];	    //定义NETSEND_FIFO_SIZE个发送FIFO	

//NET RECV FIFO
u8*  netrecv_line_buf;
vu16 netrecvfifordpos=0;					    		//接收FIFO读位置
vu16 netrecvfifowrpos=0;					    		//接收FIFO写位置
u8 *netrecvfifobuf[NETRECV_FIFO_NUM];	    //定义NETSEND_FIFO_SIZE个接收FIFO	
volatile SendPacket* CurrentCamPara;
//网络接收完成信号量
OS_EVENT * msg_netrecvok;
//网络发送准备好信号量			
OS_EVENT * msg_netsendready;

vu8         currentcatch=0;
vu8 				CamVer=10;
u32* 				imgbuf2592x480x32[3];
vu8         StartY240=120;                  //240行起点占480行中的位置
struct netconn *udpconn;					          //udp CLIENT网络连接结构体
//短包的话就是80个字节分别为5a,5a,a5,a5，W高字节，W低字节，H高字节，H低字节
//1024（调试代码） 0x01 0x02 0x03 0x04(当前帧号)  相机版本号 积分时间高  积分时间低  增益  cmos地址  公司代码  c3 c2 c1 c0
u8          UdpShort[23]={0x5a,0x5a,0xa5,0xa5,(u8)(2592>>8),(u8)(2592),(u8)(480>>8),(u8)(480),(u8)(1024>>8),\
													(u8)(1024),0x01,0x02,0x03,0x04,10,(u8)(300>>8),(u8)(300),0xBA,01,\
                          0xc3,0xc2,0xc1,0xc0};                   //udp 网络短包

//读取FIFO
//buf:数据缓存区首地址
//返回值:0,没有数据可读;
//      1,读到了1个数据块
u8 netsend_fifo_read(u8 **buf)
{
	if(netsendfifordpos==netsendfifowrpos)return 0;
	netsendfifordpos++;		//读位置加1
	if(netsendfifordpos>=NETSEND_FIFO_NUM)netsendfifordpos=0;//归零 
	*buf=netsendfifobuf[netsendfifordpos];
	return 1;
}
//写一个FIFO
//buf:数据缓存区首地址
//返回值:0,写入成功;
//       1,写入失败
u8 netsend_fifo_write(u8 *buf)
{
	u16 i;
	u16 temp=netsendfifowrpos;																									//记录当前写位置
	netsendfifowrpos++;																			 										//写位置加1
	if(netsendfifowrpos>=NETSEND_FIFO_NUM)netsendfifowrpos=0;										//归零  
	if(netsendfifordpos==netsendfifowrpos)
	{
		netsendfifowrpos=temp;																										//还原原来的写位置,此次写入失败
		return 1;	
	}
	for(i=0;i<NETSEND_LINE_SIZE;i++)netsendfifobuf[netsendfifowrpos][i]=buf[i];	//拷贝数据
	return 0;
}   

//读取接收FIFO
//buf:数据缓存区首地址
//返回值:0,没有数据可读;
//      1,读到了1个数据块
u8 netrecv_fifo_read(u8 **buf)
{
	if(netrecvfifordpos==netrecvfifowrpos)return 0;
	netrecvfifordpos++;		//读位置加1
	if(netrecvfifordpos>=NETRECV_FIFO_NUM)netrecvfifordpos=0;//归零 
	*buf=netrecvfifobuf[netrecvfifordpos];
	return 1;
}
//写一个FIFO
//buf:数据缓存区首地址
//返回值:0,写入成功;
//       1,写入失败
u8 netrecv_fifo_write(u8 *buf)
{
	u16 i;
	u16 temp=netrecvfifowrpos;																									//记录当前写位置
	netrecvfifowrpos++;																			 										//写位置加1
	if(netrecvfifowrpos>=NETRECV_FIFO_NUM)netrecvfifowrpos=0;										//归零  
	if(netrecvfifordpos==netrecvfifowrpos)
	{
		netrecvfifowrpos=temp;																										//还原原来的写位置,此次写入失败
		return 1;	
	}
	for(i=0;i<NETRECV_LINE_SIZE;i++)netrecvfifobuf[netrecvfifowrpos][i]=buf[i];	//拷贝数据
	return 0;
}   



//相应变量的内存申请
//返回值:0 成功；其他 失败
u8 netmem_malloc(void)
{
  u16 t=0;
  netcam_line_buf0=mymalloc(SRAMIN,NETCAM_LINE_SIZE*4);
	netcam_line_buf1=mymalloc(SRAMIN,NETCAM_LINE_SIZE*4);	
	imgbuf2592x480x32[0]=mymalloc(SRAMEX,2592*240*4);	
	imgbuf2592x480x32[1]=mymalloc(SRAMEX,2592*240*4);	
	imgbuf2592x480x32[2]=mymalloc(SRAMEX,2592*240*4);	
  //给发送行FIFO申请内存
	for(t=0;t<NETSEND_FIFO_NUM;t++) 
	{
		netsendfifobuf[t]=mymalloc(SRAMEX,NETSEND_LINE_SIZE);
	}  	
	 //给接收行FIFO申请内存
	for(t=0;t<NETRECV_FIFO_NUM;t++) 
	{
		netrecvfifobuf[t]=mymalloc(SRAMEX,NETRECV_LINE_SIZE);
	}  	
	//发送行缓存
	netsend_line_buf=mymalloc(SRAMEX,NETSEND_LINE_SIZE);
	//接收行缓存
	netrecv_line_buf=mymalloc(SRAMEX,NETRECV_LINE_SIZE);
  if(!netsendfifobuf[NETSEND_FIFO_NUM-1]||!netcam_line_buf1||!netcam_line_buf0||!netsend_line_buf||!netrecvfifobuf[NETRECV_FIFO_NUM-1]||!netrecv_line_buf)//内存申请失败  
  {
    netmem_free();//释放内存
    return 1;
  }
  return 0;
}

//相应变量的内存释放
void netmem_free(void)
{
   u16 t=0;
   myfree(SRAMIN,netcam_line_buf0);
   myfree(SRAMIN,netcam_line_buf1);
   //释放FIFO的内存
   for(t=0;t<NETSEND_FIFO_NUM;t++) 
	{
     myfree(SRAMEX,netsendfifobuf[t]);
	}  
	
	for(t=0;t<NETRECV_FIFO_NUM;t++)
	{
		myfree(SRAMEX,netrecvfifobuf[t]);
	}
	myfree(SRAMEX,netsend_line_buf);
	myfree(SRAMEX,netrecv_line_buf);
	myfree(SRAMEX,imgbuf2592x480x32[0]);
	myfree(SRAMEX,imgbuf2592x480x32[1]);
	myfree(SRAMEX,imgbuf2592x480x32[2]);
}

//udp接收函数
static void udp_thread(void *arg)
{
	OS_CPU_SR cpu_sr;
	uint32_t RegVal=0;
	struct pbuf *q;
	err_t err,recv_err;
	static ip_addr_t server_ipaddr,loca_ipaddr;
	static u16_t 		 server_port,loca_port;
	LWIP_UNUSED_ARG(arg);
	server_port = REMOTE_PORT;
	
	while (1) 
	{
		udpconn = netconn_new(NETCONN_UDP);  //创建一个TCP链接
		udpconn->recv_timeout = 10;
		if(udpconn!=NULL)
		{	
			IP4_ADDR(&server_ipaddr, lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
			err = netconn_bind(udpconn,IP_ADDR_ANY,REMOTE_PORT);
			err = netconn_connect(udpconn,&server_ipaddr,server_port);//连接服务器
		}
		
		if(err != ERR_OK) //返回值不等于ERR_OK,删除tcp_clientconn连接
			netconn_delete(udpconn); 
		else if (err == ERR_OK)    //处理新连接的数据
		{ 
			struct netbuf *recvbuf;
			udp_flag=1;
			netconn_getaddr(udpconn,&loca_ipaddr,&loca_port,1); //获取本地IP主机IP地址和端口号
			while(1)
			{	
				//数据接收部分
				if((recv_err = netconn_recv(udpconn,&recvbuf)) == ERR_OK)  //接收到数据
				{	
					OS_ENTER_CRITICAL(); //关中断				
					//将数据保存到接收缓存区中
					for(q=recvbuf->p;q!=NULL;q=q->next)  //遍历完整个pbuf链表
					{
						if(q->len<=NETRECV_LINE_SIZE)
							mymemcpy(netrecv_line_buf,q->payload,q->len);
						if((netrecv_line_buf[0]==0xAA)&&(netrecv_line_buf[1]==0XAC))
						{	
							//数据包正确
							netrecv_fifo_write(netrecv_line_buf);
						 	OSSemPost(msg_netrecvok);    
						}							
					}					
					OS_EXIT_CRITICAL();  //开中断					
					netbuf_delete(recvbuf);
				}else if((recv_err == ERR_CLSD)||(recv_err==ERR_RST)||(recv_err==ERR_ABRT)||(recv_err==ERR_CONN))  //关闭连接
				{
					if(recvbuf!=NULL)
						netbuf_delete(recvbuf);
					if(udpconn!=NULL)
					{
						netconn_close(udpconn);
						netconn_delete(udpconn);
						udp_flag=0;
						break;
					}
				}
				//OSTimeDlyHMSM(0,0,0,1);  //延时500ms
			}
		}
			//OSTimeDlyHMSM(0,0,0,1);  //延时500ms
	}	
}


//创建UDP线程
//返回值:0 UDP创建成功
//		其他 UDP创建失败
INT8U udp_demo_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;	
	OS_ENTER_CRITICAL();	//关中断
	res = OSTaskCreate(udp_thread,(void*)0,(OS_STK*)&UDP_TASK_STK[UDP_STK_SIZE-1],UDP_PRIO); //创建UDP线程
	OS_EXIT_CRITICAL();		//开中断
	return res;
}

//网络发送任务函数
void udp_send_task(void *pdata)
{
	u8 res=0;
	u8 err;
	vu16 Count=0;
	u8* tbuf;
	err_t     send_err; 																																			//数据发送错误标志
	SendPacket m_Packet;
	static struct netbuf  *sentbuf;
	//发送数据缓存
	while(1)
	{
		//做图像发送2592*240 一个数据包为1296个字节,放在图像处理任务中做了
		//请求信号量   
		OSSemPend(msg_netsendready,0,&err);  
		res=netsend_fifo_read(&tbuf);                                                            //读取FIFO中的数据
		if(res)                                                                                  //有数据要发送
		{
			sentbuf = netbuf_new();
			netbuf_alloc(sentbuf,NETSEND_LINE_SIZE);
			memcpy(sentbuf->p->payload,tbuf,NETSEND_LINE_SIZE);
			send_err = netconn_send(udpconn,sentbuf);  	//将netbuf中的数据发送出去
			if((send_err==ERR_CLSD)||(send_err==ERR_RST)||(send_err==ERR_ABRT)||(send_err==ERR_CONN))                                          //关闭连接,或者重启网络 
			{
				//发送失败，关闭连接,或者重启网络
				if(udpconn!=NULL)
				{
					netconn_close(udpconn);
					netconn_delete(udpconn);
					udp_flag=0;
				}
			}
			
			netbuf_delete(sentbuf); 
			
/*    mymemcpy((u8*)&m_Packet,tbuf,NETSEND_LINE_SIZE);
			send_err=netconn_write(udpconn,(u8*)(&m_Packet),NETSEND_LINE_SIZE,NETCONN_COPY);//发送数据
			if((send_err==ERR_CLSD)||(send_err==ERR_RST)||(send_err==ERR_ABRT)||(send_err==ERR_CONN))                                          //关闭连接,或者重启网络 
			{
				//发送失败，关闭连接,或者重启网络
				if(udpconn!=NULL)
				{
					netconn_close(udpconn);
					netconn_delete(udpconn);
					udp_flag=0;
				}
			}	
*/			
		}else
		{ 
			//发送缓冲区中无数据可以发送了
			;
		}
		//OSTimeDlyHMSM(0,0,0,1);  //延时500ms		
	}
}

//网络接收数据处理任务函数
void udp_dataprocess_task(void *pdata)
{
	u8 res=0;
	u8* tbuf;
	u8 err;
	SendPacket m_Packet;
	while(1)
	{
		if(udp_flag==1)                  																						//判断TCP连接是否已建立
		{
			//请求信号量   
			OSSemPend(msg_netrecvok,0,&err);      
			res=netrecv_fifo_read(&tbuf);    																									//读取接收FIFO中的数据
			if(res)     																																			//接收缓存区有数据要处理
			{
				mymemcpy((u8*)&m_Packet,tbuf,NETRECV_LINE_SIZE);
				switch(m_Packet.event)
				{
					case WATCH_IMAGE:
					{
						//需要发送图像了,到图像处理imgcamera_task里面去
						if(jpeg_data_send==1)
							break;
						/*if(m_Packet.data3==0)
						{
							u16 TempVal=0;
							CurrentCamPara->exposureh=m_Packet.exposureh;
							CurrentCamPara->exposurel=m_Packet.exposurel;
							TempVal=CurrentCamPara->exposureh+(CurrentCamPara->exposurel<<8);
							AT24CXX_WriteLenByte(PACKETEXPOSUREH,TempVal,2);	
							//配置积分时间
							cmos_set_Shutter(CurrentCamPara->exposurel+(CurrentCamPara->exposureh<<8));  
						}*/
						jpeg_data_pcsend_flag=m_Packet.data2;
						
						jpeg_data_send=1;		
						break;
					}
					
					
					case SET_DETECTPARA:
					{
						//设置检测相关参数保存参数到eeprom						
						u16 TempVal=0;
						if(m_Packet.data1==0)
						{
							//下位机发送给相机的设置所有相机参数命令
							if((((m_Packet.exposureh<<8)+m_Packet.exposurel)>=2)&&(((m_Packet.exposureh<<8)+m_Packet.exposurel)<=29999))
							{
								CurrentCamPara->exposureh=m_Packet.exposureh;
								CurrentCamPara->exposurel=m_Packet.exposurel;
								TempVal=CurrentCamPara->exposureh+(CurrentCamPara->exposurel<<8);
								AT24CXX_WriteLenByte(PACKETEXPOSUREH,TempVal,2);	
								//配置积分时间
								cmos_set_Shutter(CurrentCamPara->exposurel+(CurrentCamPara->exposureh<<8));  
							}	
						}
						else
						{
							//PC发送给相机的只是设置积分时间命令
							if((((m_Packet.exposureh<<8)+m_Packet.exposurel)>=2)&&(((m_Packet.exposureh<<8)+m_Packet.exposurel)<=29999))
							{
								CurrentCamPara->exposureh=m_Packet.exposureh;
								CurrentCamPara->exposurel=m_Packet.exposurel;
								TempVal=CurrentCamPara->exposureh+(CurrentCamPara->exposurel<<8);
								AT24CXX_WriteLenByte(PACKETEXPOSUREH,TempVal,2);	
								//配置积分时间
								cmos_set_Shutter(CurrentCamPara->exposurel+(CurrentCamPara->exposureh<<8));  
							}
						}
						
						break;
					}
					
					case SET_CAMID:
					{
						//接收到设置相机ID命令
						if((m_Packet.changeid>0)&&(m_Packet.changeid<21))
						{
							if(m_Packet.changeid!=m_Packet.id)
							{
								//将changeid保存到eeprom中
								AT24CXX_WriteOneByte(PACKETID,m_Packet.changeid);							
							}
						}
						
						break;
					}
									
					case GET_CAMVER:
					{
						//将相机的版本号发送给下位机
						CurrentCamPara->event=GET_CAMVER;
						CurrentCamPara->data1=CamVer;
						netsend_fifo_write((u8*)CurrentCamPara);
						OSSemPost(msg_netsendready);
						break;
					}
					
					case GET_CAMINFO:
					{
						u16 RecvAoiStartY=((m_Packet.aoishowstartyh<<8)+m_Packet.aoishowstartyl);
						u8 RecvAoiHeight=m_Packet.aoishowheight;
						
						CurrentCamPara->event=GET_CAMINFO;
						CurrentCamPara->data1=CamVer;
						CurrentCamPara->data2=(unsigned char)StartY240;
						CurrentCamPara->data3=(unsigned char)(StartY240>>8);
						
						
						if((RecvAoiStartY>=0)&&(RecvAoiStartY<=440))
						{
							if(RecvAoiStartY!=((CurrentCamPara->aoishowstartyh<<8)+CurrentCamPara->aoishowstartyl))
							{
								//AOI检测起点有改变
								CurrentCamPara->aoishowstartyh=m_Packet.aoishowstartyh;
								AT24CXX_WriteOneByte(PACKETAOISHOWSTARTYH,CurrentCamPara->aoishowstartyh);	
								CurrentCamPara->aoishowstartyl=m_Packet.aoishowstartyl;
								AT24CXX_WriteOneByte(PACKETAOISHOWSTARTYL,CurrentCamPara->aoishowstartyl);	
							}
						}
						
						if((RecvAoiHeight>=48)&&(RecvAoiHeight<=255))
						{
							if(RecvAoiHeight!=CurrentCamPara->aoishowheight)
							{
								//AOI检测宽度有改变
								CurrentCamPara->aoishowheight=RecvAoiHeight;
								AT24CXX_WriteOneByte(PACKETAOISHOWHEIGHT,CurrentCamPara->aoishowheight);	
							}
						}
						
						netsend_fifo_write((u8*)CurrentCamPara);
						OSSemPost(msg_netsendready);
					}
									
					case SET_STARTY240:
					{
						if(m_Packet.data1<=240)
						{
							StartY240=m_Packet.data1;
							AT24CXX_WriteOneByte(STARTY240,StartY240);
						}
						break;
					}
					
					case SET_AOI:
					{
						u16 RecvAoiStartY=((m_Packet.aoishowstartyh<<8)+m_Packet.aoishowstartyl);
						u8 RecvAoiHeight=m_Packet.aoishowheight;
						
						if((RecvAoiStartY>=0)&&(RecvAoiStartY<=440))
						{
							if(RecvAoiStartY!=((CurrentCamPara->aoishowstartyh<<8)+CurrentCamPara->aoishowstartyl))
							{
								//AOI检测起点有改变
								CurrentCamPara->aoishowstartyh=m_Packet.aoishowstartyh;
								AT24CXX_WriteOneByte(PACKETAOISHOWSTARTYH,CurrentCamPara->aoishowstartyh);	
								CurrentCamPara->aoishowstartyl=m_Packet.aoishowstartyl;
								AT24CXX_WriteOneByte(PACKETAOISHOWSTARTYL,CurrentCamPara->aoishowstartyl);	
							}
						}
						
						if((RecvAoiHeight>=48)&&(RecvAoiHeight<=255))
						{
							if(RecvAoiHeight!=CurrentCamPara->aoishowheight)
							{
								//AOI检测宽度有改变
								CurrentCamPara->aoishowheight=RecvAoiHeight;
								AT24CXX_WriteOneByte(PACKETAOISHOWHEIGHT,CurrentCamPara->aoishowheight);	
							}
						}
						
						break;
					}
					
					case SET_PARA_RESET:
					{
						//收到下位机发送的恢复出厂设置命令
						CurrentCamPara->head1=0xAA;
						CurrentCamPara->head2=0xAC;
						CurrentCamPara->tail1=0xFC;
						CurrentCamPara->tail2=0xFF;		
						CurrentCamPara->aoiauto=0;
						CurrentCamPara->aoishowheight=32;
						AT24CXX_WriteOneByte(PACKETAOISHOWHEIGHT,CurrentCamPara->aoishowheight);			
						CurrentCamPara->aoistartyh=(u8)(852>>8);
						AT24CXX_WriteOneByte(PACKETAOISTARTYH,CurrentCamPara->aoistartyh);	
						CurrentCamPara->aoistartyl=(u8)(852);
						AT24CXX_WriteOneByte(PACKETAOISTARTYL,CurrentCamPara->aoistartyl);	
						CurrentCamPara->aoishowstartxh=(u8)(32>>8);
						AT24CXX_WriteOneByte(PACKETAOISHOWSTARTXH,CurrentCamPara->aoishowstartxh);	
						CurrentCamPara->aoishowstartxl=(u8)(32);
						AT24CXX_WriteOneByte(PACKETAOISHOWSTARTXL,CurrentCamPara->aoishowstartxl);			
						CurrentCamPara->aoishowendxh=(u8)(2559>>8);
						AT24CXX_WriteOneByte(PACKETAOISHOWENDXH,CurrentCamPara->aoishowendxh);	
						CurrentCamPara->aoishowendxl=(u8)(2559);
						AT24CXX_WriteOneByte(PACKETAOISHOWENDXL,CurrentCamPara->aoishowendxl);
						CurrentCamPara->data2=0;
						CurrentCamPara->data3=0;		
						CurrentCamPara->aoishowstartyh=(u8)(10>>8);
						AT24CXX_WriteOneByte(PACKETAOISHOWSTARTYH,CurrentCamPara->aoishowstartyh);	
						CurrentCamPara->aoishowstartyl=(u8)(10);
						AT24CXX_WriteOneByte(PACKETAOISHOWSTARTYL,CurrentCamPara->aoishowstartyl);		
						CurrentCamPara->autoexposure=1;	
						CurrentCamPara->autoexposurevalue=128;	
						CurrentCamPara->changeid=0;
						CurrentCamPara->alg1=0;
						AT24CXX_WriteOneByte(PACKETALG1,CurrentCamPara->alg1);	
						CurrentCamPara->alg2=70;
						AT24CXX_WriteOneByte(PACKETALG2,CurrentCamPara->alg2);		
						CurrentCamPara->data1=0;
						CurrentCamPara->exposurel=(u8)(300);
						AT24CXX_WriteOneByte(PACKETEXPOSUREL,CurrentCamPara->exposurel);	
						CurrentCamPara->exposureh=(u8)(300>>8);
						AT24CXX_WriteOneByte(PACKETEXPOSUREH,CurrentCamPara->exposureh);
						//执行配置积分时间命令操作
						cmos_set_Shutter(300);
						CurrentCamPara->gainh=(u8)(9>>8);
						CurrentCamPara->gainl=(u8)(9);	
						CurrentCamPara->event=WATCH_SIGNAL;
						CurrentCamPara->mode=25;
						AT24CXX_WriteOneByte(PACKETMODE,CurrentCamPara->mode);	
						CurrentCamPara->senstive1h=(u8)(4>>8);
						AT24CXX_WriteOneByte(PACKETSENSTIVE1H,CurrentCamPara->senstive1h);
						CurrentCamPara->senstive1l=(u8)(4);
						AT24CXX_WriteOneByte(PACKETSENSTIVE1L,CurrentCamPara->senstive1l);
						CurrentCamPara->senstive2h=(u8)(100>>8);
						AT24CXX_WriteOneByte(PACKETSENSTIVE2H,CurrentCamPara->senstive2h);
						CurrentCamPara->senstive2l=(u8)(100);
						AT24CXX_WriteOneByte(PACKETSENSTIVE2L,CurrentCamPara->senstive2l);		
						CurrentCamPara->openclothnum=0;
						CurrentCamPara->openclothid=0;
						CurrentCamPara->status=0;
						break;
					}
					default:
						break;
				}
				
			}else
			{ 	
				//接收缓存区无数据要接收了
			}					
		}else//end of if(tcp_client_flag==1)  
		{
			OSTimeDlyHMSM(0,0,0,1);  //延时500ms
		} 
	}	//end of while(1)
}

//jpeg数据接收回调函数
static void jpeg_dcmi_rx_callback(void)
{  
	u16 i;
	u32 *pbuf;
	pbuf=imgbuf2592x480x32[currentcatch]+jpeg_data_len;//偏移到有效数据末尾
	SCB_CleanInvalidateDCache();        //清除无效化DCache
	//SCB_CleanDCache();
	//SCB_InvalidateDCache();
	if(DMA1_Stream1->CR&(1<<19))//buf0已满,正常处理buf1
	{ 
		for(i=0;i<NETCAM_LINE_SIZE;i++)
			pbuf[i]=netcam_line_buf0[i];//读取buf0里面的数据
		//mymemcpy(pbuf,netcam_line_buf0,NETCAM_LINE_SIZE<<2);
		jpeg_data_len+=NETCAM_LINE_SIZE;//偏移
	}else //buf1已满,正常处理buf0
	{
		for(i=0;i<NETCAM_LINE_SIZE;i++)
			pbuf[i]=netcam_line_buf1[i];//读取buf1里面的数据
		//mymemcpy(pbuf,netcam_line_buf1,NETCAM_LINE_SIZE<<2);
		jpeg_data_len+=NETCAM_LINE_SIZE;//偏移 
	} 
  //SCB_CleanInvalidateDCache();        //清除无效化DCache
}


//网络摄像头初始化
u8 camera_init(void)
{
  u8 rval=0;  
	//初始化mt9p031
  if(MT9P031_Init())	    
	{
		delay_ms(500);  
    rval=1;
	} 
	
  if(rval==0)
  {
		//DCMI配置
    DCMI_Init();	
		//CMOS接收数据回调函数
    dcmi_rx_callback=jpeg_dcmi_rx_callback;
    DCMI_DMA_Init((u32)netcam_line_buf0,(u32)netcam_line_buf1,NETCAM_LINE_SIZE,DMA_MDATAALIGN_WORD,DMA_MINC_ENABLE);        
    delay_ms(100);
  } 
  return rval;
}
//初始化相机参数结构体,涉及到EEPROM的读写操作
void InitCamParaPacket(void)
{
	int j=0;
	u8 ReadOneVal=0;
	//给相机参数结构
	if(CurrentCamPara==NULL)
		CurrentCamPara=mymalloc(SRAMEX,sizeof(SendPacket));
	
	//指定地址读取一个字节
	ReadOneVal=AT24CXX_ReadOneByte(255);	
	if(ReadOneVal==0x25)
	{
		//代表已经初始化过相机参数了直接读取并赋值给CurrentCamPara就行了
		CurrentCamPara->head1=0xAA;
		CurrentCamPara->head2=0xAC;
		CurrentCamPara->tail1=0xFC;
		CurrentCamPara->tail2=0xFF;
		CurrentCamPara->id=AT24CXX_ReadOneByte(PACKETID);	
		CurrentCamPara->aoiauto=0;	
		
		CurrentCamPara->aoistartyh=AT24CXX_ReadOneByte(PACKETAOISTARTYH);
		CurrentCamPara->aoistartyl=AT24CXX_ReadOneByte(PACKETAOISTARTYL);
		CurrentCamPara->aoishowstartxh=AT24CXX_ReadOneByte(PACKETAOISHOWSTARTXH);
		CurrentCamPara->aoishowstartxl=AT24CXX_ReadOneByte(PACKETAOISHOWSTARTXL);
		CurrentCamPara->aoishowendxh=AT24CXX_ReadOneByte(PACKETAOISHOWENDXH);
		CurrentCamPara->aoishowendxl=AT24CXX_ReadOneByte(PACKETAOISHOWENDXL);
		
		CurrentCamPara->aoishowstartyh=AT24CXX_ReadOneByte(PACKETAOISHOWSTARTYH);
		CurrentCamPara->aoishowstartyl=AT24CXX_ReadOneByte(PACKETAOISHOWSTARTYL);
		
		CurrentCamPara->aoishowheight=AT24CXX_ReadOneByte(PACKETAOISHOWHEIGHT);
		
		CurrentCamPara->autoexposure=1;
		CurrentCamPara->autoexposurevalue=128;
		CurrentCamPara->alg1=AT24CXX_ReadOneByte(PACKETALG1);
		CurrentCamPara->alg2=AT24CXX_ReadOneByte(PACKETALG2);
		CurrentCamPara->exposurel=AT24CXX_ReadOneByte(PACKETEXPOSUREL);
		CurrentCamPara->exposureh=AT24CXX_ReadOneByte(PACKETEXPOSUREH);
		CurrentCamPara->gainh=0;
		CurrentCamPara->gainl=9;
		CurrentCamPara->mode=AT24CXX_ReadOneByte(PACKETMODE);
		CurrentCamPara->senstive1h=AT24CXX_ReadOneByte(PACKETSENSTIVE1H);
		CurrentCamPara->senstive1l=AT24CXX_ReadOneByte(PACKETSENSTIVE1L);
		CurrentCamPara->senstive2h=AT24CXX_ReadOneByte(PACKETSENSTIVE2H);
		CurrentCamPara->senstive2l=AT24CXX_ReadOneByte(PACKETSENSTIVE2L);
		CurrentCamPara->status=AT24CXX_ReadOneByte(PACKETSTATUS);
		for(j=0;j<6;j++)
		{
			CurrentCamPara->openclothstart[j]=AT24CXX_ReadLenByte(PACKETOPENCLOTHSTART+2*j,2);
			CurrentCamPara->openclothend[j]=AT24CXX_ReadLenByte(PACKETOPENCLOTHEND+2*j,2);	
		}		
		
		StartY240=AT24CXX_ReadOneByte(STARTY240);	
		if(StartY240>240)
		{
			StartY240=240;
			AT24CXX_WriteOneByte(STARTY240,StartY240);	
		}
	}
	else
	{
		//代表还没有设置过相机参数，那么就将默认的值写入eeprom，并将255地址写入0x25，并以默认的值来初始化CurrentCamPara共72个字节
		CurrentCamPara->head1=0xAA;
		CurrentCamPara->head2=0xAC;
		CurrentCamPara->tail1=0xFC;
		CurrentCamPara->tail2=0xFF;
		CurrentCamPara->id=1;
		AT24CXX_WriteOneByte(PACKETID,CurrentCamPara->id);			
		CurrentCamPara->aoiauto=0;	
		CurrentCamPara->aoishowheight=32;
		AT24CXX_WriteOneByte(PACKETAOISHOWHEIGHT,CurrentCamPara->aoishowheight);			
		CurrentCamPara->aoistartyh=(u8)(852>>8);
		AT24CXX_WriteOneByte(PACKETAOISTARTYH,CurrentCamPara->aoistartyh);	
		CurrentCamPara->aoistartyl=(u8)(852);
		AT24CXX_WriteOneByte(PACKETAOISTARTYL,CurrentCamPara->aoistartyl);	
		CurrentCamPara->aoishowstartxh=(u8)(32>>8);
		AT24CXX_WriteOneByte(PACKETAOISHOWSTARTXH,CurrentCamPara->aoishowstartxh);	
		CurrentCamPara->aoishowstartxl=(u8)(32);
		AT24CXX_WriteOneByte(PACKETAOISHOWSTARTXL,CurrentCamPara->aoishowstartxl);			
		CurrentCamPara->aoishowendxh=(u8)(2559>>8);
		AT24CXX_WriteOneByte(PACKETAOISHOWENDXH,CurrentCamPara->aoishowendxh);	
		CurrentCamPara->aoishowendxl=(u8)(2559);
		AT24CXX_WriteOneByte(PACKETAOISHOWENDXL,CurrentCamPara->aoishowendxl);
		CurrentCamPara->data2=0;
		CurrentCamPara->data3=0;		
		CurrentCamPara->aoishowstartyh=(u8)(10>>8);
		AT24CXX_WriteOneByte(PACKETAOISHOWSTARTYH,CurrentCamPara->aoishowstartyh);	
		CurrentCamPara->aoishowstartyl=(u8)(10);
		AT24CXX_WriteOneByte(PACKETAOISHOWSTARTYL,CurrentCamPara->aoishowstartyl);		
		CurrentCamPara->autoexposure=1;
		CurrentCamPara->autoexposurevalue=128;	
		CurrentCamPara->changeid=0;
		CurrentCamPara->alg1=0;
		AT24CXX_WriteOneByte(PACKETALG1,CurrentCamPara->alg1);	
		CurrentCamPara->alg2=70;
		AT24CXX_WriteOneByte(PACKETALG2,CurrentCamPara->alg2);		
		CurrentCamPara->data1=0;
		CurrentCamPara->exposurel=(u8)(300);
		AT24CXX_WriteOneByte(PACKETEXPOSUREL,CurrentCamPara->exposurel);	
		CurrentCamPara->exposureh=(u8)(300>>8);
		AT24CXX_WriteOneByte(PACKETEXPOSUREH,CurrentCamPara->exposureh);
		CurrentCamPara->gainh=(u8)(9>>8);
		CurrentCamPara->gainl=(u8)(9);
		CurrentCamPara->event=WATCH_SIGNAL;
		CurrentCamPara->mode=25;
		AT24CXX_WriteOneByte(PACKETMODE,CurrentCamPara->mode);	
		CurrentCamPara->senstive1h=(u8)(4>>8);
		AT24CXX_WriteOneByte(PACKETSENSTIVE1H,CurrentCamPara->senstive1h);
		CurrentCamPara->senstive1l=(u8)(4);
		AT24CXX_WriteOneByte(PACKETSENSTIVE1L,CurrentCamPara->senstive1l);
		CurrentCamPara->senstive2h=(u8)(100>>8);
		AT24CXX_WriteOneByte(PACKETSENSTIVE2H,CurrentCamPara->senstive2h);
		CurrentCamPara->senstive2l=(u8)(100);
		AT24CXX_WriteOneByte(PACKETSENSTIVE2L,CurrentCamPara->senstive2l);		
		CurrentCamPara->openclothnum=0;
		CurrentCamPara->openclothid=0;
		CurrentCamPara->status=0;
		for(j=0;j<6;j++)
		{
			CurrentCamPara->openclothstart[j]=0;
			AT24CXX_WriteLenByte(PACKETOPENCLOTHSTART+2*j,CurrentCamPara->openclothstart[j],2);	
			CurrentCamPara->openclothend[j]=0;
			AT24CXX_WriteLenByte(PACKETOPENCLOTHEND+2*j,CurrentCamPara->openclothend[j],2);	
		}			
		StartY240=120;
		AT24CXX_WriteOneByte(STARTY240,StartY240);	
		AT24CXX_WriteOneByte(255,0x25);		
	}
}
