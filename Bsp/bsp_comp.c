#include "bsp_comp.h"

#include "main.h"
#include "comp.h"
#include "dac.h"
#include "spi.h"
#include "stm32g4xx_hal_dac.h"
#include "tim.h"
#include "gpio.h"

void BSP_COMP_Init(void)
{
    HAL_COMP_Start(&hcomp1);
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0); // 默认比较电压为0V
}

void BSP_COMP_SetVref(float vref)
{
    if (vref < 0) vref = 0;
    if (vref > 3.3) vref = 3.3;
    uint32_t dac_value = (uint32_t)((vref / 3.3) * 4095);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
}
