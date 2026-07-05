#include "AD9833.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_flash_ramfunc.h"
#include "stm32g4xx_hal_gpio.h"
#include <stdint.h>

HAL_StatusTypeDef AD9833_Init(AD9833_HandleTypeDef *had9833) {
    // Initialize control register with default values
    had9833->controlReg.raw = 0x0000;
    had9833->controlReg.bits.B28 = 1; // Enable 28-bit frequency writes


    had9833->freqReg0.raw[0] = 0x4000;
    had9833->freqReg0.raw[1] = 0x4000;
    had9833->freqReg1.raw[0] = 0x8000;
    had9833->freqReg1.raw[1] = 0x8000;

    had9833->phaseReg0.bits.addr = 3;
    had9833->phaseReg0.bits.PHASESEL = 0;
    had9833->phaseReg0.bits.PHASEx = 0x0000;

    had9833->phaseReg1.bits.addr = 3;
    had9833->phaseReg1.bits.PHASESEL = 1;
    had9833->phaseReg1.bits.PHASEx = 0x0000;

    return HAL_OK;
}

HAL_StatusTypeDef AD9833_Reset(AD9833_HandleTypeDef *had9833) {
    had9833->controlReg.bits.Reset = 1; // Set the reset bit
    uint16_t controlWord = had9833->controlReg.raw;
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(had9833->hspi, (uint8_t*)&controlWord, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_SET);
    had9833->controlReg.bits.Reset = 0; // Clear the reset bit
    HAL_Delay(10); // Wait for 200 ms to ensure the reset is processed
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(had9833->hspi, (uint8_t*)&had9833->controlReg.raw, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef AD9833_ResetWithoutFsyn(AD9833_HandleTypeDef *had9833)
{
    had9833->controlReg.bits.Reset = 1; // Set the reset bit
    uint16_t controlWord = had9833->controlReg.raw;
    HAL_StatusTypeDef status = HAL_SPI_Transmit(had9833->hspi, (uint8_t*)&controlWord, 1, HAL_MAX_DELAY);
    had9833->controlReg.bits.Reset = 0; // Clear the reset bit
    HAL_Delay(200); // Wait for 200 ms to ensure the reset is processed
    return status;
}

HAL_StatusTypeDef AD9833_SetFrequency(AD9833_HandleTypeDef *had9833, uint32_t frequency) {
    uint32_t freqWord = (uint32_t)(((long long)(frequency) * (1UL << 28)) / had9833->MCLK_Hz);
    
    had9833->freqReg0.bits.FREQL = freqWord & 0x3FFF; // Lower 14 bits
    had9833->freqReg0.bits.FREQH = (freqWord >> 14) & 0x3FFF; // Upper 14 bits
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_RESET);
    if(HAL_SPI_Transmit(had9833->hspi, (uint8_t*)&had9833->controlReg.raw, 1, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(had9833->hspi, (uint8_t*)had9833->freqReg0.raw, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(had9833->hspi, (uint8_t*)&(had9833->freqReg0.raw[1]), 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef AD9833_SetWaveform(AD9833_HandleTypeDef *had9833, AD9833_WaveformMode waveform) {
    had9833->controlReg.bits.OPBITEN = (waveform == AD9833_MODE_SQUARE) ? 1 : 0;
    had9833->controlReg.bits.Mode = (waveform == AD9833_MODE_TRIANGLE) ? 1 : 0;
    had9833->controlReg.bits.DIV2 = (waveform == AD9833_MODE_SQUARE) ? 1 : 0;
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(had9833->hspi, (uint8_t*)&had9833->controlReg.raw, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef AD9833_SetPhase(AD9833_HandleTypeDef *had9833, uint16_t phase) {
    had9833->phaseReg0.bits.PHASEx = phase & 0x0FFF; // Ensure phase is 12 bits
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(had9833->hspi, (uint8_t*)&had9833->phaseReg0.raw, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(had9833->FSYN_GPIO_Port, had9833->FSYN_Pin, GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef AD9833_SetPhaseDeg(AD9833_HandleTypeDef *had9833, float phaseDeg)
{
    uint16_t phase = (uint16_t)(phaseDeg * 4096.0f / 360.0f);
    return AD9833_SetPhase(had9833, phase);
}

HAL_StatusTypeDef AD9833_SetPhaseWithoutFsyn(AD9833_HandleTypeDef *had9833, uint16_t phase)
{
    had9833->phaseReg0.bits.PHASEx = phase & 0x0FFF; // Ensure phase is 12 bits
    return HAL_SPI_Transmit(had9833->hspi, (uint8_t*)&had9833->phaseReg0.raw, 1, HAL_MAX_DELAY);
}

HAL_StatusTypeDef AD9833_SetPhaseDegWithoutFsyn(AD9833_HandleTypeDef *had9833, float phaseDeg)
{
    uint16_t phase = (uint16_t)(phaseDeg * 4096.0f / 360.0f);
    return AD9833_SetPhaseWithoutFsyn(had9833, phase);
}