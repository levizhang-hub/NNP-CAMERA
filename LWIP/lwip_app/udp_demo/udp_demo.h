#ifndef __UDP_DEMO_H
#define __UDP_DEMO_H
#include "sys.h"
#include "includes.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F4&F7&H7������
//NETCONN API��̷�ʽ��UDP���Դ���	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2016/2/29
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//*******************************************************************************
//�޸���Ϣ
//��
////////////////////////////////////////////////////////////////////////////////// 	   
#define REMOTE_PORT					8088		//����Զ�������Ķ˿ں���
#define PACKETID             2
#define PACKETEVENT          3
#define PACKETSTATUS         4
#define PACKETSENSTIVE1H     5
#define PACKETSENSTIVE1L     6
#define PACKETSENSTIVE2H     7
#define PACKETSENSTIVE2L     8
#define PACKETOPENCLOTHNUM   9
#define PACKETOPENCLOTHID    10
#define PACKETGAINH           11
#define PACKETGAINL           12
#define PACKETCHANGEID        13
#define PACKETEXPOSUREH       14
#define PACKETEXPOSUREL       15
#define PACKETMODE            16
#define PACKETALG1            17
#define PACKETALG2            18
#define PACKETAUTOEXPOSURE    19
#define PACKETAUTOEXPOSUREV   20
#define PACKETAUTOAOI         21
#define PACKETAOISHOWHEIGHT   22
#define PACKETAOISTARTYH      23
#define PACKETAOISTARTYL      24

#define PACKETAOISHOWSTARTYH  25
#define PACKETAOISHOWSTARTYL  26
#define PACKETDATA2           27
#define PACKETDATA3           28

#define PACKETAOISHOWSTARTXH			29
#define PACKETAOISHOWSTARTXL      30
#define PACKETAOISHOWENDXH        31

#define PACKETDATA1           		73
#define PACKETOPENCLOTHSTART  		32
#define PACKETOPENCLOTHEND    		52
#define PACKETAOISHOWENDXL        72

#define OPENCLOTHALIVEFLAG1   76
#define OPENCLOTHALIVEFLAG2   77
#define OPENCLOTHALIVEFLAG3   78
#define OPENCLOTHALIVEFLAG4   79
#define OPENCLOTHALIVEFLAG5   80
#define OPENCLOTHALIVEFLAG6   81
#define OPENCLOTHALIVEFLAG7   82
#define OPENCLOTHALIVEFLAG8   83
#define OPENCLOTHALIVEFLAG9   84
#define OPENCLOTHALIVEFLAG10  85

#define STARTY240             86

//������������eeprom����
#define REPEAT          			87
#define ISLOW           			88
#define CUSED           			89

//�������ݰ������¼�����
#define WATCH_SIGNAL          1                   //��ѯ�ź���
#define WATCH_IMAGE           2                   //��ѯͼ��
#define WATCH_VERSION         3                   //��ѯ����汾��
#define WATCH_ALL             4                   //��ѯ���е�������Ϣ
#define SET_DETECTPARA        5                   //���ü�����
#define SET_CAMID             6                   //�������ID�¼�
#define SET_OPENCLOTH_ALIVE   7                   //���ÿ�����Ч����
#define SET_CAM_STARTX        8                   //��������������
#define SET_CAM_ENDX          9                   //��������յ�����
#define SET_AOIYHEIGHT        10                  //�������AOI Y������߶�����
#define SET_AOIYSTART         11                  //�������AOI Y�������������
#define SET_RUNSTATUS         12                  //�������ֹͣ������������
#define SET_ALARM_INFO        13                  //������Ͷ�ɴ��Ϣ����λ��
#define SET_PARA_RESET        14                  //��λ�����ͻָ��������ð�ť�����
#define SET_EXPOSURE_OK       15                  //���͸���λ������ʱ���������ָ��
#define SET_STARTY240         16                  //PC���͸����StartY240,�����浽eeprom��
#define SET_DETECTPARA_REPEAT 17                  //��λ�����͸�����������
#define SET_DETECTPARA_USED   18                  //��λ�����͸���������ʹ�ܽ������
#define SET_DETECTPARA_ISSLOW 19                  //��λ�����͸���������ټ�⻹�����ټ��
#define SET_LEARNOK           20                  //������͸���λ��ѧϰ���
#define SET_AOI               24                  //�������AOI����Ҳ���������е�ʱ��ֻ����AOI����ͼ��

#define GET_LEARNIMAGE        21                  //��λ���������ѧϰ���   
#define GET_CAMVER            22                  //��λ����������汾��
#define GET_CAMINFO           23

