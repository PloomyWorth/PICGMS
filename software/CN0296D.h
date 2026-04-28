/*
/* 
 * File:   CN0296D.h
 * Author: Schn1881
 *
 * Created on April 17, 2026, 1:54 PM
 */

#ifndef CN0296D_H
#define	CN0296D_H

#ifdef	__cplusplus
extern "C" {
#endif

// Config
#define LCD_I2C_ADDR    0x27
#define I2C1BRG_VALUE   157   // 100 kHz at FCY=16MHz
#define LCD_ROWS        4
#define LCD_COLS        20

// PCF8574T Pin Mapping
#define LCD_BACKLIGHT   0x08
#define LCD_NOBACKLIGHT 0x00
#define EN_PIN          0x04
#define RW_PIN          0x02
#define RS_PIN          0x01

void lcd_init(void);
void lcd_cmd(char command);
void lcd_clear(void);
void lcd_setCursor(char x, char y);
void lcd_printChar(char myChar);
void lcd_printStr(const char *str);

// Print sensor data
void lcd_printInt(int number);

// Turn the backlight on (1) or off (0)
void lcd_backlight(unsigned char state);

// Shift the entire display left or right
void lcd_shiftLeft(void);
void lcd_shiftRight(void);

// Create a custom 5x8 pixel character (e.g., a degree symbol or battery icon)
// location: 0-7, charmap: array of 8 bytes representing the pixels
void lcd_createChar(unsigned char location, const unsigned char charmap[]);

#endif


#ifdef	__cplusplus
}
#endif
