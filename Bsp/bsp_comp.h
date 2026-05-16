#include "main.h"
#ifndef BSP_COMP_H
#define BSP_COMP_H

void BSP_COMP_Init(void);//默认比较电压为0
void BSP_COMP_SetVref(float vref);
void BSP_COMP_SetVrefDACValue(uint16_t dac_value);

#endif // BSP_COMP_H