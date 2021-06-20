/*
 * ssd1306.h
 *
 * Created: 5/29/2021 11:59:40 AM
 *  Author: joe
 */ 


#ifndef SSD1306_H_
#define SSD1306_H_

#include <avr/io.h>

typedef struct {
	uint8_t width;
	uint8_t height;
	i2c_address_t address;
} SSD1306_t;

SSD1306_t SSD1306_Init(const i2c_address_t address,const  uint8_t width, const uint8_t height);
void SSD1306_Poweron(const SSD1306_t display);
void SSD1306_Invert(const SSD1306_t display, const bool invert);
void SSD1306_Contrast(const SSD1306_t display, const int8_t contrast);
void SSD1306_ShowFill(const SSD1306_t display, const uint8_t data);
void SSD1306_ShowString(const SSD1306_t display, const uint8_t x, const uint8_t y, const char* stringData);
void SSD1306_Printf(const SSD1306_t display, const uint8_t x, const uint8_t y, const char* str, ...);
void SSD1306_WriteCmd(const SSD1306_t display, const int8_t cmd);

#endif /* SSD1306_H_ */