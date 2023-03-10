/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_sleep.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "sdkconfig.h"

#include <inttypes.h>
#include <bme680.h>
#include <sgp40.h>

#define PORT 0
#if defined(CONFIG_EXAMPLE_I2C_ADDRESS_0)
#define ADDR BME680_I2C_ADDR_0
#endif
#if defined(CONFIG_EXAMPLE_I2C_ADDRESS_1)
#define ADDR BME680_I2C_ADDR_1
#endif

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif


/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "10.0.0.194"
#define WEB_PORT 3001
#define WEB_PATH "/sgp40"

static const char *TAG = "example";

static const char *REQUEST = "POST " WEB_PATH " HTTP/1.1\r\n"
    "Host: "WEB_SERVER":3001\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 41\r\n\r\n";



static const char *voc_index_name(int32_t voc_index)
{
    if (voc_index <= 0) return "INVALID VOC INDEX";
    else if (voc_index <= 10) return "unbelievable clean";
    else if (voc_index <= 30) return "extremely clean";
    else if (voc_index <= 50) return "higly clean";
    else if (voc_index <= 70) return "very clean";
    else if (voc_index <= 90) return "clean";
    else if (voc_index <= 120) return "normal";
    else if (voc_index <= 150) return "moderately polluted";
    else if (voc_index <= 200) return "higly polluted";
    else if (voc_index <= 300) return "extremely polluted";

    return "RUN!";
}

static void build_http_str(char *http_req, float temp, float humidity, int voc)
{
    char buffer[50];
    sprintf(buffer, "temp=%f&humidity=%f&voc=%03d\r\n\r\n", temp, humidity, voc);
    strcpy(http_req, REQUEST);
    strcat(http_req, buffer);
}

static void getAverageReading(float *temp, float *humidity, int *voc) {
    bme680_t sensor;
    sgp40_t sgp;

    // setup SGP40
    memset(&sgp, 0, sizeof(sgp));
    ESP_ERROR_CHECK(sgp40_init_desc(&sgp, 0, CONFIG_EXAMPLE_I2C_MASTER_SDA, CONFIG_EXAMPLE_I2C_MASTER_SCL));
    ESP_ERROR_CHECK(sgp40_init(&sgp));
    ESP_LOGI(TAG, "SGP40 initilalized. Serial: 0x%04x%04x%04x",
            sgp.serial[0], sgp.serial[1], sgp.serial[2]);
    // setup bme680
    memset(&sensor, 0, sizeof(bme680_t));
    ESP_ERROR_CHECK(bme680_init_desc(&sensor, ADDR, PORT, CONFIG_EXAMPLE_I2C_MASTER_SDA, CONFIG_EXAMPLE_I2C_MASTER_SCL));
    // init the sensor
    ESP_ERROR_CHECK(bme680_init_sensor(&sensor));
    // Changes the oversampling rates to 4x oversampling for temperature
    // and 2x oversampling for humidity. Pressure measurement is skipped.
    bme680_set_oversampling_rates(&sensor, BME680_OSR_4X, BME680_OSR_NONE, BME680_OSR_2X);
    // Change the IIR filter size for temperature and pressure to 7.
    bme680_set_filter_size(&sensor, BME680_IIR_SIZE_7);
    // Change the heater profile 0 to 200 degree Celsius for 100 ms.
    bme680_set_heater_profile(&sensor, 0, 200, 100);
    bme680_use_heater_profile(&sensor, 0);
    // Set ambient temperature to 10 degree Celsius
    bme680_set_ambient_temperature(&sensor, 10);
    // as long as sensor configuration isn't changed, duration is constant
    uint32_t duration;
    bme680_get_measurement_duration(&sensor, &duration);
    bme680_values_float_t values;
    // Wait until all set up
    vTaskDelay(pdMS_TO_TICKS(250));

    int32_t voc_index;
    int iterations = 500; int32_t total = 0;
    for (int i = 0; i <= iterations; i++)
    {
        // trigger the sensor to start one TPHG measurement cycle
        if (bme680_force_measurement(&sensor) == ESP_OK)
        {
            // passive waiting until measurement results are available
            vTaskDelay(duration);

            // get the results and do something with them
            if (bme680_get_results_float(&sensor, &values) == ESP_OK)
                printf("BME680 Sensor: %.2f °C, %.2f %%, %.2f hPa, %.2f Ohm\n",
                        values.temperature, values.humidity, values.pressure, values.gas_resistance);
            
            // Feed it to SGP40
            ESP_ERROR_CHECK(sgp40_measure_voc(&sgp, values.humidity, values.temperature, &voc_index));
            ESP_LOGI(TAG, "%.2f °C, %.2f %%, VOC index: %" PRIi32 ", Air is [%s]",
                values.temperature, values.humidity, voc_index, voc_index_name(voc_index));
        }
        if (i > 300)
        {
            total = total + voc_index;
        }
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
    *voc = total / 200;
    *temp = values.temperature;
    *humidity = values.humidity;

}


static void http_get_task()
{
    float temp, humidity;
    int voc;
    getAverageReading(&temp, &humidity, &voc);

    int s;
    struct sockaddr_in serv_addr;
    
    while (true) {
        s = socket(AF_INET, SOCK_STREAM, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(WEB_PORT);

        if(inet_pton(AF_INET, WEB_SERVER, &serv_addr.sin_addr)<=0)
        {
            ESP_LOGE(TAG,"\n inet_pton error occured\n");
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }

        if(connect(s, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        int req_size = strlen(REQUEST)+52;
        char http_req[req_size];
        build_http_str(http_req, temp, humidity, voc);
        ESP_LOGI(TAG, "%s", http_req);

        if (write(s, http_req, req_size) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            close(s);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        ESP_LOGI(TAG, "%s", http_req);
        close(s);
        break;
    }
}




void app_main(void)
{
    ESP_ERROR_CHECK(i2cdev_init());
    ESP_ERROR_CHECK(nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    http_get_task();

    const int wakeup_time_sec = 180; // 900 seconds = 15 minutes
    ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", wakeup_time_sec);
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

    ESP_LOGI(TAG, "Entering deep sleep\n");
    esp_deep_sleep_start();
}
