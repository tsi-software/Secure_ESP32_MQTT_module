/*  app_spi.cpp
    Created: 2019-02-25
    Author: Warren Taylor

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

#include <cstring>
#include <string>
//#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "driver/spi_slave.h"

#include "app_queues.h"
#include "app_spi.h"


static const char *LOG_TAG = "APP_SPI";

// https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html
// VSPI

static const gpio_num_t PIN_NUM_MISO = GPIO_NUM_19;
static const gpio_num_t PIN_NUM_MOSI = GPIO_NUM_23;
static const gpio_num_t PIN_NUM_CLK  = GPIO_NUM_18;
static const gpio_num_t PIN_NUM_CS   = GPIO_NUM_32; // ** GPIO5 is STRAPPING **

static const gpio_num_t PIN_NUM_HANDSHAKE = GPIO_NUM_33; // OUTPUT - Set to HIGH when requesting to send to the Master.
#define GPIO_SEL_HANDSHAKE_PIN GPIO_SEL_33

static const char       *APP_SPI_TASK_NAME = "App SPI";
static const uint32_t    APP_SPI_STACK_DEPTH = 4000;
static const UBaseType_t APP_SPI_DEFAULT_TASK_PRIORITY = 5;

static AppSPI static_app_spi;


//-------------------------------------
// Local Declarations.
//-------------------------------------
//static void releaseSpiTrans(spi_transaction_t *spiTrans);


//-------------------------------------
//
//-------------------------------------
// Called after a transaction is queued and ready for pickup by master. We use this to set the handshake line high.
static void slave_transaction_post_setup_callback(spi_slave_transaction_t *trans) {
    //WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (1<<GPIO_HANDSHAKE));
}

// Called after transaction is sent/received. We use this to set the handshake line low.
static void slave_transaction_post_trans_callback(spi_slave_transaction_t *trans) {
    //WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (1<<GPIO_HANDSHAKE));
}


/***
// transactionLength MUST be divisible by 4!!!
AppSPI::AppSPI(const unsigned queueSize, const unsigned transactionLength)
    : transactionPool { queueSize, transactionLength }
    , busConfig {
        PIN_NUM_MOSI, //mosi_io_num
        PIN_NUM_MISO, //miso_io_num
        PIN_NUM_CLK,  //sclk_io_num
        -1,  //quadwp_io_num
        -1,  //quadhd_io_num
        0,   //max_transfer_sz, Defaults to 4094 if 0.
        SPICOMMON_BUSFLAG_SLAVE , //flags
        0    //intr_flags - Interrupt flag for the bus to set the priority, and IRAM attribute
    }
    , slaveConfig {
        PIN_NUM_CS, //spics_io_num
        0, //flags -- Bitwise OR of SPI_SLAVE_* flags
        static_cast<int>(queueSize), //queue_size
        0, //mode -- SPI mode (0-3)
        slave_transaction_post_setup_callback, //slave_transaction_cb_t post_setup_cb
        slave_transaction_post_trans_callback  //slave_transaction_cb_t post_trans_cb
    }
{
}
***/

    /***
    AppSPI(unsigned queueSize = 4, unsigned transactionLength = 32)
        : transactionPool ( queueSize, transactionLength )
        , busConfig {
            static_cast<int>(PIN_NUM_MOSI), //mosi_io_num
            static_cast<int>(PIN_NUM_MISO), //miso_io_num
            static_cast<int>(PIN_NUM_CLK),  //sclk_io_num
            -1,  //quadwp_io_num
            -1,  //quadhd_io_num
            0,   //max_transfer_sz, Defaults to 4094 if 0.
            static_cast<uint32_t>(SPICOMMON_BUSFLAG_SLAVE), //flags
            0    //intr_flags - Interrupt flag for the bus to set the priority, and IRAM attribute
        }
        , slaveConfig {
            static_cast<int>(PIN_NUM_CS), //spics_io_num
            0, //flags -- Bitwise OR of SPI_SLAVE_* flags
            static_cast<int>(queueSize), //queue_size
            0, //mode -- SPI mode (0-3)
            slave_transaction_post_setup_callback, //slave_transaction_cb_t post_setup_cb
            slave_transaction_post_trans_callback  //slave_transaction_cb_t post_trans_cb
        }
    {
    }
    ***/



