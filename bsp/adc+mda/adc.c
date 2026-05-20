#include "board.h"
#include "adc.h"

void ADC_config(void)
{
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_GPIOC);

	rcu_periph_clock_enable(RCU_ADC0);

  adc_clock_config(ADC_ADCCK_PCLK2_DIV6);

	gpio_mode_set(GPIOC,GPIO_MODE_ANALOG,GPIO_PUPD_NONE,GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2);
	
	adc_deinit();
	
	adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT); //独立模式 
	
	adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    /* ADC scan mode disable */
  adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
	
	adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);//ADC  数据右对齐 
	
	adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 3);//常规通道长度配置 

	adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_56); 
	adc_routine_channel_config(ADC0, 1, ADC_CHANNEL_11, ADC_SAMPLETIME_56); 
	adc_routine_channel_config(ADC0, 2, ADC_CHANNEL_12, ADC_SAMPLETIME_56);
	
  adc_external_trigger_config(ADC0,ADC_ROUTINE_CHANNEL,EXTERNAL_TRIGGER_DISABLE);
	
	adc_dma_request_after_last_enable(ADC0);
	
  adc_dma_mode_enable(ADC0);

	/* enable ADC interface */ 
	adc_enable(ADC0); 
	
	delay_ms(1);
	
	adc_calibration_enable(ADC0);

  adc_software_trigger_enable(ADC0,ADC_ROUTINE_CHANNEL); 
}

uint16_t adc_value[3];

void DMA_config(void)
{
	dma_single_data_parameter_struct dma_single_data_parameter;
	
  rcu_periph_clock_enable(RCU_DMA1);
	/* ADC DMA_channel configuration */
	dma_deinit(DMA1, DMA_CH0);

	/* initialize DMA single data mode */
	dma_single_data_parameter.periph_addr = (uint32_t)(&ADC_RDATA(ADC0));
	dma_single_data_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_single_data_parameter.memory0_addr = (uint32_t)(&adc_value);
	dma_single_data_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_single_data_parameter.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
	dma_single_data_parameter.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
	dma_single_data_parameter.direction = DMA_PERIPH_TO_MEMORY;
	dma_single_data_parameter.number = 3;
	dma_single_data_parameter.priority = DMA_PRIORITY_HIGH;
	dma_single_data_mode_init(DMA1, DMA_CH0, &dma_single_data_parameter);
		
	/* enable DMA circulation mode */
	dma_circulation_enable(DMA1, DMA_CH0);

	/* enable DMA channel */
	dma_channel_enable(DMA1, DMA_CH0);
}

unsigned int Get_Adc_Dma_Value(char CHx)
{
        unsigned char i = 0;
        unsigned int AdcValue = 0;
    
    /* 因为采集 135 次，故循环 135 次 */
        for(i=0; i< 135; i++)
        {
        /*    累加    */
                AdcValue+=adc_value[CHx];
        }
    /* 求平均值 */
        AdcValue=AdcValue / 135;
    
        return AdcValue;
}

unsigned int Get_Percentage_value(char CHx)
{
    int adc_max = 4095;
    int adc_new = 0;
    int Percentage_value = 0;
    
    adc_new = Get_Adc_Dma_Value(CHx);
    
    
		if(CHx==1)
		{
			Percentage_value = ((float)adc_new/adc_max) * 100;
		}
		else if(CHx==0)
		{
			Percentage_value = (1-((float)adc_new/adc_max)) * 100 ;
		}
		else if(CHx==2)
		{	
			Percentage_value = (1-((float)adc_new/adc_max)) * 100;
		}
    return Percentage_value;
}