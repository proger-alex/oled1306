/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include "driver/i2c_master.h"
#include "ssd1306.h"
#include "bitmap_icon.h"

// Координаты OLED
#define I2C_SSD1306_ADDRESS UINT8_C(0x3c)
#define I2C_SSD1306_SCL_IO_NUM GPIO_NUM_20
#define I2C_SSD1306_SDA_IO_NUM GPIO_NUM_21


/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "httpbin.org"
#define WEB_PORT "80"
#define WEB_PATH "/get"


static const char* SSD_1306_TAG = "SSD1306";
static const char* TAG = "example";

static i2c_master_bus_handle_t i2c0_bus_hdl = NULL;

static const char* REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

#define vTaskDelaySecUntil(x, sec) vTaskDelayUntil(x, pdMS_TO_TICKS(sec * 1000))

void i2c0_ssd1306_task(void* pvParameters)
{
    // initialize the xLastWakeTime variable with the current time.
    TickType_t last_wake_time = xTaskGetTickCount();
    //

    // Scan I2C bus
    if (i2c0_bus_hdl == NULL)
    {
        ESP_LOGE(SSD_1306_TAG, "I2C bus handle is NULL!");
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(SSD_1306_TAG, "Scanning I2C bus for devices...");
    for (uint8_t addr = 0x08; addr <= 0x78; addr++)
    {
        esp_err_t ret = i2c_master_probe(i2c0_bus_hdl, addr, 100); // 100 ms timeout
        if (ret == ESP_OK)
        {
            ESP_LOGI(SSD_1306_TAG, "Found device at address: 0x%02X", addr);
        }
    }
    ESP_LOGI(SSD_1306_TAG, "I2C scan complete.");


    // ssd1306_config_t dev_cfg = I2C_SSD1306_128x64_CONFIG_DEFAULT;

    ssd1306_config_t dev_cfg = {
        .i2c_address = I2C_SSD1306_ADDRESS,
        .i2c_clock_speed = I2C_SSD1306_DEV_CLK_SPD,
        .panel_size = SSD1306_PANEL_128x64,
        .offset_x = 0,
        .flip_enabled = false,
        .display_enabled = true
    };

    ssd1306_handle_t dev_hdl;
    ssd1306_init(i2c0_bus_hdl, &dev_cfg, &dev_hdl);
    if (dev_hdl == NULL)
    {
        ESP_LOGE(SSD_1306_TAG, "ssd1306 handle init failed");
        vTaskDelete(NULL);
        return;
    }

    // Явно включить дисплей
    ssd1306_enable_display(dev_hdl);
    // Установить максимальный контраст
    ssd1306_set_contrast(dev_hdl, 0xFF);
    // Очистить и вывести тестовую строку
    ssd1306_clear_display(dev_hdl,false);
    ssd1306_display_text(dev_hdl, 0, "TEST!!", false);

    ESP_LOGI(SSD_1306_TAG, "Display forced ON, contrast max.");


    // task loop entry point
    while (1)
    {
        ESP_LOGI(SSD_1306_TAG, "######################## SSD1306 - START #########################");
        //
        int center = 1, top = 1, bottom = 4;

        ESP_LOGI(SSD_1306_TAG, "Panel is 128x64");

        // Display x3 text
        ESP_LOGI(SSD_1306_TAG, "Display x3 Text");
        ssd1306_clear_display(dev_hdl, false);
        ssd1306_display_text_x3(dev_hdl, 0, "Hello", false);
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // Display bitmap icons (закомментируем, пока нет изображений)
        ESP_LOGI(SSD_1306_TAG, "Display bitmap icons");
        ssd1306_clear_display(dev_hdl, false);

        ssd1306_display_bitmap(dev_hdl, 31, 0, data_rx_icon_32x32, 32, 32, false);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        ssd1306_display_bitmap(dev_hdl, 31, 0, data_tx_icon_32x32, 32, 32, false);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Display x2 text
        ESP_LOGI(SSD_1306_TAG, "Display x2 Text");
        ssd1306_clear_display(dev_hdl, false);
        ssd1306_display_text_x2(dev_hdl, 0, "{xTEXTx}", false);
        ssd1306_display_text_x2(dev_hdl, 2, " X2-X2", false);
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // Display text
        ESP_LOGI(SSD_1306_TAG, "Display Text");
        ssd1306_clear_display(dev_hdl, false);
        ssd1306_display_text(dev_hdl, 0, "SSD1306 128x64", false);
        ssd1306_display_text(dev_hdl, 1, "Hello World!!", false);
        ssd1306_display_text(dev_hdl, 2, "SSD1306 128x64", true);
        ssd1306_display_text(dev_hdl, 3, "Hello World!!", true);
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // Display Count Down (упрощенная версия)
        ESP_LOGI(SSD_1306_TAG, "Display Count Down");
        for (int count = 9; count > 0; count--)
        {
            char count_str[2] = {count + '0', '\0'};
            ssd1306_clear_display(dev_hdl, false);
            ssd1306_display_text_x3(dev_hdl, 2, count_str, false);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        // Простая демонстрация вместо скроллинга
        ESP_LOGI(SSD_1306_TAG, "Simple Demo");
        ssd1306_clear_display(dev_hdl, false);
        ssd1306_display_text(dev_hdl, 0, "WiFi Connected", false);
        ssd1306_display_text(dev_hdl, 2, "HTTP GET Demo", false);
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // Invert
        ESP_LOGI(SSD_1306_TAG, "Invert");
        ssd1306_clear_display(dev_hdl, true);
        ssd1306_display_text(dev_hdl, center, "  Good Bye!!", true);
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        //
        ESP_LOGI(SSD_1306_TAG, "######################## SSD1306 - END ###########################");
        //
        // pause the task per defined wait period
        vTaskDelaySecUntil(&last_wake_time, 10);
    }
    //
    // free resources
    // ssd1306_delete( dev_hdl );
    // vTaskDelete( NULL );
}

static void i2c_init(void)
{
    const i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SSD1306_SCL_IO_NUM,
        .sda_io_num = I2C_SSD1306_SDA_IO_NUM,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };


    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c0_bus_hdl));

    if (i2c0_bus_hdl == NULL)
    {
        ESP_LOGE(SSD_1306_TAG, "i2c handle init failed");
        vTaskDelete(NULL);
    }
}

static void http_get_task(void* pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo* res;
    struct in_addr* addr;
    int s, r;
    char recv_buf[64];

    while (1)
    {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if (err != 0 || res == NULL)
        {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in*)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if (s < 0)
        {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if (connect(s, res->ai_addr, res->ai_addrlen) != 0)
        {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0)
        {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                       sizeof(receiving_timeout)) < 0)
        {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do
        {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf) - 1);
            for (int i = 0; i < r; i++)
            {
                putchar(recv_buf[i]);
            }
        }
        while (r > 0);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);

        // Выводим результат на дисплей
        char status[32];
        snprintf(status, sizeof(status), "HTTP GET OK");
        // Здесь можно добавить код для отображения на дисплее

        for (int countdown = 10; countdown >= 0; countdown--)
        {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Инициализируем I2C
    i2c_init();

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(&i2c0_ssd1306_task, "ssd1306_task", 4096, NULL, 5, NULL);
    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
