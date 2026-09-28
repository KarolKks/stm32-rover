#include "pwm.h"

PwmStatus PwmTimer::init(uint32_t frequencyHz)
{
    if (frequencyHz == 0) {
        return PwmStatus::ErrParam;
    }

    // Enable APB bus clock for the selected timer instance
    if (m_instance == TIM3) {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
    } else if (m_instance == TIM2) {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
    } else if (m_instance == TIM4) {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);
    } else if (m_instance == TIM1) {
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
    } else {
        return PwmStatus::ErrInit;
    }

    m_frequencyHz = frequencyHz;

    // Calculate prescaler and auto-reload (ARR) for target PWM frequency
    uint32_t timerClock = SystemCoreClock;
    uint32_t prescaler = timerClock / (frequencyHz * 1000U);
    if (prescaler == 0) {
        prescaler = 1;
    }
    uint32_t psc = prescaler - 1;
    m_arr = (timerClock / ((psc + 1) * frequencyHz)) - 1;

    // Initialize timer base in up-counting mode
    LL_TIM_InitTypeDef tim_init;
    LL_TIM_StructInit(&tim_init);
    tim_init.Prescaler         = psc;
    tim_init.CounterMode       = LL_TIM_COUNTERMODE_UP;
    tim_init.Autoreload        = m_arr;
    tim_init.ClockDivision     = LL_TIM_CLOCKDIVISION_DIV1;
    tim_init.RepetitionCounter = 0;

    if (LL_TIM_Init(m_instance, &tim_init) != SUCCESS) {
        return PwmStatus::ErrInit;
    }

    // Enable ARR preload buffer for glitch-free duty cycle updates
    LL_TIM_EnableARRPreload(m_instance);

    // Advanced timer (TIM1) requires main output enable
    if (m_instance == TIM1) {
        LL_TIM_EnableAllOutputs(m_instance);
    }

    m_initialized = true;
    return PwmStatus::Ok;
}

PwmStatus PwmTimer::configureChannel(PwmChannel channel, GPIO_TypeDef* port, uint32_t pin, uint32_t af)
{
    if (!m_initialized) {
        return PwmStatus::ErrInit;
    }

    // Enable GPIO port clock
    if (port == GPIOA) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    } else if (port == GPIOB) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    } else if (port == GPIOC) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    } else {
        return PwmStatus::ErrInit;
    }

    // Route timer channel signal to physical pin via alternate function (AF)
    LL_GPIO_InitTypeDef gpio_init;
    LL_GPIO_StructInit(&gpio_init);
    gpio_init.Pin        = pin;
    gpio_init.Mode       = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull       = LL_GPIO_PULL_NO;
    gpio_init.Alternate  = af;

    if (LL_GPIO_Init(port, &gpio_init) != SUCCESS) {
        return PwmStatus::ErrInit;
    }

    // Configure channel in PWM Mode 1: High while counter < compare register
    LL_TIM_OC_InitTypeDef oc_init;
    LL_TIM_OC_StructInit(&oc_init);
    oc_init.OCMode       = LL_TIM_OCMODE_PWM1;
    oc_init.OCState      = LL_TIM_OCSTATE_ENABLE;
    oc_init.OCPolarity   = LL_TIM_OCPOLARITY_HIGH;
    oc_init.CompareValue = 0;

    auto chVal = static_cast<uint32_t>(channel);
    if (LL_TIM_OC_Init(m_instance, chVal, &oc_init) != SUCCESS) {
        return PwmStatus::ErrInit;
    }

    // Enable preload and output compare for this channel
    LL_TIM_OC_EnablePreload(m_instance, chVal);
    LL_TIM_CC_EnableChannel(m_instance, chVal);

    return PwmStatus::Ok;
}

void PwmTimer::setDutyCycle(PwmChannel channel, float percent)
{
    // Clamp duty cycle percentage between 0% and 100%
    if (percent < 0.0f) {
        percent = 0.0f;
    } else if (percent > 100.0f) {
        percent = 100.0f;
    }

    // Convert percentage to compare register value (0 to ARR)
    auto compareVal = static_cast<uint32_t>((percent / 100.0f) * static_cast<float>(m_arr));
    setDutyCycleRaw(channel, compareVal);
}

void PwmTimer::setDutyCycleRaw(PwmChannel channel, uint32_t compareValue)
{
    // Clamp compare value to maximum counter period
    if (compareValue > m_arr) {
        compareValue = m_arr;
    }

    switch (channel) {
    case PwmChannel::Ch1:
        LL_TIM_OC_SetCompareCH1(m_instance, compareValue);
        break;
    case PwmChannel::Ch2:
        LL_TIM_OC_SetCompareCH2(m_instance, compareValue);
        break;
    case PwmChannel::Ch3:
        LL_TIM_OC_SetCompareCH3(m_instance, compareValue);
        break;
    case PwmChannel::Ch4:
        LL_TIM_OC_SetCompareCH4(m_instance, compareValue);
        break;
    }
}

void PwmTimer::start()
{
    if (m_initialized) {
        LL_TIM_EnableCounter(m_instance);
    }
}

void PwmTimer::stop()
{
    if (m_initialized) {
        LL_TIM_DisableCounter(m_instance);
    }
}
