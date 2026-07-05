#ifndef __AD9833_H
#define __AD9833_H

#include "main.h"

typedef union {
    struct {
        uint16_t Reserved0 : 1;
        uint16_t Mode : 1;
        uint16_t Reserved1 : 1;
        uint16_t DIV2 : 1;
        uint16_t Reserved2 : 1;
        uint16_t OPBITEN : 1;
        uint16_t SLEEP12 : 1;
        uint16_t SLEEP1 : 1;
        uint16_t Reset : 1;
        uint16_t Reserved3 : 1;
        uint16_t PSELECT : 1;
        uint16_t FSELECT : 1;
        uint16_t HLB : 1;
        uint16_t B28 : 1;
        uint16_t addr : 2;
    }bits;
    uint16_t raw;   
} AD9833_ControlRegister_TypeDef;

typedef union {
    struct {
        uint32_t FREQL : 14;
        uint32_t addr : 2;
        uint32_t FREQH : 14;
        uint32_t addr2 : 2;
    }bits;
    uint16_t raw[2]; // Use an array of two 16-bit values to represent the 28-bit frequency word
} AD9833_FrequencyRegister_TypeDef;

typedef union {
    struct {
        uint16_t PHASEx : 12;
        uint16_t Reserved : 1;
        uint16_t PHASESEL : 1;
        uint16_t addr : 2;
    }bits;
    uint16_t raw;
} AD9833_PhaseRegister_TypeDef;

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *FSYN_GPIO_Port;
    uint16_t FSYN_Pin;
    uint32_t MCLK_Hz;
    AD9833_ControlRegister_TypeDef controlReg;
    AD9833_FrequencyRegister_TypeDef freqReg0;
    AD9833_FrequencyRegister_TypeDef freqReg1;
    AD9833_PhaseRegister_TypeDef phaseReg0;
    AD9833_PhaseRegister_TypeDef phaseReg1;
} AD9833_HandleTypeDef;

typedef enum {
    AD9833_MODE_SINE = 0,
    AD9833_MODE_TRIANGLE = 1,
    AD9833_MODE_SQUARE = 2
} AD9833_WaveformMode;

HAL_StatusTypeDef AD9833_Init(AD9833_HandleTypeDef *had9833);
HAL_StatusTypeDef AD9833_Reset(AD9833_HandleTypeDef *had9833);
HAL_StatusTypeDef AD9833_ResetWithoutFsyn(AD9833_HandleTypeDef *had9833);
HAL_StatusTypeDef AD9833_SetFrequency(AD9833_HandleTypeDef *had9833, uint32_t frequency);
HAL_StatusTypeDef AD9833_SetWaveform(AD9833_HandleTypeDef *had9833, AD9833_WaveformMode waveform);
HAL_StatusTypeDef AD9833_SetPhase(AD9833_HandleTypeDef *had9833, uint16_t phase);
HAL_StatusTypeDef AD9833_SetPhaseDeg(AD9833_HandleTypeDef *had9833, float phaseDeg);
HAL_StatusTypeDef AD9833_SetPhaseWithoutFsyn(AD9833_HandleTypeDef *had9833, uint16_t phase);
HAL_StatusTypeDef AD9833_SetPhaseDegWithoutFsyn(AD9833_HandleTypeDef *had9833, float phaseDeg);

#endif // __AD9833_H