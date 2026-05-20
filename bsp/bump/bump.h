#ifndef _BUMP_H_
#define _BUMP_H_

/* PA5  TIMER1_CH0 */
#define BSP_PWM_RCU      	RCU_GPIOA
#define BSP_PWM_PORT    	GPIOA
#define BSP_PWM_PIN      	GPIO_PIN_5
#define BSP_PWM_AF			GPIO_AF_1

/* TIMER */
#define BSP_PWM_TIMER_RCU  	RCU_TIMER1  // 定时器时钟
#define BSP_PWM_TIMER    	TIMER1   	// 定时器
#define BSP_PWM_TIMER_CH	TIMER_CH_0	// 定时器输出通道

//#define TARGET_HUMIDITY 30.0f  // 目标湿度值，单位：%
#define MAX_PWM_VALUE 10000  // 最大 PWM 值（1KHz周期）

// PID 参数定义
#define Kp 1.0f   // 比例增益
#define Ki 0.1f   // 积分增益
#define Kd 0.01f  // 微分增益


void bsp_pwm_init(void);
void pid_update(int current_humidity,int TARGET_HUMIDITY);

#endif