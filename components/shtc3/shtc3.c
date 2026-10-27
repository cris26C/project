/* SHTC3 driver (see shtc3.h). Command codes and conversions from the
   Sensirion SHTC3 datasheet, sections 5.3 to 5.10.
*/
#include "shtc3.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#define SHTC3_ADDR              0x70
#define SHTC3_CMD_WAKEUP        0x3517
#define SHTC3_CMD_SLEEP         0xB098
#define SHTC3_CMD_READ_ID       0xEFC8
#define SHTC3_CMD_MEASURE_T_RH  0x7866          // Normal mode, T first, no clock stretching
#define SHTC3_ID_MASK           0x083F
#define SHTC3_ID_VALUE          0x0807

#define SHTC3_WAKEUP_US         300             // Datasheet: 240 us max
#define SHTC3_MEASURE_MS        13              // Datasheet: 12.1 ms max in normal mode
#define SHTC3_I2C_TIMEOUT_MS    50

/* vTaskDelay(n) blocks between n-1 and n ticks, and pdMS_TO_TICKS rounds down
   (with the default 100 Hz tick, pdMS_TO_TICKS(1) == 0). Round up and add a
   tick so the wait is never shorter than ms. */
#define DELAY_AT_LEAST_MS(ms)   vTaskDelay(((ms) + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS + 1)

static const char *TAG = "shtc3";

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;

static esp_err_t send_cmd(uint16_t cmd)
{
    uint8_t buf[2] = { cmd >> 8, cmd & 0xFF };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), SHTC3_I2C_TIMEOUT_MS);
}

/* CRC-8, polynomial 0x31, init 0xFF (datasheet section 5.9) */
static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1;
        }
    }
    return crc;
}

/* Reads one 16-bit word followed by its CRC */
static esp_err_t read_words(uint8_t *buf, size_t words, uint16_t *out)
{
    ESP_RETURN_ON_ERROR(i2c_master_receive(s_dev, buf, words * 3, SHTC3_I2C_TIMEOUT_MS), TAG, "read failed");
    for (size_t i = 0; i < words; i++) {
        const uint8_t *w = &buf[i * 3];
        ESP_RETURN_ON_FALSE(crc8(w, 2) == w[2], ESP_ERR_INVALID_CRC, TAG, "bad CRC");
        out[i] = (w[0] << 8) | w[1];
    }
    return ESP_OK;
}

esp_err_t shtc3_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = -1,                         // Any free I2C controller
        .sda_io_num = CONFIG_SHTC3_SDA_GPIO,
        .scl_io_num = CONFIG_SHTC3_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "bus init failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHTC3_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev), TAG, "add device failed");

    ESP_RETURN_ON_ERROR(send_cmd(SHTC3_CMD_WAKEUP), TAG, "sensor not found at 0x%02x", SHTC3_ADDR);
    esp_rom_delay_us(SHTC3_WAKEUP_US);         // Too short for a tick: busy-wait

    uint8_t buf[3];
    uint16_t id;
    ESP_RETURN_ON_ERROR(send_cmd(SHTC3_CMD_READ_ID), TAG, "read ID failed");
    ESP_RETURN_ON_ERROR(read_words(buf, 1, &id), TAG, "read ID failed");
    ESP_RETURN_ON_FALSE((id & SHTC3_ID_MASK) == SHTC3_ID_VALUE, ESP_ERR_NOT_FOUND, TAG, "unexpected ID 0x%04x", id);
    ESP_LOGI(TAG, "SHTC3 found (ID 0x%04x)", id);

    return send_cmd(SHTC3_CMD_SLEEP);
}

esp_err_t shtc3_read(shtc3_reading_t *out)
{
    ESP_RETURN_ON_ERROR(send_cmd(SHTC3_CMD_WAKEUP), TAG, "wakeup failed");
    esp_rom_delay_us(SHTC3_WAKEUP_US);         // Too short for a tick: busy-wait

    ESP_RETURN_ON_ERROR(send_cmd(SHTC3_CMD_MEASURE_T_RH), TAG, "measure failed");
    DELAY_AT_LEAST_MS(SHTC3_MEASURE_MS);

    uint8_t buf[6];
    uint16_t raw[2];                            // raw[0] = temperature, raw[1] = humidity
    esp_err_t err = read_words(buf, 2, raw);
    send_cmd(SHTC3_CMD_SLEEP);                  // Sleep even if the read failed
    ESP_RETURN_ON_ERROR(err, TAG, "measure read failed");

    out->temperature = -45.0f + 175.0f * raw[0] / 65536.0f;
    out->humidity = 100.0f * raw[1] / 65536.0f;
    return ESP_OK;
}
