#include "BatteryTask.h"
#include "BoardConfig.h"

#define BATTERY_SAMPLE_PERIOD_MS 100U

static BatteryMonitor g_battery;
static BatteryLevel g_reported = BATTERY_UNKNOWN;
static uint32_t g_next_sample_ms;

#if BOARD_HAS_VBAT_SENSE

void BatteryTask_Init(void)
{
    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;

    Battery_Init(&g_battery);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);               /* 72 MHz / 6 = 12 MHz (<14 MHz) */

    gpio.GPIO_Pin = VBAT_ADC_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(VBAT_ADC_PORT, &gpio);

    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc);
    /* 239.5 cycles: high-impedance divider (100k||33k ~ 25k) needs long sampling */
    ADC_RegularChannelConfig(ADC1, VBAT_ADC_CHANNEL, 1, ADC_SampleTime_239Cycles5);
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET) { }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET) { }

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);         /* first conversion */
}

void BatteryTask_Run(uint32_t now_ms)
{
    if ((int32_t)(now_ms - g_next_sample_ms) < 0)
    {
        return;
    }
    g_next_sample_ms = now_ms + BATTERY_SAMPLE_PERIOD_MS;

    /* Non-blocking: read the conversion started 100 ms ago, start the next. */
    if (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == SET)
    {
        (void)Battery_AddSample(&g_battery, ADC_GetConversionValue(ADC1));
    }
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

#else  /* original wiring: no divider, battery always reported UNKNOWN */

void BatteryTask_Init(void) { Battery_Init(&g_battery); }
void BatteryTask_Run(uint32_t now_ms) { (void)now_ms; (void)g_next_sample_ms; }

#endif

BatteryLevel BatteryTask_Level(void)
{
    return g_battery.level;
}

uint16_t BatteryTask_Millivolts(void)
{
    return g_battery.mv;
}

uint8_t BatteryTask_ConsumeChange(BatteryLevel *new_level)
{
    if (g_battery.level != g_reported)
    {
        g_reported = g_battery.level;
        *new_level = g_reported;
        return 1U;
    }
    return 0U;
}
