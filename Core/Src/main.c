/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "comp.h"
#include "dac.h"
#include "dma.h"
#include "i2c.h"
#include "opamp.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "clock_gen.h"
#include "arm_math.h"
#include "SEGGER_RTT.h"
#include "AD9833.h"
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "oled.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t adcValues[1024]={0}; // 存储 ADC 采样值的数组
q15_t fftOutput[2048]={0}; // 存储 FFT 输出的数组
uint32_t fftMagnitude[512/5]={0}; // 存储 FFT 幅值的数组
arm_rfft_instance_q15 S; // 定义 RFFT 实例

AD9833_HandleTypeDef h9833_1 = {
  .FSYN_GPIO_Port = GPIOE,
  .FSYN_Pin = GPIO_PIN_15,
  .hspi = &hspi1,
  .MCLK_Hz = 1280000u
};

AD9833_HandleTypeDef h9833_2 = {
  .FSYN_GPIO_Port = GPIOC,
  .FSYN_Pin = GPIO_PIN_4,
  .hspi = &hspi1,
  .MCLK_Hz = 1280000u
};

typedef struct {
  uint32_t peak_freq;
  uint32_t peak_value;
} peak_Info_t;

typedef struct {
  uint8_t num_peak;
  peak_Info_t peak_info[5];
  uint8_t highest_peak_index;
} waveform_info_t;

typedef struct {
  uint32_t Freq;
  uint32_t Phase;
  AD9833_WaveformMode Waveform;
} DDS_OUTPUT;

