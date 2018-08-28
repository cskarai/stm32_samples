/*
  Mintavételezés DMA-val.
*/

#include "dma_samples.h"
#include <inttypes.h>
#include <stdio.h>
#include <stm32f10x.h>

#define ADC_MINTAVETELEK       20

#define BELSO_REFERENCIA_mV  1210

uint16_t adc_puffer[ADC_MINTAVETELEK];

static void adc_konfiguralasa()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	ADC_DeInit(ADC1);

	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_Vrefint, 1, ADC_SampleTime_239Cycles5);

	ADC_TempSensorVrefintCmd(ENABLE);
	ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));

	ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

static DMA_InitTypeDef  DMA_InitStructure;
static void dma_konfiguralasa()
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	                    // DMA1 órajel engedélyezése (bekapcsoljuk)
    DMA_DeInit(DMA1_Channel1);                                              // DMA1 alapállabotba állítás

    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                            // perifériából memóriába fogunk másolni

    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           // normál üzemmód
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;                   // közepes prioritás

    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;     // memória adatméret: fél szó
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word; // periféria adatméret: szó

    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 // a memóriacím növelhető
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;        // a perifériacím állandó

    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                      // irány: perifériától a memóriába

    DMA_InitStructure.DMA_BufferSize = ADC_MINTAVETELEK;                    // puffer méret: adc mintavételek száma
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&ADC1->DR);       // periféria címe: ADC1->DR

    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)adc_puffer;            // adc pufferbe másolunk
}

static void dma_vegrehajtasa()
{
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);        // DMA1 felkonfigurálása
    DMA_Cmd(DMA1_Channel1, ENABLE);

    /* Várakozni a DMA végére */
    while((DMA_GetFlagStatus(DMA1_FLAG_TC1)) == RESET);

    /* Flag törlése */
    DMA_ClearFlag(DMA1_FLAG_TC1);
}

static int adc_millivolt_konvertalas(int meres)
{
    if( meres == 0 )
        return -1; // hibás érték
    return  BELSO_REFERENCIA_mV * 0xFFF / meres;
}

static void eredmeny_kiirasa()
{
    int min = 0xFFFF, max = -1, atlag = 0;

    for( int i=0; i < ADC_MINTAVETELEK; i++ )
    {
        int meres = adc_puffer[i];
        atlag += meres;
        if( min > meres )
            min = meres;
        if( max < meres )
            max = meres;
    }
    atlag /= ADC_MINTAVETELEK;

    printf("Tapfeszultseg: %d mV [%d mV - %d mV]\n", adc_millivolt_konvertalas(atlag),
           adc_millivolt_konvertalas(max), adc_millivolt_konvertalas(min));
}

int volatile TC_pozicio = -1; // átvitel befejeződött
int volatile HT_pozicio = -1; // félig megtelt a puffer
int volatile TE_pozicio = -1; // átviteli hiba

void DMA1_Channel1_IRQHandler()
{
    // puffer fele megtelt
    if (DMA_GetITStatus(DMA1_IT_HT1)) {
        HT_pozicio = DMA1_Channel1->CNDTR;
        DMA_ClearITPendingBit(DMA1_IT_HT1);
    }
    // átvitel befejeződött
    if (DMA_GetITStatus(DMA1_IT_TC1)) {
        TC_pozicio = DMA1_Channel1->CNDTR;
        DMA_ClearITPendingBit(DMA1_IT_TC1);
    }
    // átviteli hiba
    if (DMA_GetITStatus(DMA1_IT_TE1)) {
        TE_pozicio = DMA1_Channel1->CNDTR;
        DMA_ClearITPendingBit(DMA1_IT_TE1);
    }
}

void interruptok_konfiguralasa()
{
    NVIC_PriorityGroupConfig( NVIC_PriorityGroup_2 );

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // prioritás
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // alprioritás
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_Init(&NVIC_InitStructure);
}

void dma_vegrehajtasa_interruptokkal()
{
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_HT, ENABLE);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TE, ENABLE);

    TC_pozicio = HT_pozicio = TE_pozicio = -1;

    DMA_Init(DMA1_Channel1, &DMA_InitStructure);        // DMA1 felkonfigurálása
    DMA_Cmd(DMA1_Channel1, ENABLE);

    while( (TC_pozicio == -1) && (TE_pozicio == -1) );
}

void dma_interrupt_eredmenyek_kiirasa()
{
    if( HT_pozicio != -1 )
        printf("Fel atvitel pozicio:   %d\n", ADC_MINTAVETELEK - HT_pozicio);
    if( TC_pozicio != -1 )
        printf("Teljes atvitel pozicio:%d\n", ADC_MINTAVETELEK - TC_pozicio);
    if( TE_pozicio != -1 )
        printf("Atviteli hiba pozicio: %d\n", ADC_MINTAVETELEK - TE_pozicio);
}

void dma_teszt_adc_mintavetelezes()
{
    printf("->[ADC teszt: egyszeru mintavetelezes]\n");

    adc_konfiguralasa();
    dma_konfiguralasa();
    dma_vegrehajtasa();
    eredmeny_kiirasa();
}

void dma_teszt_adc_interruptok()
{
    printf("->[ADC teszt: interruptok hasznalata]\n");

    adc_konfiguralasa();
    dma_konfiguralasa();
    interruptok_konfiguralasa();
    dma_vegrehajtasa_interruptokkal();
    dma_interrupt_eredmenyek_kiirasa();
}

void dma_teszt_adc_atviteli_hiba()
{
    printf("->[ADC teszt: interruptok hasznalata]\n");

    adc_konfiguralasa();
    dma_konfiguralasa();
    DMA_InitStructure.DMA_MemoryBaseAddr = 0x3F000000; // RAM túlírása
    interruptok_konfiguralasa();
    dma_vegrehajtasa_interruptokkal();
    dma_interrupt_eredmenyek_kiirasa();
}

void dma_teszt_adc_korkoros()
{
    printf("->[ADC teszt: korkoros DMA]\n");

    adc_konfiguralasa();
    dma_konfiguralasa();
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    interruptok_konfiguralasa();

    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

    TC_pozicio = -1;

    DMA_Init(DMA1_Channel1, &DMA_InitStructure);        // DMA1 felkonfigurálása
    DMA_Cmd(DMA1_Channel1, ENABLE);

    int atlag = 0;
    // várjunk 10 kört
    for(int i=0; i < 10; i++)
    {
        // várjuk meg az interrupt-ot
        while(TC_pozicio == -1);
        TC_pozicio = -1;
        // várjunk a következőre
        for(int j=0; j < ADC_MINTAVETELEK; j++)
            atlag += adc_puffer[j];
    }

    printf("10 kor atlaga: %d mV\n", adc_millivolt_konvertalas(atlag / ADC_MINTAVETELEK / 10));
}

void dma_teszt_03_mintavetelezes()
{
    dma_teszt_adc_mintavetelezes();
    dma_teszt_adc_interruptok();
    dma_teszt_adc_atviteli_hiba();
    dma_teszt_adc_korkoros();
}
