#ifndef _MT9P031D_
#define _MT9P031D_
#include "sys.h"
#include "sccb.h"
#include "includes.h"
#include "math.h"

#ifndef TYPES
#define TYPES
typedef unsigned char bool;
#endif

#define CAPCAMERA_TASK_PRIO       		   13                  //设置任务优先级
#define CAPCAMERA_STK_SIZE  		 	   		 1500								//设置任务堆栈大小

#define IMGCAMERA_TASK_PRIO       		   14                  //设置任务优先级
#define IMGCAMERA_STK_SIZE  		 	   		 1500								//设置任务堆栈大小

#define MT9P031D_RST(n)  	(n?HAL_GPIO_WritePin(GPIOI,GPIO_PIN_1,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOI,GPIO_PIN_1,GPIO_PIN_RESET))//复位控制信号 

#define GRAYTORGB16(t) ((t >> 3)|((t & ~3) << 3)|((t & ~7) << 8))

#define MT9M034_I2C_ADDR    0x30
#define MT9P031_I2C_ADDR    /*0xBA*/ 0x90

/* defines for extra pixels/line added */
#define MT9P031_EXTRAPIXELS 	16
#define MT9P031_EXTRALINES	  8

/* Definitions to access the various sensor registers */
#define MT9P031_CHIP_VERSION			(0x00)
#define MT9P031_ROW_START			(0x01)
#define MT9P031_COL_START			(0x02)
#define MT9P031_HEIGHT				(0x03)
#define MT9P031_WIDTH				(0x04)
#define MT9P031_HBLANK				(0x05)
#define MT9P031_VBLANK				(0x06)
#define MT9P031_OUTPUT_CTRL			(0x07)
#define MT9P031_SHUTTER_WIDTH_UPPER		(0x08)
#define MT9P031_SHUTTER_WIDTH			(0x09)
#define MT9P031_PIXEL_CLK_CTRL			(0x0A)
#define MT9P031_RESTART				  (0x0B)
#define MT9P031_SHUTTER_DELAY		(0x0C)
#define MT9P031_RESET				    (0x0D)
#define MT9P031_READ_MODE1			(0x1E)
#define MT9P031_READ_MODE2			(0x20)
#define MT9P031_READ_MODE3			(0x21)
#define MT9P031_ROW_ADDR_MODE			(0x22)
#define MT9P031_COL_ADDR_MODE			(0x23)
#define MT9P031_RESERVED_27_REG                 (0x27)
#define MT9P031_GREEN1_GAIN			(0x2B)
#define MT9P031_BLUE_GAIN			(0x2C)
#define MT9P031_RED_GAIN			(0x2D)
#define MT9P031_GREEN2_GAIN			(0x2E)
#define MT9P031_GLOBAL_GAIN			(0x35)
#define MT9P031_BLACK_LEVEL			(0x49)
#define MT9P031_ROW_BLK_DEF_OFFSET		(0x4B)
#define MT9P031_RESERVED_4E_REG                 (0x4e)
#define MT9P031_RESERVED_50_REG                 (0x50)
#define MT9P031_RESERVED_51_REG                 (0x51)
#define MT9P031_RESERVED_52_REG                 (0x52)
#define MT9P031_RESERVED_53_REG                 (0x53)
#define MT9P031_CAL_COARSE			(0x5D)
#define MT9P031_CAL_TARGET			(0x5F)
#define MT9P031_GREEN1_OFFSET			(0x60)
#define MT9P031_GREEN2_OFFSET			(0x61)
#define MT9P031_BLK_LVL_CALIB			(0x62)
#define MT9P031_RED_OFFSET			(0x63)
#define MT9P031_BLUE_OFFSET			(0x64)
#define MT9P031_CHIP_ENABLE_SYNC		(0xF8)
#define MT9P031_CHIP_VERSION_END		(0xFF)

/* Define Shift and Mask for gain register*/

#define	MT9P031_ANALOG_GAIN_SHIFT	(0x0000)
#define	MT9P031_DIGITAL_GAIN_SHIFT	(8)
#define	MT9P031_ANALOG_GAIN_MASK	(0x007F)
#define	MT9P031_DIGITAL_GAIN_MASK	(0x7F00)

