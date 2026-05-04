#include "clock_gen.h"
#include "bsp_comp.h"
#include "stm32_hal_legacy.h"
#include "stm32g4xx_hal_tim.h"
#include "tim.h"
#include <stdint.h>

/* ---------- 私有定义 ---------- */



/* ---------- 公有函数 ---------- */

void ClockGen_Init(void)
{
    BSP_COMP_Init();
    BSP_COMP_SetVref(0.5); 
    /* 设置一个初始分频比, 假设输入 5kHz → ratio=1, ARR=0 */
    __HAL_TIM_SET_AUTORELOAD(&htim1, 1);  
    /* 启动 TIM1 OC Toggle 输出 */
    HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_2);
}

void ClockGen_Update(void)
{
    __HAL_TIM_SET_AUTORELOAD(&htim1, 65535);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);   // 先清除残留标志
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __disable_irq();                                  // 关中断，避免抖动影响测量

    uint16_t lastCounter = __HAL_TIM_GET_COUNTER(&htim1);

    /* 启动 TIM2 OPM 单次定时 (2ms 窗口) */
    HAL_TIM_Base_Start(&htim2);                       // OPM 模式，计数到 ARR 自动停止

    while (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) == RESET);

    uint16_t currentCounter = __HAL_TIM_GET_COUNTER(&htim1);
    __enable_irq();
    HAL_TIM_Base_Stop(&htim2);                                   // 恢复中断
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    __HAL_TIM_SET_COUNTER(&htim2, 0);// 清除计数器，为下一次测量做准备
    uint16_t countDiff = currentCounter - lastCounter;
    if(countDiff%200 > 180) countDiff = countDiff - countDiff%200 + 200; // 考虑到测量窗口内可能的抖动，进行简单的误差修正
    else if(countDiff%200 < 20) countDiff = countDiff - countDiff%200 ;
    else countDiff = countDiff - countDiff%200;
    uint16_t targetARR = (countDiff / 200)-1;
    __HAL_TIM_SET_AUTORELOAD(&htim1, targetARR);
    __HAL_TIM_SET_COUNTER(&htim1, 0);
}
