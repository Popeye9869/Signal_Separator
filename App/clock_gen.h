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

void ClockGen_Update(void);

#endif /* CLOCK_GEN_H */
