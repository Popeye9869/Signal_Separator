#ifndef CLOCK_GEN_H
#define CLOCK_GEN_H

#include "main.h"

/**
 * @brief  目标输出频率 (Hz)
 */
#define CLOCK_GEN_TARGET_FREQ  5000U

/**
 * @brief  初始化时钟发生器模块
 *         启动 TIM2 输入捕获 (测量比较器频率) 和 TIM1 OC Toggle 输出
 */
void ClockGen_Init(void);

/**
 * @brief  TIM2 输入捕获回调处理 (在 HAL_TIM_IC_CaptureCallback 中调用)
 * @param  htim  定时器句柄
 */
void ClockGen_IC_CaptureCallback(TIM_HandleTypeDef *htim);

#endif /* CLOCK_GEN_H */
