#include <string.h>
#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"

#include "include/wifi_conn.h"
#include "include/mqtt_comm.h"
#include "include/ssd1306.h"

/* ---------- Config ---------- */
#define BOTAO_GPIO   5
#define LED_GPIO     12
#define WIFI_SSID    "AGUIA 2.4G"
#define WIFI_PASS    "Leticia150789"
#define MQTT_BROKER  "192.168.15.18"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

/* ---------- Globals ---------- */
uint8_t oled_buf[ssd1306_buffer_length];
struct  render_area frame_area;

/* ---------- Utils ---------- */
static float read_temperature(void)
{
    adc_select_input(4);
    uint16_t raw = adc_read();
    const float conv = 3.3f / (1 << 12);
    float volt = raw * conv;
    return 27.0f - (volt - 0.706f) / 0.001721f;
}

/* ---------- Callback LED ---------- */
static void mqtt_led_cb(const char *topic,
                        const uint8_t *payload,
                        size_t len)
{
    ssd1306_clear_buffer(oled_buf);

    if (strncmp((const char *)payload, "1", len) == 0) {
        gpio_put(LED_GPIO, true);
        ssd1306_draw_string(oled_buf, 0, 0, "LED ligado via MQTT");
    } else if (strncmp((const char *)payload, "0", len) == 0) {
        gpio_put(LED_GPIO, false);
        ssd1306_draw_string(oled_buf, 0, 0, "LED desligado via MQTT");
    } else {
        ssd1306_draw_string(oled_buf, 0, 0, "Comando invalido");
    }
    render_on_display(oled_buf, &frame_area);
}

/* ---------- Main ---------- */
int main(void)
{
    stdio_init_all();
    sleep_ms(500);

    /* OLED init */
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA); gpio_pull_up(I2C_SCL);
    ssd1306_init();

    frame_area.start_column = 0;
    frame_area.end_column   = ssd1306_width - 1;
    frame_area.start_page   = 0;
    frame_area.end_page     = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&frame_area);

    /* GPIO */
    gpio_init(BOTAO_GPIO); gpio_set_dir(BOTAO_GPIO, GPIO_IN);
    gpio_pull_up(BOTAO_GPIO);
    gpio_init(LED_GPIO);   gpio_set_dir(LED_GPIO, GPIO_OUT);
    gpio_put(LED_GPIO, false);

    /* ADC */
    adc_init(); adc_set_temp_sensor_enabled(true);

    /* Wi-Fi */
    printf("Conectando ao Wi-Fi…\n");
    connect_to_wifi(WIFI_SSID, WIFI_PASS);

    /* MQTT */
    printf("Conectando ao broker %s…\n", MQTT_BROKER);
    mqtt_setup("bitdog1", MQTT_BROKER, "", "");
    mqtt_comm_subscribe("bitdoglab/saida/led1", mqtt_led_cb);
    sleep_ms(1500);

    /* OLED boot */
    ssd1306_clear_buffer(oled_buf);
    ssd1306_draw_string(oled_buf, 0, 0, "BitDog Lab online");
    render_on_display(oled_buf, &frame_area);

    /* Watchdog 8 s */
    watchdog_enable(8000, 1);
    uint32_t t_last = to_ms_since_boot(get_absolute_time());
    uint32_t hb = 0;

    while (true) {
        cyw43_arch_poll();
        watchdog_update();

        if (++hb >= 500) {           /* LED heartbeat 5 s */
            gpio_xor_mask(1u << LED_GPIO);
            hb = 0;
        }

        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - t_last >= 1000) {  /* 1 s */
            t_last = now;

            int   btn  = gpio_get(BOTAO_GPIO);
            float temp = read_temperature();

            char json[96];
            snprintf(json, sizeof(json),
                     "{\"botao\":%d,\"temp\":%.2f}", btn, temp);

            bool ok = mqtt_comm_publish("bitdoglab/entrada/sensores",
                                        (uint8_t *)json, strlen(json));

            if (!ok) {
                printf("Publish falhou — reconectando MQTT…\n");
                sleep_ms(2000);
                mqtt_setup("bitdog1", MQTT_BROKER, "", "");
                continue;
            }
            printf("Publicado: %s\n", json);

            /* OLED */
            ssd1306_clear_buffer(oled_buf);
            ssd1306_draw_string(oled_buf, 0, 0, "Enviado:");
            char buf[22];
            snprintf(buf, sizeof(buf), "Botao: %d", btn);
            ssd1306_draw_string(oled_buf, 0, 15, buf);
            snprintf(buf, sizeof(buf), "Temp: %.2f C", temp);
            ssd1306_draw_string(oled_buf, 0, 30, buf);
            render_on_display(oled_buf, &frame_area);
        }

        sleep_ms(10);
    }
}
