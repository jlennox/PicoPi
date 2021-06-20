/*
 * attiny217_test.c
 *
 * Created: 5/25/2021 5:36:23 PM
 * Author : joe
 */ 

#include "system.h"

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "i2c.h"
#include "ssd1306.h"
#include "font_petme128_8x8_full.h"
#include "millis_tcb.h"

//#define debug 0

// uh, for some reason input isn't working unless the DIR pin is set...?!?
#define SET_PIN_WRITE(pinCfg) (pinCfg.port->DIRSET = pinCfg.pin)
#define SET_PIN_READ(pinCfg)  (pinCfg.port->DIRSET = pinCfg.pin)

#define SET_PIN_ON(pinCfg)  (pinCfg.port->OUTSET = pinCfg.pin)
#define SET_PIN_OFF(pinCfg) (pinCfg.port->OUTCLR = pinCfg.pin)
#define SET_PIN_TOGGLE(pinCfg) (pinCfg.port->OUT ^= pinCfg.pin)

#define READ_PIN(pinCfg) ((pinCfg.port->IN & pinCfg.pin))

#define EEPROM_MAGIC (uint32_t)0x7A7A3166
#define EEPROM_READ_MAGIC() eeprom_read_dword((uint32_t*)00)
#define EEPROM_WRITE_MAGIC() eeprom_write_dword((uint32_t*)0, EEPROM_MAGIC)
#define EEPROM_READ_HIGHSCORE() eeprom_read_byte((uint8_t*)4)
#define EEPROM_WRITE_HIGHSCORE(value) eeprom_write_byte((uint8_t*)4, value)

typedef uint8_t difficulty_level_t;

typedef struct _simon_pin_struct {
    PORT_t* port;
    const uint8_t pin;
} simon_pin_t;

typedef struct _simon_io_config_struct {
    const uint16_t toneFrequency;
    const simon_pin_t input;
    const simon_pin_t led;
} simon_io_config_t;

typedef struct _simon_hardware_config_struct {
    const simon_io_config_t* io;
    const uint8_t ioCount;
    const simon_pin_t* speedInput;
    const uint8_t speedInputCount;
} simon_hardware_config_t;

SSD1306_t g_display;

// `static const` keeps everything in the .text, not in the much more limited ram.
static const simon_io_config_t simonIO[] = {
    {
        .toneFrequency = 262,
        .input = {
            .port = PORTC_ADDR,
            .pin = PIN0_bm
        },
        .led = {
            .port = PORTA_ADDR,
            .pin = PIN1_bm
        }
    }, {
        .toneFrequency = 294,
        .input = {
            .port = PORTC_ADDR,
            .pin = PIN1_bm
        },
        .led = {
            .port = PORTA_ADDR,
            .pin = PIN2_bm
        }
    }, {
        .toneFrequency = 330,
        .input = {
            .port = PORTC_ADDR,
            .pin = PIN2_bm
        },
        .led = {
            .port = PORTA_ADDR,
            .pin = PIN3_bm
        }
    }, {
        .toneFrequency = 349,
        .input = {
            .port = PORTC_ADDR,
            .pin = PIN3_bm
        },
        .led = {
            .port = PORTA_ADDR,
            .pin = PIN4_bm
        }
    }
};

static const simon_pin_t speedInput[] = {
    {
        .port = PORTA_ADDR,
        .pin = PIN5_bm,
    }, {
        .port = PORTA_ADDR,
        .pin = PIN6_bm,
    }
};

static const simon_hardware_config_t simonConfig = {
    .io = simonIO,
    .ioCount = 4,
    .speedInput = speedInput,
    .speedInputCount = 2,
};

void simon_debug(const char* string)
{
#ifdef debug
    SSD1306_ShowString(g_display, 1, 6, "              ");
    SSD1306_ShowString(g_display, 1, 6, string);
#endif
}   

void simon_debug_int(const uint32_t d, const uint8_t y)
{
#ifdef debug
    utoa(d, g_tmpBuffer, 10);
    SSD1306_ShowString(g_display, 1, y, "        ");
    SSD1306_ShowString(g_display, 1, y, g_tmpBuffer);
#endif
}

void delay(double delay)
{
    while (delay > 0) {
        _delay_ms(10);
        delay -= 10;
    }        
}    

void simon_buzz(const simon_io_config_t config)
{
    TCA0.SINGLE.CNT = 0;
    TCA0.SINGLE.PERBUF = F_CPU / (2 * config.toneFrequency);
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;  
}

void simon_buzz_off()
{
    TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;
}

void simon_attract(const simon_hardware_config_t config)
{
    SSD1306_ShowFill(g_display, 0);
    simon_debug("Attract");
    
    SSD1306_Printf(g_display, 1, 5, "Highscore %", EEPROM_READ_HIGHSCORE());
    
    for (uint8_t z = 0; z < 2; ++z) {
        for (uint8_t i = 0; i < config.ioCount; ++i) {
            simon_io_config_t pinConfig = config.io[i];
            SET_PIN_ON(pinConfig.led);
            simon_buzz(pinConfig);
            _delay_ms(100);
        }

        simon_buzz_off();

        for (int8_t i = config.ioCount - 1; i >= 0; --i) {
            simon_io_config_t pinConfig = config.io[i];
            SET_PIN_OFF(pinConfig.led);
            _delay_ms(100);
        }
    }

    for (uint8_t z = 0; z < 4; ++z) {
        for (uint8_t i = 0; i < config.ioCount; ++i) {
            simon_io_config_t pinConfig = config.io[i];
            SET_PIN_TOGGLE(pinConfig.led);
        }

        _delay_ms(100);
    }
}

