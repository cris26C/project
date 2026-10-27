#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "shtc3.h"

static const char *TAG = "main"; 
typedef struct {
    uint32_t period_ms;
    shtc3_reading_t reading;
} sampler_args_t;

static sampler_args_t sampler_args = { .period_ms = 1000 };

static void sampler_task(void *pvParameters)
{
    sampler_args_t *args = (sampler_args_t *)pvParameters;

    ESP_ERROR_CHECK(shtc3_init());

    while(1){
        shtc3_read(&args->reading);
        vTaskDelay(pdMS_TO_TICKS(args->period_ms));
    }
}

void app_main(void)
{
    printf("Starting SHTC3 sampler task...\n");
    printf("Priority: %u\n", (unsigned) uxTaskPriorityGet(NULL));
    xTaskCreate(sampler_task, "sampler", 4096, &sampler_args, 5, NULL);

    while(1){
        ESP_LOGI(TAG, "Temperature: %.2f °C, Humidity: %.2f %%", sampler_args.reading.temperature, sampler_args.reading.humidity);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
