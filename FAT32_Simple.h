#ifndef FAT32_SIMPLE_H
#define FAT32_SIMPLE_H

#include <stdint.h>
#include "SD_SPI.h"

// Info mínima de FAT32
typedef struct {
    uint32_t cluster_begin_lba;
    uint32_t root_cluster;
} FAT32_Info;

extern FAT32_Info fat32;

uint8_t FAT32_Init(void);
uint8_t FAT32_AppendLine(const char *filename, const char *line);

#endif