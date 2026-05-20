#include "bsp_gps.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define GPSRX_LEN_MAX 255

unsigned char GPSRX_BUFF[GPSRX_LEN_MAX];     
unsigned char GPSRX_LEN = 0;

_SaveData Save_Data;

/******************************************************************
 * 函 数 名 称：GPS_GPIO_Init
 * 函 数 说 明：GPS引脚初始化
 * 函 数 形 参：band_rate GPS通信波特率
 * 函 数 返 回：无
 * 备       注：默认波特率为9600
******************************************************************/
void GPS_GPIO_Init(uint32_t band_rate)
{
    /* 开启时钟 */
	rcu_periph_clock_enable(BSP_GPS_TX_RCU);   // 开启串口时钟
	rcu_periph_clock_enable(BSP_GPS_RX_RCU);   // 开启端口时钟
	rcu_periph_clock_enable(BSP_GPS_RCU);      // 开启端口时钟
	
	/* 配置GPIO复用功能 */
    gpio_af_set(BSP_GPS_TX_PORT,BSP_GPS_AF,BSP_GPS_TX_PIN);	
	gpio_af_set(BSP_GPS_RX_PORT,BSP_GPS_AF,BSP_GPS_RX_PIN);	
	
	/* 配置GPIO的模式 */
	/* 配置TX为复用模式 上拉模式 */
	gpio_mode_set(BSP_GPS_TX_PORT,GPIO_MODE_AF,GPIO_PUPD_PULLUP,BSP_GPS_TX_PIN);
	/* 配置RX为复用模式 上拉模式 */
	gpio_mode_set(BSP_GPS_RX_PORT, GPIO_MODE_AF,GPIO_PUPD_PULLUP,BSP_GPS_RX_PIN);
	
	/* 配置TX为推挽输出 50MHZ */
	gpio_output_options_set(BSP_GPS_TX_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,BSP_GPS_TX_PIN);
	/* 配置RX为推挽输出 50MHZ */
	gpio_output_options_set(BSP_GPS_RX_PORT,GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_GPS_RX_PIN);

	/* 配置串口的参数 */
	usart_deinit(BSP_GPS_USART);                                 // 复位串口
	usart_baudrate_set(BSP_GPS_USART,band_rate);                 // 设置波特率
	usart_parity_config(BSP_GPS_USART,USART_PM_NONE);            // 没有校验位
	usart_word_length_set(BSP_GPS_USART,USART_WL_8BIT);          // 8位数据位
	usart_stop_bit_set(BSP_GPS_USART,USART_STB_1BIT);     			 // 1位停止位

    /* 使能串口 */
	usart_enable(BSP_GPS_USART);                          			 // 使能串口
	usart_transmit_config(BSP_GPS_USART,USART_TRANSMIT_ENABLE);  // 使能串口发送
	usart_receive_config(BSP_GPS_USART, USART_RECEIVE_ENABLE);//使能UART4接收
    
    /*	使能UART接收中断标志位 */
	usart_interrupt_enable(BSP_GPS_USART, USART_INT_RBNE);   	
	/* 配置中断优先级 */
	nvic_irq_enable(BSP_GPS_IRQn, 2, 2); // 配置中断优先级
}

/******************************************************************
 * 函 数 名 称：GPS_Send_Bit
 * 函 数 说 明：向GPS发送单个字符
 * 函 数 形 参：ch发送的字符
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void GPS_Send_Bit(unsigned char ch)
{
	//发送字符
	usart_data_transmit(UART4, ch);
	// 等待发送数据缓冲区标志自动置位
	while(RESET == usart_flag_get(UART4, USART_FLAG_TBE) );
}  

/******************************************************************
 * 函 数 名 称：GPS_send_String
 * 函 数 说 明：GPS发送字符串
 * 函 数 形 参：str要发送的字符串
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void GPS_send_String(unsigned char *str)
{
	while( str && *str ) // 地址为空或者值为空跳出
	{	
		GPS_Send_Bit(*str++);
	}	
}

/******************************************************************
 * 函 数 名 称：Hand
 * 函 数 说 明：在GPS数据中，识别是否有想要的串口命令
 * 函 数 形 参：需要识别的命令
 * 函 数 返 回：1识别成功  0识别失败
 * 备       注：无
******************************************************************/
uint8_t Hand(char *a)                   
{ 
    if(strstr((const char*)GPSRX_BUFF,a)!=NULL)
	    return 1;
	else
		return 0;
}
/******************************************************************
 * 函 数 名 称：CLR_Buf
 * 函 数 说 明：清除串口接收的数据
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void CLR_Buf(void)                           
{
	memset(GPSRX_BUFF, 0, GPSRX_LEN_MAX);      //清空
    GPSRX_LEN = 0;                    
}

/******************************************************************
 * 函 数 名 称：clrStruct
 * 函 数 说 明：清除GPS结构体数据
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void clrStruct(void)
{
	Save_Data.isGetData = 0;
	Save_Data.isParseData = 0;
	Save_Data.isUsefull = 0;
	memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);      //清空
	memset(Save_Data.UTCTime,    0, UTCTime_Length);
	memset(Save_Data.latitude,   0, latitude_Length);
	memset(Save_Data.N_S,        0, N_S_Length);
	memset(Save_Data.longitude,  0, longitude_Length);
	memset(Save_Data.E_W,        0, E_W_Length);
	
}

/******************************************************************
 * 函 数 名 称：BSP_GPS_IRQHandler
 * 函 数 说 明：串口中断服务函数
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void BSP_GPS_IRQHandler(void)
{
    uint8_t Res;
	if(usart_interrupt_flag_get(BSP_GPS_USART,USART_INT_FLAG_RBNE) != RESET) // 接收缓冲区不为空
	{
		Res = usart_data_receive(BSP_GPS_USART);

        if(Res == '$')
        {
            GPSRX_LEN = 0;	
        }
        
        GPSRX_BUFF[GPSRX_LEN++] = Res;

        if(GPSRX_BUFF[0] == '$' && GPSRX_BUFF[4] == 'M' && GPSRX_BUFF[5] == 'C')//确定是否收到"GPRMC/GNRMC"这一帧数据
        {
            if(Res == '\n')									   
            {
                memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);      //清空
                memcpy(Save_Data.GPS_Buffer, GPSRX_BUFF, GPSRX_LEN); 	//保存数据
                Save_Data.isGetData = 1;
                GPSRX_LEN = 0;
                memset(GPSRX_BUFF, 0, GPSRX_LEN_MAX);      //清空				
            }	
        } 
        
        if(GPSRX_LEN >= GPSRX_LEN_MAX)
        {
            GPSRX_LEN = GPSRX_LEN_MAX;
        }	       
	}

}