AppSPI::AppSPI(const unsigned queueSize, const unsigned transactionLength)
              : transactionPool(queueSize, transactionLength)
{

}


#ifdef IGNORE_THIS //never defined (i.e. commented out)!
// This is a configuration structure for a SPI bus.
typedef struct {
    int mosi_io_num;                ///< GPIO pin for Master Out Slave In (=spi_d) signal, or -1 if not used.
    int miso_io_num;                ///< GPIO pin for Master In Slave Out (=spi_q) signal, or -1 if not used.
    int sclk_io_num;                ///< GPIO pin for Spi CLocK signal, or -1 if not used.
    int quadwp_io_num;              ///< GPIO pin for WP (Write Protect) signal which is used as D2 in 4-bit communication modes, or -1 if not used.
    int quadhd_io_num;              ///< GPIO pin for HD (HolD) signal which is used as D3 in 4-bit communication modes, or -1 if not used.
    int max_transfer_sz;            ///< Maximum transfer size, in bytes. Defaults to 4094 if 0.
    uint32_t flags;                 ///< Abilities of bus to be checked by the driver. Or-ed value of ``SPICOMMON_BUSFLAG_*`` flags.
    int intr_flags;    /**< Interrupt flag for the bus to set the priority, and IRAM attribute, see
                         *  ``esp_intr_alloc.h``. Note that the EDGE, INTRDISABLED attribute are ignored
                         *  by the driver. Note that if ESP_INTR_FLAG_IRAM is set, ALL the callbacks of
                         *  the driver, and their callee functions, should be put in the IRAM.
                         */
} spi_bus_config_t;

//This is a configuration for a SPI host acting as a slave device.
typedef struct {
    int spics_io_num;               ///< CS GPIO pin for this device
    uint32_t flags;                 ///< Bitwise OR of SPI_SLAVE_* flags
    int queue_size;                 ///< Transaction queue size. This sets how many transactions can be 'in the air' (queued using spi_slave_queue_trans but not yet finished using spi_slave_get_trans_result) at the same time
    uint8_t mode;                   ///< SPI mode (0-3)
    slave_transaction_cb_t post_setup_cb;  /**< Callback called after the SPI registers are loaded with new data.
                                             *
                                             *  This callback is called within interrupt
                                             *  context should be in IRAM for best
                                             *  performance, see "Transferring Speed"
                                             *  section in the SPI Master documentation for
                                             *  full details. If not, the callback may crash
                                             *  during flash operation when the driver is
                                             *  initialized with ESP_INTR_FLAG_IRAM.
                                             */
    slave_transaction_cb_t post_trans_cb;  /**< Callback called after a transaction is done.
                                             *
                                             *  This callback is called within interrupt
                                             *  context should be in IRAM for best
                                             *  performance, see "Transferring Speed"
                                             *  section in the SPI Master documentation for
                                             *  full details. If not, the callback may crash
                                             *  during flash operation when the driver is
                                             *  initialized with ESP_INTR_FLAG_IRAM.
                                             */
} spi_slave_interface_config_t;

//Configuration parameters of GPIO pad for gpio_config function
typedef struct {
    uint64_t pin_bit_mask;          /*!< GPIO pin: set with bit mask, each bit maps to a GPIO */
    gpio_mode_t mode;               /*!< GPIO mode: set input/output mode                     */
    gpio_pullup_t pull_up_en;       /*!< GPIO pull-up                                         */
    gpio_pulldown_t pull_down_en;   /*!< GPIO pull-down                                       */
    gpio_int_type_t intr_type;      /*!< GPIO interrupt type                                  */
} gpio_config_t;
#endif // IGNORE_THIS


