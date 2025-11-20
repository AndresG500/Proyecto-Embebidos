#ifndef DHT22_H
#define DHT22_H

#define _XTAL_FREQ 20000000  // O la frecuencia que uses

#include <xc.h>
#include <stdint.h>

#define TRIS_DHT TRISCbits.TRISC2
#define PORT_DHT PORTCbits.RC2
// ELIMINAR: #define LAT_DHT (no existe en PIC16F887)

void DHT22_init(void);
uint8_t DHT22_read(float *dht_temperatura, float *dht_humedad);

#endif