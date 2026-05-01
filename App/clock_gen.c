#include "clock_gen.h"
#include "bsp_comp.h"
#include "stm32_hal_legacy.h"
#include "stm32g4xx_hal_tim.h"
#include "tim.h"
#include <stdint.h>

/* ---------- 私有定义 ---------- */

#define TIM_CLK_HZ       170000000UL   /* TIM2 内部时钟频率 */
#define SAMPLES_TIME       16U          /* 累计采样时间 */


/* ---------- 公有函数 ---------- */

void ClockGen_Init(void)
{
    BSP_COMP_Init();
    BSP_COMP_SetVref(2.5); 
    /* 设置一个初始分频比, 假设输入 5kHz → ratio=1, ARR=0 */
    __HAL_TIM_SET_AUTORELOAD(&htim1, 1);  
    /* 启动 TIM1 OC Toggle 输出 */
    HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_2);
}

void ClockGen_Update(void)
{
    __HAL_TIM_SET_AUTORELOAD(&htim1, 65535);  // 设置 ARR 为最大值以获得更长的计数周期
    uint8_t lastCounter = __HAL_TIM_GET_COUNTER(&htim1);
    HAL_TIM_Base_Start(&htim2);
    while(__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) == RESET);// 等待计数完成
    uint8_t currentCounter = __HAL_TIM_GET_COUNTER(&htim1);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Stop(&htim2);
    uint8_t countDiff = currentCounter - lastCounter; // 计算计数差
    __HAL_TIM_SET_AUTORELOAD(&htim1, ((countDiff+1)/10)-1);  // 根据计数差调整 ARR, 以实现频率调整
    __HAL_TIM_SET_COUNTER(&htim1, 0); // 重置计数器
}
