#include "tft.h"

#include <stdlib.h>
#include <string.h>
#include "axp192.h"
#include "st7789.h"
#include "fontx.h"

FontxFile fx16G[2];
FontxFile fx24G[2];
FontxFile fx32G[2];
TFT_t dev;


void init_i2c(){
	i2c_master_init();
}


void init_tft(){
	AXP192_PowerOn();
	AXP192_ScreenBreath(11);
	// set font file

	InitFontx(fx16G,"/fonts/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24G,"/fonts/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32G,"/fonts/ILGH32XB.FNT",""); // 16x32Dot Gothic
	
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
}

void clear_screen(){
	lcdFillScreen(&dev, BLACK);
}


void draw_steps(int count){
	char charBuffer[6];
	itoa(count, charBuffer, 10); // base 
	uint16_t color;
	color = WHITE;
	lcdSetFontDirection(&dev, 1);
	uint16_t xpos = CONFIG_WIDTH - CONFIG_WIDTH / 4;
	uint16_t ypos = CONFIG_HEIGHT / 5;
	lcdDrawString(&dev, fx24G, xpos, ypos, (uint8_t*)charBuffer, color);
}

void draw_text(char* text){
	uint8_t ascii[40];
	uint16_t color;
	color = WHITE;
	lcdSetFontDirection(&dev, 1);
	uint16_t xpos = CONFIG_WIDTH / 3;
	uint16_t ypos = CONFIG_HEIGHT / 4;
	strcpy((char *)ascii, text);
	lcdDrawString(&dev, fx32G, xpos, ypos, ascii, color);
}
