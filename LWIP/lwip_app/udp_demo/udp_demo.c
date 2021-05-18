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
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F4&F7&H7������
//NETCONN API��̷�ʽ��UDP���Դ���	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2016/8/5
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//*******************************************************************************
//�޸���Ϣ
//��
////////////////////////////////////////////////////////////////////////////////// 	   
 
//UDP�ͻ�������
#define UDP_PRIO		6
//�����ջ��С
#define UDP_STK_SIZE	300
//�����ջ
OS_STK UDP_TASK_STK[UDP_STK_SIZE];


//UDP��������
OS_STK UDP_SEND_TASK_STK[UDP_SEND_STK_SIZE];	

//UDP�������ݴ�������
OS_STK UDP_DATAPROCESS_TASK_STK[UDP_DATAPROCESS_STK_SIZE];	
u8 udp_flag;															//UDP���ݷ��ͱ�־λ
u32 *netcam_line_buf0;					    			//�����л���0  
u32 *netcam_line_buf1;					    			//�����л���1   

//NET SEND FIFO
u8*  netsend_line_buf;
vu16 netsendfifordpos=0;					    		//FIFO��λ��
vu16 netsendfifowrpos=0;					    		//FIFOдλ��
u8 *netsendfifobuf[NETSEND_FIFO_NUM];	    //����NETSEND_FIFO_SIZE������FIFO	

//NET RECV FIFO
u8*  netrecv_line_buf;
vu16 netrecvfifordpos=0;					    		//����FIFO��λ��
vu16 netrecvfifowrpos=0;					    		//����FIFOдλ��
u8 *netrecvfifobuf[NETRECV_FIFO_NUM];	    //����NETSEND_FIFO_SIZE������FIFO	
volatile SendPacket* CurrentCamPara;
//�����������ź���
OS_EVENT * msg_netrecvok;
//���緢��׼�����ź���			
OS_EVENT * msg_netsendready;

vu8         currentcatch=0;
vu8 				CamVer=10;
u32* 				imgbuf2592x480x32[3];
vu8         StartY240=120;                  //240�����ռ480���е�λ��
struct netconn *udpconn;					          //udp CLIENT�������ӽṹ��
//�̰��Ļ�����80���ֽڷֱ�Ϊ5a,5a,a5,a5��W���ֽڣ�W���ֽڣ�H���ֽڣ�H���ֽ�
//1024�����Դ��룩 0x01 0x02 0x03 0x04(��ǰ֡��)  ����汾�� ����ʱ���  ����ʱ���  ����  cmos��ַ  ��˾����  c3 c2 c1 c0
u8          UdpShort[23]={0x5a,0x5a,0xa5,0xa5,(u8)(2592>>8),(u8)(2592),(u8)(480>>8),(u8)(480),(u8)(1024>>8),\
													(u8)(1024),0x01,0x02,0x03,0x04,10,(u8)(300>>8),(u8)(300),0xBA,01,\
                          0xc3,0xc2,0xc1,0xc0};                   //udp ����̰�

//��ȡFIFO
//buf:���ݻ������׵�ַ
//����ֵ:0,û�����ݿɶ�;
//      1,������1�����ݿ�
u8 netsend_fifo_read(u8 **buf)
{
	if(netsendfifordpos==netsendfifowrpos)return 0;
	netsendfifordpos++;		//��λ�ü�1
	if(netsendfifordpos>=NETSEND_FIFO_NUM)netsendfifordpos=0;//���� 
	*buf=netsendfifobuf[netsendfifordpos];
	return 1;
}
//дһ��FIFO
//buf:���ݻ������׵�ַ
//����ֵ:0,д��ɹ�;
//       1,д��ʧ��
u8 netsend_fifo_write(u8 *buf)
{
	u16 i;
	u16 temp=netsendfifowrpos;																									//��¼��ǰдλ��
	netsendfifowrpos++;																			 										//дλ�ü�1
	if(netsendfifowrpos>=NETSEND_FIFO_NUM)netsendfifowrpos=0;										//����  
	if(netsendfifordpos==netsendfifowrpos)
	{
		netsendfifowrpos=temp;																										//��ԭԭ����дλ��,�˴�д��ʧ��
		return 1;	
	}
	for(i=0;i<NETSEND_LINE_SIZE;i++)netsendfifobuf[netsendfifowrpos][i]=buf[i];	//��������
	return 0;
}   