/* Define Shift and Mask for black level caliberation register*/

#define	MT9P031_MANUAL_OVERRIDE_MASK		(0x0001)
#define	MT9P031_DISABLE_CALLIBERATION_SHIFT	(1)
#define	MT9P031_DISABLE_CALLIBERATION_MASK	(0x0002)
#define	MT9P031_RECAL_BLACK_LEVEL_SHIFT		(12)
#define	MT9P031_RECAL_BLACK_LEVEL_MASK		(0x1000)
#define	MT9P031_LOCK_RB_CALIBRATION_SHIFT	(13)
#define	MT9P031_LOCK_RB_CALLIBERATION_MASK	(0x2000)
#define	MT9P031_LOCK_GREEN_CALIBRATION_SHIFT	(14)
#define	MT9P031_LOCK_GREEN_CALLIBERATION_MASK	(0x4000)
#define	MT9P031_LOW_COARSE_THELD_MASK		(0x007F)
#define	MT9P031_HIGH_COARSE_THELD_SHIFT		(8)
#define	MT9P031_HIGH_COARSE_THELD_MASK		(0x7F00)
#define	MT9P031_LOW_TARGET_THELD_MASK		(0x007F)
#define	MT9P031_HIGH_TARGET_THELD_SHIFT		(8)
#define	MT9P031_HIGH_TARGET_THELD_MASK		(0x7F00)
#define	MT9P031_SHUTTER_WIDTH_LOWER_MASK	(0xFFFF)
#define	MT9P031_SHUTTER_WIDTH_UPPER_SHIFT	(16)
#define	MT9P031_SHUTTER_WIDTH_UPPER_MASK	(0xFFFF)
#define	MT9P031_ROW_START_MASK			(0x07FF)
#define	MT9P031_COL_START_MASK			(0x0FFF)
#define	 MT9P031_GREEN1_OFFSET_MASK 		(0x01FF)
#define	 MT9P031_GREEN2_OFFSET_MASK 		(0x01FF)
#define	 MT9P031_RED_OFFSET_MASK 		(0x01FF)
#define	 MT9P031_BLUE_OFFSET_MASK 		(0x01FF)

/* defines for MT9P031 register values */
#define	MT9P031_NORMAL_OPERATION_MODE		(0x0002)
#define	MT9P031_HALT_MODE			(0x0003)
#define MT9P031_RESET_ENABLE			(0x0001)
#define MT9P031_RESET_DISABLE			(0x0000)
#define	MT9P031_INVERT_PIXEL_CLK		(0x8000)
#define MT9P031_GAIN_MINVAL			(0)
#define MT9P031_GAIN_MAXVAL			(128)
#define MT9P031_GAIN_STEP			(1)
#define MT9P031_GAIN_DEFAULTVAL			(8)

/* Default values for MT9P031 registers */
#define MT9P031_ID                  (0X1801) 
#define MT9P031_ROW_START_DEFAULT		(0x14)
#define MT9P031_COL_START_DEFAULT		(0x20)
#define MT9P031_HEIGHT_DEFAULT			(0x5FF)
#define MT9P031_WIDTH_DEFAULT			(0x7FF)
#define MT9P031_HBLANK_DEFAULT			(0x8E)
#define MT9P031_VBLANK_DEFAULT			(0x19)
#define MT9P031_OUTPUT_CTRL_DEFAULT		(0x02)
#define MT9P031_SHUTTER_WIDTH_UPPER_DEFAULT	(0x0)
#define MT9P031_SHUTTER_WIDTH_DEFAULT		(0x619)
#define MT9P031_PIXEL_CLK_CTRL_DEFAULT		(0x0)
#define MT9P031_RESTART_DEFAULT			(0x0)
#define MT9P031_SHUTTER_DELAY_DEFAULT		(0x0)
#define MT9P031_READ_MODE1_DEFAULT		(0xC040)
#define MT9P031_READ_MODE2_DEFAULT		(0x0)
#define MT9P031_READ_MODE3_DEFAULT		(0x0)
#define MT9P031_ROW_ADDR_MODE_DEFAULT		(0x0)
#define MT9P031_COL_ADDR_MODE_DEFAULT		(0x0)
#define MT9P031_GREEN1_GAIN_DEFAULT		(0x08)
#define MT9P031_BLUE_GAIN_DEFAULT		(0x08)
#define MT9P031_RED_GAIN_DEFAULT		(0x08)
#define MT9P031_GREEN2_GAIN_DEFAULT		(0x08)
#define MT9P031_GLOBAL_GAIN_DEFAULT		(0x08)
#define MT9P031_BLACK_LEVEL_DEFAULT		(0xA8)
#define MT9P031_CAL_COARSE_DEFAULT		(0x2D13)
#define MT9P031_CAL_TARGET_DEFAULT		(0x231D)
#define MT9P031_GREEN1_OFFSET_DEFAULT		(0x20)
#define MT9P031_GREEN2_OFFSET_DEFAULT		(0x20)
#define MT9P031_BLK_LVL_CALIB_DEFAULT		(0x0)
#define MT9P031_RED_OFFSET_DEFAULT		(0x20)
#define MT9P031_BLUE_OFFSET_DEFAULT		(0x20)
#define MT9P031_CHIP_ENABLE_SYNC_DEFAULT	(0x01)

