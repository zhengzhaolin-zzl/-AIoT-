#include "board.h"
#include "time.h"

void bsp_timer_init1(uint16_t pre, uint16_t per)
{
	/* 开启时钟 */
	rcu_periph_clock_enable(BSP_TIMER_RCU);
	
	rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4); // 配置定时器时钟  
	
	/* 复位定时器 */
	timer_deinit(BSP_TIMER);
	
	timer_parameter_struct timer_initpara; 				// 定义定时器结构体
	/* 配置定时器参数 */
	timer_initpara.prescaler = pre -1; 					//  时钟预分频值 0-65535  psc_clk = CK_TIMER / pre
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE; 	// 边缘对齐
	timer_initpara.counterdirection = TIMER_COUNTER_UP; // 向上计数
	timer_initpara.period = per  - 1; 					// 周期
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1; 	// 分频因子
	timer_initpara.repetitioncounter = 0; 				// 重复计数器 0-255  
	timer_init(BSP_TIMER,&timer_initpara); 				// 初始化定时器
	
	/* 配置中断优先级 */
	nvic_irq_enable(BSP_TIMER_IRQ, 3, 2); // 设置中断优先级为 3,2断
	
	/* 使能中断 */
	timer_interrupt_enable(BSP_TIMER, TIMER_INT_UP); // 使能更新事件中断 
	
	/* 使能定时器 */
	timer_enable(BSP_TIMER);
}

void bsp_timer_init2(uint16_t pre, uint16_t per)
{
	/* 开启时钟 */
	rcu_periph_clock_enable(BSP_TIMER_RCU2);
	
	rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4); // 配置定时器时钟  
	
	/* 复位定时器 */
	timer_deinit(BSP_TIMER2);
	
	timer_parameter_struct timer_initpara; 				// 定义定时器结构体
	/* 配置定时器参数 */
	timer_initpara.prescaler = pre -1; 					//  时钟预分频值 0-65535  psc_clk = CK_TIMER / pre
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE; 	// 边缘对齐
	timer_initpara.counterdirection = TIMER_COUNTER_UP; // 向上计数
	timer_initpara.period = per  - 1; 					// 周期
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1; 	// 分频因子
	timer_initpara.repetitioncounter = 0; 				// 重复计数器 0-255  
	timer_init(BSP_TIMER2,&timer_initpara); 				// 初始化定时器
	
	/* 配置中断优先级 */
	nvic_irq_enable(BSP_TIMER_IRQ2, 2, 2); // 设置中断优先级为 2,2断
	
	/* 使能中断 */
	timer_interrupt_enable(BSP_TIMER2, TIMER_INT_UP); // 使能更新事件中断 
	
	/* 使能定时器 */
	timer_enable(BSP_TIMER2);
}
