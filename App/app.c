#include "app.h"
#include "main.h"
#include "bsp_comp.h"

void App_Init(void)
{
    BSP_COMP_Init();
    BSP_COMP_SetVref(0.5); // 设置比较电压为0.5V
}