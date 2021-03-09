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

//static const char *CLOCK = "clock";

menu_t *menu = NULL;
SemaphoreHandle_t clockMutex;

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "api.openweathermap.org"
#define WEB_PORT "80"
#define WEB_PATH "http://api.openweathermap.org/data/2.5/weather?q=Rotterdam&appid=8a844dfb8a5c5fef30713f8ca4fb4aca"



static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

void clock_task(void*pvParameter){
    initialize_sntp();
    while(1)
    {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(CLOCKTAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    // update 'now' variable with current time
    time(&now);

    char time_buffer[64];
    char date_buffer[64];
    
    // set timezone
    setenv("TZ", "CET-1", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    // convert time to string
    strftime(time_buffer, sizeof(time_buffer), "%X", &timeinfo); // time
    strftime(date_buffer, sizeof(date_buffer), "%x", &timeinfo); // date
    
    //if(menu != NULL){
    if (xSemaphoreTake(clockMutex, (TickType_t) 10) == pdTRUE && menu != NULL){  
        menu_displayTime(menu, time_buffer,date_buffer);
        xSemaphoreGive(clockMutex);
        ESP_LOGI(CLOCKTAG, "The current date/time is: %s  %s", time_buffer,date_buffer);
       
    }
     printf("Test clock task\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    }    
}

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

void menu_task(void * pvParameter)
{

    menu = menu_createMenu();

    menu_displayWelcomeMessage(menu);

   
    // menu_displayScrollMenu(menu);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    
    // // Menu auto navigation demo code 
    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);

    // menu_handleKeyEvent(menu, MENU_KEY_OK);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);

    // menu_handleKeyEvent(menu, MENU_KEY_OK);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_LEFT);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_OK);
    // vTaskDelay(2500 / portTICK_RATE_MS);

    // menu_handleKeyEvent(menu, MENU_KEY_OK);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_OK);
    // vTaskDelay(2500 / portTICK_RATE_MS);

    // menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    // vTaskDelay(2500 / portTICK_RATE_MS);
    // menu_handleKeyEvent(menu, MENU_KEY_OK);
    // vTaskDelay(2500 / portTICK_RATE_MS);

    // End of menu auto navigation demo code 

    while(1)
    {
        while (uxSemaphoreGetCount(clockMutex) == 0)
        {
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        printf("Test menu task\n");
        vTaskDelay(1000 / portTICK_RATE_MS);
        
    }

    menu_freeMenu(menu);
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
    clockMutex = xSemaphoreCreateMutex();
    xTaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
    xTaskCreate(&clock_task, "clock_task", 4096, NULL, 5, NULL);
    //xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}

