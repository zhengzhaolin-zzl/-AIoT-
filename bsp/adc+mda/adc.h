#ifndef __ADC_H__
#define __ADC_H__

#include "gd32f4xx.h"

void ADC_config(void);
void DMA_config(void);

unsigned int Get_Adc_Dma_Value(char CHx);
unsigned int Get_Percentage_value(char CHx);
#endif