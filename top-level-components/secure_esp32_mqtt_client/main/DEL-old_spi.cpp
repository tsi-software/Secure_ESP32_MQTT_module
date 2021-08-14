#ifdef IGNORE


old_spi.cpp
/*

*/


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
    // To optimize:
    // * allocated in DMA-capable memory using pvPortMallocCaps(size, MALLOC_CAP_DMA);
    // * 32-bit aligned (start from the boundary and have length of multiples of 4 bytes).
    // Note: Deprecated - pvPortMallocCaps(size, MALLOC_CAP_DMA), instead use heap_caps_malloc(...).
    ptr1 = static_cast<char *>(heap_caps_malloc(messageLength, MALLOC_CAP_32BIT | MALLOC_CAP_DMA));
    configASSERT(ptr1);
    memset( ptr1, 0, messageLength );
    memcpy( ptr1 + offset, node.getTopic().c_str(), node.getTopic().size() );
    offset += node.getTopic().size();
    ptr1[offset] = ',';
    offset++;
    memcpy( ptr1 + offset, node.getData().c_str(), node.getData().size() );
    spiTrans->tx_buffer = ptr1;

    offset = 0;
    ptr1 = static_cast<char *>(heap_caps_malloc(messageLength, MALLOC_CAP_32BIT | MALLOC_CAP_DMA));
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
        // The message could not be sent via SPI so release the resources now.
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
        processReceivedData(static_cast<char *>(spiTrans->rx_buffer), spiTrans->rxlength);
        releaseSpiTrans(spiTrans);
        --spiTransactionsPendingCount;
    }
}


void AppSPI::processReceivedData(const char *rxBuffer, size_t rxLength) {
    for (int ndx = 0; ndx < rxLength; ++ndx) {
        char rxChar = rxBuffer[ndx];
        if (rxChar) {
            // Append the give 'char' to rxStream.
            ESP_LOGI(LOG_TAG, "AppSPI::processReceivedData(...): put(%c)", rxChar);
            rxStream.put(rxChar);
        } else {
            // rxChar is '\0' - process rxStream and then clear it before starting over.
            ESP_LOGI(LOG_TAG, "AppSPI::processReceivedData(...): received Null.");
            std::string strBuffer{ rxStream.str() };
            if (!strBuffer.empty()) {
                processReceivedString(strBuffer);
            }
            rxStream.str(std::string());
            rxStream.clear();
        }
    }
}


void AppSPI::processReceivedString(const std::string &strBuffer) {
    ESP_LOGI(LOG_TAG, "AppSPI::processReceivedString(...):\n%s", strBuffer.c_str());
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


#endif // IGNORE