#define MT9P031_I2C_REGISTERED			(1)
#define MT9P031_I2C_UNREGISTERED		(0)

/*	Defines for mt9p031_ctrl() functions command*/

#define	MT9P031_SET_PARAMS	1
#define	MT9P031_GET_PARAMS	2
#define	MT9P031_SET_GAIN	3
#define	MT9P031_SET_STD		4
#define	MT9P031_INIT		5
#define	MT9P031_CLEANUP		6

/* define for various video format supported by MT9P031 driver */
/* Here all mode defines will be assigned values of v4l2 mode defines */
#define	MT9P031_MODE_VGA_30FPS  	(10)
#define	MT9P031_MODE_VGA_60FPS		(11)
#define	MT9P031_MODE_SVGA_30FPS		(12)
#define	MT9P031_MODE_SVGA_60FPS		(13)
#define	MT9P031_MODE_XGA_30FPS		(14)
#define	MT9P031_MODE_480p_30FPS		(15)
#define	MT9P031_MODE_480p_60FPS		(16)
#define	MT9P031_MODE_576p_25FPS		(17)
#define	MT9P031_MODE_576p_50FPS		(18)
#define	MT9P031_MODE_720p_24FPS		(19)
#define	MT9P031_MODE_720p_30FPS		(20)
#define	MT9P031_MODE_1080p_18FPS	(21)

extern OS_STK CAPCAMERA_TASK_STK[CAPCAMERA_STK_SIZE];	
extern OS_STK IMGCAMERA_TASK_STK[IMGCAMERA_STK_SIZE];

extern volatile u32 jpeg_data_len; 		
extern volatile u8 	jpeg_data_ok;		
extern vu8         	jpeg_data_send;           //是否需要通过网络发送图像数据
extern vu8         	jpeg_data_pcsend_flag;    //PC传图标志
extern volatile u8 	DirectShow;               //直接在显示屏上显示图像
//cmos寄存器的读写操作
u8  cmos_rset( u8 regnum, u16 regval );
u16 cmos_rget( u16 regnum );
//利用相机参数结构体初始化MT9P031D传感器
u16 cmos_init(void);
//设置相机参数函数
u16 cmos_set_params(void);
//设置相机增益
u16 cmos_set_Gain(u16 GainVal);
//设置快门时间
u16 cmos_set_Shutter(u32 ShutterVal);
//设置aoi
u16 cmos_set_Aoi(u16 Y_start,u16 Y_size,u16 X_Start,u16 X_Size);
u8 MT9P031_Init(void);  
//采集任务函数
void capcamera_task(void *pdata); 
//处理任务函数
void imgcamera_task(void *pdata);
void bayer2rgb24_bingo(unsigned short *dst, unsigned char *src, long width, long height);
void gray2rgb565_bingo(unsigned short *dst, unsigned char *src, long width, long height);
u16 rgb_24_2_565(int r, int g, int b);
#endif
