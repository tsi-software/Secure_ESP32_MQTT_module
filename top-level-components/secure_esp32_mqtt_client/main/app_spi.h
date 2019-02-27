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
#include "driver/spi_master.h"

class AppSPI {
public:
    AppSPI();
    virtual ~AppSPI() { }

    void connect();

private:
    spi_device_handle_t           spi;
    spi_bus_config_t              buscfg;
    spi_device_interface_config_t devcfg;
};

#endif //__cplusplus
//-------------------


#ifdef __cplusplus
extern "C"
{
#endif

// c wrapper.
extern void app_spi_init(void);


#ifdef __cplusplus
}
#endif


#endif // _APP_SPI_H_
