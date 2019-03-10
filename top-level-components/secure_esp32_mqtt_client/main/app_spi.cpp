/*  app_spi.cpp
    Created: 2019-02-25
    Author: Warren Taylor

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
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


//-------------------------------------
// Local Declarations.
//-------------------------------------
static void releaseSpiTrans(spi_transaction_t *spiTrans);


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
        1*1000*1000, //clock_speed_hz - Clock out at 1 MHz. //TODO: try 4mhz and 8mhz
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
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spiHandle);
    ESP_ERROR_CHECK(ret);
}


void AppSPI::task() {
    spiTransactionsPendingCount = 0;

    while(1) {
        ESP_LOGI(LOG_TAG, "AppSPI::task() - loop.");

        // Process data that was received from MQTT subscriptions.
        AppMQTTQueueNode *node = nullptr;
        TickType_t queueReceiveDelay = 1;

        if (spiTransactionsPendingCount == 0) {
            // All SPI Transactions have completed and their resources released.
            // So wait for any new incoming MQTT messages to process.
            queueReceiveDelay = portMAX_DELAY;
        }

        BaseType_t result = xQueueReceive(
            mqttReceivedQueue,
            (void *)&node,
            queueReceiveDelay
        );
        ESP_LOGV(LOG_TAG, "AppSPI::task() - xQueueReceive(...) node is %s, result=%d",
            node ? "NOT NULL" : "NULL", result
        );

        if (node && result == pdTRUE) {
            processMqttNode(*node);
        }
        delete node;
        node = nullptr;

        processCompletedSpiTransactions();
    }

    // This should never be reached, but just incase...
    if(taskHandle) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}


void AppSPI::processMqttNode(AppMQTTQueueNode &node) {
    ESP_LOGI(LOG_TAG, "AppSPI::processMqttNode()\ntopic:%s\ndata:%s",
             node.getTopic().c_str(), node.getData().c_str()
    );

    // Add 2 for the comma and null terminator.
    size_t messageLength = node.getTopic().size() + node.getData().size() + 2;
    // Make sure messageLength is divisable by 4 (4 bytes = 32 bits) to ensure DMA efficiency.
    size_t tmpLength = messageLength & ~3; // chop off the last 2 bits (i.e. divisible by 4).
    if (messageLength == tmpLength) {
        messageLength = tmpLength;
    } else {
        messageLength = tmpLength + 4; // round up to the next "divisible by 4".
    }

    spi_transaction_t *spiTrans = static_cast<spi_transaction_t *>( malloc(sizeof(spi_transaction_t)) );
    configASSERT(spiTrans);
    memset( static_cast<void *>(spiTrans), 0, sizeof(spi_transaction_t) );
    spiTrans->length = messageLength * 8; // in bits!
    //spiTrans->rxlength = 0; // (0 defaults this to the value of 'length').
    //spiTrans->user = nullptr;

    //TODO: the following would be more easily implemented with a std::stringstream.

    char *ptr1;
    size_t offset;

    offset = 0;
    ptr1 = static_cast<char *>(heap_caps_malloc(messageLength, MALLOC_CAP_DMA));
    configASSERT(ptr1);
    memset( ptr1, 0, messageLength );
    memcpy( ptr1 + offset, node.getTopic().c_str(), node.getTopic().size() );
    offset += node.getTopic().size();
    ptr1[offset] = ',';
    offset++;
    memcpy( ptr1 + offset, node.getData().c_str(), node.getData().size() );
    spiTrans->tx_buffer = ptr1;

    offset = 0;
    ptr1 = static_cast<char *>(heap_caps_malloc(messageLength, MALLOC_CAP_DMA));
    configASSERT(ptr1);
    memset(ptr1, 0, messageLength);
    spiTrans->rx_buffer = ptr1;

    esp_err_t err_code = spi_device_queue_trans(spiHandle, spiTrans, 1);
    //esp_err_t spi_device_queue_trans(spi_device_handle_t handle, spi_transaction_t *trans_desc, TickType_t ticks_to_wait)
    // ESP_ERR_INVALID_ARG if parameter is invalid
    // ESP_ERR_TIMEOUT if there was no room in the queue before ticks_to_wait expired
    // ESP_ERR_NO_MEM if allocating DMA-capable temporary buffer failed
    // ESP_ERR_INVALID_STATE if previous transactions are not finished
    // ESP_OK on success
    if (err_code == ESP_OK) {
        // spiTrans will be released in 'processCompletedSpiTransactions()'.
        ++spiTransactionsPendingCount;
        ESP_LOGI(LOG_TAG, "AppSPI::processMqttNode(...) - spi_device_queue_trans(...) success.");
        ESP_LOGI(LOG_TAG, "tx-length:%d, msg:%s", spiTrans->length/8, (char *)spiTrans->tx_buffer);
    } else {
        // The message could not be sent so release the resources now.
        releaseSpiTrans(spiTrans);
        ESP_LOGE(LOG_TAG, "AppSPI::processMqttNode(...) - spi_device_queue_trans(...) ERROR %d.", err_code);
    }
}


void AppSPI::processCompletedSpiTransactions() {
    spi_transaction_t *spiTrans = nullptr;
    esp_err_t err_code = spi_device_get_trans_result(spiHandle, &spiTrans, 1);
    //esp_err_t spi_device_get_trans_result(spi_device_handle_thandle, spi_transaction_t **trans_desc, TickType_t ticks_to_wait)
    // ESP_ERR_INVALID_ARG if parameter is invalid
    // ESP_ERR_TIMEOUT if there was no completed transaction before ticks_to_wait expired
    // ESP_OK on success
    if (err_code == ESP_OK && spiTrans) {
        releaseSpiTrans(spiTrans);
        --spiTransactionsPendingCount;
    }

    /***
    if (err_code == ESP_OK && spiTrans) {
        if (spiTrans->tx_buffer && !(spiTrans->flags & SPI_TRANS_USE_TXDATA)) {
            heap_caps_free( (void *)spiTrans->tx_buffer );
        }
        if (spiTrans->rx_buffer && !(spiTrans->flags & SPI_TRANS_USE_RXDATA)) {
            heap_caps_free(spiTrans->rx_buffer);
        }
        free(spiTrans);
        --spiTransactionsPendingCount;
    }
    ***/
}


