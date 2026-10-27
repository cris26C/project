// Ejercicio 3: sampler_task --(esp_event_post_to)--> event loop --> new_sample_handler
// Requiere "esp_event" en PRIV_REQUIRES de main/CMakeLists.txt

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "shtc3.h"

// Evento = (BASE, ID). DECLARE normalmente va en un .h; DEFINE la crea (una vez)
ESP_EVENT_DECLARE_BASE(SENSOR_EVENT);
ESP_EVENT_DEFINE_BASE(SENSOR_EVENT);

enum {
    SENSOR_EVENT_NEWSAMPLE,
};

static const char *TAG = "main";

// xTaskCreate solo pasa un void *, así que agrupamos los argumentos
typedef struct {
    uint32_t period_ms;
    esp_event_loop_handle_t loop;
} sampler_args_t;

// static: app_main termina pero la tarea sigue usando estos argumentos
static sampler_args_t sampler_args = { .period_ms = 1000 };

static void sampler_task(void *pvParameters)
{
    sampler_args_t *args = (sampler_args_t *)pvParameters;

    ESP_ERROR_CHECK(shtc3_init());

    while(1){
        shtc3_reading_t r;
        if (shtc3_read(&r) == ESP_OK) {
            // El loop copia r; timeout 0 = si está lleno, se descarta la muestra
            if (esp_event_post_to(args->loop, SENSOR_EVENT, SENSOR_EVENT_NEWSAMPLE,
                                  &r, sizeof(r), 0) != ESP_OK) {
                ESP_LOGW(TAG, "Event loop queue full, sample dropped");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(args->period_ms));
    }
}

// Lo llama la tarea del loop (no app_main) cada vez que llega el evento
static void new_sample_handler(void *handler_args, esp_event_base_t base,
                               int32_t id, void *event_data)
{
    shtc3_reading_t *r = (shtc3_reading_t *)event_data;
    ESP_LOGI(TAG, "Temperature: %.2f °C, Humidity: %.2f %%", r->temperature, r->humidity);
}

void app_main(void)
{
    printf("Starting SHTC3 sampler task...\n");
    printf("Priority: %u\n", (unsigned) uxTaskPriorityGet(NULL));

    // Con task_name, el loop tiene su propia tarea que despacha los eventos
    esp_event_loop_args_t loop_args = {
        .queue_size = 5,
        .task_name = "sensor_loop",
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY,
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &sampler_args.loop));

    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(sampler_args.loop, SENSOR_EVENT,
                                                             SENSOR_EVENT_NEWSAMPLE,
                                                             new_sample_handler, NULL, NULL));

    // Se crea después del loop y el handler para que el primer evento tenga destino
    xTaskCreate(sampler_task, "sampler", 4096, &sampler_args, 5, NULL);

    // app_main puede terminar: imprime la tarea del loop
    ESP_LOGI(TAG, "app_main done, returning");
}
