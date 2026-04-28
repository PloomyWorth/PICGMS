/*
 * File:   CN0296D.c
 * Author: jimba
 *
 * Created on April 17, 2026, 1:53 PM
 */


#include "xc.h"
#include "CN0296D.h"

#ifndef FCY
#define FCY 16000000UL 
#endif

#include <libpic30.h>
#include <stdio.h>

static const char ROW_ADDR[4] = { 0x00, 0x40, 0x14, 0x54 };

static void i2c_idle(void) {
    unsigned int timeout = 10000;
    while ((I2C1CONbits.SEN  || I2C1CONbits.RSEN  ||
            I2C1CONbits.PEN  || I2C1CONbits.RCEN  ||
            I2C1CONbits.ACKEN || I2C1STATbits.TRSTAT)
            && --timeout);
}
 
static void i2c_start(void) {
    i2c_idle();
    IFS1bits.MI2C1IF = 0;
    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN); // Wait for hardware to clear SEN
}
 
static void i2c_stop(void) {
    i2c_idle();
    I2C1CONbits.PEN = 1;
    while (I2C1CONbits.PEN); // Wait for hardware to clear PEN     
}
 
static void i2c_write_byte(unsigned char byte) {
    IFS1bits.MI2C1IF = 0;        
    I2C1TRN = byte;
    while (!IFS1bits.MI2C1IF);   // Wait for transaction
    while (I2C1STATbits.TRSTAT); // Wait for shift register to empty
}

// Formatting Functions

static unsigned char backlight_val = LCD_BACKLIGHT; 

static void pcf8574_write(unsigned char data) {
    i2c_start();
    i2c_write_byte((LCD_I2C_ADDR << 1) | 0x00); 
    i2c_write_byte(data | backlight_val); 
    i2c_stop();
}

static void lcd_write_nibble(unsigned char nibble, unsigned char mode) {
    unsigned char data_byte = (nibble & 0xF0) | mode; 
    pcf8574_write(data_byte | EN_PIN); // EN = 1
    __delay_us(1);                     // Pulse width
    pcf8574_write(data_byte & ~EN_PIN);// EN = 0
    __delay_us(50);                    // Execution time
}

static void lcd_write(unsigned char value, unsigned char mode) {
    lcd_write_nibble(value & 0xF0, mode);        // High nibble
    lcd_write_nibble((value << 4) & 0xF0, mode); // Low nibble
}

// Public LCD Functions

void lcd_cmd(char command) {
    lcd_write(command, 0); // RS = 0 for command
    if (command == 0x01 || command == 0x02) {
        __delay_ms(2); // Clear and Home require more time
    }
}

void lcd_clear(void) {
    lcd_cmd(0x01);
}

void lcd_printChar(char myChar) {
    lcd_write(myChar, RS_PIN); // RS = 1 for data
}

void lcd_setCursor(char x, char y) {
    if ((unsigned char)y >= LCD_ROWS || (unsigned char)x >= LCD_COLS) return; 
    lcd_cmd(0x80 | (ROW_ADDR[(unsigned char)y] + (unsigned char)x));
}

void lcd_printStr(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        lcd_printChar(str[i]);
    }
}

// Print Numbers
void lcd_printInt(int number) {
    char buffer[16]; // Buffer to hold the converted number string
    sprintf(buffer, "%d", number); 
    lcd_printStr(buffer);
}

// Backlight Control
void lcd_backlight(unsigned char state) {
    if (state) {
        backlight_val = LCD_BACKLIGHT;
    } else {
        backlight_val = LCD_NOBACKLIGHT;
    }
    pcf8574_write(0); // Send a dummy byte to update the physical pin immediately
}

// Scrolling Functions
void lcd_shiftLeft(void) {
    lcd_cmd(0x18); // 0x18 is the HD44780 command to shift display left
}

void lcd_shiftRight(void) {
    lcd_cmd(0x1C); // 0x1C is the HD44780 command to shift display right
}

// Custom Characters
void lcd_createChar(unsigned char location, const unsigned char charmap[]) {
    location &= 0x07; // memory slots (0 to 7)
    lcd_cmd(0x40 | (location << 3)); // Command to set CGRAM address
    for (int i = 0; i < 8; i++) {
        lcd_printChar(charmap[i]);   // Write the 8 rows of pixels
    }
}

void lcd_init(void) {   
    // Configure PIC I2C Hardware
    I2C1CONbits.I2CEN = 0;          
    TRISBbits.TRISB8 = 1;    // SCL Input 
    TRISBbits.TRISB9 = 1;    // SDA Input 
    I2C1BRG = I2C1BRG_VALUE;  
    IFS1bits.MI2C1IF = 0;          
    IEC1bits.MI2C1IE = 0;       
    I2C1STAT = 0;               
    I2C1CONbits.I2CEN = 1;          

    __delay_ms(50); // Wait for LCD power to stabilize

    // 4-bit Initialization Sequence
    lcd_write_nibble(0x30, 0); 
    __delay_ms(5); 
    
    lcd_write_nibble(0x30, 0);
    __delay_us(150); 
    
    lcd_write_nibble(0x30, 0);
    
    // Set to 4-bit interface
    lcd_write_nibble(0x20, 0); 

    // Standard Setup
    lcd_cmd(0x28); // 4-bit, 2 lines, 5x8 font
    lcd_cmd(0x08); // Display off
    lcd_clear();   // Clear display
    lcd_cmd(0x06); // Entry mode: Increment cursor, no shift
    lcd_cmd(0x0C); // Display ON, Cursor OFF, Blink OFF
}
