/*  app_spi.cpp
    Created: 2019-02-25
    Author: Warren Taylor

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"

#include "app_queues.h"
#include "app_spi.h"


//static const char *LOG_TAG = "APP_SPI";


// https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html
// VSPI
static const int PIN_NUM_MISO = 19;
static const int PIN_NUM_MOSI = 23;
static const int PIN_NUM_CLK  = 18;
static const int PIN_NUM_CS   = 32; // ** GPIO5 is STRAPPING **
static const int PIN_NUM_ClientTxReq = 33; // INPUT - When HIGH the Client is requesting to send to the Master.

static AppSPI static_app_spi;


AppSPI::AppSPI()
    : buscfg {
        PIN_NUM_MISO, //miso_io_num
        PIN_NUM_MOSI, //mosi_io_num
        PIN_NUM_CLK,  //sclk_io_num
        -1,  //quadwp_io_num
        -1,  //quadhd_io_num
        0,   //max_transfer_sz, Defaults to 4094 if 0.
        SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO | SPICOMMON_BUSFLAG_MOSI, //flags
        0    //intr_flags - Interrupt flag for the bus to set the priority, and IRAM attribute
    }
    , devcfg {
        0, //command_bits
        0, //address_bits
        0, //dummy_bits
        0, //SPI mode 0
        128, //duty_cycle_pos - (128 = 50%/50% duty)
        0, //cs_ena_pretrans
        0, //cs_ena_posttrans
        8*1000*1000, //clock_speed_hz - Clock out at 8 MHz.
        0, //input_delay_ns - Leave at 0 unless you know you need a delay.
        PIN_NUM_CS, //spics_io_num - CS pin.
        0, //flags - Bitwise OR of SPI_DEVICE_* flags
        2, //queue_size - Able to queue this many transactions at a time.
        nullptr, //pre_cb
        nullptr, //post_cb
    }
{
}


void AppSPI::connect() {
    esp_err_t ret;

    //Initialize the SPI bus.
    ret = spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);

    //Attach to the SPI bus.
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
}


// c wrapper.
void app_spi_init(void) {
    static_app_spi.connect();
}
