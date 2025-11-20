#include "DS3231.h"

#define I2C_DELAY() __delay_us(5)

static inline void SDA_release(void){ SDA_TRIS = 1; }
static inline void SDA_drive0(void){ SDA_TRIS = 0; SDA_PIN = 0; }
static inline void SCL_release(void){ SCL_TRIS = 1; }
static inline void SCL_drive0(void){ SCL_TRIS = 0; SCL_PIN = 0; }

static void I2C_Init(void){
    SDA_release();
    SCL_release();
    I2C_DELAY();
}
static void I2C_Start(void){
    SDA_release();
    SCL_release(); I2C_DELAY();
    SDA_drive0();  I2C_DELAY();
    SCL_drive0();
}
static void I2C_Stop(void){
    SDA_drive0();  I2C_DELAY();
    SCL_release(); I2C_DELAY();
    SDA_release(); I2C_DELAY();
}
static uint8_t I2C_Write(uint8_t data){
    for(uint8_t i=0;i<8;i++){
        if(data & 0x80) SDA_release(); else SDA_drive0();
        SCL_release(); I2C_DELAY();
        SCL_drive0();
        data <<= 1;
    }
    SDA_release();
    SCL_release(); I2C_DELAY();
    uint8_t ack = (SDA_PIN == 0);
    SCL_drive0();
    return ack;
}
static uint8_t I2C_Read(uint8_t ack){
    uint8_t d=0;
    SDA_release();
    for(uint8_t i=0;i<8;i++){
        d <<= 1;
        SCL_release(); I2C_DELAY();
        if(SDA_PIN) d |= 1;
        SCL_drive0();
    }
    if(ack) SDA_drive0(); else SDA_release();
    SCL_release(); I2C_DELAY();
    SCL_drive0();  SDA_release();
    return d;
}

uint8_t bcd2dec(uint8_t b){ return ((b>>4)*10u) + (b & 0x0Fu); }

void DS3231_Init(void){ I2C_Init(); __delay_ms(30); }

uint8_t DS3231_IsPresent(void){
    I2C_Start();
    uint8_t ok = I2C_Write(DS3231_ADDRESS<<1); // write
    I2C_Stop();
    return ok; // 1=ACK
}

static void clr(DS3231_Time *t){
    t->seconds=t->minutes=t->hours=0;
    t->day=t->date=t->month=t->year=0;
}

uint8_t DS3231_ReadTime(DS3231_Time *time){
    uint8_t r;

    if(!DS3231_IsPresent()){ clr(time); return 0; }

    I2C_Start();
    if(!I2C_Write(DS3231_ADDRESS<<1)) { I2C_Stop(); clr(time); return 0; }
    if(!I2C_Write(DS3231_REG_SECONDS)) { I2C_Stop(); clr(time); return 0; }

    I2C_Start();
    if(!I2C_Write((DS3231_ADDRESS<<1)|1)) { I2C_Stop(); clr(time); return 0; }

    r = I2C_Read(1); time->seconds = bcd2dec(r & 0x7F);
    r = I2C_Read(1); time->minutes = bcd2dec(r & 0x7F);
    r = I2C_Read(1); time->hours   = bcd2dec(r & 0x3F); // 24h
    r = I2C_Read(1); time->day     = bcd2dec(r & 0x07);
    r = I2C_Read(1); time->date    = bcd2dec(r & 0x3F);
    r = I2C_Read(1); time->month   = bcd2dec(r & 0x1F);
    r = I2C_Read(0); time->year    = bcd2dec(r);

    I2C_Stop();

    // Validación mínima
    if(time->seconds>59 || time->minutes>59 || time->hours>23 ||
       time->date<1 || time->date>31 || time->month<1 || time->month>12){
        clr(time); return 0;
    }
    return 1;
}
