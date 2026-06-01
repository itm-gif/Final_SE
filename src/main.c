#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include "driver/uart.h"
#include "driver/timer.h"

#define PIN_MOSI 23
#define PIN_CLK 18
#define PIN_CS 5

#define TXD_PIN 1
#define RXD_PIN 3

static spi_device_handle_t spi_handle;

//Punto 3
//a)
void spi_bus_init() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1, // No se utiliza MISO, solo escritura
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };
    spi_bus_initialize(HSPI_HOST, &buscfg, 0);
}

//b)
void mpc4132_write_register(uint8_t adress, uint8_t value) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); 
    t.length = 16; 
    t.tx_buffer = (uint8_t[]){adress, value}; // Dirección seguida del valor
    spi_device_transmit(spi_handle, &t); // Transmitir la transacción
}

//c)
void mpc4132_read_register(uint8_t adress, uint8_t *value) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); 
    t.length = 16; 
    t.tx_buffer = (uint8_t[]){adress | 0x0C}; //dirección con bit de lectura 
    t.rx_buffer = value; 
    spi_device_transmit(spi_handle, &t); // Retorno
}

//Punto 4
//a)
void mpc4132_set_wiper(uint8_t value) {
    if (value > 128) {
        value = 128; 
    } else if (value < 0) {
        value = 0; 
    }
    mpc4132_write_register(0x00, value); // wiper 0
}

//b)
void mpc4132_set_cutoff_frecuency (float frequency) {
    float Rw = 10000; 
    float C = 1e-6; 
    float Rwb = 1 / (2 * M_PI * frequency * C);
    uint8_t N = (uint8_t)(Rwb / Rw);  
    mpc4132_write_register(0x01, N); 
}

//Punto 5
void app_main(){
    uart_config_t uart_config = {
        .baud_rate = 112500,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    timer_config_t timer_config = {
        .divider = 80, // Prescaler para 1us por tick
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_config);

    int señal_geofono;
    while (1) {
        timer_start(TIMER_GROUP_0, TIMER_0); 
        while (timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &timer_config) < 1000);
        timer_pause(TIMER_GROUP_0, TIMER_0); 
        señal_geofono = rand() % 1024; 

        char buffer[20];
        sprintf(buffer, "Señal: %d\n", señal_geofono);
        uart_write_bytes(UART_NUM_0, buffer, strlen(buffer));

        if (señal_geofono > 1.4) {
            mpc4132_set_wiper(95); 
        } else if (señal_geofono < 0.9) {
            mpc4132_set_wiper(42); 
        }
        
    }



}
