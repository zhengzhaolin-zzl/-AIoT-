#ifndef _TIME_H_
#define _TIME_H_

#include "gd32f4xx.h"	

#define  BSP_TIMER_RCU    	RCU_TIMER5  		// 定时器时钟
#define  BSP_TIMER   	  	TIMER5   			// 定时器
#define  BSP_TIMER_IRQ   	TIMER5_DAC_IRQn   	// 定时器中断

/* TIMER2 */
#define BSP_TIMER_RCU2             RCU_TIMER2  // 定时器时钟
#define BSP_TIMER2                 TIMER2   // 定时器
#define BSP_TIMER_IRQ2             TIMER2_IRQn   // 定时器中断

void bsp_timer_init1(uint16_t pre, uint16_t per);
void TIMER5_DAC_IRQHandler(void);


void bsp_timer_init2(uint16_t pre, uint16_t per);
void TIMER2_IRQHandler(void);
#endif