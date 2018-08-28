/*
  I2C LCD DMA-val
*/

#include "dma_samples.h"
#include <inttypes.h>
#include <stdio.h>
#include <stm32f10x.h>

#define LCD_SZELESSEG                128
#define LCD_MAGASSAG                  64
#define LCD_PUFFER_MERET             (LCD_SZELESSEG * LCD_MAGASSAG / 8)

#define KEP_SZELESSEG                 72
#define KEP_MAGASSAG                  32

#define I2C_CIM                     0x78

// SSD1306 LCD konstansok
#define SSD1306_COMMAND             0x00
#define SSD1306_COMMAND_CONT        0x80
#define SSD1306_DATA                0x40

#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_MEMORYMODE          0x20
#define SSD1306_COMSCANDEC          0xC8
#define SSD1306_SETLOWCOLUMN        0x00
#define SSD1306_SETHIGHCOLUMN       0x10
#define SSD1306_SETSTARTLINE        0x40
#define SSD1306_SETCONTRAST         0x81
#define SSD1306_SEGREMAP            0xA0
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETVCOMDETECT       0xDB
#define SSD1306_CHARGEPUMP          0x8D
#define SSD1306_DISPLAYON           0xAF

#define SSD1306_COLUMNADDR          0x21
#define SSD1306_PAGEADDR            0x22

static const uint8_t dma_image [] = {
    0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
    0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x0E, 0xFE, 0xFC, 0xFC, 0xF0, 0xC0,
    0x00, 0x00, 0x01, 0x01, 0xFF, 0x0F, 0x7F, 0xFF, 0xFC, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0,
    0x3C, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0,
    0x78, 0x0F, 0x3F, 0xFF, 0xFF, 0xF8, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0x3F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x03,
    0x1F, 0xFF, 0xFF, 0xF8, 0xC0, 0x78, 0x0F, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x80, 0xF0, 0x1E, 0x0B, 0x08, 0x08, 0x08, 0x09, 0x0F, 0x7F, 0xFF, 0xFE,
    0xF0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80, 0x80, 0x82, 0x82,
    0x83, 0x83, 0x83, 0x83, 0x82, 0x82, 0x82, 0x82, 0x83, 0x83, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x82, 0x83, 0x83, 0x83, 0x82, 0x80, 0x80, 0x80, 0x83, 0x83, 0x83, 0x80, 0x80, 0x80,
    0x82, 0x82, 0x83, 0x83, 0x83, 0x83, 0x82, 0x82, 0x80, 0x82, 0x82, 0x83, 0x83, 0x83, 0x82, 0x82,
    0x80, 0x80, 0x80, 0x80, 0x82, 0x82, 0x83, 0x83, 0x83, 0x83, 0x82, 0x82, 0x80, 0x80, 0x80, 0xFF,
};

static void i2c_beallitas()
{
    // órajelek engedélyezése
    RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd (RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd (RCC_APB2Periph_AFIO, ENABLE);

    GPIO_PinRemapConfig (GPIO_Remap_I2C1, ENABLE);

    // GPIO beállítás
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_Init (GPIOB, &GPIO_InitStruct);

    I2C_Cmd (I2C1, DISABLE);
    I2C_DeInit (I2C1);
    I2C_InitTypeDef I2C_InitStruct;
    I2C_InitStruct.I2C_ClockSpeed = 400000;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStruct.I2C_OwnAddress1 = 0; // mester, nincs saját címe
    I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;

    // I2C1 konfigurálása
    I2C_Init (I2C1, &I2C_InitStruct);
    // I2C indítása
    I2C_DMACmd(I2C1, ENABLE);
    I2C_Cmd (I2C1, ENABLE);
}

static void dma_beallitas()
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_DeInit(DMA1_Channel6);
    DMA_Cmd(DMA1_Channel6, DISABLE);
}

static void dma_kuldes(uint8_t *puffer, int meret, int kitoltes)
{
	DMA_DeInit(DMA1_Channel6);

	DMA_InitTypeDef  DMA_InitStructure;

	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // a perifériába írunk
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; // nem memória másolás van

	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // normál mód
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; // közepes prioritás

	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; // szó méret
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // szó méret

	DMA_InitStructure.DMA_MemoryInc = kitoltes ? DMA_MemoryInc_Disable : DMA_MemoryInc_Enable; // memóriát nem
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // a perifériát nem


	DMA_InitStructure.DMA_BufferSize = meret; // puffer méret
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(I2C1->DR); // I2C1->DR regisztert írjuk

	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)puffer;

	DMA_Init(DMA1_Channel6, &DMA_InitStructure);        // DMA1 felkonfigurálása

    // DMA indul
    DMA_Cmd(DMA1_Channel6, ENABLE);

    /* Várakozás a DMA végére */
    while((DMA_GetFlagStatus(DMA1_FLAG_TC6)) == RESET);

    /* Flag törlése */
    DMA_ClearFlag(DMA1_FLAG_TC6);
    DMA_Cmd(DMA1_Channel6, DISABLE);

    // megvárjuk, míg I2C-n kimegy az adat
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
}

