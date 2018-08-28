//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/Trace.h"
#include "platform_config.h"

// ----------------------------------------------------------------------------
//
// Standalone STM32F1 led blink sample (trace via DEBUG).
//
// In debug configurations, demonstrate how to print a greeting message
// on the trace device. In release configurations the message is
// simply discarded.
//
// Then demonstrates how to blink a led with 1 Hz, using a
// continuous loop and SysTick delays.
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//
// The external clock frequency is specified as a preprocessor definition
// passed to the compiler via a command line option (see the 'C/C++ General' ->
// 'Paths and Symbols' -> the 'Symbols' tab, if you want to change it).
// The value selected during project creation was HSE_VALUE=8000000.
//
// Note: The default clock settings take the user defined HSE_VALUE and try
// to reach the maximum possible system clock. For the default 8 MHz input
// the result is guaranteed, but for other values it might not be possible,
// so please adjust the PLL settings in system/src/cmsis/system_stm32f10x.c
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x.h"
#include "platform_config.h"



/* Private define ------------------------------------------------------------*/
#define BufferSize 32

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
SPI_InitTypeDef   SPI_InitStructure;

uint8_t SPIz_Buffer_Tx[BufferSize] = {0x1, 0x2, 0x4, 0x54, 0x55, 0x56, 0x57,
                                      0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E,
                                      0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65,
                                      0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C,
                                      0x6D, 0x6E, 0x6F, 0x70};

uint8_t SPIz_Buffer_Rx[BufferSize];

__IO uint8_t TxIdx = 0, RxIdx = 0, k = 0;

/* Private functions ---------------------------------------------------------*/
void RCC_Configuration(void);
void GPIO_Configuration(uint16_t SPIz_Mode);

void ConfigureDMA(void)
{
    DMA_InitTypeDef     DMA_InitStructure;

    // Enable DMA1 Peripheral Clock (SPI_DECAWAVE and SPI_BUS)
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // Configure SPI_BUS RX Channel
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; // From SPI to memory
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryBaseAddr = 0; // To be set later
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_BufferSize = 1; // To be set later
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);

    // Configure SPI_BUS TX Channel
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // From memory to SPI
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryBaseAddr = 0; // To be set later
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_BufferSize = 1; // To be set later
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);

} // end ConfigureDMA()

void spiTransfer(uint8_t CommandLength, const uint8_t *CommandBuffer,
        uint16_t DataLength, uint8_t *DataBuffer)
{
    // Prepare the DMA
    DMA1_Channel5->CNDTR = CommandLength;
    DMA1_Channel5->CMAR = (uint32_t)CommandBuffer;
    DMA1_Channel4->CNDTR = DataLength;
    DMA1_Channel4->CMAR = (uint32_t)DataBuffer;

    // Enable the DMAs - They will await signals from the SPI hardware
    DMA_Cmd(DMA1_Channel5, ENABLE); // TX
    DMA_Cmd(DMA1_Channel4, ENABLE); // RX

    // Activate the Flash CS
    //GPIO_ResetBits(SPI_MEM_CS_GPIO, SPI_MEM_CS);

    // Enable the SPI communication to the TX DMA, which will send the command
    SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);

    // Wait until the command is sent to the DR
    while (!DMA_GetFlagStatus(DMA1_FLAG_TC5));

    // Wait until the transmission is completed
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == RESET);

    // Disable the TX DMA and clear DMA flags
    SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
    DMA_Cmd(DMA1_Channel5, DISABLE);
    DMA_ClearFlag(DMA1_FLAG_GL4 | DMA1_FLAG_HT4 | DMA1_FLAG_TC4 | DMA1_FLAG_GL5 | DMA1_FLAG_HT5 | DMA1_FLAG_TC5);
    //NOTE: I checked the SPI OVR flag here, and it wasn't set...

    // Enable SPI communication to the RX DMA, which should receive the data
    SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE);

    // Wait until the data is received
    while (!DMA_GetFlagStatus(DMA1_FLAG_TC4));

    // Disable the DMAs
    DMA_Cmd(DMA1_Channel4, DISABLE); // RX
    DMA_Cmd(DMA1_Channel5, DISABLE); // TX

    // Release the Flash CS
    //GPIO_SetBits(SPI_MEM_CS_GPIO, SPI_MEM_CS);

} // end spiFlashRead()

int main(void)
{


      /* System clocks configuration ---------------------------------------------*/
      RCC_Configuration();

      /* GPIO configuration ------------------------------------------------------*/
      GPIO_Configuration(SPI_Mode_Master);

      /* SPIy Config -------------------------------------------------------------*/
      SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
      SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
      SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
      SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
      SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
      SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
      SPI_InitStructure.SPI_CRCPolynomial = 7;
      SPI_Init(SPIz, &SPI_InitStructure);

      /* Enable SPIz */
      SPI_Cmd(SPIz, ENABLE);

      /* DMA */

      ConfigureDMA();

      spiTransfer(BufferSize, SPIz_Buffer_Tx, BufferSize, SPIz_Buffer_Rx);

      while (1)
      {}


} /***** END OF MAIN *****/


void RCC_Configuration(void)
{
  /* PCLK2 = HCLK/2 */
  RCC_PCLK2Config(RCC_HCLK_Div2);

  /* Enable GPIO clock for SPIz */
  RCC_APB2PeriphClockCmd(SPIz_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);

  /* Enable SPIz Periph clock */
  RCC_APB1PeriphClockCmd(SPIz_CLK, ENABLE);
}


void GPIO_Configuration(uint16_t SPIz_Mode)
{
      GPIO_InitTypeDef GPIO_InitStructure;

      /* Configure SPIz pins: SCK, MISO and MOSI ---------------------------------*/
      GPIO_InitStructure.GPIO_Pin = SPIz_PIN_SCK | SPIz_PIN_MOSI;
      /* Configure SCK and MOSI pins as Alternate Function Push Pull */
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;

      /**** ADDED THIS TO WORK ****/
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      /**** ADDED THIS TO WORK ****/

      GPIO_Init(SPIz_GPIO, &GPIO_InitStructure);


      GPIO_InitStructure.GPIO_Pin = SPIz_PIN_MISO;
      /* Configure MISO pin as Input Floating  */
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
      GPIO_Init(SPIz_GPIO, &GPIO_InitStructure);
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}

#endif

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
