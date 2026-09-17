#ifndef SPI_H
#define SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
typedef enum {
    SPI_OK = 0,
    SPI_ERR_INIT,
    SPI_ERR_TIMEOUT,
    SPI_ERR_PARAM
} spi_status_t;

spi_status_t spi_bus_init(void);

uint8_t spi_bus_transfer_byte(uint8_t tx_data);

spi_status_t spi_bus_transfer(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t length);

spi_status_t spi_bus_write(const uint8_t *tx_buf, uint16_t length);

spi_status_t spi_bus_read(uint8_t *rx_buf, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* SPI_H */
