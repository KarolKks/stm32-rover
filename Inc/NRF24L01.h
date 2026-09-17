#ifndef NRF24L01_H
#define NRF24L01_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Status codes returned by NRF24L01 functions.
 */
typedef enum {
    NRF24_OK = 0,
    NRF24_ERR_NOT_FOUND,
    NRF24_ERR_PARAM
} nrf24_status_t;

/**
 * @brief Output power levels for NRF24L01 (used during auto-ACK response).
 */
typedef enum {
    NRF24_PA_MIN = 0,   /*!< -18 dBm */
    NRF24_PA_LOW,       /*!< -12 dBm */
    NRF24_PA_HIGH,      /*!<  -6 dBm */
    NRF24_PA_MAX        /*!<   0 dBm */
} nrf24_pa_level_t;

/**
 * @brief Air data rates for NRF24L01.
 */
typedef enum {
    NRF24_RATE_1MBPS = 0,
    NRF24_RATE_2MBPS,
    NRF24_RATE_250KBPS
} nrf24_data_rate_t;

/**
 * @brief Structure representing the control packet sent to the rover.
 */
typedef struct __attribute__((packed)) {
    int8_t   steering;  /*!< Steering command: -100 (full left) to +100 (full right) */
    int8_t   throttle;  /*!< Throttle command: -100 (full reverse) to +100 (full forward) */
    uint8_t  button;    /*!< Stick button state: 1 if pressed, 0 if released */
    uint8_t  sequence;  /*!< Incremental packet counter for diagnostics */
} rover_packet_t;

/**
 * @brief  Initializes NRF24L01+ hardware pins and configures the radio directly in receiver (RX) mode on pipe 0.
 * @return nrf24_status_t NRF24_OK on success, or NRF24_ERR_NOT_FOUND if radio absent.
 */
nrf24_status_t nrf24_init_rx(void);

/**
 * @brief  Verifies if NRF24L01+ chip is responsive over SPI.
 * @return bool true if chip responds correctly, false otherwise.
 */
bool nrf24_is_connected(void);

/**
 * @brief  Sets the 5-byte receive address for RX pipe 0.
 * @param  addr Pointer to 5-byte array.
 */
void nrf24_set_rx_address(const uint8_t *addr);

/**
 * @brief  Sets the RF communication channel (0 to 125). Frequency = 2400 + channel MHz.
 * @param  channel Channel number.
 */
void nrf24_set_channel(uint8_t channel);

/**
 * @brief  Sets RF output power level (for auto-ACK responses).
 * @param  level Power level from nrf24_pa_level_t.
 */
void nrf24_set_pa_level(nrf24_pa_level_t level);

/**
 * @brief  Sets wireless air data transmission rate.
 * @param  rate Data rate from nrf24_data_rate_t.
 */
void nrf24_set_data_rate(nrf24_data_rate_t rate);

/**
 * @brief  Checks whether a payload is waiting in the RX FIFO.
 * @return bool true when a payload is available, false otherwise.
 */
bool nrf24_is_data_available(void);

/**
 * @brief  Reads one payload from the RX FIFO and clears RX_DR.
 * @param  data Pointer to the destination buffer.
 * @param  length Number of bytes to read (maximum 32).
 * @return nrf24_status_t NRF24_OK on success, or NRF24_ERR_PARAM if arguments are invalid.
 */
nrf24_status_t nrf24_read_payload(void *data, uint8_t length);

/**
 * @brief  Clears the RX_DR interrupt flag in the radio status register.
 */
void nrf24_clear_rx_flag(void);

/**
 * @brief  Flushes the RX FIFO.
 */
void nrf24_flush_rx(void);

/**
 * @brief  Prints main configuration registers over UART for debugging receiver state.
 */
void nrf24_print_details(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF24L01_H */
