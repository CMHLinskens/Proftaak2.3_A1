#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp32/rom/uart.h"

#include "smbus.h"
#include "qwiic_twist.h"
#include "i2c-lcd1602.h"
#include "lcd-menu.h"

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

static const char *TAG = "MAIN";

i2c_port_t i2c_num;
qwiic_twist_t *qwiic_twist_rotary;
menu_t *menu;

void pressed(void);
void onMove(int16_t);

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


static void component_init(void){
    //INIT rotary encoder
    qwiic_twist_rotary = (qwiic_twist_t*)malloc(sizeof(*qwiic_twist_rotary));
    qwiic_twist_rotary->port = i2c_num;
    qwiic_twist_rotary->onButtonClicked = &pressed;
    qwiic_twist_rotary->onMoved = &onMove;
    qwiic_twist_init(qwiic_twist_rotary);
}

void menu_task(void * pvParameter)
{
    menu = menu_createMenu(lcd_init());

    menu_displayWelcomeMessage(menu);
    menu_displayScrollMenu(menu);

    xTaskCreate(&rotary_task, "rotary_task", 4096, NULL, 5, NULL);

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

void pressed(void){
    ESP_LOGI(TAG, "pressed rotary encoder");
    menu->handleKeyEvent(MENU_KEY_OK);
}

void onMove(int16_t move_value){
    if(move_value > 0){
        menu->handleKeyEvent(MENU_KEY_RIGHT);
    }
    else if(move_value < 0){
        menu->handleKeyEvent(MENU_KEY_LEFT);
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
        vTaskDelay(100 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}



void app_main()
{
    //I^2C initialization + the I^2C port
    i2c_master_init();
    i2c_num = I2C_MASTER_NUM;

    //initialize the components
    component_init();

    TaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
}