//��ȡ����FIFO
//buf:���ݻ������׵�ַ
//����ֵ:0,û�����ݿɶ�;
//      1,������1�����ݿ�
u8 netrecv_fifo_read(u8 **buf)
{
	if(netrecvfifordpos==netrecvfifowrpos)return 0;
	netrecvfifordpos++;		//��λ�ü�1
	if(netrecvfifordpos>=NETRECV_FIFO_NUM)netrecvfifordpos=0;//���� 
	*buf=netrecvfifobuf[netrecvfifordpos];
	return 1;
}
//дһ��FIFO
//buf:���ݻ������׵�ַ
//����ֵ:0,д��ɹ�;
//       1,д��ʧ��
u8 netrecv_fifo_write(u8 *buf)
{
	u16 i;
	u16 temp=netrecvfifowrpos;																									//��¼��ǰдλ��
	netrecvfifowrpos++;																			 										//дλ�ü�1
	if(netrecvfifowrpos>=NETRECV_FIFO_NUM)netrecvfifowrpos=0;										//����  
	if(netrecvfifordpos==netrecvfifowrpos)
	{
		netrecvfifowrpos=temp;																										//��ԭԭ����дλ��,�˴�д��ʧ��
		return 1;	
	}
	for(i=0;i<NETRECV_LINE_SIZE;i++)netrecvfifobuf[netrecvfifowrpos][i]=buf[i];	//��������
	return 0;
}   



//��Ӧ�������ڴ�����
//����ֵ:0 �ɹ������� ʧ��
u8 netmem_malloc(void)
{
  u16 t=0;
  netcam_line_buf0=mymalloc(SRAMIN,NETCAM_LINE_SIZE*4);
	netcam_line_buf1=mymalloc(SRAMIN,NETCAM_LINE_SIZE*4);	
	imgbuf2592x480x32[0]=mymalloc(SRAMEX,2592*240*4);	
	imgbuf2592x480x32[1]=mymalloc(SRAMEX,2592*240*4);	
	imgbuf2592x480x32[2]=mymalloc(SRAMEX,2592*240*4);	
  //��������FIFO�����ڴ�
	for(t=0;t<NETSEND_FIFO_NUM;t++) 
	{
		netsendfifobuf[t]=mymalloc(SRAMEX,NETSEND_LINE_SIZE);
	}  	
	 //��������FIFO�����ڴ�
	for(t=0;t<NETRECV_FIFO_NUM;t++) 
	{
		netrecvfifobuf[t]=mymalloc(SRAMEX,NETRECV_LINE_SIZE);
	}  	
	//�����л���
	netsend_line_buf=mymalloc(SRAMEX,NETSEND_LINE_SIZE);
	//�����л���
	netrecv_line_buf=mymalloc(SRAMEX,NETRECV_LINE_SIZE);
  if(!netsendfifobuf[NETSEND_FIFO_NUM-1]||!netcam_line_buf1||!netcam_line_buf0||!netsend_line_buf||!netrecvfifobuf[NETRECV_FIFO_NUM-1]||!netrecv_line_buf)//�ڴ�����ʧ��  
  {
    netmem_free();//�ͷ��ڴ�
    return 1;
  }
  return 0;
}

