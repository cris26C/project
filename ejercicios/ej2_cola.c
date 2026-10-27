#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "shtc3.h"

#define QUEUE_LENGTH 10

static const char *TAG = "main";
typedef struct {
    uint32_t period_ms;
    QueueHandle_t shtc3_queue;
} sampler_args_t;

static sampler_args_t sampler_args = { .period_ms = 1000 };

static void sampler_task(void *pvParameters)
{
    sampler_args_t *args = (sampler_args_t *)pvParameters;

    ESP_ERROR_CHECK(shtc3_init());

    while(1){
        shtc3_reading_t r;
        if (shtc3_read(&r) == ESP_OK) {
            // The queue copies r, so reusing the local on the next iteration is safe
            if (xQueueSend(args->shtc3_queue, &r, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Queue full, sample dropped");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(args->period_ms));
    }
}

void app_main(void)
{
    printf("Starting SHTC3 sampler task...\n");
    printf("Priority: %u\n", (unsigned) uxTaskPriorityGet(NULL));

    // The queue must exist before the task that writes to it starts
    sampler_args.shtc3_queue = xQueueCreate(QUEUE_LENGTH, sizeof(shtc3_reading_t));
    if (sampler_args.shtc3_queue == NULL) {
        ESP_LOGE(TAG, "Could not create queue");
        return;
    }
    xTaskCreate(sampler_task, "sampler", 4096, &sampler_args, 5, NULL);

    while(1){
        shtc3_reading_t r;
        // Blocks until a new sample arrives, so no vTaskDelay is needed here
        xQueueReceive(sampler_args.shtc3_queue, &r, portMAX_DELAY);
        ESP_LOGI(TAG, "Temperature: %.2f °C, Humidity: %.2f %%", r.temperature, r.humidity);
    }
}
