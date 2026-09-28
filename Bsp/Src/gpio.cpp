#include "gpio.h"

namespace Bsp::Gpio {

bool enablePortClock(GPIO_TypeDef* port)
{
    // Enable AHB2 bus clock for the requested GPIO port
    if (port == GPIOA) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    } else if (port == GPIOB) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    } else if (port == GPIOC) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    } else if (port == GPIOD) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
    } else {
        return false;
    }
    return true;
}

bool initOutput(GPIO_TypeDef* port, uint32_t pinMask, uint32_t speed)
{
    if (!enablePortClock(port)) {
        return false;
    }

    // Set pin low before mode change to prevent glitching connected devices
    LL_GPIO_ResetOutputPin(port, pinMask);

    // Configure pin as standard push-pull digital output
    LL_GPIO_InitTypeDef gpio_init;
    LL_GPIO_StructInit(&gpio_init);
    gpio_init.Pin        = pinMask;
    gpio_init.Mode       = LL_GPIO_MODE_OUTPUT;
    gpio_init.Speed      = speed;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull       = LL_GPIO_PULL_NO;

    return (LL_GPIO_Init(port, &gpio_init) == SUCCESS);
}

} // namespace Bsp::Gpio