DDS_OUTPUT dds_output[2] = {0};
waveform_info_t waveform_info = {0};


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void WaveIdentify(void)
{
  memset(&waveform_info, 0, sizeof(waveform_info)); // 清空波形信息结构体
  for (int i = 0; i < 512/5; i++)
  {
    if (fftMagnitude[i] > 130)
    {
      waveform_info.peak_info[waveform_info.num_peak].peak_freq = i*5000; // 记录峰值频率
      waveform_info.peak_info[waveform_info.num_peak].peak_value = fftMagnitude[i]; // 记录峰值幅值
      waveform_info.num_peak++;
    }
  }
  waveform_info.highest_peak_index = 0;
  for (int i = 1; i < waveform_info.num_peak; i++)
  {
    if (waveform_info.peak_info[i].peak_value > waveform_info.peak_info[waveform_info.highest_peak_index].peak_value)
    {
      waveform_info.highest_peak_index = i; // 更新最高峰值索引
    }
  }
  switch (waveform_info.num_peak) {
    case 2:
      float ratio = (float)waveform_info.peak_info[1].peak_value / (float)waveform_info.peak_info[0].peak_value;
      if(ratio>1.1)
      {
        dds_output[0].Waveform = AD9833_MODE_TRIANGLE;
        dds_output[0].Freq = waveform_info.peak_info[0].peak_freq;
        dds_output[1].Waveform = AD9833_MODE_SINE;
        dds_output[1].Freq = waveform_info.peak_info[1].peak_freq;
      }
      else
      {
        dds_output[0].Waveform = AD9833_MODE_SINE;
        dds_output[0].Freq = waveform_info.peak_info[0].peak_freq;
        dds_output[1].Waveform = AD9833_MODE_SINE;
        dds_output[1].Freq = waveform_info.peak_info[1].peak_freq;
      }
      break;
    case 3:
      uint8_t compare_index = waveform_info.peak_info[waveform_info.highest_peak_index].peak_freq/5000*3;
      if(fftMagnitude[compare_index]>1000||fftMagnitude[compare_index]<150)
      {
        uint8_t k = 0;
        for(int i=0;i<waveform_info.num_peak;i++)
        {
          if(waveform_info.peak_info[i].peak_value > 1000)
          {
            if(i==waveform_info.highest_peak_index)
            {
              dds_output[k].Waveform = AD9833_MODE_SINE;
              dds_output[k].Freq = waveform_info.peak_info[i].peak_freq;
              k++;
            }
            else
            {
              dds_output[k].Waveform = AD9833_MODE_TRIANGLE;
              dds_output[k].Freq = waveform_info.peak_info[i].peak_freq;
              k++;
            }
          }
          if(k>=2)
          {
            break;
          }
        }
      }
      else
      {
        dds_output[0].Waveform = AD9833_MODE_TRIANGLE;
        dds_output[1].Waveform = AD9833_MODE_TRIANGLE;
        int k = 0;
        for(int i=0;i<waveform_info.num_peak;i++)
        {
          if(waveform_info.peak_info[i].peak_value > 1000)
          {
            dds_output[k].Freq = waveform_info.peak_info[i].peak_freq;
            k++;
            if(k>=2)
            {
              break;
            }
          }
        }
      }
      break;
    case 4:
      dds_output[0].Waveform = AD9833_MODE_TRIANGLE;
      dds_output[0].Freq = waveform_info.peak_info[0].peak_freq;
      dds_output[1].Waveform = AD9833_MODE_TRIANGLE;
      if(waveform_info.peak_info[1].peak_value > 1000)
      {
        dds_output[1].Freq = waveform_info.peak_info[1].peak_freq;
      }
      else
      {
        dds_output[1].Freq = waveform_info.peak_info[2].peak_freq;
      }
      break;
    default:
      dds_output[0].Waveform = AD9833_MODE_SINE;
      dds_output[0].Freq = waveform_info.peak_info[0].peak_freq;
      dds_output[1].Waveform = AD9833_MODE_SINE;
      dds_output[1].Freq = waveform_info.peak_info[1].peak_freq;
      break;
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    for (int i = 0; i < 1024; i++) {
      adcValues[i] = adcValues[i] >> 1; // 将 12 位 ADC 数据转换为 15 位 Q15 格式
    }
    arm_rfft_init_q15(&S, 1024, 0, 1); // 初始化 RFFT 实例，长度为 1024，正向 FFT，使用 bit-reversal 顺序
    arm_rfft_q15(&S, (q15_t*)adcValues, fftOutput);
    //arm_cmplx_mag_q15(fftOutput, fftMagnitude, 512 ); // 计算 FFT 输出的幅值512
    for (int i = 0; i < 512/5; i++) {// 只计算i为5的倍数的幅值
      uint32_t real = fftOutput[2*i*5]; // 实部
      uint32_t imag = fftOutput[2*i*5 + 1]; // 虚部
      fftMagnitude[i] = sqrt(real * real + imag * imag); // 计算幅值
    }
    fftMagnitude[0] = 0; // 直流分量幅值设为0，忽略直流偏置对频谱的影响
    WaveIdentify(); // 进行波形识别，更新 dds_output 数组
    HAL_GPIO_WritePin(h9833_1.FSYN_GPIO_Port, h9833_1.FSYN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(h9833_2.FSYN_GPIO_Port, h9833_2.FSYN_Pin, GPIO_PIN_RESET);
    AD9833_Reset(&h9833_1); // 重置第一个 DDS
    HAL_GPIO_WritePin(h9833_1.FSYN_GPIO_Port, h9833_1.FSYN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(h9833_2.FSYN_GPIO_Port, h9833_2.FSYN_Pin, GPIO_PIN_SET);
    AD9833_SetWaveform(&h9833_1, dds_output[0].Waveform); // 设置第一个 DDS 的波形
    AD9833_SetFrequency(&h9833_1, dds_output[0].Freq); // 设置第一个 DDS 的频率
    AD9833_SetWaveform(&h9833_2, dds_output[1].Waveform); // 设置第二个 DDS 的波形
    AD9833_SetFrequency(&h9833_2, dds_output[1].Freq); // 设置第二个 DDS 的频率
    AD9833_SetPhaseDeg(&h9833_1, dds_output[0].Phase); // 设置第一个 DDS 的相位
    AD9833_SetPhaseDeg(&h9833_2, dds_output[1].Phase); // 设置第二个 DDS 的相位


    HAL_GPIO_WritePin(h9833_1.FSYN_GPIO_Port, h9833_1.FSYN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(h9833_2.FSYN_GPIO_Port, h9833_2.FSYN_Pin, GPIO_PIN_RESET);
    AD9833_Reset(&h9833_1); // 重置第一个 DDS
    HAL_GPIO_WritePin(h9833_1.FSYN_GPIO_Port, h9833_1.FSYN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(h9833_2.FSYN_GPIO_Port, h9833_2.FSYN_Pin, GPIO_PIN_SET);
    AD9833_SetWaveform(&h9833_1, dds_output[0].Waveform); // 设置第一个 DDS 的波形
    AD9833_SetFrequency(&h9833_1, dds_output[0].Freq); // 设置第一个 DDS 的频率
    AD9833_SetWaveform(&h9833_2, dds_output[1].Waveform); // 设置第二个 DDS 的波形
    AD9833_SetFrequency(&h9833_2, dds_output[1].Freq); // 设置第二个 DDS 的频率
    AD9833_SetPhaseDeg(&h9833_1, dds_output[0].Phase); // 设置第一个 DDS 的相位
    AD9833_SetPhaseDeg(&h9833_2, dds_output[1].Phase); // 设置第二个 DDS 的相位
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == KEY0_Pin) // 如果是 KEY0 触发的外部中断
  {
    HAL_Delay(30); // 延时 30 ms，进行按键消抖
    if(HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
    {
      HAL_ADC_Stop_DMA(&hadc1); // 停止 ADC DMA 采样
      ClockGen_Update(); // 进行一次时钟更新，调整分频比以匹配输入信号频率
      HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
      HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcValues, 1024); // 启动 ADC DMA 采样，结果存储在 adcValues 数组中
    }
  }
  if(GPIO_Pin == KEY1_Pin) // 如果是 KEY1 触发的外部中断
  {
    HAL_Delay(30); // 延时 30 ms，进行按键消抖
    if(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
    {
      while(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET); // 等待按键释放
      dds_output[0].Phase = 0; // 将第一个 DDS 的相位重置为 0 度
      dds_output[1].Phase = dds_output[1].Phase + 5; // 将第二个 DDS 的相位增加 5 度
      if(dds_output[1].Phase >= 185)
      {
        dds_output[1].Phase = 0; // 如果相位超过 185 度，则重置为 0 度
      }
      //AD9833_SetPhase(&h9833_1, dds_output[0].Phase); // 设置第一个 DDS 的相位
      //uint32_t phaseBias = (double)0.00000375*(double)dds_output[1].F
      AD9833_SetPhaseDeg(&h9833_2, dds_output[1].Phase); // 设置第二个 DDS 的相位
    }
    
  }
    
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_COMP1_Init();
  MX_SPI1_Init();
  MX_DAC1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_I2C2_Init();
  MX_OPAMP3_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  ClockGen_Init();
  HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET); // 指示灯亮，表示时钟发生器已启动
  HAL_Delay(500); // 等待时钟发生器稳定
  AD9833_Init(&h9833_1); // 初始化第一个 AD9833 DDS
  AD9833_Init(&h9833_2); // 初始化第二个 AD9833 DDS
  OLED_Init(); // 初始化 OLED 显示屏
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    OLED_NewFrame(); // 开始新的 OLED 帧
    char buffer[64];
    sprintf(buffer, "FA:%ld PhA:%ld", dds_output[0].Freq/1000, dds_output[0].Phase);
    OLED_PrintString(0, 0, buffer, &font16x16, OLED_COLOR_NORMAL);
    switch (dds_output[0].Waveform) {
      case AD9833_MODE_SINE:
        OLED_PrintString(0, 16, "WA:SINE", &font16x16, OLED_COLOR_NORMAL);
        break;
      case AD9833_MODE_TRIANGLE:
        OLED_PrintString(0, 16, "WA:TRIANGLE", &font16x16, OLED_COLOR_NORMAL);
        break;
      case AD9833_MODE_SQUARE:
        OLED_PrintString(0, 16, "WA:SQUARE", &font16x16, OLED_COLOR_NORMAL);
        break;
      default:
        OLED_PrintString(0, 16, "WA:UNKNOWN", &font16x16, OLED_COLOR_NORMAL);
        break;
    }

    sprintf(buffer, "FB:%ld PhB:%ld", dds_output[1].Freq/1000, dds_output[1].Phase);
    OLED_PrintString(0, 32, buffer, &font16x16, OLED_COLOR_NORMAL);
    switch (dds_output[1].Waveform) {
      case AD9833_MODE_SINE:
        OLED_PrintString(0, 48, "WB:SINE", &font16x16, OLED_COLOR_NORMAL);
        break;
      case AD9833_MODE_TRIANGLE:
        OLED_PrintString(0, 48, "WB:TRIANGLE", &font16x16, OLED_COLOR_NORMAL);
        break;
      case AD9833_MODE_SQUARE:
        OLED_PrintString(0, 48, "WB:SQUARE", &font16x16, OLED_COLOR_NORMAL);
        break;
      default:
        OLED_PrintString(0, 48, "WB:UNKNOWN", &font16x16, OLED_COLOR_NORMAL);
        break;
    }

    OLED_ShowFrame(); // 显示 OLED 帧
    HAL_Delay(100); // 每 100 ms 更新一次显示
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 32;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
