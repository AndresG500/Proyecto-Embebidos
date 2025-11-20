#include "FAT32_Simple.h"

FAT32_Info fat32;
static uint32_t data_sector = 1000;

// Inicialización mínima
uint8_t FAT32_Init(void){
    data_sector = 1000;  // Iniciar desde sector 1000
    return 1;
}

// Guardar línea ASCII en SD (optimizado para PIC16F887)
uint8_t FAT32_AppendLine(const char *filename, const char *line){
    uint8_t len = 0;
    
    // Contar longitud del string (máximo 50 caracteres para CSV)
    while(line[len] && len < 50) len++;
    
    // Escribir directamente en el sector (sin buffer adicional)
    SD_WriteBytes(data_sector, 0, (uint8_t*)line, len);
    
    // Incrementar sector para la siguiente escritura
    data_sector++;
    
    return 1;
}

