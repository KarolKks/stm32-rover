#include "uart.h"

static Uart* s_debug_uart = nullptr;

UartStatus Uart::init(uint32_t baudrate)
{
    if (m_instance == USART2) {
        // Enable GPIOA clock and configure PA2 (TX)
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

        LL_GPIO_InitTypeDef gpio_init;
        LL_GPIO_StructInit(&gpio_init);
        gpio_init.Pin        = DefaultTxPin;
        gpio_init.Mode       = LL_GPIO_MODE_ALTERNATE;
        gpio_init.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpio_init.Pull       = LL_GPIO_PULL_NO;
        gpio_init.Alternate  = DefaultAf;

        if (LL_GPIO_Init(DefaultTxPort, &gpio_init) != SUCCESS) {
            return UartStatus::ErrInit;
        }

        // Enable USART2 clock
        LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    } else {
        return UartStatus::ErrInit;
    }

    LL_USART_Disable(m_instance);

    LL_USART_InitTypeDef usart_init;
    LL_USART_StructInit(&usart_init);
    usart_init.BaudRate          = baudrate;
    usart_init.TransferDirection = LL_USART_DIRECTION_TX;

    if (LL_USART_Init(m_instance, &usart_init) != SUCCESS) {
        return UartStatus::ErrInit;
    }

    LL_USART_Enable(m_instance);

    // Wait until transmitter is ready
    uint32_t timeout = TimeoutCycles;
    while (!LL_USART_IsActiveFlag_TEACK(m_instance) && (--timeout > 0U)) {
    }

    if (timeout == 0U) {
        return UartStatus::ErrTimeout;
    }

    m_initialized = true;
    setAsDebugOutput();

    return UartStatus::Ok;
}

void Uart::setAsDebugOutput()
{
    s_debug_uart = this;
}

bool Uart::sendChar(char c)
{
    uint32_t timeout = TimeoutCycles;
    while (!LL_USART_IsActiveFlag_TXE(m_instance) && (--timeout > 0U)) {
    }

    if (timeout == 0U) {
        return false;
    }

    LL_USART_TransmitData8(m_instance, static_cast<uint8_t>(c));
    return true;
}

void Uart::sendString(std::string_view str)
{
    for (char c : str) {
        if (!sendChar(c)) {
            break;
        }
    }
}

void Uart::sendInt(int32_t num)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%ld", static_cast<long>(num));
    sendString(buffer);
}

void Uart::sendFloat(float num, uint8_t decimals)
{
    if (num < 0.0f) {
        sendChar('-');
        num = -num;
    }

    auto int_part = static_cast<uint32_t>(num);
    sendInt(static_cast<int32_t>(int_part));

    if (decimals > 0) {
        sendChar('.');
        float frac = num - static_cast<float>(int_part);

        for (uint8_t i = 0; i < decimals; ++i) {
            frac *= 10.0f;
            auto digit = static_cast<uint8_t>(frac);
            sendChar(static_cast<char>('0' + digit));
            frac -= static_cast<float>(digit);
        }
    }
}

// Low-level hook allowing standard printf() to output to UART
extern "C" int _write(int file, char *ptr, int len)
{
    (void)file;
    if (ptr == nullptr || len <= 0 || s_debug_uart == nullptr) {
        return 0;
    }

    for (int i = 0; i < len; ++i) {
        if (!s_debug_uart->sendChar(ptr[i])) {
            return i;
        }
    }

    return len;
}