extern u8 udp_flag;		//UDP���ݷ��ͱ�־λ
//�������ݰ���С
#define IMAGEPACKETSIZE       622080	//ͼ�����ݰ���С
#define UDPIMAGESIZE          1296    //ͼ��UDP����С
#define UDPIMAGEDC            480     //����ʱ����Ҫ����İ��ĸ���
#define UDPIAMGEVC            960     //Ԥ����ʱ����Ҫ����İ��ĸ���
#define NETSEND_FIFO_NUM			60			//���巢��FIFO����
#define NETRECV_FIFO_NUM			60    	//�������FIFO����
#define NETCAM_LINE_SIZE      4096  	//�������DMA�����д�С
#define NETSEND_LINE_SIZE		  76			//���巢���д�С(*4�ֽ�)
#define NETRECV_LINE_SIZE		  76			//�������

extern u8 UdpShort[23];                      //udp ����̰�


//UDP�����������ȼ�
#define UDP_SEND_TASK_PRIO       		   3
//���������ջ��С
#define UDP_SEND_STK_SIZE  		 	   		1500
extern OS_STK UDP_SEND_TASK_STK[UDP_SEND_STK_SIZE];	

//UDP�������ݴ����������ȼ�
#define UDP_DATAPROCESS_TASK_PRIO       		   4
//���������ջ��С
#define UDP_DATAPROCESS_STK_SIZE  		 	   		1500
//�����ջ	
extern OS_STK UDP_DATAPROCESS_TASK_STK[UDP_DATAPROCESS_STK_SIZE];



//�������ݰ�������
typedef struct
{
	vu8 head1;
	vu8 head2;
	vu8 id;
	vu8 event;
	vu8 status;
	vu8 senstive1h;
	vu8 senstive1l;
	vu8 senstive2h;
	vu8 senstive2l;
	vu8 openclothnum;
	vu8 openclothid;
	vu8 gainh;
	vu8 gainl;
	vu8 changeid;
	vu8 exposureh;
	vu8 exposurel;
	vu8 mode;
	vu8 alg1;
	vu8 alg2;
	vu8 autoexposure;
	vu8 autoexposurevalue;
	vu8 aoiauto;
	vu8 aoishowheight;
	vu8 aoistartyh;
	vu8 aoistartyl;
	vu8 aoishowstartyh;
	vu8 aoishowstartyl;
	vu8 data2;
	vu8 data3;
	vu8 aoishowstartxh;
	vu8 aoishowstartxl;
	vu8 aoishowendxh;
	vu16 openclothstart[10];
	vu16 openclothend[10];
	vu8 aoishowendxl;
	vu8 data1;
	vu8 tail1;
	vu8 tail2;	
}SendPacket;
extern u32 *netcam_line_buf0;					    				//�����л���0  
extern u32 *netcam_line_buf1;					    				//�����л���1

extern u8*  netsend_line_buf;
extern vu16 netsendfifordpos;					    				//FIFO��λ��
extern vu16 netsendfifowrpos;					    				//FIFOдλ��
extern u8 *netsendfifobuf[NETSEND_FIFO_NUM];	    //����NETSEND_FIFO_SIZE������FIFO	

//NET RECV FIFO
extern u8*  netrecv_line_buf;
extern vu16 netrecvfifordpos;					    		//����FIFO��λ��
extern vu16 netrecvfifowrpos;					    		//����FIFOдλ��
extern u8 *netrecvfifobuf[NETRECV_FIFO_NUM];	//����NETSEND_FIFO_SIZE������FIFO	

extern volatile SendPacket* CurrentCamPara;   //���嵱ǰ����Ĳ���
//�����������ź���
extern OS_EVENT * msg_netrecvok;
//���緢��׼�����ź���			
extern OS_EVENT * msg_netsendready;

extern vu8         currentcatch;
extern vu8 				 CamVer;
extern u32* 			 imgbuf2592x480x32[3];
extern vu8         StartY240;                  //240�����ռ480���е�λ��
extern struct netconn *udpconn;	

u8 netsend_fifo_read(u8 **buf);
u8 netrecv_fifo_read(u8 **buf);
u8 netsend_fifo_write(u8 *buf);
u8 netrecv_fifo_write(u8 *buf);
u8 netmem_malloc(void);
void netmem_free(void);
u8 camera_init(void);
INT8U udp_demo_init(void);
void udp_send_task(void *pdata);
void udp_dataprocess_task(void *pdata);
void InitCamParaPacket(void);
#endif

