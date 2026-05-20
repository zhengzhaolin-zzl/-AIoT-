#include "bsp_ESP01S.h"
#include "stdio.h"
#include "hmacsha1.h"
#include "string.h"
#include "dht11.h"
#include "adc.h"

unsigned char WIFI_RX_BUFF[WIFI_RX_LEN_MAX];     
unsigned char WIFI_RX_FLAG = 0;
unsigned char WIFI_RX_LEN = 0;
int bump_Flag;
double lat_value,lon_value;
int fire_Flag;

uint8_t wifi_link_flag = 0;//设备连接状态

//配合    Get_Device_connection_status    函数使用
// *              0=没有设备连接 
// *              1=有设备连接了WIFI   
// *              0=有设备断开了WIFI     
// *              2=有设备连接了服务器
// *              3=有设备断开了服务器



/************************************************************
 * 函数名称：WIFI_USART_Init
 * 函数说明：连接WIFI的初始化
 * 型    参：bund=串口波特率
 * 返 回 值：无
 * 备    注：无
*************************************************************/
void WIFI_USART_Init(unsigned int bund)
{
        /* 使能 WIFI_USART 的时钟 */
        rcu_periph_clock_enable(RCU_WIFI_USART);
        /* 使能时钟 */
        rcu_periph_clock_enable(RCU_WIFI_TX);
        rcu_periph_clock_enable(RCU_WIFI_RX);
        /*        配置引脚为复用功能 */
        gpio_af_set(PORT_WIFI_TX, BSP_WIFI_AF, GPIO_WIFI_TX);
        
        /*        配置引脚为复用功能 */
        gpio_af_set(PORT_WIFI_RX, BSP_WIFI_AF, GPIO_WIFI_RX);
        
        /*        配置TX引脚为复用上拉模式 */
        gpio_mode_set(PORT_WIFI_TX, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_WIFI_TX);
        
        /*        配置RX引脚为复用上拉模式 */
        gpio_mode_set(PORT_WIFI_RX, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_WIFI_RX);
        
        /*        配置PA2引脚为为输出模式 */
        gpio_output_options_set(PORT_WIFI_TX, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_WIFI_TX);
        
        /*        配置PA3引脚为为输出模式 */
        gpio_output_options_set(PORT_WIFI_RX, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_WIFI_RX);
        
        /*        设置WIFI_USART的波特率为115200 */
        usart_baudrate_set(WIFI_USART, bund);
        
        /*        设置WIFI_USART的校验位为无 */
        usart_parity_config(WIFI_USART, USART_PM_NONE);
        
        /*        设置WIFI_USART的数据位为8位 */
        usart_word_length_set(WIFI_USART, USART_WL_8BIT);
        
        /*        设置WIFI_USART的停止位为1位 */
        usart_stop_bit_set(WIFI_USART, USART_STB_1BIT);
        
        /*        使能串口1 */
        usart_enable(WIFI_USART);
        
        /*        使能WIFI_USART传输 */
        usart_transmit_config(WIFI_USART, USART_TRANSMIT_ENABLE);
        
        /*        使能WIFI_USART接收 */
        usart_receive_config(WIFI_USART, USART_RECEIVE_ENABLE);
        
        /*        使能WIFI_USART接收中断标志位 */
        usart_interrupt_enable(WIFI_USART, USART_INT_RBNE);   
        
  /*        使能WIFI_USART空闲中断标志位 */
        usart_interrupt_enable(WIFI_USART, USART_INT_IDLE); // DLE 线检测中断

        /* 配置中断优先级 */
        nvic_irq_enable(WIFI_USART_IRQ, 2, 2); // 配置中断优先级
}

