

#include "def.h"
#include "driver/gpio.h"
#include "driver/dac.h"


bool esp32_dac_set(int chan,int val)
{
    if (val < 0) val = 0;
    if (val > 255) val = 255;
    if (chan == 1) {
        return ESP_OK == dac_output_voltage(DAC_CHANNEL_1,val);
    }
    if (chan == 2) {
        return ESP_OK == dac_output_voltage(DAC_CHANNEL_2,val);
    }
    return false;
}

bool esp32_dac_enable(int chan)
{
    if (chan == 1) {
        return ESP_OK == dac_output_enable(DAC_CHANNEL_1);
    }
    if (chan == 2) {
        return ESP_OK == dac_output_enable(DAC_CHANNEL_2);
    }
    return false;
}

bool esp32_dac_disable(int chan)
{
    if (chan == 1) {
        return ESP_OK == dac_output_disable(DAC_CHANNEL_1);
    }
    if (chan == 2) {
        return ESP_OK == dac_output_disable(DAC_CHANNEL_2);
    }
    return false;
}


