#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp32/rom/uart.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/event_groups.h"
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

#undef USE_STDIN

#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN    0                     // disabled
#define I2C_MASTER_RX_BUF_LEN    0                     // disabled
#define I2C_MASTER_FREQ_HZ       100000
#define I2C_MASTER_SDA_IO        CONFIG_I2C_MASTER_SDA
#define I2C_MASTER_SCL_IO        CONFIG_I2C_MASTER_SCL
#define LCD_NUM_ROWS			 4
#define LCD_NUM_COLUMNS			 40
#define LCD_NUM_VIS_COLUMNS		 20

#define MAINTAG "main"

static const char *TAG = "clock";

void clock_task(void*pvParameter){
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    initialize_sntp();
    while(true)
    {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    // update 'now' variable with current time
    time(&now);

    char strftime_buf[64];
    // set timezone
    setenv("TZ", "CET-1", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    // convert time to string
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}

static void i2c_master_init(void)
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

i2c_lcd1602_info_t * lcd_init()
{
    i2c_port_t i2c_num = I2C_MASTER_NUM;
    uint8_t address = CONFIG_LCD1602_I2C_ADDRESS;

    // Set up the SMBus
    smbus_info_t * smbus_info = smbus_malloc();
    smbus_init(smbus_info, i2c_num, address);
    smbus_set_timeout(smbus_info, 1000 / portTICK_RATE_MS);

    // Set up the LCD1602 device with backlight off
    i2c_lcd1602_info_t * lcd_info = i2c_lcd1602_malloc();
    i2c_lcd1602_init(lcd_info, smbus_info, true, LCD_NUM_ROWS, LCD_NUM_COLUMNS, LCD_NUM_VIS_COLUMNS);

    // turn off backlight
    ESP_LOGI(MAINTAG, "backlight off");
    i2c_lcd1602_set_backlight(lcd_info, false);

    // turn on backlight
    ESP_LOGI(MAINTAG, "backlight on");
    i2c_lcd1602_set_backlight(lcd_info, true);

    // turn on cursor 
    ESP_LOGI(MAINTAG, "cursor on");
    i2c_lcd1602_set_cursor(lcd_info, true);

    return lcd_info;
}

void menu_task(void * pvParameter)
{
    i2c_master_init();
    // i2c_lcd1602_info_t *lcd_info = lcd_init();
    menu_t *menu = menu_createMenu(lcd_init());

    menu_displayWelcomeMessage(menu);
    menu_displayScrollMenu(menu);
    vTaskDelay(2500 / portTICK_RATE_MS);
    
    // Menu auto navigation demo code 
    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);

    menu_handleKeyEvent(menu, MENU_KEY_OK);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);

    menu_handleKeyEvent(menu, MENU_KEY_OK);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_LEFT);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_OK);
    vTaskDelay(2500 / portTICK_RATE_MS);

    menu_handleKeyEvent(menu, MENU_KEY_OK);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_OK);
    vTaskDelay(2500 / portTICK_RATE_MS);

    menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
    vTaskDelay(2500 / portTICK_RATE_MS);
    menu_handleKeyEvent(menu, MENU_KEY_OK);
    vTaskDelay(2500 / portTICK_RATE_MS);

    // End of menu auto navigation demo code 

    while(1)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    menu_freeMenu(menu);
    vTaskDelete(NULL);
}



void app_main()
{
    xTaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
    xTaskCreate(&clock_task, "clock_task", 4096, NULL, 5, NULL);
}