//��Ӧ�������ڴ��ͷ�
void netmem_free(void)
{
   u16 t=0;
   myfree(SRAMIN,netcam_line_buf0);
   myfree(SRAMIN,netcam_line_buf1);
   //�ͷ�FIFO���ڴ�
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

//udp���պ���
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
		udpconn = netconn_new(NETCONN_UDP);  //����һ��TCP����
		udpconn->recv_timeout = 10;
		if(udpconn!=NULL)
		{	
			IP4_ADDR(&server_ipaddr, lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
			err = netconn_bind(udpconn,IP_ADDR_ANY,REMOTE_PORT);
			err = netconn_connect(udpconn,&server_ipaddr,server_port);//���ӷ�����
		}
		
		if(err != ERR_OK) //����ֵ������ERR_OK,ɾ��tcp_clientconn����
			netconn_delete(udpconn); 
		else if (err == ERR_OK)    //���������ӵ�����
		{ 
			struct netbuf *recvbuf;
			udp_flag=1;
			netconn_getaddr(udpconn,&loca_ipaddr,&loca_port,1); //��ȡ����IP����IP��ַ�Ͷ˿ں�
			while(1)
			{	
				//���ݽ��ղ���
				if((recv_err = netconn_recv(udpconn,&recvbuf)) == ERR_OK)  //���յ�����
				{	
					OS_ENTER_CRITICAL(); //���ж�				
					//�����ݱ��浽���ջ�������
					for(q=recvbuf->p;q!=NULL;q=q->next)  //����������pbuf����
					{
						if(q->len<=NETRECV_LINE_SIZE)
							mymemcpy(netrecv_line_buf,q->payload,q->len);
						if((netrecv_line_buf[0]==0xAA)&&(netrecv_line_buf[1]==0XAC))
						{	
							//���ݰ���ȷ
							netrecv_fifo_write(netrecv_line_buf);
						 	OSSemPost(msg_netrecvok);    
						}							
					}					
					OS_EXIT_CRITICAL();  //���ж�					
					netbuf_delete(recvbuf);
				}else if((recv_err == ERR_CLSD)||(recv_err==ERR_RST)||(recv_err==ERR_ABRT)||(recv_err==ERR_CONN))  //�ر�����
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
				//OSTimeDlyHMSM(0,0,0,1);  //��ʱ500ms
			}
		}
			//OSTimeDlyHMSM(0,0,0,1);  //��ʱ500ms
	}	
}


//����UDP�߳�
//����ֵ:0 UDP�����ɹ�
//		���� UDP����ʧ��
INT8U udp_demo_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;	
	OS_ENTER_CRITICAL();	//���ж�
	res = OSTaskCreate(udp_thread,(void*)0,(OS_STK*)&UDP_TASK_STK[UDP_STK_SIZE-1],UDP_PRIO); //����UDP�߳�
	OS_EXIT_CRITICAL();		//���ж�
	return res;
}

