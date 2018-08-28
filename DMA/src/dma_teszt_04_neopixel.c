/*
  Neopixel DMA-val
*/

#include "dma_samples.h"
#include <inttypes.h>
#include <stdio.h>
#include <stm32f10x.h>


static void neopixel_beallitas()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	// GPIO beállítás
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void timer_beallitas()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	// prescaler 24 MHz-re állítása
	TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t) (SystemCoreClock / 24000000) - 1;
	TIM_TimeBaseStructure.TIM_Period = 29; // 800kHz
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);

	// PWM beállítása
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);

	TIM_ARRPreloadConfig(TIM4, ENABLE);
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);

	TIM_SetCompare1(TIM4, 0); // ne küldjön kezdetben semmit

	TIM_DMACmd(TIM4, TIM_DMA_Update, ENABLE);
	TIM_DMAConfig(TIM4, TIM_DMABase_ARR, TIM_DMABurstLength_3Bytes );
}

#define _0     8
#define _1    21
#define VEGE   0

#define ARR   29
#define RES    0

const uint8_t neopixel_adatok [] = {
	ARR,RES,_0, ARR,RES,_0, ARR,RES,_0, ARR,RES,_0, ARR,RES,_1, ARR,RES,_1, ARR,RES,_1, ARR,RES,_1,
	ARR,RES,_0, ARR,RES,_0, ARR,RES,_0, ARR,RES,_0, ARR,RES,_1, ARR,RES,_1, ARR,RES,_1, ARR,RES,_1,
	ARR,RES,_0, ARR,RES,_0, ARR,RES,_0, ARR,RES,_0, ARR,RES,_0, ARR,RES,_0, ARR,RES,_0, ARR,RES,_0,
	ARR,RES,VEGE, ARR,RES,VEGE,
};

static void dma_beallitas()
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_InitTypeDef  DMA_InitStructure;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;						// data shifted from memory to peripheral
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							// stop DMA feed after buffer size is reached
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;

	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					// automatically increase buffer index

	DMA_InitStructure.DMA_BufferSize = sizeof(neopixel_adatok);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(TIM4->DMAR);	// TODO
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) neopixel_adatok;		// this is the buffer memory

	DMA_Init(DMA1_Channel7, &DMA_InitStructure);
}

static void inditas()
{
	// DMA indul
	DMA_Cmd(DMA1_Channel7, ENABLE);
	// Timer indul
	TIM_Cmd(TIM4, ENABLE);

	// Várakozni a DMA végére
	while((DMA_GetFlagStatus(DMA1_FLAG_TC7)) == RESET);

	// Flag törlése
	DMA_ClearFlag(DMA1_FLAG_TC7);

	// kész
	TIM_Cmd(TIM4, DISABLE);
	DMA_Cmd(DMA1_Channel7, DISABLE);
}

void dma_teszt_04_neopixel()
{
	neopixel_beallitas();
	timer_beallitas();
	dma_beallitas();
	inditas();
}
