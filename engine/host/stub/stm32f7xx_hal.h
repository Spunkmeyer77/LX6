#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define __IO volatile
#define I2S_AUDIOFREQ_44K 44100
#define __disable_irq()
#define __enable_irq()
#define __DSB()
#define __NOP()
#define __SSAT(x,b) ((x)>((1<<((b)-1))-1)?((1<<((b)-1))-1):((x)<-(1<<((b)-1))?-(1<<((b)-1)):(x)))
#define __USAT(x,b) ((x)<0?0:((x)>((1<<(b))-1)?((1<<(b))-1):(x)))
typedef struct{int dummy;} TIM_HandleTypeDef; typedef struct{int dummy;} UART_HandleTypeDef; typedef struct{int dummy;} I2S_HandleTypeDef; typedef struct{int dummy;} SPI_HandleTypeDef; typedef struct{int dummy;} DMA_HandleTypeDef; typedef struct{int dummy;} ADC_HandleTypeDef; typedef struct{int dummy;}RNG_HandleTypeDef;
typedef enum {HAL_OK=0,HAL_ERROR,HAL_BUSY,HAL_TIMEOUT} HAL_StatusTypeDef;
typedef enum {RESET=0,SET=1} FlagStatus, ITStatus;
typedef enum {GPIO_PIN_RESET=0,GPIO_PIN_SET} GPIO_PinState;
typedef struct{uint32_t ODR,IDR,BSRR;}GPIO_TypeDef;
static inline uint32_t HAL_GetTick(void){return 0;}
#define __HAL_TIM_DISABLE_IT(a,b)
#define __HAL_TIM_ENABLE_IT(a,b)
#define __HAL_TIM_SetCounter(a,b)
#define __HAL_TIM_SET_COUNTER(a,b)
#define __HAL_TIM_GET_COUNTER(a) 0
typedef struct{uint32_t CTRL,CYCCNT;}DWT_Type; static DWT_Type lx6_dwt;
#define DWT (&lx6_dwt)
#define GPIO_PIN_3 8
#define GPIO_PIN_4 16
#define GPIO_PIN_6 64
#define GPIO_PIN_7 128
static GPIO_TypeDef lx6_gpio; 
#define GPIOA (&lx6_gpio)
#define GPIOB (&lx6_gpio)
#define GPIOC (&lx6_gpio)
#define GPIOD (&lx6_gpio)
#define GPIOE (&lx6_gpio)
#define __STATIC_INLINE static inline
static inline uint32_t HAL_RCC_GetHCLKFreq(void){return 216000000;}
/* ARM DSP intrinsics, portable versions */
static inline uint32_t __CLZ(uint32_t x){ return x ? __builtin_clz(x) : 32; }
static inline int16_t lx6_sat16(int32_t v){ return v>32767?32767:(v<-32768?-32768:(int16_t)v); }
static inline uint32_t __QADD16(uint32_t a,uint32_t b){ int16_t lo=lx6_sat16((int32_t)(int16_t)(a&0xffff)+(int16_t)(b&0xffff)); int16_t hi=lx6_sat16((int32_t)(int16_t)(a>>16)+(int16_t)(b>>16)); return ((uint32_t)(uint16_t)hi<<16)|(uint16_t)lo; }
static inline uint32_t __QSUB16(uint32_t a,uint32_t b){ int16_t lo=lx6_sat16((int32_t)(int16_t)(a&0xffff)-(int16_t)(b&0xffff)); int16_t hi=lx6_sat16((int32_t)(int16_t)(a>>16)-(int16_t)(b>>16)); return ((uint32_t)(uint16_t)hi<<16)|(uint16_t)lo; }
#define __PKHBT(a,b,s) ((((uint32_t)(a))&0x0000FFFFu)|((((uint32_t)(b))<<(s))&0xFFFF0000u))
uint32_t HAL_RNG_GetRandomNumber(RNG_HandleTypeDef* h);