void AppSPI::connect() {
    esp_err_t ret;

    //Configure handshake line as output
    gpio_config_t gpioConfig {
        GPIO_SEL_HANDSHAKE_PIN, //uint64_t pin_bit_mask
        GPIO_MODE_OUTPUT,       //gpio_mode_t mode
        GPIO_PULLUP_DISABLE,    //gpio_pullup_t pull_up_en
        GPIO_PULLDOWN_DISABLE,  //gpio_pulldown_t pull_down_en
        GPIO_INTR_DISABLE       //gpio_int_type_t intr_type
    };
    gpio_config(&gpioConfig);

    //Enable pull-ups on SPI lines so we don't detect rogue pulses when no master is connected.
    gpio_set_pull_mode(PIN_NUM_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_CLK,  GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_CS,   GPIO_PULLUP_ONLY);

    spi_bus_config_t busConfig = {
        PIN_NUM_MOSI, //mosi_io_num
        PIN_NUM_MISO, //miso_io_num
        PIN_NUM_CLK,  //sclk_io_num
        -1,  //quadwp_io_num
        -1,  //quadhd_io_num
        0,   //max_transfer_sz, Defaults to 4094 if 0.
        SPICOMMON_BUSFLAG_SLAVE , //flags
        0    //intr_flags - Interrupt flag for the bus to set the priority, and IRAM attribute
    };

    spi_slave_interface_config_t slaveConfig = {
        PIN_NUM_CS, //spics_io_num
        0, //flags -- Bitwise OR of SPI_SLAVE_* flags
        static_cast<int>(transactionPool.poolSize), //queue_size
        0, //mode -- SPI mode (0-3)
        slave_transaction_post_setup_callback, //slave_transaction_cb_t post_setup_cb
        slave_transaction_post_trans_callback  //slave_transaction_cb_t post_trans_cb
    };

    //Initialize SPI slave interface
    ret = spi_slave_initialize(VSPI_HOST, &busConfig, &slaveConfig, 1);
    ESP_ERROR_CHECK(ret);
}


void AppSPI::taskStart() {
    taskFirstTime();
    task();
}


void AppSPI::taskFirstTime() {
    AppMQTTQueueNode pingNode{ "ping", 4, "ready", 5 };
    processMqttNode(pingNode);
}


void AppSPI::task() {
    txPendingCount = 0;

    while(1) {
        ESP_LOGI(LOG_TAG, "AppSPI::task() - loop.");

        processIncomingMqttMessages();

        processCompletedSpiTransaction();
    }//while(1)

    // This should never be reached, but just incase...
    if(taskHandle) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}


void AppSPI::processIncomingMqttMessages() {
    // Process messages that was received from MQTT subscriptions.
    AppMQTTQueueNode *node = nullptr;
    TickType_t queueReceiveDelay = 1;

    // TODO: uncomment the following if/when appropriate...
    //if (txPendingCount == 0) {
    //    // All SPI Transactions have completed and their resources released.
    //    // So wait for any new incoming MQTT messages to process.
    //    queueReceiveDelay = portMAX_DELAY;
    //}

    BaseType_t result = xQueueReceive(
        mqttReceivedQueue,
        (void *)&node,
        queueReceiveDelay
    );
    ESP_LOGV(LOG_TAG,
        "AppSPI::processIncomingMqttMessages() - xQueueReceive(...) node is %s, result=%d",
        node ? "NOT NULL" : "NULL", result
    );

    if (node && result == pdTRUE) {
        processMqttNode(*node);
    }
    delete node;
    node = nullptr;
}


void AppSPI::processMqttNode(const AppMQTTQueueNode &node) {
    ESP_LOGD(LOG_TAG,
        "AppSPI::processMqttNode()\ntopic:%s\ndata:%s",
        node.getTopic().c_str(), node.getData().c_str()
    );

    //std::stringstream sstr << node.getTopic() << ',' << node.getData();
    std::string str;
    str.reserve( node.getTopic().size() + node.getData().size() + 1 );
    str = node.getTopic() + ',' + node.getData();
    queueString(str);
}