//���緢��������
void udp_send_task(void *pdata)
{
	u8 res=0;
	u8 err;
	vu16 Count=0;
	u8* tbuf;
	err_t     send_err; 																																			//���ݷ��ʹ����־
	SendPacket m_Packet;
	static struct netbuf  *sentbuf;
	//�������ݻ���
	while(1)
	{
		//��ͼ����2592*240 һ�����ݰ�Ϊ1296���ֽ�,����ͼ��������������
		//�����ź���   
		OSSemPend(msg_netsendready,0,&err);  
		res=netsend_fifo_read(&tbuf);                                                            //��ȡFIFO�е�����
		if(res)                                                                                  //������Ҫ����
		{
			sentbuf = netbuf_new();
			netbuf_alloc(sentbuf,NETSEND_LINE_SIZE);
			memcpy(sentbuf->p->payload,tbuf,NETSEND_LINE_SIZE);
			send_err = netconn_send(udpconn,sentbuf);  	//��netbuf�е����ݷ��ͳ�ȥ
			if((send_err==ERR_CLSD)||(send_err==ERR_RST)||(send_err==ERR_ABRT)||(send_err==ERR_CONN))                                          //�ر�����,������������ 
			{
				//����ʧ�ܣ��ر�����,������������
				if(udpconn!=NULL)
				{
					netconn_close(udpconn);
					netconn_delete(udpconn);
					udp_flag=0;
				}
			}
			
			netbuf_delete(sentbuf); 
			
/*    mymemcpy((u8*)&m_Packet,tbuf,NETSEND_LINE_SIZE);
			send_err=netconn_write(udpconn,(u8*)(&m_Packet),NETSEND_LINE_SIZE,NETCONN_COPY);//��������
			if((send_err==ERR_CLSD)||(send_err==ERR_RST)||(send_err==ERR_ABRT)||(send_err==ERR_CONN))                                          //�ر�����,������������ 
			{
				//����ʧ�ܣ��ر�����,������������
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
			//���ͻ������������ݿ��Է�����
			;
		}
		//OSTimeDlyHMSM(0,0,0,1);  //��ʱ500ms		
	}
}

//����������ݴ���������
void udp_dataprocess_task(void *pdata)
{
	u8 res=0;
	u8* tbuf;
	u8 err;
	SendPacket m_Packet;
	while(1)
	{
		if(udp_flag==1)                  																						//�ж�TCP�����Ƿ��ѽ���
		{
			//�����ź���   
			OSSemPend(msg_netrecvok,0,&err);      
			res=netrecv_fifo_read(&tbuf);    																									//��ȡ����FIFO�е�����
			if(res)     																																			//���ջ�����������Ҫ����
			{
				mymemcpy((u8*)&m_Packet,tbuf,NETRECV_LINE_SIZE);
				switch(m_Packet.event)
				{
					case WATCH_IMAGE:
					{
						//��Ҫ����ͼ����,��ͼ����imgcamera_task����ȥ
						if(jpeg_data_send==1)
							break;
						/*if(m_Packet.data3==0)
						{
							u16 TempVal=0;
							CurrentCamPara->exposureh=m_Packet.exposureh;
							CurrentCamPara->exposurel=m_Packet.exposurel;
							TempVal=CurrentCamPara->exposureh+(CurrentCamPara->exposurel<<8);
							AT24CXX_WriteLenByte(PACKETEXPOSUREH,TempVal,2);	
							//���û���ʱ��
							cmos_set_Shutter(CurrentCamPara->exposurel+(CurrentCamPara->exposureh<<8));  
						}*/
						jpeg_data_pcsend_flag=m_Packet.data2;
						
						jpeg_data_send=1;		
						break;
					}
					
					
					case SET_DETECTPARA:
					{
						//���ü����ز������������eeprom						
						u16 TempVal=0;
						if(m_Packet.data1==0)
						{
							//��λ�����͸�������������������������
							if((((m_Packet.exposureh<<8)+m_Packet.exposurel)>=2)&&(((m_Packet.exposureh<<8)+m_Packet.exposurel)<=29999))
							{
								CurrentCamPara->exposureh=m_Packet.exposureh;
								CurrentCamPara->exposurel=m_Packet.exposurel;
								TempVal=CurrentCamPara->exposureh+(CurrentCamPara->exposurel<<8);
								AT24CXX_WriteLenByte(PACKETEXPOSUREH,TempVal,2);	
								//���û���ʱ��
								cmos_set_Shutter(CurrentCamPara->exposurel+(CurrentCamPara->exposureh<<8));  
							}	
						}
						else
						{
							//PC���͸������ֻ�����û���ʱ������
							if((((m_Packet.exposureh<<8)+m_Packet.exposurel)>=2)&&(((m_Packet.exposureh<<8)+m_Packet.exposurel)<=29999))
							{
								CurrentCamPara->exposureh=m_Packet.exposureh;
								CurrentCamPara->exposurel=m_Packet.exposurel;
								TempVal=CurrentCamPara->exposureh+(CurrentCamPara->exposurel<<8);
								AT24CXX_WriteLenByte(PACKETEXPOSUREH,TempVal,2);	
								//���û���ʱ��
								cmos_set_Shutter(CurrentCamPara->exposurel+(CurrentCamPara->exposureh<<8));  
							}
						}
						
						break;
					}
					
					case SET_CAMID:
					{
						//���յ��������ID����
						if((m_Packet.changeid>0)&&(m_Packet.changeid<21))
						{
							if(m_Packet.changeid!=m_Packet.id)
							{
								//��changeid���浽eeprom��
								AT24CXX_WriteOneByte(PACKETID,m_Packet.changeid);							
							}
						}
						
						break;
					}
									
					case GET_CAMVER:
					{
						//������İ汾�ŷ��͸���λ��
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
								//AOI�������иı�
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
								//AOI������иı�
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
								//AOI�������иı�
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
								//AOI������иı�
								CurrentCamPara->aoishowheight=RecvAoiHeight;
								AT24CXX_WriteOneByte(PACKETAOISHOWHEIGHT,CurrentCamPara->aoishowheight);	
							}
						}
						
						break;
					}
					
					case SET_PARA_RESET:
					{
						//�յ���λ�����͵Ļָ�������������
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
						//ִ�����û���ʱ���������
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
				//���ջ�����������Ҫ������
			}					
		}else//end of if(tcp_client_flag==1)  
		{
			OSTimeDlyHMSM(0,0,0,1);  //��ʱ500ms
		} 
	}	//end of while(1)
}

