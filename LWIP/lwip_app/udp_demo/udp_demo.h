#ifndef __UDP_DEMO_H
#define __UDP_DEMO_H
#include "sys.h"
#include "includes.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F4&F7&H7开发板
//NETCONN API编程方式的UDP测试代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2016/2/29
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//*******************************************************************************
//修改信息
//无
////////////////////////////////////////////////////////////////////////////////// 	   
#define REMOTE_PORT					8088		//定义远端主机的端口号码
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

//其他参数设置eeprom保存
#define REPEAT          			87
#define ISLOW           			88
#define CUSED           			89

//网络数据包处理事件定义
#define WATCH_SIGNAL          1                   //查询信号量
#define WATCH_IMAGE           2                   //查询图像
#define WATCH_VERSION         3                   //查询软件版本号
#define WATCH_ALL             4                   //查询所有的配置信息
#define SET_DETECTPARA        5                   //设置检测参数
#define SET_CAMID             6                   //设置相机ID事件
#define SET_OPENCLOTH_ALIVE   7                   //设置开福有效命令
#define SET_CAM_STARTX        8                   //设置相机起点命令
#define SET_CAM_ENDX          9                   //设置相机终点命令
#define SET_AOIYHEIGHT        10                  //设置相机AOI Y方向检测高度命令
#define SET_AOIYSTART         11                  //设置相机AOI Y方向检测起点命令
#define SET_RUNSTATUS         12                  //设置相机停止检测或则检测命令
#define SET_ALARM_INFO        13                  //相机发送断纱信息给下位机
#define SET_PARA_RESET        14                  //下位机发送恢复出厂设置按钮给相机
#define SET_EXPOSURE_OK       15                  //发送给下位机积分时间设置完毕指令
#define SET_STARTY240         16                  //PC发送给相机StartY240,并保存到eeprom中
#define SET_DETECTPARA_REPEAT 17                  //下位机发送给相机复检次数
#define SET_DETECTPARA_USED   18                  //下位机发送给相机，相机使能禁用情况
#define SET_DETECTPARA_ISSLOW 19                  //下位机发送给相机，快速检测还是慢速检测
#define SET_LEARNOK           20                  //相机发送给下位机学习完毕
#define SET_AOI               24                  //设置相机AOI命令也就是再运行的时候只传输AOI区域图像

#define GET_LEARNIMAGE        21                  //下位机请求相机学习结果   
#define GET_CAMVER            22                  //下位机请求相机版本号
#define GET_CAMINFO           23

extern u8 udp_flag;		//UDP数据发送标志位
//网络数据包大小
#define IMAGEPACKETSIZE       622080	//图像数据包大小
#define UDPIMAGESIZE          1296    //图像UDP包大小
#define UDPIMAGEDC            480     //检测的时候需要传输的包的个数
#define UDPIAMGEVC            960     //预览的时候需要传输的包的个数
#define NETSEND_FIFO_NUM			60			//定义发送FIFO数量
#define NETRECV_FIFO_NUM			60    	//定义接收FIFO数量
#define NETCAM_LINE_SIZE      4096  	//定义相机DMA接收行大小
#define NETSEND_LINE_SIZE		  76			//定义发送行大小(*4字节)
#define NETRECV_LINE_SIZE		  76			//定义接收

extern u8 UdpShort[23];                      //udp 网络短包


//UDP发送任务优先级
#define UDP_SEND_TASK_PRIO       		   3
//设置任务堆栈大小
#define UDP_SEND_STK_SIZE  		 	   		1500
extern OS_STK UDP_SEND_TASK_STK[UDP_SEND_STK_SIZE];	

//UDP接收数据处理任务优先级
#define UDP_DATAPROCESS_TASK_PRIO       		   4
//设置任务堆栈大小
#define UDP_DATAPROCESS_STK_SIZE  		 	   		1500
//任务堆栈	
extern OS_STK UDP_DATAPROCESS_TASK_STK[UDP_DATAPROCESS_STK_SIZE];



//发送数据包缓存区
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
extern u32 *netcam_line_buf0;					    				//定义行缓存0  
extern u32 *netcam_line_buf1;					    				//定义行缓存1

extern u8*  netsend_line_buf;
extern vu16 netsendfifordpos;					    				//FIFO读位置
extern vu16 netsendfifowrpos;					    				//FIFO写位置
extern u8 *netsendfifobuf[NETSEND_FIFO_NUM];	    //定义NETSEND_FIFO_SIZE个发送FIFO	

//NET RECV FIFO
extern u8*  netrecv_line_buf;
extern vu16 netrecvfifordpos;					    		//接收FIFO读位置
extern vu16 netrecvfifowrpos;					    		//接收FIFO写位置
extern u8 *netrecvfifobuf[NETRECV_FIFO_NUM];	//定义NETSEND_FIFO_SIZE个接收FIFO	

extern volatile SendPacket* CurrentCamPara;   //定义当前相机的参数
//网络接收完成信号量
extern OS_EVENT * msg_netrecvok;
//网络发送准备好信号量			
extern OS_EVENT * msg_netsendready;

extern vu8         currentcatch;
extern vu8 				 CamVer;
extern u32* 			 imgbuf2592x480x32[3];
extern vu8         StartY240;                  //240行起点占480行中的位置
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