void AppSPI::queueString(const std::string &str) {
    esp_err_t err_code;
    spi_slave_transaction_t *slaveTrans;
    TickType_t ticks_to_wait = 1;
    const char *sendPtr = str.c_str();
    size_t sendLength = str.size() + 1; // Add 1 to force inclusion of the string null terminator.

    for (size_t sendIndex = 0;
         sendIndex < sendLength;
         sendIndex += transactionPool.transactionLength)
    {
        //
        slaveTrans = transactionPool.getFromPool();
        configASSERT(slaveTrans);

        slaveTrans->length = transactionPool.transactionLength;
        slaveTrans->trans_len = transactionPool.transactionLength;
        std::memset((void*)slaveTrans->tx_buffer, 0, transactionPool.transactionLength);
        std::memset(slaveTrans->rx_buffer, 0, transactionPool.transactionLength);
        slaveTrans->user = (void *)this;

        size_t copyNum = sendLength - sendIndex;
        if (copyNum > transactionPool.transactionLength) {
            copyNum = transactionPool.transactionLength;
        }
        std::memcpy((void*)slaveTrans->tx_buffer, (sendPtr + sendIndex), copyNum);

        err_code = spi_slave_queue_trans(VSPI_HOST, slaveTrans, ticks_to_wait);
        if (err_code == ESP_OK) {
            atomicIncrementTxPendingCount(+1);
        } else {
            transactionPool.returnToPool(slaveTrans);
            ESP_ERROR_CHECK(err_code);
        }
    }
}


void AppSPI::processCompletedSpiTransaction() {
    spi_slave_transaction_t *slaveTrans = nullptr;
    TickType_t ticks_to_wait = 1;
    esp_err_t err_code = spi_slave_get_trans_result(VSPI_HOST, &slaveTrans, ticks_to_wait);
    //esp_err_t spi_slave_get_trans_result(spi_host_device_t host, spi_slave_transaction_t **trans_desc, TickType_t ticks_to_wait)
    //ESP_ERR_INVALID_ARG if parameter is invalid
    //ESP_ERR_TIMEOUT if there was no completed transaction before ticks_to_wait expired
    //ESP_OK on success
    if (err_code == ESP_OK && slaveTrans) {

        //TODO: process slaveTrans->rx_buffer
        //      i.e. re-assemble and queue up MQTT commands.

        transactionPool.returnToPool(slaveTrans);
        atomicIncrementTxPendingCount(-1);
    }
}


void AppSPI::atomicIncrementTxPendingCount(int incrementValue) {
    //TODO: Take Mutex.
    txPendingCount += incrementValue;
    //TODO: make this a lambda
    gpio_set_level(PIN_NUM_HANDSHAKE, txPendingCount > 0 ? 1 : 0);
    //TODO: Give Mutex.
}


//-------------------------------------
// Local Functions.
//-------------------------------------


//******************************************************************************
#ifdef IGNORE_THIS //never defined!
//This structure describes one SPI transaction
struct spi_slave_transaction_t {
    size_t length;                  ///< Total data length, in bits
    size_t trans_len;               ///< Transaction data length, in bits
    const void *tx_buffer;          ///< Pointer to transmit buffer, or NULL for no MOSI phase
    void *rx_buffer;                ///< Pointer to receive buffer, or NULL for no MISO phase
    void *user;                     ///< User-defined variable. Can be used to store eg transaction ID.
};
#endif // IGNORE_THIS
//******************************************************************************


//-------------------------------------
// C wrappers.
//-------------------------------------

static void app_spi_task_callback( void * parameters ) {
    AppSPI *appSPI = static_cast<AppSPI *>(parameters);
    appSPI->taskStart();
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
