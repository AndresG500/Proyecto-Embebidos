#ifndef LCD_H
#define LCD_H

#define _XTAL_FREQ 20000000

#include <xc.h>

#define RS PORTDbits.RD2
#define EN PORTDbits.RD3
#define D4 PORTDbits.RD4
#define D5 PORTDbits.RD5
#define D6 PORTDbits.RD6
#define D7 PORTDbits.RD7

#define RS_DIR TRISDbits.TRISD2
#define EN_DIR TRISDbits.TRISD3
#define D4_DIR TRISDbits.TRISD4
#define D5_DIR TRISDbits.TRISD5
#define D6_DIR TRISDbits.TRISD6
#define D7_DIR TRISDbits.TRISD7

void Lcd_Port(char a);
void Lcd_Cmd(char a);
void Lcd_Clear(void);
void Lcd_Set_Cursor(char a, char b);
void Lcd_Init(void);
void Lcd_Write_Char(char a);
void Lcd_Write_String(const char *a);

#endif