/******************************************************************
 * 函 数 名 称：WIFI_USART_Send_Bit
 * 函 数 说 明：向WIFI模块发送单个字符
 * 函 数 形 参：ch=字符
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void WIFI_USART_Send_Bit(unsigned char ch)
{
        //发送字符
        usart_data_transmit(WIFI_USART, ch);
        // 等待发送数据缓冲区标志自动置位
        while(RESET == usart_flag_get(WIFI_USART, USART_FLAG_TBE) );
}  


/******************************************************************
 * 函 数 名 称：WIFI_USART_send_String
 * 函 数 说 明：向WIFI模块发送字符串
 * 函 数 形 参：str=发送的字符串
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void WIFI_USART_send_String(unsigned char *str)
{
        while( str && *str ) // 地址为空或者值为空跳出
        {        
                WIFI_USART_Send_Bit(*str++);
        }        
}
//清除串口接收的数据
/******************************************************************
 * 函 数 名 称：Clear_WIFI_RX_BUFF
 * 函 数 说 明：清除WIFI发过来的数据
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void Clear_WIFI_RX_BUFF(void)
{
        unsigned char i = WIFI_RX_LEN_MAX-1;
        while(i)        
        {
                WIFI_RX_BUFF[i--] = 0;
        }
        WIFI_RX_LEN = 0;
        WIFI_RX_FLAG = 0;
}


/******************************************************************
 * 函 数 名 称：WIFI_Send_Cmd
 * 函 数 说 明：向WIFI模块发送指令，并查看WIFI模块是否返回想要的数据
 * 函 数 形 参：cmd=发送的AT指令        ack=想要的应答                waitms=等待应答的时间                cnt=等待应答多少次
 * 函 数 返 回：1=得到了想要的应答                0=没有得到想要的应答
 * 备       注：无
******************************************************************/
char WIFI_Send_Cmd(char *cmd,char *ack,unsigned int waitms,unsigned char cnt)
{        
        WIFI_USART_send_String((unsigned char*)cmd);//1.发送AT指令
        while(cnt--)
        {
        //时间间隔
                delay_ms(waitms);
                //串口中断接收蓝牙应答
                if( WIFI_RX_FLAG == 1 )
                {
                        WIFI_RX_FLAG = 0;
                        WIFI_RX_LEN = 0;
            //查找是否有想要的数据
                        if( strstr((char*)WIFI_RX_BUFF, ack) != NULL )
                        {
                                return 1;
                        }
            //清除接收的数据
                        memset( WIFI_RX_BUFF, 0, sizeof(WIFI_RX_BUFF) );
                }
        }
        WIFI_RX_FLAG = 0;
        WIFI_RX_LEN = 0;
        return 0;
}

/******************************************************************
 * 函 数 名 称：WIFI_ESP01S_Init
 * 函 数 说 明：WIFI模块ESP01S初始化
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 备       注：ESP01S的默认波特率是115200
******************************************************************/
void WIFI_ESP01S_Init(void)
{
        WIFI_USART_Init(115200);//默认波特率为115200
}

/******************************************************************
 * 函 数 名 称：WIFI_MODE_AP_Init
 * 函 数 说 明：开启AP模式，即模块开启热点让手机连接
 * 函 数 形 参：无
 * 函 数 返 回：1=配置成功   其他=失败
 * 备       注：手机通过WIFI模块默认的IP地址192.168.4.1和设置的端口号，进行连接
******************************************************************/
uint8_t WIFI_MODE_AP_Init(void)
{
    uint8_t ret = 0;
        char send_buff[200];
    
    ret = WIFI_Send_Cmd("AT\r\n", "OK", 10, 3);//测试指令：AT\r\n  成功返回OK  失败返回ERROR
        if( ret != 1 ) return ret;
    
    ret = WIFI_Send_Cmd("AT+CWMODE=2\r\n","OK",30,3);   //配置WIFI AP模式
    if( ret != 1 ) return ret;
    
    sprintf(send_buff, "AT+CWSAP=\"%s\",\"%s\",11,4\r\n", AP_WIFISSID,AP_WIFIPASS );
        ret = WIFI_Send_Cmd(send_buff,"OK",30,3);  //设置wifi账号与密码
        if( ret != 1 ) return ret;
    
    ret = WIFI_Send_Cmd("AT+RST\r\n","ready",800,3); //重新复位
        if( ret != 1 ) return ret;
    ret = WIFI_Send_Cmd("AT+CIPMUX=1\r\n","OK",50,3); //开启多个连接
        if( ret != 1 ) return ret;
    ret = WIFI_Send_Cmd("AT+CIPSERVER=1,5000\r\n","OK",50,3); //开启服务器设置端口号为5000
    if( ret != 1 ) return ret;
    return ret;
}





