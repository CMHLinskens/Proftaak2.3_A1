#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp32/rom/uart.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_event.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include "esp_task_wdt.h"
#include "smbus.h"
#include "qwiic_twist.h"
#include "i2c-lcd1602.h"
#include "lcd-menu.h"
#include "clock-sync.h"
#include "esp_wifi.h" 

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define MAINTAG "main"
#define CLOCKTAG "clock"
#define APITAG "API"

i2c_port_t i2c_num;
qwiic_twist_t *qwiic_twist_rotary;
menu_t *menu;

void rotary_task(void *);
void clicked(void);
void pressed(void);
void onMove(int16_t);

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "api.openweathermap.org"
#define WEB_PORT "80"
#define WEB_PATH "http://api.openweathermap.org/data/2.5/weather?q=Rotterdam&appid=8a844dfb8a5c5fef30713f8ca4fb4aca"

static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    while(1) {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(APITAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(APITAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(APITAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(APITAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(APITAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(APITAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(APITAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(APITAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(APITAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(APITAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(APITAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(APITAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(APITAG, "Starting again!");
    }
}

void i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_MASTER_RX_BUF_LEN,
                       I2C_MASTER_TX_BUF_LEN, 0);
}

static void component_init(void){
    //INIT rotary encoder
    qwiic_twist_rotary = (qwiic_twist_t*)malloc(sizeof(*qwiic_twist_rotary));
    qwiic_twist_rotary->port = i2c_num;
    qwiic_twist_rotary->onButtonClicked = &clicked;
    qwiic_twist_rotary->onButtonPressed = &pressed;
    qwiic_twist_rotary->onMoved = &onMove;
    qwiic_twist_init(qwiic_twist_rotary);
}

void menu_task(void * pvParameter)
{
    menu = menu_createMenu();

    menu_displayWelcomeMessage(menu);
    menu_displayScrollMenu(menu);

    // xTaskCreate(&rotary_task, "rotary_task", 4096, NULL, 5, NULL);
    qwiic_twist_start_task(qwiic_twist_rotary);

    while(1)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    menu_freeMenu(menu);
    vTaskDelete(NULL);
}

char * toString(int number) {
    int length = snprintf(NULL, 0, "%d", number + 1);
    char *str = malloc(length + 1);
    snprintf(str, length + 1, "%d", number + 1);
    return str;
}

void clicked(void){
    ESP_LOGI(MAINTAG, "clicked rotary encoder");
    menu_handleKeyEvent(menu, MENU_KEY_OK);
}
void pressed(void){
    ESP_LOGI(MAINTAG, "pressed rotary encoder");
}

void onMove(int16_t move_value){
    if(move_value > 0){
        menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    }
    else if(move_value < 0){
        menu_handleKeyEvent(menu, MENU_KEY_LEFT);
    }
}

void rotary_task(void * pvParameter)
{
    //COLOR TEST WITH THE ROTARY ENCODER
    //smbus_info_t * smbus_info_rotary = smbus_malloc();
    // ESP_ERROR_CHECK(smbus_init(smbus_info_rotary, i2c_num, 0x3F));
    // ESP_ERROR_CHECK(smbus_set_timeout(smbus_info_rotary, 1000 / portTICK_RATE_MS));
    // smbus_write_byte(smbus_info_rotary, 0x0D, 255);
    // smbus_write_byte(smbus_info_rotary, 0x0E, 255);
    // smbus_write_byte(smbus_info_rotary, 0x0F, 255);

    qwiic_twist_start_task(qwiic_twist_rotary);
    while(1){
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    
    //I^2C initialization + the I^2C port
    i2c_master_init();
    i2c_num = I2C_MASTER_NUM;

    //initialize the components
    component_init();

    xTaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
    xTaskCreate(&clock_task, "clock_task", 4096, NULL, 5, NULL);
    // xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}

