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
#include "esp_log.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"

#include "app_queues.h"
#include "app_spi.h"


static const char *LOG_TAG = "APP_SPI";

// https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html
// VSPI
static const int PIN_NUM_MISO = 19;
static const int PIN_NUM_MOSI = 23;
static const int PIN_NUM_CLK  = 18;
static const int PIN_NUM_CS   = 32; // ** GPIO5 is STRAPPING **
static const int PIN_NUM_ClientTxReq = 33; // INPUT - When HIGH the Client is requesting to send to the Master.

static const char       *APP_SPI_TASK_NAME = "App SPI";
static const uint32_t    APP_SPI_STACK_DEPTH = 4000;
static const UBaseType_t APP_SPI_DEFAULT_TASK_PRIORITY = 5;

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


void AppSPI::task() {
    //
}


//-------------------------------------
// c wrappers.
//-------------------------------------

static void app_spi_task_callback( void * parameters ) {
    AppSPI *appSPI = static_cast<AppSPI *>(parameters);
    appSPI->task();
}


esp_err_t app_spi_init(void) {
    TaskHandle_t taskHandle = NULL;
    UBaseType_t priority = APP_SPI_DEFAULT_TASK_PRIORITY;
    esp_err_t err_code = ESP_OK;

    static_app_spi.connect();

    /*
    #if (configUSE_TRACE_FACILITY == 1)
    // Get the priority of the task currently running
    // so the this new task can have the same priority.
    TaskHandle_t currTask = xTaskGetCurrentTaskHandle();
    if (currTask) {
        TaskStatus_t taskDetails;
        vTaskGetInfo(currTask, &taskDetails, pdFALSE, eReady);
        priority = taskDetails.uxCurrentPriority;
    }
    #endif
    */
    ESP_LOGI(LOG_TAG, "app_spi_init(): App SPI task to run at priority %d!", static_cast<int>(priority));

    BaseType_t result = xTaskCreatePinnedToCore(
        app_spi_task_callback,
        APP_SPI_TASK_NAME,
        APP_SPI_STACK_DEPTH,
        &static_app_spi, //constpvParameters
        priority,        //uxPriority
        &taskHandle,     //constpvCreatedTask
        APP_CPU_NUM      //xCoreID
    );

    if(result != pdPASS) {
        err_code = ESP_ERR_NO_MEM;
        ESP_LOGE(LOG_TAG, "app_spi_init(): xTaskCreatePinnedToCore(...) failed!");
        ESP_ERROR_CHECK(err_code);
    }

    return err_code;
}