/******************************************************************
 * 函 数 名 称：Get_Device_connection_status
 * 函 数 说 明：获取设备连接状态(AP模式)
 * 函 数 形 参：无
 * 函 数 返 回：0=没有设备连接 
 *              1=有设备连接了WIFI   
 *              2=有设备断开了WIFI     
 *              3=有设备连接了服务器
 *              4=有设备断开了服务器
 * 备       注：手机要连接WIFI模块的步骤是先连接WIFI再连接服务器
//当有设备连接AP模式下的热点时，WIFI模块会给连接的设备分配IP地址
//我们只需检测是否有分配地址，则知道是否有设备连接。
//设备连接时WIFI返回：
//  +STA_CONNECTED:"f0:6c:5d:d6:f6:18"
//  +DIST_STA_IP:"f0:6c:5d:d6:f6:18","192.168.4.2"
//设备断开连接时返回：
//  +STA_DISCONNECTED:"f0:6c:5d:d6:f6:18"
******************************************************************/
uint8_t Get_Device_connection_status(void)
{
    //串口中断接收WIFI应答
    if( WIFI_RX_FLAG == 1 )
    {
        WIFI_RX_FLAG = 0;
        WIFI_RX_LEN = 0;
        //有设备连接了热点
        if( strstr((char*)WIFI_RX_BUFF, "+STA_CONNECTED") != NULL )
        {
            //清除接收的数据
            wifi_link_flag = 1;
            memset( WIFI_RX_BUFF, 0, sizeof(WIFI_RX_BUFF) );
#if        DEBUG
 printf("The device is connected to a hotspot.\r\n");
 #endif            
            return 1;
        }
        //有设备断开了热点
        if( strstr((char*)WIFI_RX_BUFF, "+STA_DISCONNECTED") != NULL ) 
        {
            //清除接收的数据
            wifi_link_flag = 0;
            memset( WIFI_RX_BUFF, 0, sizeof(WIFI_RX_BUFF) );
#if        DEBUG
 printf("The device is disconnected from the hotspot.\r\n");
 #endif 
            return 2;
        }
        //有设备连接了服务器
        if( strstr((char*)WIFI_RX_BUFF, ",CONNECT") != NULL ) 
        {
            //清除接收的数据
            wifi_link_flag = 2;
            memset( WIFI_RX_BUFF, 0, sizeof(WIFI_RX_BUFF) );
            return 3;
#if        DEBUG
 printf("The device is connected to the server.\r\n");
 #endif
        }        
        //有设备断开了服务器
        if( strstr((char*)WIFI_RX_BUFF, ",CLOSED") != NULL ) 
        {
            //清除接收的数据
            wifi_link_flag = 3;
            memset( WIFI_RX_BUFF, 0, sizeof(WIFI_RX_BUFF) );
            return 4;
#if        DEBUG
 printf("The device is disconnected from the server.\r\n");
 #endif
        }         
    }  
    return 0;    
}


