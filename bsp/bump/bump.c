#include "board.h"
#include "bump.h"

// PID 控制器变量
float previous_error = 0.0f;
float integral = 0.0f;
float output_pwm = 0.0f;
int falme_flag,soil_flag;

void bsp_pwm_init(void)
{     
    /* 开启时钟 */
    rcu_periph_clock_enable(BSP_PWM_RCU);
    gpio_mode_set(BSP_PWM_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, BSP_PWM_PIN);  
    gpio_output_options_set(BSP_PWM_PORT,GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,BSP_PWM_PIN);    
//    /* 配置IO为定时器的通道 */ 
//    gpio_af_set(BSP_PWM_PORT, GPIO_AF_1, BSP_PWM_PIN);
		
		gpio_bit_write(GPIOA, GPIO_PIN_5, RESET); 
		
	timer_parameter_struct timer_initpara; // 定义定时器结构体
	
	/* 开启时钟 */    
	rcu_periph_clock_enable(BSP_PWM_TIMER_RCU); // 开启定时器时钟    
	/* CK_TIMERx = 4 x CK_APB1  = 4x50M = 200MHZ */
	
	rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4); // 配置定时器时钟        
	timer_deinit(BSP_PWM_TIMER); // 复位定时器 
	
	/* 配置定时器参数 */    
	timer_initpara.prescaler = 168 - 1; //  时钟预分频值  PSC_CLK= 168MHZ / 168 = 1MHZ      
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE; // 边缘对齐    
	timer_initpara.counterdirection = TIMER_COUNTER_UP; // 向上计数    
	timer_initpara.period = 10000 - 1; // 周期   T = 10000  1MHZ = 10ms  f = 100HZ       
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1; // 分频因子       
	timer_initpara.repetitioncounter = 0; // 重复计数器 0-255  
	timer_init(BSP_PWM_TIMER,&timer_initpara); // 初始化定时器
	
	/* 使能定时器 */
	timer_enable(BSP_PWM_TIMER);
	
	timer_oc_parameter_struct timer_ocinitpara;
	/* 初始化定时器通道输出参数 */
    timer_channel_output_struct_para_init(&timer_ocinitpara);
    /* 配置定时器通道输出功能 */
    timer_ocinitpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocinitpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;


	/* 配置定时器输出功能 */
	timer_channel_output_config(BSP_PWM_TIMER,BSP_PWM_TIMER_CH,&timer_ocinitpara);
	
	/* 配置定时器通道输出脉冲值 */
	timer_channel_output_pulse_value_config(BSP_PWM_TIMER,BSP_PWM_TIMER_CH,5000 - 1);
	
	/* 配置定时器通道输出比较模式 */
	timer_channel_output_mode_config(BSP_PWM_TIMER,BSP_PWM_TIMER_CH,TIMER_OC_MODE_PWM0);
	
	/* 配置定时器通道输出影子寄存器 */
	timer_channel_output_shadow_config(BSP_PWM_TIMER,TIMER_CH_0,TIMER_OC_SHADOW_DISABLE);
	
	/* 自动重载使能 */ 
	timer_auto_reload_shadow_enable(BSP_PWM_TIMER);
}

void pid_update(int current_humidity,int TARGET_HUMIDITY)
{
    float error = TARGET_HUMIDITY - current_humidity;  // 误差计算
    integral += error;  // 积分
    float derivative = error - previous_error;  // 微分

    // PID 控制输出
    output_pwm = Kp * error + Ki * integral + Kd * derivative;

    // 限制输出范围
    if (output_pwm > 10000) 
		{
        output_pwm = 10000;
    } 
		else if (output_pwm < 0) 
		{
        output_pwm = 0;
    }

    previous_error = error;  // 保存当前误差，用于下次微分计算
		
		if(!falme_flag&&!soil_flag)
		{
			output_pwm = 0;
		}
		
		printf("%d\r\n",(int)output_pwm);
			// 更新 PWM 输出 (根据计算出的 PID 输出调整 PWM 值)
		timer_channel_output_pulse_value_config(BSP_PWM_TIMER, BSP_PWM_TIMER_CH, (uint16_t)(output_pwm));

}