#ifndef _BSP_GPS_H
#define _BSP_GPS_H

#include "gd32f4xx.h"
#include "board.h"

#define BSP_GPS_TX_RCU      RCU_GPIOC   // 串口TX的端口时钟
#define BSP_GPS_RX_RCU      RCU_GPIOC   // 串口RX的端口时钟
#define BSP_GPS_RCU         RCU_USART2  // 串口1的时钟

#define BSP_GPS_TX_PORT     GPIOC				// 串口TX的端口
#define BSP_GPS_RX_PORT     GPIOC				// 串口RX的端口
#define BSP_GPS_AF 					GPIO_AF_7   // 串口0的复用功能
#define BSP_GPS_TX_PIN      GPIO_PIN_10  // 串口TX的引脚
#define BSP_GPS_RX_PIN      GPIO_PIN_11 // 串口RX的引脚

#define BSP_GPS_USART 		USART2      // 串口1
#define BSP_GPS_IRQn 		USART2_IRQn      
#define BSP_GPS_IRQHandler 	USART2_IRQHandler      


//定义数组长度
#define GPS_Buffer_Length 80
#define UTCTime_Length 11
#define latitude_Length 11
#define N_S_Length 2
#define longitude_Length 12
#define E_W_Length 2 

typedef struct SaveData 
{
	char GPS_Buffer[GPS_Buffer_Length];
	char isGetData;		//是否获取到GPS数据
	char isParseData;	//是否解析完成
	char UTCTime[UTCTime_Length];		//UTC时间
	char latitude[latitude_Length];		//纬度
	char N_S[N_S_Length];		//N/S
	char longitude[longitude_Length];		//经度
	char E_W[E_W_Length];		//E/W
	char isUsefull;		//定位信息是否有效
} _SaveData;

extern _SaveData Save_Data;

void GPS_GPIO_Init(uint32_t band_rate);
void CLR_Buf(void);
uint8_t Hand(char *a);
void clrStruct(void);

#endif  