//jpeg���ݽ��ջص�����
static void jpeg_dcmi_rx_callback(void)
{  
	u16 i;
	u32 *pbuf;
	pbuf=imgbuf2592x480x32[currentcatch]+jpeg_data_len;//ƫ�Ƶ���Ч����ĩβ
	SCB_CleanInvalidateDCache();        //�����Ч��DCache
	//SCB_CleanDCache();
	//SCB_InvalidateDCache();
	if(DMA1_Stream1->CR&(1<<19))//buf0����,��������buf1
	{ 
		for(i=0;i<NETCAM_LINE_SIZE;i++)
			pbuf[i]=netcam_line_buf0[i];//��ȡbuf0���������
		//mymemcpy(pbuf,netcam_line_buf0,NETCAM_LINE_SIZE<<2);
		jpeg_data_len+=NETCAM_LINE_SIZE;//ƫ��
	}else //buf1����,��������buf0
	{
		for(i=0;i<NETCAM_LINE_SIZE;i++)
			pbuf[i]=netcam_line_buf1[i];//��ȡbuf1���������
		//mymemcpy(pbuf,netcam_line_buf1,NETCAM_LINE_SIZE<<2);
		jpeg_data_len+=NETCAM_LINE_SIZE;//ƫ�� 
	} 
  //SCB_CleanInvalidateDCache();        //�����Ч��DCache
}


//��������ͷ��ʼ��
u8 camera_init(void)
{
  u8 rval=0;  
	//��ʼ��mt9p031
  if(MT9P031_Init())	    
	{
		delay_ms(500);  
    rval=1;
	} 
	
  if(rval==0)
  {
		//DCMI����
    DCMI_Init();	
		//CMOS�������ݻص�����
    dcmi_rx_callback=jpeg_dcmi_rx_callback;
    DCMI_DMA_Init((u32)netcam_line_buf0,(u32)netcam_line_buf1,NETCAM_LINE_SIZE,DMA_MDATAALIGN_WORD,DMA_MINC_ENABLE);        
    delay_ms(100);
  } 
  return rval;
}
//��ʼ����������ṹ��,�漰��EEPROM�Ķ�д����
void InitCamParaPacket(void)
{
	int j=0;
	u8 ReadOneVal=0;
	//����������ṹ
	if(CurrentCamPara==NULL)
		CurrentCamPara=mymalloc(SRAMEX,sizeof(SendPacket));
	
	//ָ����ַ��ȡһ���ֽ�
	ReadOneVal=AT24CXX_ReadOneByte(255);	
	if(ReadOneVal==0x25)
	{
		//�����Ѿ���ʼ�������������ֱ�Ӷ�ȡ����ֵ��CurrentCamPara������
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
		//����û�����ù������������ô�ͽ�Ĭ�ϵ�ֵд��eeprom������255��ַд��0x25������Ĭ�ϵ�ֵ����ʼ��CurrentCamPara��72���ֽ�
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
