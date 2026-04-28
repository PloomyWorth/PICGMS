/*
 * File:   LCD_IMP.c
 * Author: jimba
 *
 * Created on April 23, 2026, 11:32 PM
 */


#include <xc.h>
#include "CN0296D.h"

#ifndef FCY
#define FCY 16000000UL 
#endif

#include <libpic30.h> 
#include <stdio.h>

// CW1: FLASH CONFIGURATION WORD 1
#pragma config ICS = PGx1
#pragma config FWDTEN = OFF
#pragma config GWRP = OFF
#pragma config GCP = OFF
#pragma config JTAGEN = OFF
 
// CW2: FLASH CONFIGURATION WORD 2
#pragma config I2C1SEL = PRI
#pragma config IOL1WAY = OFF
#pragma config OSCIOFNC = ON
#pragma config FCKSM = CSECME
#pragma config FNOSC = FRCPLL

// 28-Pin Button Macros
#define BTN_UP     PORTBbits.RB11 // Pin 22
#define BTN_DOWN   PORTBbits.RB10 // Pin 21
#define BTN_SELECT PORTBbits.RB7  // Pin 16
#define BTN_BACK   PORTBbits.RB6  // Pin 15

typedef enum {
    SCREEN_HOME,
    SCREEN_MAIN_MENU,
    WATER_SETTINGS, 
    POWER_STATUS,
    FLOW_RATE,
    WATERING_FREQ
} UI_State;

UI_State currentState = SCREEN_HOME;
int menuCursor = 0;

// System Variables 
int sys_water_level = 85;       // Dummy sensor value (85%)
int sys_watering_freq = 1;      // Days (1 = 1 Day, 7 = 1 Week)
int sys_flow_rate = 50;         // mL (50, 100, or 150)
int sys_rate_of_charge = 14;    // Watts (Simulated solar sensor reading)

// Initialize the hardware pins
void buttons_init(void) {
    // Set pins as inputs (1 = Input)
    TRISBbits.TRISB11 = 1; // Up
    TRISBbits.TRISB10 = 1; // Down
    TRISBbits.TRISB7  = 1; // Select
    TRISBbits.TRISB6  = 1; // Back

    // Disable analog mode on Pins 15 & 16 (RB6/RB7)
    AD1PCFG |= 0x00C0; 
    
    CNPU1bits.CN15PUE = 1; // Pull-up for RB11 (Up)
    CNPU2bits.CN16PUE = 1; // Pull-up for RB10 (Down)
    CNPU2bits.CN23PUE = 1; // Pull-up for RB7 (Select)
    CNPU2bits.CN24PUE = 1; // Pull-up for RB6 (Back)
} 

int button_up_pressed(void) {
    static int last_state = 1;
    int current_state = BTN_UP;
    // Trigger only if transitioning from HIGH (unpressed) to LOW (pressed)
    if (current_state == 0 && last_state == 1) { last_state = current_state; return 1; }
    last_state = current_state; return 0;
}

int button_down_pressed(void) {
    static int last_state = 1;
    int current_state = BTN_DOWN;
    if (current_state == 0 && last_state == 1) { last_state = current_state; return 1; }
    last_state = current_state; return 0;
}

int button_select_pressed(void) {
    static int last_state = 1;
    int current_state = BTN_SELECT;
    if (current_state == 0 && last_state == 1) { last_state = current_state; return 1; }
    last_state = current_state; return 0;
}

int button_back_pressed(void) {
    static int last_state = 1;
    int current_state = BTN_BACK;
    if (current_state == 0 && last_state == 1) { last_state = current_state; return 1; }
    last_state = current_state; return 0;
}

