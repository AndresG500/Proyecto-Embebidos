main:
// CONFIG1
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = ON
#pragma config CP = OFF
#pragma config CPD = OFF
#pragma config BOREN = OFF
#pragma config IESO = OFF
#pragma config FCMEN = OFF
#pragma config LVP = OFF

// CONFIG2
#pragma config BOR4V = BOR40V
#pragma config WRT = OFF

#define _XTAL_FREQ 20000000

#include <xc.h>
#include "LCD.h"
#include "DHT22.h"
#include "DS3231.h"
#include "SD_SPI.h"
#include "FAT32_Simple.h"

// Pin para botón de cambio de vista (RA2)
#define BUTTON PORTAbits.RA2

// Buffer CSV reducido (optimización RAM PIC16F887: 368 bytes)
static char csv[40];

// Estructura para estadísticas de temperatura
typedef struct {
    uint8_t count;
    float mean;
    float std;
    float min;
    float q25;
    float median;
    float q75;
    float max;
} TempStats;

// Buffer para almacenar valores de temperatura (últimas 6 muestras - optimizado)
#define TEMP_BUFFER_SIZE 6
static float temp_buffer[TEMP_BUFFER_SIZE];
static uint8_t temp_buffer_idx = 0;
static uint8_t temp_buffer_full = 0;
static TempStats temp_stats = {0, 0, 0, 0, 0, 0, 0, 0};

// Forward declarations
static void add_temp_to_buffer(float temp);
static void calculate_temp_stats(void);
static uint8_t read_button_press(void);
static void display_stats_screen(void);
static void display_quartiles_screen(void);
static void display_main_screen(DS3231_Time *t, float temp, float hum);
static void sort_temps(float *arr, uint8_t n);

static void p2(uint8_t v) {
    Lcd_Write_Char('0' + ((v / 10) & 0x0F));
    Lcd_Write_Char('0' + (v % 10));
}

// Mostrar float compacto
static void pf(float f) {
    uint16_t v = (uint16_t)(f * 10.0 + (f >= 0 ? 0.5 : -0.5));
    if(f < 0) { Lcd_Write_Char('-'); v = -v; }
    Lcd_Write_Char('0' + ((v / 100) % 10));
    Lcd_Write_Char('0' + ((v / 10) % 10));
    Lcd_Write_Char('.');
    Lcd_Write_Char('0' + (v % 10));
}

// Agregar temperatura al buffer circular
static void add_temp_to_buffer(float temp) {
    temp_buffer[temp_buffer_idx] = temp;
    temp_buffer_idx = (temp_buffer_idx + 1) % TEMP_BUFFER_SIZE;
    if(temp_buffer_idx == 0) {
        temp_buffer_full = 1;
    }
}

