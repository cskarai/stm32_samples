#include <stdio.h>
#include "dma_samples.h"

/****************************************************
 * Ez a program az STM32 DMA képességeit mutatja be *
 * blue pill (STM32F103C8T6) panel segítségével     *
 ****************************************************/

int main(void)
{
    printf("\fSTM32 DMA tesztek\n ");

    dma_teszt_01_memcpy();
    dma_teszt_02_led_villogo();
    dma_teszt_03_mintavetelezes();
    dma_teszt_04_neopixel();
    dma_teszt_05_lcd_i2c();
    dma_teszt_06_lcd_spi();
    dma_teszt_07_lcd_parhuzamos();
}
