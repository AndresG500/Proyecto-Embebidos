#include "SD_SPI.h"

#define SD_CS_LOW()   SD_CS_PIN = 0
#define SD_CS_HIGH()  SD_CS_PIN = 1

void SD_SPI_Init(void){
    SD_CS_TRIS = 0;
    SD_MOSI_TRIS = 0;
    SD_MISO_TRIS = 1;
    SD_SCK_TRIS = 0;
    SD_CS_HIGH();
    SD_SCK_PIN = 0;
    SD_MOSI_PIN = 1;
}

uint8_t SD_SPI_Transfer(uint8_t data){
    uint8_t i, result = 0;
    for(i = 0; i < 8; i++){
        SD_MOSI_PIN = (data & 0x80) ? 1 : 0;
        data <<= 1;
        SD_SCK_PIN = 1;
        __delay_us(1);
        result = (result << 1) | (SD_MISO_PIN ? 1 : 0);
        SD_SCK_PIN = 0;
        __delay_us(1);
    }
    return result;
}

static uint8_t SD_Cmd(uint8_t cmd, uint32_t arg){
    uint8_t r, i, crc = 0xFF;
    if(cmd == CMD0) crc = 0x95;
    if(cmd == CMD8) crc = 0x87;
    
    // CRÍTICO: CS debe estar bajo antes del comando
    // pero alto entre comandos
    
    SD_SPI_Transfer(0xFF);  // Byte dummy antes del comando
    
    SD_SPI_Transfer(cmd | 0x40);
    SD_SPI_Transfer((uint8_t)(arg >> 24));
    SD_SPI_Transfer((uint8_t)(arg >> 16));
    SD_SPI_Transfer((uint8_t)(arg >> 8));
    SD_SPI_Transfer((uint8_t)arg);
    SD_SPI_Transfer(crc);
    
    // Esperar respuesta (máximo 10 intentos)
    for(i = 0; i < 10; i++){
        r = SD_SPI_Transfer(0xFF);
        if(!(r & 0x80)) return r;  // Bit 7 = 0 indica respuesta válida
    }
    return 0xFF;
}

uint8_t SD_Init(void){
    uint8_t i, r;
    uint16_t retry;
    
    SD_SPI_Init();
    SD_CS_HIGH();
    for(i = 0; i < 10; i++) SD_SPI_Transfer(0xFF);
    
    SD_CS_LOW();
    __delay_ms(1);
    
    for(retry = 0; retry < 200; retry++){
        if(SD_Cmd(CMD0, 0) == 0x01) break;
    }
    if(retry >= 200){ SD_CS_HIGH(); return 0; }
    
    r = SD_Cmd(CMD8, 0x000001AA);
    if(r == 0x01){
        for(i = 0; i < 4; i++) SD_SPI_Transfer(0xFF);
    }
    
    for(retry = 0; retry < 500; retry++){
        SD_Cmd(CMD55, 0);
        if(SD_Cmd(CMD41, 0x40000000) == 0x00) break;
        __delay_ms(10);
    }
    if(retry >= 500){ SD_CS_HIGH(); return 0; }
    
    if(SD_Cmd(CMD16, 512) != 0x00){ SD_CS_HIGH(); return 0; }
    
    SD_CS_HIGH();
    SD_SPI_Transfer(0xFF);
    return 1;
}

// Leer solo bytes necesarios
void SD_ReadBytes(uint32_t sector, uint16_t offset, uint8_t *buf, uint8_t len){
    uint8_t r;
    uint16_t i, retry;
    
    SD_CS_LOW();
    if(SD_Cmd(CMD17, sector) != 0x00){ SD_CS_HIGH(); return; }
    
    for(retry = 0; retry < 5000; retry++){
        r = SD_SPI_Transfer(0xFF);
        if(r == 0xFE) break;
    }
    if(retry >= 5000){ SD_CS_HIGH(); return; }
    
    for(i = 0; i < offset; i++) SD_SPI_Transfer(0xFF);
    for(i = 0; i < len; i++) buf[i] = SD_SPI_Transfer(0xFF);
    for(i = offset + len; i < 512; i++) SD_SPI_Transfer(0xFF);
    
    SD_SPI_Transfer(0xFF);
    SD_SPI_Transfer(0xFF);
    SD_CS_HIGH();
    SD_SPI_Transfer(0xFF);
}

// Escritura SIMPLIFICADA Y DEPURADA para PIC16F887
void SD_WriteBytes(uint32_t sector, uint16_t offset, uint8_t *buf, uint8_t len){
    uint16_t i;
    uint8_t resp, data_byte;
    
    // Asegurar CS alto inicialmente
    SD_CS_HIGH();
    SD_SPI_Transfer(0xFF);
    __delay_ms(1);
    
    // Bajar CS para iniciar comunicación
    SD_CS_LOW();
    __delay_ms(1);
    
    // Enviar comando CMD24 (WRITE_SINGLE_BLOCK)
    resp = SD_Cmd(CMD24, sector);
    if(resp != 0x00){ 
        SD_CS_HIGH(); 
        SD_SPI_Transfer(0xFF);
        return; 
    }
    
    // Pequeña espera antes del token de datos
    __delay_ms(1);
    
    // Enviar Start Block Token (0xFE)
    SD_SPI_Transfer(0xFE);
    
    // Escribir exactamente 512 bytes
    for(i = 0; i < 512; i++){
        if(i >= offset && i < (offset + len)){
            // Leer byte del buffer y escribirlo
            data_byte = buf[i - offset];
            SD_SPI_Transfer(data_byte);
        } else {
            // Rellenar resto con 0xFF (visible en hex dump)
            SD_SPI_Transfer(0xFF);
        }
    }
    
    // Enviar CRC dummy (2 bytes, no se usa en modo SPI)
    SD_SPI_Transfer(0xFF);
    SD_SPI_Transfer(0xFF);
    
    // Leer Data Response Token
    resp = SD_SPI_Transfer(0xFF);
    
    // Validar: bits [4:0] deben ser 0x05 (Data Accepted)
    // Si es 0x0B (CRC Error) o 0x0D (Write Error), abortar
    if((resp & 0x1F) != 0x05){ 
        SD_CS_HIGH(); 
        SD_SPI_Transfer(0xFF);
        return; 
    }
    
    // Esperar mientras SD está ocupada (busy)
    // Cuando busy, MISO = 0x00. Cuando termina, MISO = 0xFF
    i = 0;
    while(i < 60000){
        resp = SD_SPI_Transfer(0xFF);
        if(resp != 0x00){
            break;  // SD terminó de escribir
        }
        i++;
    }
    
    // Liberar CS y dar tiempo a SD
    SD_CS_HIGH();
    SD_SPI_Transfer(0xFF);
    __delay_ms(20);  // Espera crítica para que SD complete escritura interna
}