difficulty_level_t simon_get_difficulty(const simon_hardware_config_t config)
{
    uint8_t difficulty = 0;
    for (uint8_t i = 0; i < config.speedInputCount; ++i) {
        if (READ_PIN(config.speedInput[i])) ++difficulty;
    }

    return difficulty;
}

void simon_display_score(const difficulty_level_t difficultyLevel, const uint8_t score)
{
    SSD1306_ShowFill(g_display, 0);
    SSD1306_Printf(g_display, 1, 1, "Difficulty: %", difficultyLevel);
    SSD1306_Printf(g_display, 1, 5, "Score %", score);
}

int8_t simon_read_input(const simon_hardware_config_t config)
{
    for (uint8_t i = 0; i < config.ioCount; ++i) {
        simon_io_config_t pinConfig = config.io[i];
        if (READ_PIN(pinConfig.input)) {
            SET_PIN_ON(pinConfig.led);
            simon_buzz(pinConfig);
            // Loop until released
            while (READ_PIN(pinConfig.input)) {};
            SET_PIN_OFF(pinConfig.led);
            simon_buzz_off();
            return i;
        }            
    }
    
    return -1;
}

uint8_t simon_play(const simon_hardware_config_t config)
{
    uint8_t moves[50] = {};
    uint8_t moveCount = 0;
    uint8_t difficultyLevel = simon_get_difficulty(config);
    const double difficulty = 1.0 - difficultyLevel * .4;
    const uint16_t inputTime = 5000 - (difficultyLevel * 400);
    simon_display_score(difficultyLevel, 0);
        
    while (true) {
        moves[moveCount] = rand() % config.ioCount;
        ++moveCount;
        
        simon_debug_int(moveCount, 3);

        // Replay existing moves.
        simon_debug("replaying...");
        for (uint8_t i = 0; i < moveCount; ++i) {
            const uint8_t entry = moves[i];
            simon_io_config_t pinConfig = config.io[entry];
            SET_PIN_ON(pinConfig.led);
            simon_buzz(pinConfig);
            delay(500 * difficulty);
            
            SET_PIN_OFF(pinConfig.led);
            simon_buzz_off();
            delay(200 * difficulty);
        }
        
        for (uint8_t i = 0; i < moveCount; ++i) {
            const uint8_t entry = moves[i];
            
            millis_restart();
            simon_debug("input...");
            while (true) {
                const int8_t input = simon_read_input(config);
                
                // They got it correct.
                if (input == entry) break;
                
                const uint32_t elapsed = millis();
                simon_debug_int(elapsed, 2);
                
                // Timed out of wrong input.
                if (elapsed > inputTime || (input != -1 && input != entry)) {
                    simon_debug("Failed!");
                    const simon_io_config_t pinConfig = config.io[entry];
                    simon_buzz(pinConfig);
                    for (uint8_t i = 0; i < 12; ++i) {
                        SET_PIN_TOGGLE(pinConfig.led);
                        _delay_ms(100);
                    }
                    simon_buzz_off();
                    
                    return moveCount;
                }
            }
        }
        
        simon_display_score(difficultyLevel, moveCount + 1);
        
        // Add a bit of a delay so it doesn't jump right into the replay when they release the button.
        _delay_ms(1000);
    }
}

void simon_init(const simon_hardware_config_t config)
{
    // Initialize eeprom to 0 if needed.
    if (EEPROM_READ_MAGIC() != EEPROM_MAGIC) {
        for (uint32_t* i = 0; i < EEPROM_SIZE / sizeof(uint32_t); ++i) {
            eeprom_update_dword(i, 0);
        }            
        eeprom_update_dword(0, EEPROM_MAGIC);
    }   
    
    // Initialize all pin configurations.
    for (uint8_t i = 0; i < config.ioCount; ++i) {
        const simon_io_config_t ioConfig = config.io[i];
        SET_PIN_READ(ioConfig.input);
        SET_PIN_WRITE(ioConfig.led);
    }
    
    for (uint8_t i = 0; i < config.speedInputCount; ++i) {
        const simon_pin_t pin = config.speedInput[i];
        SET_PIN_READ(pin);
    }
}

void pwm_init()
{
    // Use PB3 for PWM instead of PB0, which is used by TWI.
    PORTMUX.CTRLC |= PORTMUX_TCA00_bm;
    VPORTB.DIR |= PIN3_bm;
    
    TCA0.SINGLE.PERBUF = F_CPU / (2 * 262);
    TCA0.SINGLE.CMP0BUF = 1000 / 4;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_DSTOP_gc | (1 << TCA_SINGLE_CMP0EN_bp);
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc;
}

int main()
{
    // Set CPU to 20mhz with no divisor.
    CPU_CCP = 0xD8;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_OSC20M_gc;
    CPU_CCP = 0xD8;
    CLKCTRL.MCLKCTRLB = 0;
    
    pwm_init();
    millis_init(&TCB0);

    // Make SDA/SLC writable for TWI.
    VPORTB.DIR |= PIN0_bm | PIN1_bm;
    twi_init();
    
    g_display = SSD1306_Init(0x3c, 128, 64);

    simon_init(simonConfig);
    
    while (1) {
        simon_attract(simonConfig);
        uint8_t score = simon_play(simonConfig);
        
        if (score > EEPROM_READ_HIGHSCORE()) {
            EEPROM_WRITE_HIGHSCORE(score);
        }            
        
        _delay_ms(1000);
    }
}