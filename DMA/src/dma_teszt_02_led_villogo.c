/*
  LED villogó DMA-val.
*/

#include "dma_samples.h"
#include <inttypes.h>
#include <stdio.h>
#include <stm32f10x.h>

#define LED_PIN              13
#define LED_GPIO             GPIOC
#define LED_GPIO_RCC         RCC_APB2Periph_GPIOC

#define LED_PIN_MASK         (1 << LED_PIN)

#define LED_KI               LED_PIN_MASK
#define LED_BE               (LED_PIN_MASK << 16)
#define LED_MARAD            0

#define LED_VILLOGO_SEBESSEG 2000  // 200 ms

const uint32_t led_minta[] = {
    LED_BE, LED_MARAD, LED_KI, LED_MARAD, LED_BE, LED_MARAD, LED_MARAD,
    LED_KI, LED_MARAD, LED_MARAD, LED_BE, LED_KI, LED_BE, LED_KI, LED_BE,
    LED_KI, LED_BE, LED_KI
};

static void led_port_bekapcsolasa()
{
    RCC_APB2PeriphClockCmd(LED_GPIO_RCC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = LED_PIN_MASK;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(LED_GPIO, &GPIO_InitStructure);

    // LED kikapcsolása
    GPIO_SetBits(LED_GPIO, LED_PIN_MASK);
}

static void dma_konfiguralasa()
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_DeInit(DMA1_Channel3);

    DMA_InitTypeDef  DMA_InitStructure;

    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // a perifériába írunk
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; // nem memória másolás van

    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // normál mód
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; // közepes prioritás

    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word; // szó méret
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word; // szó méret

    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // a memória címet növeljük
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // a perifériát nem


    DMA_InitStructure.DMA_BufferSize = sizeof(led_minta) / sizeof(uint32_t); // puffer méret a minták száma
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(LED_GPIO->BSRR); // BSRR regisztert írjuk

    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)led_minta;

    DMA_Init(DMA1_Channel3, &DMA_InitStructure);        // DMA1 felkonfigurálása
}

static void timer_konfiguralasa()
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 , ENABLE);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = LED_VILLOGO_SEBESSEG;
    TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock / 10000; // 0.1 ms felbontás
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // Timer engedélyezése
    TIM_Cmd(TIM3, ENABLE);

    // DMA engedélyezése a timeren
    TIM_DMACmd(TIM3, TIM_DMA_Update, ENABLE);
}

static void dma_indul()
{
    DMA_Cmd(DMA1_Channel3, ENABLE);

    /* Várakozni a DMA végére */
    while((DMA_GetFlagStatus(DMA1_FLAG_TC3)) == RESET);

    /* Flag törlése */
    DMA_ClearFlag(DMA1_FLAG_TC3);
}

void dma_teszt_02_led_villogo()
{
    printf("->[LED villogo teszt]\n");

    led_port_bekapcsolasa();
    dma_konfiguralasa();
    timer_konfiguralasa();
    dma_indul();
}