/**********************************************************
 * 函 数 名 称：Get_WIFI_AP_Data
 * 函 数 功 能：解析设备发送过来的数据
 * 传 入 参 数：ap_parameter要将数据保存的地址
 * 函 数 返 回：1：有设备发送过来数据        0：没有设备发送过来数据
 * 备       注：device_id最大5个  //+IPD,1,4:abcd
**********************************************************/    
uint8_t Get_WIFI_AP_Data(AP_PARAMETER *ap_parameter)
{
    char buff[50];
    char *test;
    
    char i=0;

    //接收到设备发过来的数据
    if( strstr((char*)WIFI_RX_BUFF,"+IPD,") != NULL )
    {
        test = strstr((char*)WIFI_RX_BUFF,"+IPD,");
        
        //记录设备ID号
        strncpy(buff,test+5,1);  
        buff[1] ='\0';
        ap_parameter->device_id = atoi(buff);
        printf("device_id = %s\r\n",buff);
                
        //记录发送过来的数据长度
        strncpy(buff,test+7,strcspn(test+7,":") );  
        buff[ strcspn(test+7,":") ] ='\0';
                printf("device_data = %s\r\n",buff);
        ap_parameter->device_datalen = atoi(buff);
        printf("device_datalen = %s\r\n",buff);
        //记录发送过来的数据
        memset(buff,0,sizeof(buff));  
        while(test[i++]!=':');
        strncpy(buff, test+i,strcspn(test+i,"\r") );
                printf("device_data = %s\r\n",buff);
        strcpy((char*)ap_parameter->device_data, buff);
        
        //清除串口接近缓存
        Clear_WIFI_RX_BUFF();
        return 1;
    }
    return 0;
}
/******************************************************************
 * 函 数 名 称：WIFI_Send_To_Client
 * 函 数 说 明：AP模式下，WIFI发送数据至客户端（连接AP模式下热点的设备）
 * 函 数 形 参：id=向第几个客户端发送数据   data=要发送的数据（字符串形式）
 * 函 数 返 回：0=发送失败   1=发送成功
 * 备       注：使用该函数函数请确保WIFI模块处于AP模式
******************************************************************/
uint8_t WIFI_Send_To_Client(uint8_t id,char * data)
{
        uint8_t send_buf[20]={0};
        sprintf((char*)send_buf,"AT+CIPSEND=%d,%d\r\n",id,strlen(data));
        if(WIFI_Send_Cmd((char*)send_buf,">",20,3))
        {
                WIFI_USART_send_String((unsigned char *)data);
                return 1;
        }
        return 0;        
}

/******************************************************************
 * 函 数 名 称：mstrcat
 * 函 数 说 明：字符串连接
 * 函 数 形 参：s1：目标字符串， s2：源字符串
 * 函 数 返 回：无
 * 备       注：哈希使用
******************************************************************/
static void mstrcat(char *s1, const char *s2)
{
        if(*s1 != NULL)
                while(*++s1);
        while((*s1++ = *s2++));
}

/******************************************************************
 * 函 数 名 称：CalculateSha1
 * 函 数 说 明：计算sha1密匙
 * 函 数 形 参：password：密匙存放缓冲区
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
static void CalculateSha1(unsigned char *password)         
{           
        unsigned char temp[3] = {0};
        unsigned char digest[30]={0};
        unsigned char cnt = 0;
        hmac_sha1((unsigned char *)DeviceSecret,32,(unsigned char *)Encryption,46,digest);
        memset(temp, 0, sizeof(temp));
        for(cnt=0;cnt<20;cnt++)
        {
                sprintf((char *)temp,"%02X",digest[cnt]);
                mstrcat((char *)password,(char *)temp);
        }
}

/******************************************************************
 * 函 数 名 称：WIFI_MODE_STA_Aliyun_Init
 * 函 数 说 明：配置WIFI模块连接阿里云物联网平台
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void WIFI_MODE_STA_Aliyun_Init(void)
{
        char AT_CMD[250]={0};
        uint8_t PassWord[50] = {0};  //存放的是哈希计算的密钥
        
        RST:
    //测试指令AT
        WIFI_Send_Cmd("AT\r\n","OK",100,3);

        //配置WIFI STA
        WIFI_Send_Cmd("AT+CWMODE=1\r\n","OK",300,3);
    
        //设置时区  NTSP服务器  用于调整客户端自身所在系统的时间，达到同步时间的目的        
        WIFI_Send_Cmd("AT+CIPSNTPCFG=1,8,\"ntp1.alliyun.com\"\r\n","OK",100,3);
    
        //连接wifi 账号&密码
        sprintf(AT_CMD,"AT+CWJAP=\"%s\",\"%s\"\r\n",WIFISSID,WIFIPASS);
    
        if( WIFI_Send_Cmd(AT_CMD,"OK",3000,3) == 0 )
        {
                printf("WIFI名称或密码有错,复位重启\r\n");
        //wifi连接不上，重启
                WIFI_Send_Cmd("AT+RST\r\n","ready",1000,1);  
                goto RST;
        }
        //清0数组，备用
        memset(AT_CMD,0,sizeof(AT_CMD));  
    
    //计算哈希     
        CalculateSha1(PassWord);        

    #if        DEBUG
//    sprintf(PassWord,"%s","AF7596ADFFD4C57C5FD43F1CA1A20DE961634360");
        printf("haxi=%s\r\n",PassWord);
        printf("UserName=%s\r\n",UserName);
    #endif
    
        sprintf(AT_CMD,"AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"\r\n", UserName, PassWord);
        WIFI_Send_Cmd(AT_CMD,"OK",2000,3);
        
        //设置连接客户端ID
        memset(AT_CMD,0,sizeof(AT_CMD));  //清0数组，备用
        sprintf(AT_CMD,"AT+MQTTCLIENTID=0,\"%s\"\r\n",ClientId);
        WIFI_Send_Cmd(AT_CMD,"OK",1000,3);
        
        //连接到MQTT代理（阿里云平台）
        memset(AT_CMD,0,sizeof(AT_CMD));
        sprintf(AT_CMD,"AT+MQTTCONN=0,\"%s\",1883,1\r\n",IP);
        if(WIFI_Send_Cmd(AT_CMD,"OK",2000,3)==0)
        {
                printf("连接aliyu失败,复位STM32重连\r\n");
        //wifi连接不上，重启  1000延时1S    2链接次数
                WIFI_Send_Cmd("AT+RST\r\n","ready",1000,2);  
                __set_FAULTMASK(1); //STM32程序软件复位
                NVIC_SystemReset();
        }
        
        //订阅主题
        memset(AT_CMD,0,sizeof(AT_CMD));
        sprintf(AT_CMD, "AT+MQTTSUB=0,\"%s\",1\r\n", PublishMessageTopSet);
        WIFI_Send_Cmd(AT_CMD,"OK",1000,3);
        printf("连接aliyu成功\r\n");
        Clear_WIFI_RX_BUFF();//清除串口接收缓存
    //上电就上传数据至手机
//        Publish_MQTT_message(publish_mqtt_message,5,); //发布主题             
}    




/*点击LED开关
+MQTTSUBRECV:0,"/sys/a1PJRLOWo3p/TEST/thing/service/property/set",100,
{"method":"thing.service.property.set","id":"367399823","params":{"LED_Switch":1},"version":"1.0.0"}
*/

