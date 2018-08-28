/*
  DMA memcpy tesztek.

  Ezek a tesztek nem igényelnek hardvert, elég maga a blue pill panel.
*/

#include "dma_samples.h"
#include <stdio.h>
#include <inttypes.h>
#include <stm32f10x.h>
#include <string.h>

/* Ezek a teszt adataink */
const uint8_t teszt_adatok[] = {
    1,  2,  3,   4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
};

uint8_t cel_puffer[256]; // 256 byte puffer kimeneti adatoknak

// a puffer tartalmát ezzel a függvénnyel írjuk ki
void cel_puffer_kiirasa(int hossz)
{
    for(int i=0; i < hossz; i++ )
    {
        if( (i != 0) && ((i & 15) == 0) ) // minden 16. karakternél új sor
          printf("\n");

        printf(" %02x", cel_puffer[i]);
    }
    printf("\n"); // új sorral zárjunk
}

/************************************************************
 * DMA alapértékek, minden teszt ezeket módosítja           *
 ************************************************************/

static DMA_InitTypeDef  DMA_InitStructure;
static void dma_alapertekek()
{
    memset(cel_puffer, 0, sizeof(cel_puffer) );                             // kinullázzuk a cél puffert

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	                    // DMA1 órajel engedélyezése (bekapcsoljuk)
    DMA_DeInit(DMA1_Channel1);                                              // DMA1 alapállabotba állítás

    DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;                             // memóriából memóriába fogunk másolni

    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           // normál üzemmód
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;                   // közepes prioritás

    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         // memória adatméret: BYTE
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // periféria(memória) adatméret: BYTE

    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 // a memóriacím növelhető
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;         // a perifériacím növelhető

    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                      // irány: perifériától a memóriába

    DMA_InitStructure.DMA_BufferSize = sizeof(teszt_adatok);                // puffer méret a 'teszt_adatok' tömb mérete
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)teszt_adatok;      // periféria címe: teszt_adatok

    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)cel_puffer;
}

/************************************************************
 * Itt hajtjuk végre a beállított DMA-t és megvárjuk, míg   *
 * befejeződik                                              *
 ************************************************************/

static void dma_vegrehajtasa()
{
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);        // DMA1 felkonfigurálása
    DMA_Cmd(DMA1_Channel1, ENABLE);

    /* Várakozni a DMA végére */
    while((DMA_GetFlagStatus(DMA1_FLAG_TC1)) == RESET);

    /* Flag törlése */
    DMA_ClearFlag(DMA1_FLAG_TC1);

    cel_puffer_kiirasa(32);                             // cél puffer kiírása
}

/************************************************************
 * Ez a teszt egy egyszerű byte másolás DMA segítségével    *
 ************************************************************/

void dma_teszt_egyszeru_byte_masolas()
{
    printf("->[memcpy teszt: egyszeru byte masolas]\n");

    dma_alapertekek();              // feltöltése alapbeállításokkal

    // itt nem írunk fölül semmit, az alapértékek a byte másolást tartalmazzák

    dma_vegrehajtasa();
}

void dma_teszt_32bit_szo_masolas()
{
    printf("->[memcpy teszt: 32bit szo masolas]\n");

    dma_alapertekek();              // feltöltése alapbeállításokkal

    // az adat-méret most 32 bites
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    // a pufferméretet most 4-gyel osztjuk
    DMA_InitStructure.DMA_BufferSize = sizeof(teszt_adatok) / 4;

    dma_vegrehajtasa();
}

void dma_teszt_8bit_masolasa_32bit_re()
{
    printf("->[memcpy teszt: 8bit masolasa 32bit-re]\n");

    dma_alapertekek();              // feltöltése alapbeállításokkal

    // az adat-méret most 32 bites
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    // a pufferméretet most 4-gyel osztjuk
    DMA_InitStructure.DMA_BufferSize = sizeof(teszt_adatok) / 4;

    dma_vegrehajtasa();
}

void dma_teszt_32bit_masolasa_8bit_re()
{
    printf("->[memcpy teszt: 32bit masolasa 8bit-re]\n");

    dma_alapertekek();              // feltöltése alapbeállításokkal

    // az adat-méret most 32 bites
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    // a pufferméretet most 4-gyel osztjuk
    DMA_InitStructure.DMA_BufferSize = sizeof(teszt_adatok) / 4;

    dma_vegrehajtasa();
}

void dma_teszt_periferia_cimet_nem_noveljuk()
{
    printf("->[memcpy teszt: a periferia cimet nem noveljuk]\n");

    dma_alapertekek();              // feltöltése alapbeállításokkal

    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // a periféria címét nem növeljük

    dma_vegrehajtasa();
}


void dma_teszt_memoria_cimet_nem_noveljuk()
{
    printf("->[memcpy teszt: a memoria cimet nem noveljuk]\n");

    dma_alapertekek();              // feltöltése alapbeállításokkal

    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable; // a periféria címét nem növeljük

    dma_vegrehajtasa();
}

void dma_teszt_01_memcpy()
{
    dma_teszt_egyszeru_byte_masolas();
    dma_teszt_32bit_szo_masolas();
    dma_teszt_8bit_masolasa_32bit_re();
    dma_teszt_32bit_masolasa_8bit_re();
    dma_teszt_periferia_cimet_nem_noveljuk();
    dma_teszt_memoria_cimet_nem_noveljuk();
}
