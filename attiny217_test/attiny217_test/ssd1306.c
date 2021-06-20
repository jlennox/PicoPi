/*
 * ssd1306.c
 *
 * Created: 5/29/2021 11:59:46 AM
 *  Author: joe
 */

// Code based on logic from https://github.com/micropython/micropython/blob/master/drivers/display/ssd1306.py

#include <stdarg.h>
#include <stdlib.h>

#include "system.h"
#include "ssd1306.h"
#include "i2c.h"
#include "font_petme128_8x8_full.h"

#define SSD1306_SET_CONTRAST 0x81
#define SSD1306_SET_ENTIRE_ON 0xA4
#define SSD1306_SET_NORM_INV 0xA6
#define SSD1306_SET_DISP 0xAE
#define SSD1306_SET_MEM_ADDR 0x20
#define SSD1306_SET_COL_ADDR 0x21
#define SSD1306_SET_PAGE_ADDR 0x22
#define SSD1306_SET_DISP_START_LINE 0x40
#define SSD1306_SET_SEG_REMAP 0xA0
#define SSD1306_SET_MUX_RATIO 0xA8
#define SSD1306_SET_COM_OUT_DIR 0xC0
#define SSD1306_SET_DISP_OFFSET 0xD3
#define SSD1306_SET_COM_PIN_CFG 0xDA
#define SSD1306_SET_DISP_CLK_DIV 0xD5
#define SSD1306_SET_PRECHARGE 0xD9
#define SSD1306_SET_VCOM_DESEL 0xDB
#define SSD1306_SET_CHARGE_PUMP 0x8D

void SSD1306_WriteCmd(const SSD1306_t display, const int8_t cmd)
{
    twi_start(0x3C, 0);
    twi_write(0x00);
    twi_write(cmd);
    twi_stop();
}

SSD1306_t SSD1306_Init(const i2c_address_t address, const uint8_t width, const uint8_t height)
{
    SSD1306_t display = {
        .width = width,
        .height = height,
        .address = address
    };
    
    SSD1306_WriteCmd(display, SSD1306_SET_DISP);
    SSD1306_WriteCmd(display, SSD1306_SET_MEM_ADDR);
    SSD1306_WriteCmd(display, 0x00); // horizontal
    SSD1306_WriteCmd(display, SSD1306_SET_DISP_START_LINE); // start at line 0
    SSD1306_WriteCmd(display, SSD1306_SET_SEG_REMAP | 0x01); // column addr 127 mapped to SEG0
    SSD1306_WriteCmd(display, SSD1306_SET_MUX_RATIO);
    SSD1306_WriteCmd(display, height - 1);
    SSD1306_WriteCmd(display, SSD1306_SET_COM_OUT_DIR | 0x08); // scan from COM[N] to COM0
    SSD1306_WriteCmd(display, SSD1306_SET_DISP_OFFSET);
    SSD1306_WriteCmd(display, 0x00);
    SSD1306_WriteCmd(display, SSD1306_SET_COM_PIN_CFG);
    SSD1306_WriteCmd(display, width > 2 * height ? 0x02 : 0x12);
    SSD1306_WriteCmd(display, SSD1306_SET_DISP_CLK_DIV);
    SSD1306_WriteCmd(display, 0x80);
    SSD1306_WriteCmd(display, SSD1306_SET_PRECHARGE);
    SSD1306_WriteCmd(display, 0xF1);
    SSD1306_WriteCmd(display, SSD1306_SET_VCOM_DESEL);
    SSD1306_WriteCmd(display, 0x30); // 0.83*Vcc
    
    SSD1306_Contrast(display, 0xFF);
    SSD1306_WriteCmd(display, SSD1306_SET_ENTIRE_ON);
    SSD1306_WriteCmd(display, SSD1306_SET_NORM_INV);
    SSD1306_WriteCmd(display, SSD1306_SET_CHARGE_PUMP);
    SSD1306_WriteCmd(display, 0x14);
    SSD1306_ShowFill(display, 0);
    SSD1306_Poweron(display);
    
    return display;
}