/*滑动亮度条
+MQTTSUBRECV:0,"/sys/a1PJRLOWo3p/TEST/thing/service/property/set",101,
{"method":"thing.service.property.set","id":"812539841","params":{"brightness":75},"version":"1.0.0"}
*/
/******************************************************************
 * 函 数 名 称：Get_Aliyun_json_data
 * 函 数 说 明：获取阿里云JSON格式的数据（接收手机发送过来的数据）
 * 函 数 形 参：data=数据的保存地址
 * 函 数 返 回：1=接收到JSON数据并处理   0=没有接收到数据
 * 备       注：无
******************************************************************/
uint8_t Get_Aliyun_json_data(JSON_PUBLISH *data)
{

    char *buff = 0;

    // 串口中断接收WIFI应答
    if (WIFI_RX_FLAG == SET)
    {
        printf("\r\n--\r\n");
        WIFI_RX_FLAG = 0;
        WIFI_RX_LEN = 0;

        // 确认接收到的数据包含 "params":
        if (strstr((char*)WIFI_RX_BUFF, "params\":") != NULL)
        {
            printf("Received data: %s\n", WIFI_RX_BUFF);  // 打印接收到的原始数据

            // 获取功能名称
            buff = strstr((char*)WIFI_RX_BUFF, "params\":");
            buff += strlen("params\":{\"");
            // 获取 keyname
            strcpy(data->keyname, strtok(buff, "\""));
            printf("data->keyname = %s\r\n", data->keyname);

            // 获取功能值
            buff = strstr((char*)WIFI_RX_BUFF, "params\":") + strlen("params\":{\"") + strlen(data->keyname) + 2;
            strcpy(data->value, strtok(buff, "}"));
            printf("data->value = %s\r\n", data->value);  // 打印解析的value值

            return 1;
        }
    }

    return 0;
}


