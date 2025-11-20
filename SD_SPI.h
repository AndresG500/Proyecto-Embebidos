#ifndef SD_SPI_H
#define SD_SPI_H

#define _XTAL_FREQ 20000000
#include <xc.h>
#include <stdint.h>

// Pines SPI por SOFTWARE
#define SD_CS_PIN    PORTBbits.RB0
#define SD_CS_TRIS   TRISBbits.TRISB0
#define SD_MOSI_PIN  PORTBbits.RB1
#define SD_MOSI_TRIS TRISBbits.TRISB1
#define SD_MISO_PIN  PORTBbits.RB2
#define SD_MISO_TRIS TRISBbits.TRISB2
#define SD_SCK_PIN   PORTBbits.RB3
#define SD_SCK_TRIS  TRISBbits.TRISB3

// Comandos SD
#define CMD0    0x00
#define CMD8    0x08
#define CMD16   0x10
#define CMD17   0x11
#define CMD24   0x18
#define CMD41   0x29
#define CMD55   0x37

// Funciones básicas

void SD_SPI_Init(void);
uint8_t SD_SPI_Transfer(uint8_t data);
uint8_t SD_Init(void);
void SD_ReadBytes(uint32_t sector, uint16_t offset, uint8_t *buf, uint8_t len);
uint8_t SD_ReadSector(uint32_t sector, uint8_t *buf);
uint8_t SD_WriteSector(uint32_t sector, uint8_t *buf);

// Funciones de lectura/escritura simplificadas
uint8_t SD_ReadByte(uint32_t sector, uint16_t offset);
void SD_ReadBytes(uint32_t sector, uint16_t offset, uint8_t *buf, uint8_t len);
void SD_WriteBytes(uint32_t sector, uint16_t offset, uint8_t *buf, uint8_t len);


#endif