// Ordenar temperaturas (bubble sort simple)
static void sort_temps(float *arr, uint8_t n) {
    uint8_t i, j;
    float temp;
    for(i = 0; i < n - 1; i++) {
        for(j = 0; j < n - i - 1; j++) {
            if(arr[j] > arr[j+1]) {
                temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
}

// Calcular estadísticas (optimizado para ROM)
static void calculate_temp_stats(void) {
    uint8_t n = temp_buffer_full ? TEMP_BUFFER_SIZE : temp_buffer_idx;
    uint8_t i;
    float sum = 0;
    float sorted[TEMP_BUFFER_SIZE];
    
    if(n == 0) return;
    
    temp_stats.count = n;
    temp_stats.min = temp_buffer[0];
    temp_stats.max = temp_buffer[0];
    
    // Copiar para ordenar
    for(i = 0; i < n; i++) {
        sorted[i] = temp_buffer[i];
        sum += temp_buffer[i];
        if(temp_buffer[i] < temp_stats.min) temp_stats.min = temp_buffer[i];
        if(temp_buffer[i] > temp_stats.max) temp_stats.max = temp_buffer[i];
    }
    
    // Calcular media
    temp_stats.mean = sum / n;
    
    // Ordenar para calcular quartiles
    sort_temps(sorted, n);
    
    // Calcular quartiles
    temp_stats.q25 = sorted[n / 4];
    temp_stats.median = sorted[n / 2];
    temp_stats.q75 = sorted[(3 * n) / 4];
}

// Mostrar pantalla principal
static void display_main_screen(DS3231_Time *t, float temp, float hum) {
    Lcd_Set_Cursor(1, 1);
    p2(t->date); Lcd_Write_Char('/');
    p2(t->month); Lcd_Write_Char('/');
    p2(t->year); Lcd_Write_String(" ");
    p2(t->hours); Lcd_Write_Char(':');
    p2(t->minutes);
    
    Lcd_Set_Cursor(2, 1);
    Lcd_Write_String("T:");
    pf(temp);
    Lcd_Write_String("C H:");
    pf(hum);
    Lcd_Write_String("%   ");
}

// Mostrar estadísticas Min/Max/Media
static void display_stats_screen(void) {
    Lcd_Set_Cursor(1, 1);
    Lcd_Write_String("Min:");
    pf(temp_stats.min);
    Lcd_Write_String(" Max:");
    pf(temp_stats.max);
    
    Lcd_Set_Cursor(2, 1);
    Lcd_Write_String("Med:");
    pf(temp_stats.mean);
    Lcd_Write_String("C n=");
    Lcd_Write_Char('0' + temp_stats.count);
}

// Mostrar cuartiles
static void display_quartiles_screen(void) {
    Lcd_Set_Cursor(1, 1);
    Lcd_Write_String("Q1:");
    pf(temp_stats.q25);
    Lcd_Write_String(" Q3:");
    pf(temp_stats.q75);
    
    Lcd_Set_Cursor(2, 1);
    Lcd_Write_String("Q2:");
    pf(temp_stats.median);
    Lcd_Write_String("C      ");
}

// Leer estado del botón con debounce simple
static uint8_t read_button_press(void) {
    static uint8_t last_button = 1;
    uint8_t current_button = BUTTON;
    
    // Detectar flanco de bajada (0 = presionado)
    if(current_button == 0 && last_button == 1) {
        __delay_ms(30);  // Anti-rebote
        if(BUTTON == 0) {  // Confirmar
            last_button = 0;
            return 1;  // Botón presionado
        }
    }
    
    // Actualizar estado previo
    if(current_button == 1) {
        last_button = 1;
    }
    
    return 0;
}

void main(void) {
    DS3231_Time t;
    float temp = 0, hum = 0;
    uint8_t sd = 0, p, cnt = 0, dht_ok = 0;
    uint8_t view_mode = 0;  // 0: principal, 1: stats, 2: quartiles
    uint8_t view_changed = 0;
    uint8_t stats_displayed = 0;
    uint16_t h10;
    int16_t temp_int;
    static uint8_t last_min = 0xFF;
    
    ANSEL = 0x00;
    ANSELH = 0x00;
    
    // Configurar botón como entrada (RA2)
    TRISAbits.TRISA2 = 1;   // RA2 como entrada
    // NOTA: PORTA no tiene pull-ups internos, usar resistencia externa de 10k?
    
    Lcd_Init();
    Lcd_Clear();
    Lcd_Set_Cursor(1, 1);
    Lcd_Write_String("Init Sensors");
    
    DHT22_init();
    DS3231_Init();
    __delay_ms(100);
    
    Lcd_Set_Cursor(2, 1);
    Lcd_Write_String("Init SD...");
    __delay_ms(100);
    
    if(SD_Init()) {
        __delay_ms(50);
        if(FAT32_Init()) {
            sd = 1;
            Lcd_Set_Cursor(2, 1);
            Lcd_Write_String("SD OK!        ");
            __delay_ms(200);
            
            // Escribir encabezado CSV en sector 1000
            FAT32_AppendLine("DATA.TXT", "Fecha,Hora,Temp,Hum\r\n");
            __delay_ms(100);
        }
    }
    
    if(!sd) {
        Lcd_Set_Cursor(2, 1);
        Lcd_Write_String("SD Error!     ");
    }
    
    __delay_ms(500);
    Lcd_Clear();
    
    // Leer sensores iniciales
    DS3231_ReadTime(&t);
    dht_ok = DHT22_read(&temp, &hum);
    add_temp_to_buffer(temp);
    
    // FORZAR vista principal al inicio
    view_mode = 0;
    display_main_screen(&t, temp, hum);
    
    __delay_ms(1000);  // Mostrar pantalla principal 1 segundo
    
    while(1) {
        // Verificar botón PRIMERO
        if(read_button_press()) {
            view_mode = (view_mode + 1) % 3;  // Ciclar: 0->1->2->0
            Lcd_Clear();
            
            if(view_mode == 1) {
                // Pantalla 2: Estadísticas Min/Max/Media
                calculate_temp_stats();
                display_stats_screen();
            } else if(view_mode == 2) {
                // Pantalla 3: Cuartiles
                display_quartiles_screen();
            } else {
                // Pantalla 1: Principal
                display_main_screen(&t, temp, hum);
            }
            
            __delay_ms(200);  // Pequeña pausa
        }
        
        // Leer hora
        DS3231_ReadTime(&t);
        
        // Leer DHT22 cada 5 ciclos (2.5 segundos)
        if(cnt == 0) {
            dht_ok = DHT22_read(&temp, &hum);
            add_temp_to_buffer(temp);
        }
        
        // Actualizar SOLO pantalla principal (estadísticas es estática)
        if(view_mode == 0) {
            display_main_screen(&t, temp, hum);
        }
        
        cnt = (cnt + 1) % 5;
        
        // Guardar cada 5 minutos solo en vista principal
        if(view_mode == 0 && t.minutes % 5 == 0 && t.minutes != last_min) {
            last_min = t.minutes;
            
            // Validar que tengamos datos válidos
            if(sd && dht_ok && temp > -40.0 && temp < 80.0 && hum > 0.0 && hum < 100.0) {
                
                p = 0;
                
                // Construir string CSV optimizado
                // Formato: DD/MM/YY,HH:MM,TT.T,HH.H
                
                // Fecha: DD/MM/YY
                csv[p++] = '0' + (t.date / 10);
                csv[p++] = '0' + (t.date % 10);
                csv[p++] = '/';
                csv[p++] = '0' + (t.month / 10);
                csv[p++] = '0' + (t.month % 10);
                csv[p++] = '/';
                csv[p++] = '0' + (t.year / 10);
                csv[p++] = '0' + (t.year % 10);
                csv[p++] = ',';
                
                // Hora: HH:MM
                csv[p++] = '0' + (t.hours / 10);
                csv[p++] = '0' + (t.hours % 10);
                csv[p++] = ':';
                csv[p++] = '0' + (t.minutes / 10);
                csv[p++] = '0' + (t.minutes % 10);
                csv[p++] = ',';
                
                // Temperatura: ±TT.T (sin modificar valor original)
                temp_int = (int16_t)(temp * 10.0 + (temp >= 0 ? 0.5 : -0.5));
                
                if(temp_int < 0) {
                    csv[p++] = '-';
                    temp_int = -temp_int;
                }
                
                // Dígitos de temperatura
                csv[p++] = '0' + ((temp_int / 100) & 0x0F);
                csv[p++] = '0' + (((temp_int / 10) % 10) & 0x0F);
                csv[p++] = '.';
                csv[p++] = '0' + ((temp_int % 10) & 0x0F);
                csv[p++] = ',';
                
                // Humedad: HH.H
                h10 = (uint16_t)(hum * 10.0 + 0.5);
                csv[p++] = '0' + ((h10 / 100) % 10);
                csv[p++] = '0' + ((h10 / 10) % 10);
                csv[p++] = '.';
                csv[p++] = '0' + (h10 % 10);
                
                // Fin de línea
                csv[p++] = '\r';
                csv[p++] = '\n';
                csv[p] = '\0';
                
                // Guardar en SD con confirmación visual
                if(FAT32_AppendLine("DATA.TXT", csv)) {
                    // Mensaje breve de guardado exitoso
                    Lcd_Set_Cursor(2, 15);
                    Lcd_Write_Char('S');  // Indicador de "Saved"
                    __delay_ms(500);
                } else {
                    sd = 0;
                    Lcd_Set_Cursor(2, 1);
                    Lcd_Write_String("SD FAIL!      ");
                    __delay_ms(1000);
                }
            }
        }
        
        __delay_ms(500);
    }
}