/******************************************************************
 * 函 数 名 称：Clear_Aliyun_json_data
 * 函 数 说 明：清除JSON接收过的数据
 * 函 数 形 参：data=要清除的数据
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void Clear_Aliyun_json_data(JSON_PUBLISH *data)
{
        uint16_t i = 0;
        while( data->keyname[i] != 0 )
        {
                data->keyname[i++] = '\0';
        }
        i= 0;
        while( data->value[i] != 0 )
        {
                data->keyname[i++] = '\0';
        }
}

//标识符
JSON_PUBLISH publish_mqtt_message[9]=        
{
        { "\\\"temperature\\\":","0" },
        { "\\\"humidity\\\":","0" },
				{"\\\"soil\\\":","0" },
				{"\\\"mq135\\\":","0" },
				{"\\\"flame\\\":","0" },
				{"\\\"bump_Switch\\\":","0" },
				{"\\\"jingdu\\\":","0" },//东西
				{"\\\"weidu\\\":","0" },//南北
				{"\\\"fire_Flag\\\":","0" }
};            

/******************************************************************
 * 函 数 名 称：Publish_MQTT_message
 * 函 数 说 明：发布主题 ，上发多个数据(设备将数据发送至手机)
 * 函 数 形 参：data=publish_mqtt_message， data_num=上传的数据个数
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void Publish_MQTT_message(JSON_PUBLISH *data,uint8_t data_num)  
{
        char AT_CMD[384]={0};
        char params[256]={0},i,*sp;
        
        sp=params;
        
        sprintf(data[0].value,"%f",Get_temperature());   //把传感器的值赋值给json结构体的value
        sprintf(data[1].value,"%f",Get_humidity());
				sprintf(data[2].value,"%d",Get_Percentage_value(0));
        sprintf(data[3].value,"%d",Get_Percentage_value(1));
        sprintf(data[4].value,"%d",Get_Percentage_value(2));	
				sprintf(data[5].value,"%d",bump_Flag);
				sprintf(data[6].value, "%lf", lon_value);
				sprintf(data[7].value, "%lf", lat_value);
				sprintf(data[8].value,"%d",fire_Flag);
				
        //          4
        for(i=0;i<data_num;i++)
        {        // 3
                if(i<(data_num-1))
                {   
                        sprintf(sp,"%s%s%s",data[i].keyname,data[i].value,"\\,");
                        while(*sp!=0) {sp++;} //防止覆盖
                }
                else
                        sprintf(sp,"%s%s",data[i].keyname,data[i].value);
        }
        sprintf(AT_CMD,"AT+MQTTPUB=0,\"%s\",\"{\\\"params\\\":{%s}}\",1,0\r\n",PublishMessageTopPost,params);
        //发送指令后等待2000ms再判断是否发送成功，不发送成功则重发3-1次
    //这里会阻塞CPU的运行,可以直接使用    WIFI_USART_send_String(AT_CMD); 发送，但是无法判断是否发送成功。
    WIFI_Send_Cmd(AT_CMD,"OK",2000,3);                
}


/******************************************************************
 * 函 数 名 称：WIFI_USART_IRQHandler
 * 函 数 说 明：连接WIFI的串口中断服务函数
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 备       注：无
******************************************************************/
void WIFI_USART_IRQHandler(void)
{
        if(usart_interrupt_flag_get(WIFI_USART,USART_INT_FLAG_RBNE) != RESET) // 接收缓冲区不为空
        {
        //接收数据
                WIFI_RX_BUFF[ WIFI_RX_LEN ] = usart_data_receive(WIFI_USART);
        
#if DEBUG
        //测试，查看接收到了什么数据
        printf("%c", WIFI_RX_BUFF[ WIFI_RX_LEN ]);
#endif
                //接收长度限制
        WIFI_RX_LEN = ( WIFI_RX_LEN + 1 ) % WIFI_RX_LEN_MAX;
        }
        if(usart_interrupt_flag_get(WIFI_USART,USART_INT_FLAG_IDLE) == SET) // 检测到空闲中断
        {
                usart_data_receive(WIFI_USART); // 必须要读，读出来的值不能要
                WIFI_RX_BUFF[WIFI_RX_LEN] = '\0'; //字符串结尾补 '\0'
                WIFI_RX_FLAG = SET;            // 接收完成
				usart_interrupt_flag_clear(WIFI_USART, USART_INT_FLAG_IDLE);
        }
}

