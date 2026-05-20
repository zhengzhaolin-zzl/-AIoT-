#ifndef _BSP_DHT11_H_
#define _BSP_DHT11_H_
 
#include "gd32f4xx.h"
 
 /**************引脚修改此处****************/
#define RCU_DHT11   RCU_GPIOB
#define PORT_DHT11  GPIOB
#define GPIO_DHT11  GPIO_PIN_0


//设置DHT11输出高或低电平
#define DATA_GPIO_OUT(x)    gpio_bit_write(PORT_DHT11, GPIO_DHT11, x?SET:RESET)
//获取DHT11数据引脚高低电平状态
#define DATA_GPIO_IN        gpio_input_bit_get(PORT_DHT11, GPIO_DHT11)

extern float temperature;
extern float humidity;


void DHT11_GPIO_Init(void);//引脚初始化
unsigned int DHT11_Read_Data(void);//读取模块数据
float Get_temperature(void);//返回读取模块后的温度数据
float Get_humidity(void);//返回读取模块后的湿度数据

#endif