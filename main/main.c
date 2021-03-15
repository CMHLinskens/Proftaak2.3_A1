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

#include "wifi_connect.h"
#include "sdcard_player.h"
#include "http_request.h"

#define MAINTAG "main"
#define CLOCKTAG "clock"

i2c_port_t i2c_num;
qwiic_twist_t *qwiic_twist_rotary;
menu_t *menu;

void rotary_task(void *);
void clicked(void);
void pressed(void);
void onMove(int16_t);

static void http_get_task(void *pvParameters)
{
    api_request();
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
    
    start_sdcard_task();
    vTaskDelay(1000);

    //I^2C initialization + the I^2C port
    i2c_master_init();
    i2c_num = I2C_MASTER_NUM;

    //initialize the components
    component_init();

    xTaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
    xTaskCreate(&clock_task, "clock_task", 4096, NULL, 5, NULL);
    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}