//-------------------------------------
// Local Functions.
//-------------------------------------

static void releaseSpiTrans(spi_transaction_t *spiTrans) {
    if (spiTrans) {
        if (spiTrans->tx_buffer && !(spiTrans->flags & SPI_TRANS_USE_TXDATA)) {
            heap_caps_free( (void *)spiTrans->tx_buffer );
            spiTrans->tx_buffer = nullptr;
        }
        if (spiTrans->rx_buffer && !(spiTrans->flags & SPI_TRANS_USE_RXDATA)) {
            heap_caps_free(spiTrans->rx_buffer);
            spiTrans->rx_buffer = nullptr;
        }
        free(spiTrans);
    }
}


//******************************************************************************
#ifdef IGNORE_THIS //never defined!
/* This structure describes one SPI transaction. The descriptor should not be modified until the transaction finishes. */
struct spi_transaction_t {
    uint32_t flags;                 ///< Bitwise OR of SPI_TRANS_* flags
    uint16_t cmd;                   /**< Command data, of which the length is set in the ``command_bits`` of spi_device_interface_config_t.
                                      *
                                      *  <b>NOTE: this field, used to be "command" in ESP-IDF 2.1 and before, is re-written to be used in a new way in ESP-IDF 3.0.</b>
                                      *
                                      *  Example: write 0x0123 and command_bits=12 to send command 0x12, 0x3_ (in previous version, you may have to write 0x3_12).
                                      */
    uint64_t addr;                  /**< Address data, of which the length is set in the ``address_bits`` of spi_device_interface_config_t.
                                      *
                                      *  <b>NOTE: this field, used to be "address" in ESP-IDF 2.1 and before, is re-written to be used in a new way in ESP-IDF3.0.</b>
                                      *
                                      *  Example: write 0x123400 and address_bits=24 to send address of 0x12, 0x34, 0x00 (in previous version, you may have to write 0x12340000).
                                      */
    size_t length;                  ///< Total data length, in bits
    size_t rxlength;                ///< Total data length received, should be not greater than ``length`` in full-duplex mode (0 defaults this to the value of ``length``).
    void *user;                     ///< User-defined variable. Can be used to store eg transaction ID.
    union {
        const void *tx_buffer;      ///< Pointer to transmit buffer, or NULL for no MOSI phase
        uint8_t tx_data[4];         ///< If SPI_USE_TXDATA is set, data set here is sent directly from this variable.
    };
    union {
        void *rx_buffer;            ///< Pointer to receive buffer, or NULL for no MISO phase. Written by 4 bytes-unit if DMA is used.
        uint8_t rx_data[4];         ///< If SPI_USE_RXDATA is set, data is received directly to this variable
    };
} ;        //the rx data should start from a 32-bit aligned address to get around dma issue.
#endif // IGNORE_THIS
//******************************************************************************



//-------------------------------------
// C wrappers.
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

    if(result == pdPASS) {
        static_app_spi.setTaskHandle(taskHandle);
    } else {
        err_code = ESP_ERR_NO_MEM;
        ESP_LOGE(LOG_TAG, "app_spi_init(): xTaskCreatePinnedToCore(...) failed!");
        ESP_ERROR_CHECK(err_code);
    }

    return err_code;
}
