#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_rom_sys.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"

static const char *TAG = "HCSR04";

#define TRIG_PIN   GPIO_NUM_13
#define ECHO_PIN   GPIO_NUM_14
#define TIMEOUT_US 25000    

static void hcsr04_init(void)
{
    gpio_config_t trig = {
        .pin_bit_mask = 1ULL << TRIG_PIN,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&trig));

    gpio_config_t echo = {
        .pin_bit_mask = 1ULL << ECHO_PIN,
        .mode = GPIO_MODE_INPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&echo));

    gpio_set_level(TRIG_PIN, 0);
}

static float hcsr04_measure_cm(void)
{
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    int64_t t0 = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if (esp_timer_get_time() - t0 > TIMEOUT_US) {
            return -1;
        }
    }

    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if (esp_timer_get_time() - echo_start > TIMEOUT_US) {
            return -1;
        }
    }
    int64_t echo_us = esp_timer_get_time() - echo_start;

    return echo_us / 58.0f;
}

static void sensor_task(void *arg)
{
    hcsr04_init();

    while (1) {
        float cm = hcsr04_measure_cm();
        if (cm < 0) {
            ESP_LOGW(TAG, "Predaleko");
        } else {
            ESP_LOGI(TAG, "Udaljenost: %.1f cm", cm);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}
