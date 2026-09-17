#ifndef SPI_H
#define SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Status codes returned by SPI bus functions.
 */
typedef enum {
    SPI_OK = 0,         /*!< Operation completed successfully */
    SPI_ERR_INIT,       /*!< SPI peripheral or GPIO initialization failed */
    SPI_ERR_TIMEOUT,    /*!< Bus operation timed out waiting for flag */
    SPI_ERR_PARAM       /*!< Invalid parameter provided (e.g. NULL pointer with non-zero length) */
} spi_status_t;

/**
 * @brief  Initializes SPI1 master bus and corresponding GPIO pins (PA5 SCK, PA6 MISO, PA7 MOSI).
 * @return spi_status_t SPI_OK on success, or SPI_ERR_INIT on hardware failure.
 */
spi_status_t spi_bus_init(void);

/**
 * @brief  Transfers a single 8-bit byte over the SPI bus full duplex.
 * @param[in] tx_data Byte to transmit.
 * @return uint8_t Byte received from the slave device (or 0xFF on timeout).
 */
uint8_t spi_bus_transfer_byte(uint8_t tx_data);

/**
 * @brief  Simultaneously transmits and receives a buffer of data over SPI full duplex.
 * @param[in]  tx_buf Pointer to transmission buffer (if NULL, dummy bytes 0xFF are sent).
 * @param[out] rx_buf Pointer to receive destination buffer (if NULL, received bytes are discarded).
 * @param[in]  length Number of bytes to transfer.
 * @return spi_status_t SPI_OK on success, or SPI_ERR_TIMEOUT if the bus fails to become idle.
 */
spi_status_t spi_bus_transfer(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t length);

/**
 * @brief  Transmits a buffer of bytes over SPI, discarding any incoming data.
 * @param[in] tx_buf Pointer to source buffer containing data to send.
 * @param[in] length Number of bytes to transmit.
 * @return spi_status_t SPI_OK on success, SPI_ERR_PARAM if tx_buf is NULL, or SPI_ERR_TIMEOUT.
 */
spi_status_t spi_bus_write(const uint8_t *tx_buf, uint16_t length);

/**
 * @brief  Receives a buffer of bytes over SPI by transmitting dummy bytes (0xFF).
 * @param[out] rx_buf Pointer to destination buffer where received data will be stored.
 * @param[in]  length Number of bytes to read.
 * @return spi_status_t SPI_OK on success, SPI_ERR_PARAM if rx_buf is NULL, or SPI_ERR_TIMEOUT.
 */
spi_status_t spi_bus_read(uint8_t *rx_buf, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* SPI_H */