static int i2c_start()
{
	// start jel indítása
	I2C_GenerateSTART(I2C1, ENABLE);
	// várakozás sikeres tranzakcióra
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

	// 7 bites cím elküldése
	I2C_Send7bitAddress(I2C1, I2C_CIM, I2C_Direction_Transmitter);

	do
	{
		unsigned evnt = I2C_GetLastEvent(I2C1);

		if( (evnt & I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED )
			return 1;
		if( (evnt & I2C_EVENT_SLAVE_ACK_FAILURE) == I2C_EVENT_SLAVE_ACK_FAILURE )
			return 0;
	}while(1);

}

static void lcd_ablak(int x1, int x2, int y1, int y2)
{
	uint8_t buf[13];
	memset(buf, SSD1306_COMMAND_CONT, sizeof(buf));

	buf[1]  = SSD1306_COLUMNADDR;
	buf[3]  = x1;
	buf[5]  = x2;
	buf[7]  = SSD1306_PAGEADDR;
	buf[9]  = y1 >> 3;
	buf[11] = y2 >> 3;
	buf[12] = SSD1306_DATA;

	dma_kuldes(buf, sizeof(buf), 0);
}

#define CMD SSD1306_COMMAND_CONT

const uint8_t lcd_parancsok[] = {
	CMD, SSD1306_DISPLAYOFF,
	CMD, SSD1306_MEMORYMODE, CMD, 0,
	CMD, SSD1306_COMSCANDEC,
	CMD, SSD1306_SETLOWCOLUMN + 0,
	CMD, SSD1306_SETHIGHCOLUMN + 0,
	CMD, SSD1306_SETSTARTLINE + 0,
	CMD, SSD1306_SETCONTRAST, CMD, 0xFF,
	CMD, SSD1306_SEGREMAP + 1,
	CMD, SSD1306_NORMALDISPLAY,
	CMD, SSD1306_SETMULTIPLEX, CMD, 0x3F,
	CMD, SSD1306_DISPLAYALLON_RESUME,
	CMD, SSD1306_SETDISPLAYOFFSET, CMD, 0x00,
	CMD, SSD1306_SETDISPLAYCLOCKDIV, CMD, 0xF0,
	CMD, SSD1306_SETPRECHARGE, CMD, 0x22,
	CMD, SSD1306_SETCOMPINS, CMD, 0x12,
	CMD, SSD1306_SETVCOMDETECT, CMD, 0x20,
	CMD, SSD1306_CHARGEPUMP, CMD, 0x14,
	CMD, SSD1306_DISPLAYON,
};

static void lcd_inicializalas()
{
	i2c_start();
	dma_kuldes(lcd_parancsok, sizeof(lcd_parancsok), 0);
	I2C_GenerateSTOP(I2C1, ENABLE);
}

static void lcd_keptorles_dmaval()
{
	i2c_start();
	lcd_ablak(0, LCD_SZELESSEG-1, 0, LCD_MAGASSAG-1);

	uint8_t szin = 0x00;
	dma_kuldes(&szin, LCD_PUFFER_MERET, 1);

	I2C_GenerateSTOP(I2C1, ENABLE);
}

static void lcd_keprajzolas_dmaval()
{
	int x0 = (LCD_SZELESSEG-KEP_SZELESSEG)/2;
	int y0 = (LCD_MAGASSAG-KEP_MAGASSAG)/16 * 8; // csak 8 pixelre működik az ablakozás


	i2c_start();
	lcd_ablak(x0, KEP_SZELESSEG-1 + x0, y0, KEP_MAGASSAG-1 + y0);

	dma_kuldes(dma_image, sizeof(dma_image), 0);

	I2C_GenerateSTOP(I2C1, ENABLE);
}

void dma_teszt_05_lcd_i2c()
{
    printf("->[I2C LCD teszt]\n");

    dma_beallitas();
    i2c_beallitas();

    if( !i2c_start() )
    {
    	printf("-->Hiba: az I2C eszkoz nem elerheto!\n");
    	return;
    }
	I2C_GenerateSTOP(I2C1, ENABLE);

	lcd_inicializalas();

	lcd_keptorles_dmaval();
	lcd_keprajzolas_dmaval();
}
