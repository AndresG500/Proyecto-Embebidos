#ifndef DS3231_H
#define DS3231_H

#define _XTAL_FREQ 20000000
#include <xc.h>
#include <stdint.h>

/* Pines I2C por software */
#define SCL_PIN  PORTCbits.RC3
#define SDA_PIN  PORTCbits.RC4
#define SCL_TRIS TRISCbits.TRISC3
#define SDA_TRIS TRISCbits.TRISC4

#define DS3231_ADDRESS 0x68

#define DS3231_REG_SECONDS  0x00
#define DS3231_REG_MINUTES  0x01
#define DS3231_REG_HOURS    0x02
#define DS3231_REG_DAY      0x03
#define DS3231_REG_DATE     0x04
#define DS3231_REG_MONTH    0x05
#define DS3231_REG_YEAR     0x06

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;   // 1-7
    uint8_t date;  // 1-31
    uint8_t month; // 1-12
    uint8_t year;  // 0-99
} DS3231_Time;

void    DS3231_Init(void);
uint8_t DS3231_IsPresent(void);
uint8_t DS3231_ReadTime(DS3231_Time *time);
uint8_t bcd2dec(uint8_t bcd);

#endif
