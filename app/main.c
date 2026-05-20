/******************************************************************
 * 项 目 名 称:森警云探——基于GD32F407的森林火灾识别定位与环境监测系统
 * 作       者：郑钊霖
 * 指 导 老 师：陈毓秀
******************************************************************/
#include "board.h"
#include "bsp_esp01s.h"
#include "dht11.h"
#include "oled.h"
#include "time.h"
#include "adc.h"
#include "bsp_gps.h"
#include "bump.h"

void parseGpsBuffer(void);
void printGpsBuffer(void);

AP_PARAMETER ap_parameter={0};
JSON_PUBLISH sta_json_data={0};
uint8_t test_ret = 0,Send_Flag=1;
unsigned int value=0;
int gps_Flag = 0;
extern int bump_Flag,falme_flag,soil_flag,fire_Flag;
uint8_t lat_buff[100]={0},NS_buff[100]={0},lon_buff[100]={0},EW_buff[100]={0};
char *endptrlon;
char *endptrlat;
extern double lat_value,lon_value;

int main(void)
{
		board_init();
	
		GPS_GPIO_Init(9600);
		clrStruct();
	
		OLED_Init();
		OLED_Clear();
		
		OLED_ShowString(0,0,"Temp:      Hui:",8,1);
		OLED_ShowString(0,8,"soil:      mq :",8,1);
		OLED_ShowString(0,16,"fire:   ",8,1);
		OLED_ShowString(0,24,"lat:            N",8,1);
		OLED_ShowString(0,32,"lon:            W",8,1);
		OLED_Refresh();
	
		DHT11_GPIO_Init();
		ADC_config();
		DMA_config();
		bsp_pwm_init();
		
    usart_gpio_config(115200U);

    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);  // 优先级分组
        
    printf("start\r\n");
		delay_ms(1000);
	
    //WIFI初始化
    WIFI_ESP01S_Init();
    //链接阿里云
    WIFI_MODE_STA_Aliyun_Init();
    //上传数据
    Publish_MQTT_message(&sta_json_data,9);  
		//定时器初始化
		bsp_timer_init1(16800, 5000);
		bsp_timer_init2(16800, 3000);
		
    while(1)
    {		
			if(gps_Flag==0)
			{
				parseGpsBuffer();
				printGpsBuffer();
			}
			else if(gps_Flag==1)
			{

				lon_value=strtod((char*)lon_buff,&endptrlon)/100;//经度
				lat_value=strtod((char*)lat_buff,&endptrlat)/100;//纬度
				gps_Flag=2;
			}
			else if(gps_Flag==2)
			{
				printf("lon_value=%.7lf\r\n",lon_value);
				printf("lat_value=%.7lf\r\n",lat_value);
				printf("lon_value=%s\r\n",lon_buff);
				printf("lat_value=%s\r\n",lat_buff);
			}
			
			OLED_ShowNum(32,0,(int)Get_temperature(),3,8,1);
			OLED_ShowNum(96,0,(int)Get_humidity(),3,8,1);
			OLED_ShowNum(32,8,(int)Get_Percentage_value(0),3,8,1);
			OLED_ShowNum(96,8,(int)Get_Percentage_value(1),3,8,1);
			OLED_ShowNum(32,16,(int)Get_Percentage_value(2),3,8,1);
			OLED_ShowString(24,24,lat_buff,8,1);
			OLED_ShowString(24,32,lon_buff,8,1);
			OLED_Refresh();
			
			//pid算法智能实现灭火
			if(Get_Percentage_value(2)>20)
			{
				gpio_bit_write(GPIOA, GPIO_PIN_5, SET); 
				falme_flag=1;
				bump_Flag=1;
				fire_Flag=1;
				pid_update(100 - Get_Percentage_value(2),90);
			}
			//pid算法智能实现灌溉
			else if(Get_Percentage_value(0)<=55)
			{
				gpio_bit_write(GPIOA, GPIO_PIN_5, SET); 
				soil_flag=1;
				bump_Flag=1;
				pid_update(Get_Percentage_value(0),70);
			}
			else
			{
				gpio_bit_write(GPIOA, GPIO_PIN_5, RESET); 
				soil_flag=0;
				bump_Flag=0;
				falme_flag=0;
				fire_Flag=0;
			}
			//发送标志位
			if(Send_Flag==1)
			{
					Publish_MQTT_message(publish_mqtt_message,9);
			}
	}	
}

