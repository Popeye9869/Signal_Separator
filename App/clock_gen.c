// #include "clock_gen.h"
// #include "tim.h"

// /**
//  * 原理说明:
//  *
//  * 比较器(COMP1)输入为两路不同频率/波形信号之和，输出方波频率等于输入信号的
//  * 过零频率。两信号频率均为 5kHz 的整数倍 (5kHz ~ 1000kHz)。
//  *
//  * TIM1 配置:
//  *   - 时钟源: COMP1 输出 (ETR)
//  *   - CH1: OC Toggle 模式
//  *   - 向下计数, Period = N-1
//  *   - 输出频率 = f_comp / (2*N)
//  *   - 要得到 5kHz:  N = f_comp / 10000
//  *
//  * TIM2 配置:
//  *   - 内部时钟 170MHz
//  *   - CH1 输入捕获, 双边沿, 信号源 = COMP1
//  *   - 通过连续两次捕获的差值测量比较器输出半周期
//  *   - f_comp = 170MHz / (2 * half_period_ticks)  -- 双边沿捕获的是半周期
//  *   - 实际上双边沿捕获: 两次捕获间隔 = 半周期
//  *     f_comp = 170MHz / (2 * delta)  但 delta 是半周期对应的 ticks
//  *     更准确: f_comp = 1 / T, T = 2 * delta / 170MHz
//  *     所以 f_comp = 170MHz / (2 * delta)
//  *
//  * 为提高精度，累计多个捕获求平均。
//  */

// /* ---------- 私有定义 ---------- */

// #define TIM_CLK_HZ       170000000UL   /* TIM2 内部时钟频率 */
// #define AVG_SAMPLES       16U          /* 累计采样次数 (取平均) */

// /* ---------- 私有变量 ---------- */

// static volatile uint32_t g_last_capture  = 0;
// static volatile uint32_t g_accumulator   = 0;
// static volatile uint32_t g_sample_count  = 0;
// static volatile uint8_t  g_first_edge    = 1;

// /* ---------- 私有函数 ---------- */

// /**
//  * @brief  根据测量到的比较器频率更新 TIM1 的分频比 (ARR)
//  * @param  freq_comp  比较器输出频率 (Hz)
//  */
// static void ClockGen_UpdateDivider(uint32_t freq_comp)
// {
//     /* 输入信号频率必须是 5kHz 的倍数, 做四舍五入量化 */
//     uint32_t ratio = (freq_comp + (CLOCK_GEN_TARGET_FREQ / 2)) / CLOCK_GEN_TARGET_FREQ;
//     if (ratio == 0) ratio = 1;

//     /*
//      * TIM1 OC Toggle: 输出频率 = f_etr / (2 * (ARR+1))
//      * 因为 Toggle 模式下每 (ARR+1) 个 ETR 边沿翻转一次,
//      * 一个完整周期需要翻转两次 → 2*(ARR+1) 个 ETR 边沿
//      *
//      * 要 5kHz:  2*(ARR+1) = f_comp / 5000 = 2*ratio
//      *           ARR+1 = ratio
//      *           ARR   = ratio - 1
//      */
//     uint32_t arr = ratio - 1;
//     if (arr > 0xFFFF) arr = 0xFFFF;   /* TIM1 是 16 位定时器 */

//     __HAL_TIM_SET_AUTORELOAD(&htim1, arr);
// }

// /* ---------- 公有函数 ---------- */

// void ClockGen_Init(void)
// {
//     /* 设置一个初始分频比, 假设输入 5kHz → ratio=1, ARR=0 */
//     __HAL_TIM_SET_AUTORELOAD(&htim1, 0);

//     /* 启动 TIM1 OC Toggle 输出 */
//     HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1);

//     /* 启动 TIM2 输入捕获 (中断模式) */
//     HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
// }

// void ClockGen_IC_CaptureCallback(TIM_HandleTypeDef *htim)
// {
//     if (htim->Instance != TIM2) return;

//     uint32_t capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

//     if (g_first_edge) {
//         g_first_edge = 0;
//         g_last_capture = capture;
//         return;
//     }

//     /* 计算两次捕获间的 tick 差 (处理溢出, TIM2 是 32 位) */
//     uint32_t delta = capture - g_last_capture;   /* 无符号减法自动处理溢出 */
//     g_last_capture = capture;

//     /* delta 是半周期的 ticks (双边沿捕获) */
//     g_accumulator += delta;
//     g_sample_count++;

//     if (g_sample_count >= AVG_SAMPLES) {
//         uint32_t avg_half_period = g_accumulator / AVG_SAMPLES;

//         /* f_comp = TIM_CLK / (2 * avg_half_period)
//          * 但 delta 本身就是半周期, 所以:
//          * 半周期(秒) = avg_half_period / TIM_CLK
//          * T = 2 * avg_half_period / TIM_CLK
//          * f = TIM_CLK / (2 * avg_half_period)
//          */
//         if (avg_half_period > 0) {
//             uint32_t freq_comp = TIM_CLK_HZ / (2U * avg_half_period);
//             ClockGen_UpdateDivider(freq_comp);
//         }

//         g_accumulator  = 0;
//         g_sample_count = 0;
//     }
// }