inline void SSD1306_Poweron(const SSD1306_t display)
{
    SSD1306_WriteCmd(display, SSD1306_SET_DISP | 0x01);
}

inline void SSD1306_Invert(const SSD1306_t display, const bool invert)
{
    SSD1306_WriteCmd(display, SSD1306_SET_NORM_INV | (invert ? 1 : 0));
}

void SSD1306_Contrast(const SSD1306_t display, const int8_t contrast)
{
    SSD1306_WriteCmd(display, SSD1306_SET_CONTRAST);
    SSD1306_WriteCmd(display, contrast);
}

static void SSD1306_InitDraw(
    const SSD1306_t display,
    uint8_t x0, uint8_t x1,
    uint8_t y0, uint8_t y1)
{
    // displays with width of 64 pixels are shifted by 32
    if (display.width == 64) {
        x0 += 32;
        x1 += 32;
    }
    
    SSD1306_WriteCmd(display, SSD1306_SET_COL_ADDR);
    SSD1306_WriteCmd(display, x0);
    SSD1306_WriteCmd(display, x1);
    SSD1306_WriteCmd(display, SSD1306_SET_PAGE_ADDR);
    SSD1306_WriteCmd(display, y0);
    SSD1306_WriteCmd(display, y1);
    twi_start(display.address, 0);
    twi_write(0x40);
}    

void SSD1306_ShowFill(const SSD1306_t display, const uint8_t data)
{
    uint8_t pages = display.height / 8;
    uint16_t size = display.width * pages;
    uint8_t x0 = 0;
    uint8_t x1 = display.width - 1;

    SSD1306_InitDraw(display, x0, x1, 0, pages -1);

    for (uint16_t i = 0; i < size; ++i) {
        twi_write(data);
    }
    
    twi_stop();
}

void SSD1306_ShowString(const SSD1306_t display, const uint8_t x, const uint8_t y, const char* str)
{
    uint8_t strLen = 0;
    for (const char* charPtr = str; *charPtr != 0 ; ++charPtr) {
        ++strLen;
    }
    
    uint8_t x0 = x * 8;
    SSD1306_InitDraw(display, x0, x0 + strLen * 8, y, y);

    for (const char* charPtr = str; charPtr < str + strLen; ++charPtr) {
        const char chr = *charPtr;
        const uint8_t* pixelPtr = &font_data[(chr - 32) * 8];
        twi_writeBytes(pixelPtr, 8);
    }

    twi_stop();
}

// Only uint8_t's are supported.
void SSD1306_Printf(const SSD1306_t display, const uint8_t x, const uint8_t y, const char* str, ...)
{
    // expandedLen is the length of the final string with all the %'s expanded.
    uint8_t expandedLen = 0;
    uint8_t argCount = 0;
    for (const char* charPtr = str; *charPtr != 0 ; ++charPtr) {
        ++expandedLen;
        if (*charPtr == '%') ++argCount;
    }
    
    // strLen is the length of the format string.
    uint8_t strLen = expandedLen;
    
    va_list argp;
    va_start(argp, str);
    for (uint8_t i = 0; i < argCount; ++i) {
        int arg = va_arg(argp, int);
        if (arg > 100) expandedLen += 3;
        else if (arg > 10) expandedLen += 2;
        else ++expandedLen;
    }       
    va_end(argp);
    
    uint8_t x0 = x * 8;
    SSD1306_InitDraw(display, x0, x0 + expandedLen * 8, y, y);
    
    char buff[5];
    va_start(argp, str);
    for (const char* charPtr = str; charPtr < str + strLen; ++charPtr) {
        const char chr = *charPtr;
        
        if (*charPtr == '%') {
            int arg = va_arg(argp, int);
            char* argPtr = utoa(arg, buff, 10);
            while (true) {
                if (*argPtr == '\0') break;
                const uint8_t* pixelPtr = &font_data[(*argPtr - 32) * 8];
                twi_writeBytes(pixelPtr, 8);
                ++argPtr;
            }                
            continue;
        }
                    
        const uint8_t* pixelPtr = &font_data[(chr - 32) * 8];
        twi_writeBytes(pixelPtr, 8);
    }
    va_end(argp);

    twi_stop();
}    