//定时检测DHT11
void TIMER5_DAC_IRQHandler(void)
{
	/* 这里是定时器中断 */ 
    if(timer_interrupt_flag_get(BSP_TIMER, TIMER_INT_FLAG_UP) == SET)    
    {        
        /* 执行操作 */         
			DHT11_Read_Data();
			timer_interrupt_flag_clear(BSP_TIMER, TIMER_INT_FLAG_UP); // 清除中断标志位   
    }
}

//使用定时器中断，通过置标志位防止接收和上传数据同步出错
void TIMER2_IRQHandler()
{
	/* 这里是定时器中断 */ 
    if(timer_interrupt_flag_get(BSP_TIMER2, TIMER_INT_FLAG_UP) == SET)    
    {     
				Send_Flag=0;
        /* 云平台下发 执行操作 */         
				Get_Aliyun_json_data(&sta_json_data);

				if( strstr( sta_json_data.keyname,"bump_Switch") != NULL )
				{
						if( strstr( sta_json_data.value,"0") != NULL) //发来的数据是0
						{
								gpio_bit_write(GPIOA,GPIO_PIN_5,0);//关水泵
						}
						if( strstr( sta_json_data.value,"1") != NULL)//发来的数据是1
						{
								gpio_bit_write(GPIOD,GPIO_PIN_7,1);//开水泵
						}
						Clear_Aliyun_json_data(&sta_json_data);//清除数据
				} 
			timer_interrupt_flag_clear(BSP_TIMER2, TIMER_INT_FLAG_UP); // 清除中断标志位 
			Send_Flag=1;//用于执行发送数据标志位	
    }
}
//NEO-6M错误处理
void errorLog(int num)
{
	  OLED_ShowString(0,64-16,"ERROR",8,1);
}
//NEO-6M接收卫星信号+解码
void parseGpsBuffer(void)
{
	char *subString;
	char *subStringNext;
	char i = 0;
	if (Save_Data.isGetData)
	{
		Save_Data.isGetData = 0;

		for (i = 0 ; i <= 6 ; i++)
		{
			if (i == 0)
			{
				if ((subString = strstr(Save_Data.GPS_Buffer, ",")) == NULL)
					errorLog(1);	//解析错误
			}
			else
			{
				subString++;
				if ((subStringNext = strstr(subString, ",")) != NULL)
				{
					char usefullBuffer[2]; 
					switch(i)
					{
						case 1:memcpy(Save_Data.UTCTime, subString, subStringNext - subString);break;	//获取UTC时间
						case 2:memcpy(usefullBuffer, subString, subStringNext - subString);break;	//获取UTC时间
						case 3:memcpy(Save_Data.latitude, subString, subStringNext - subString);break;	//获取纬度信息
						case 4:memcpy(Save_Data.N_S, subString, subStringNext - subString);break;	//获取N/S
						case 5:memcpy(Save_Data.longitude, subString, subStringNext - subString);break;	//获取经度信息
						case 6:memcpy(Save_Data.E_W, subString, subStringNext - subString);break;	//获取E/W

						default:break;
					}

					subString = subStringNext;
					Save_Data.isParseData = 1;
					if(usefullBuffer[0] == 'A')
						Save_Data.isUsefull = 1;
					else if(usefullBuffer[0] == 'V')
						Save_Data.isUsefull = 0;
				}
				else
				{
					errorLog(2);	//解析错误
				}
			}
		}
	}
}
//NEO-6M打印信息
void printGpsBuffer(void)
{
    uint8_t buff[100]={0};
	if (Save_Data.isParseData)
	{
		  Save_Data.isParseData = 0;
			//时间戳
			sprintf(buff,"T=\"%s\"",Save_Data.UTCTime);
			OLED_ShowString(0,2,buff,8,1);
			OLED_Refresh();
        
		if(Save_Data.isUsefull)
		{
				Save_Data.isUsefull = 0;

				//纬度
				sprintf(lat_buff,"%s",Save_Data.latitude);
;
				//南北半球
				sprintf(NS_buff,"%s",Save_Data.N_S);
			
				//经度
				sprintf(lon_buff,"%s",Save_Data.longitude);

				//屏幕显示
				sprintf(EW_buff,"%s",Save_Data.E_W);

			gps_Flag=1;//当获取到设备gps定位时，则置为1结束定位
			 
		}
		else
		{
					 OLED_ShowString(0,64-8,"not usefull",8,1);
					 OLED_Refresh();
		}
	}
}
