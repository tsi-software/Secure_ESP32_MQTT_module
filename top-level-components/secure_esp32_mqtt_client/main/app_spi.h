/*  app_spi.h
    Created: 2019-02-25
    Author: Warren Taylor

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef _APP_SPI_H_
#define _APP_SPI_H_


//-------------------
#ifdef __cplusplus
#include <sstream>
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "soc/gpio_struct.h"
//#include "driver/gpio.h"
//#include "driver/spi_slave.h"


//------------------------------------------------------------------------------
// Very Light Weight memory pool.
class SPISlaveTransactionPool {
public:
    const unsigned poolSize;
    const unsigned transactionLength;

    // transactionLength MUST be divisible by 4!!!
    SPISlaveTransactionPool(unsigned poolSize, unsigned transactionLength) 
        : poolSize(poolSize)
        , transactionLength(transactionLength)
    {
        // TODO: assert that transactionLength is divisible by 4!

        poolItems = static_cast<PoolItem *>( malloc(sizeof(PoolItem) * poolSize) );
        configASSERT(poolItems);

        for (unsigned poolIndex = 0; poolIndex < poolSize; ++poolIndex) {
            spi_slave_transaction_t *spiSlaveTrans = static_cast<spi_slave_transaction_t *>( malloc(sizeof(spi_slave_transaction_t)) );
            configASSERT(spiSlaveTrans);
            spiSlaveTrans->length = transactionLength * 8; // in bits!
            spiSlaveTrans->trans_len = spiSlaveTrans->length;
            spiSlaveTrans->user = nullptr;

            spiSlaveTrans->tx_buffer = heap_caps_malloc(transactionLength, MALLOC_CAP_32BIT | MALLOC_CAP_DMA);
            configASSERT(spiSlaveTrans->tx_buffer);
            spiSlaveTrans->rx_buffer = heap_caps_malloc(transactionLength, MALLOC_CAP_32BIT | MALLOC_CAP_DMA);
            configASSERT(spiSlaveTrans->rx_buffer);

            poolItems[poolIndex].spiSlaveTransaction = spiSlaveTrans;
            poolItems[poolIndex].isInUse = false;
        }
    }

    virtual ~SPISlaveTransactionPool() {
        for (unsigned poolIndex = 0; poolIndex < poolSize; ++poolIndex) {
            spi_slave_transaction_t *spiSlaveTrans = poolItems[poolIndex].spiSlaveTransaction;
            free((void*)spiSlaveTrans->tx_buffer);
            free(spiSlaveTrans->rx_buffer);
            free(spiSlaveTrans);
        }
        free(poolItems);
        poolItems = nullptr;
    }

    // NOT TREAD SAFE!!!
    spi_slave_transaction_t * getFromPool() {
        for (unsigned poolIndex = 0; poolIndex < poolSize; ++poolIndex) {
            PoolItem &poolitem = poolItems[poolIndex];
            if (!poolitem.isInUse) {
                poolitem.isInUse = true;
                return poolitem.spiSlaveTransaction;
            }
        }
        // TODO: FAIL! Pool is empty!
        return nullptr;
    }

    // MIGHT BE TREAD SAFE???
    void returnToPool(spi_slave_transaction_t *spiSlaveTransaction) {
        if (!spiSlaveTransaction) {
            //TODO: properly log and handle this error condition.
            return;
        }

        for (unsigned poolIndex = 0; poolIndex < poolSize; ++poolIndex) {
            if (spiSlaveTransaction == poolItems[poolIndex].spiSlaveTransaction) {
                poolItems[poolIndex].isInUse = false;
                return;
            }
        }
        // TODO: FAIL! Throw an exception!
    }

private:
    struct PoolItem {
        spi_slave_transaction_t *spiSlaveTransaction;
        bool isInUse;
    };
    struct PoolItem *poolItems;
};


//------------------------------------------------------------------------------
class AppSPI {
public:
    // transactionLength MUST be divisible by 4!!!
    AppSPI(const unsigned queueSize = 4, const unsigned transactionLength = 32);
    virtual ~AppSPI() { }

    void connect();
    void taskStart();

    void setTaskHandle(TaskHandle_t taskHandle) {
        this->taskHandle = taskHandle;
    }

private:
    //TODO: see 'void AppSPI::connect()'
    //      implement busConfig and slaveConfig here if AppSPI::connect() is failing.
    //spi_bus_config_t              busConfig;
    //spi_slave_interface_config_t  slaveConfig;
    TaskHandle_t taskHandle = nullptr;
    SPISlaveTransactionPool transactionPool;
    //std::stringstream rxStream;
    std::string pendingRxBuffer;
    volatile int txPendingCount = 0;

    void task();
    void taskFirstTime();
    void processIncomingMqttMessages();
    void processMqttNode(const AppMQTTQueueNode &node);
    void queueString(const std::string &str);
    void processCompletedSpiTransaction();
    inline void atomicIncrementTxPendingCount(int incrementValue);
    void reassembleAndQueueRxMessage(const char *rxBuffer, const size_t bufferLength);

};

#endif //__cplusplus
//-------------------


#ifdef __cplusplus
extern "C"
{
#endif

// C wrapper.
extern esp_err_t app_spi_init(void);


#ifdef __cplusplus
}
#endif


#endif // _APP_SPI_H_