void draw_ui(void) {
    char buffer[21]; // Declare a reusable buffer
    lcd_clear(); // Clear the screen before drawing a new page

    switch (currentState) {
        
        case SCREEN_HOME:
            lcd_setCursor(0, 0);
            lcd_printStr("  SYSTEM DASHBOARD  "); 
            
            lcd_setCursor(0, 1);
            lcd_printStr("Status: ACTIVE     ");
            
            lcd_setCursor(0, 2);
            lcd_printStr("Temp  : 72 F       ");
            
            lcd_setCursor(0, 3);
            lcd_printStr(" [Press Select ->]  ");
            break;

        case SCREEN_MAIN_MENU:
            lcd_setCursor(0, 0);
            lcd_printStr("----- MAIN MENU -----"); 
            
            lcd_setCursor(0, 1);
            if (menuCursor == 0) lcd_printChar('>'); else lcd_printChar(' ');
            lcd_printStr(" Water Settings    "); 
            
            lcd_setCursor(0, 2);
            if (menuCursor == 1) lcd_printChar('>'); else lcd_printChar(' ');
            lcd_printStr(" Power Status      ");
            
            lcd_setCursor(0, 3);
            lcd_printStr("[Back]        [Next]"); 
            break;
            
        case WATER_SETTINGS:
            lcd_setCursor(0, 0);
            lcd_printStr("-- WATER SETTINGS --"); 
            
            lcd_setCursor(0, 1);
            sprintf(buffer, "  Water Level: %3d%% ", sys_water_level);
            lcd_printStr(buffer);
            
            lcd_setCursor(0, 2);
            sprintf(buffer, "%c Watering Freq: %d  ", (menuCursor == 0) ? '>' : ' ', sys_watering_freq);
            lcd_printStr(buffer);
            
            lcd_setCursor(0, 3);
            sprintf(buffer, "%c Flow Rate: %3d mL ", (menuCursor == 1) ? '>' : ' ', sys_flow_rate);
            lcd_printStr(buffer);
        break;
        
        case WATERING_FREQ:
            lcd_setCursor(0, 0);
            lcd_printStr("  How often water   ");
            
            lcd_setCursor(0, 1);
            lcd_printStr("    is dispensed:   ");
            
            lcd_setCursor(0, 3);
            if (menuCursor == 0)      lcd_printStr("  Every:  [1 Day]   "); 
            else if (menuCursor == 1) lcd_printStr("  Every:  [2 Days]  ");
            else if (menuCursor == 2) lcd_printStr("  Every:  [3 Days]  ");
            else if (menuCursor == 3) lcd_printStr("  Every:  [4 Days]  ");
            else if (menuCursor == 4) lcd_printStr("  Every:  [5 Days]  ");
            else if (menuCursor == 5) lcd_printStr("  Every:  [6 Days]  ");
            else if (menuCursor == 6) lcd_printStr("  Every:  [1 Week]  ");
            break;
        
        case FLOW_RATE:
            lcd_setCursor(0, 0);
            lcd_printStr("Water dispensed per ");
            
            lcd_setCursor(0, 1);
            lcd_printStr("     watering:      "); 
            
            lcd_setCursor(0, 3);
            if (menuCursor == 1)      lcd_printStr(" [50]  100   150 mL ");
            else if (menuCursor == 2) lcd_printStr("  50  [100]  150 mL ");
            else if (menuCursor == 3) lcd_printStr("  50   100  [150]mL ");
            break;
            
        case POWER_STATUS:
            lcd_setCursor(0, 0);
            lcd_printStr("--- POWER STATUS ---");
            
            lcd_setCursor(0, 1);
            lcd_printStr("  Rate of Charge:   ");
            
            lcd_setCursor(0, 2);
            sprintf(buffer, "       %3d W        ", sys_rate_of_charge); 
            lcd_printStr(buffer);
            
            lcd_setCursor(0, 3);
            lcd_printStr("    [Press Back]    ");
        break;
    }
}

int main(void) {
    lcd_init();
    buttons_init(); 
    draw_ui(); 

    while (1) {
        
        // UP BUTTON
        if (button_up_pressed()) {
            if (currentState == SCREEN_MAIN_MENU && menuCursor > 0) menuCursor--;
            else if (currentState == WATER_SETTINGS && menuCursor > 0) menuCursor--;
            else if (currentState == WATERING_FREQ && menuCursor > 0) menuCursor--;
            else if (currentState == FLOW_RATE && menuCursor > 1) menuCursor--; // Bottom limit is 1 here
            
            draw_ui();
        }
        
        // DOWN BUTTON
        if (button_down_pressed()) {
            if (currentState == SCREEN_MAIN_MENU && menuCursor < 1) menuCursor++;
            else if (currentState == WATER_SETTINGS && menuCursor < 1) menuCursor++;
            else if (currentState == WATERING_FREQ && menuCursor < 6) menuCursor++;
            else if (currentState == FLOW_RATE && menuCursor < 3) menuCursor++; // Top limit is 3 here
            
            draw_ui();
        }
        
        // SELECT BUTTON 
        if (button_select_pressed()) {
            if (currentState == SCREEN_HOME) {
                currentState = SCREEN_MAIN_MENU;
                menuCursor = 0;
            } 
            else if (currentState == SCREEN_MAIN_MENU) {
                if (menuCursor == 0) {
                    currentState = WATER_SETTINGS;
                    menuCursor = 0;
                } else if (menuCursor == 1) {
                    currentState = POWER_STATUS;
                    menuCursor = 0;
                }
            }
            else if (currentState == WATER_SETTINGS) {
                if (menuCursor == 0) {
                    currentState = WATERING_FREQ;
                    // Match the cursor to the currently saved frequency
                    menuCursor = sys_watering_freq - 1; 
                } else if (menuCursor == 1) {
                    currentState = FLOW_RATE;
                    // Match the cursor to the currently saved flow rate
                    if (sys_flow_rate == 50) menuCursor = 1;
                    else if (sys_flow_rate == 100) menuCursor = 2;
                    else if (sys_flow_rate == 150) menuCursor = 3;
                }
            }
            else if (currentState == WATERING_FREQ) {
                sys_watering_freq = menuCursor + 1; // Cursor 0-6 maps to 1-7 days
                currentState = WATER_SETTINGS;
                menuCursor = 0; // Set cursor back to the Freq option
            }
            else if (currentState == FLOW_RATE) {
                if (menuCursor == 1) sys_flow_rate = 50;
                else if (menuCursor == 2) sys_flow_rate = 100;
                else if (menuCursor == 3) sys_flow_rate = 150;
                
                currentState = WATER_SETTINGS;
                menuCursor = 1; // Set cursor back to the Flow Rate option
            }
            
            draw_ui();
        }
        
        // BACK BUTTON 
        if (button_back_pressed()) {
            if (currentState == SCREEN_MAIN_MENU) {
                currentState = SCREEN_HOME;
            } 
            else if (currentState == WATER_SETTINGS || currentState == POWER_STATUS) {
                currentState = SCREEN_MAIN_MENU;
                menuCursor = 0;
            }
            else if (currentState == WATERING_FREQ || currentState == FLOW_RATE) {
                currentState = WATER_SETTINGS;
                menuCursor = 0;
            }
            
            draw_ui();
        }
        
        __delay_ms(50); 
    }
    return 0;
}
