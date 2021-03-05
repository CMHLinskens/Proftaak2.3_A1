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

#define MENUTAG "menu"

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

static uint8_t _wait_for_user(void)
{
    uint8_t c = 0;
#ifdef USE_STDIN
    while (!c)
    {
       STATUS s = uart_rx_one_char(&c);
       if (s == OK) {
          printf("%c", c);
       }
    }
#else
    vTaskDelay(1000 / portTICK_RATE_MS);
#endif
    return c;
}

i2c_port_t i2c_num;

i2c_lcd1602_info_t * lcd_init()
{
    uint8_t address = CONFIG_LCD1602_I2C_ADDRESS;

    // Set up the SMBus
    smbus_info_t * smbus_info = smbus_malloc();
    smbus_init(smbus_info, i2c_num, address);
    smbus_set_timeout(smbus_info, 1000 / portTICK_RATE_MS);

    // Set up the LCD1602 device with backlight off
    i2c_lcd1602_info_t * lcd_info = i2c_lcd1602_malloc();
    i2c_lcd1602_init(lcd_info, smbus_info, true, LCD_NUM_ROWS, LCD_NUM_COLUMNS, LCD_NUM_VIS_COLUMNS);

    // turn off backlight
    ESP_LOGI(MENUTAG, "backlight off");
    _wait_for_user();
    i2c_lcd1602_set_backlight(lcd_info, false);

    // turn on backlight
    ESP_LOGI(MENUTAG, "backlight on");
    //_wait_for_user();
    i2c_lcd1602_set_backlight(lcd_info, true);

    ESP_LOGI(MENUTAG, "cursor on");
    _wait_for_user();
    i2c_lcd1602_set_cursor(lcd_info, true);

    return lcd_info;
}

void display_welcome_message(i2c_lcd1602_info_t * lcd_info)
{
    i2c_lcd1602_set_cursor(lcd_info, false);
    i2c_lcd1602_move_cursor(lcd_info, 6, 1);

    i2c_lcd1602_write_string(lcd_info, "Welcome");
    i2c_lcd1602_move_cursor(lcd_info, 8, 2);
    i2c_lcd1602_write_string(lcd_info, "User");

    vTaskDelay(2500 / portTICK_RATE_MS);
    i2c_lcd1602_clear(lcd_info);
}

void display_counter_rotary(i2c_lcd1602_info_t * lcd_info, int number)
{
    i2c_lcd1602_set_cursor(lcd_info, false);
    i2c_lcd1602_move_cursor(lcd_info, 6, 1);

    i2c_lcd1602_write_char(lcd_info, number);

    // vTaskDelay(2500 / portTICK_RATE_MS);
    // i2c_lcd1602_clear(lcd_info);
}

void menu_task(void * pvParameter)
{
    i2c_lcd1602_info_t * lcd_info = lcd_init();
    display_welcome_message(lcd_info);

    // while(1)
    // {
    //     vTaskDelay(100 / portTICK_RATE_MS);
    // }

    vTaskDelete(NULL);
}



void rotary_test_task(void * pvParameter)
{
    vTaskDelay(5000/ portTICK_RATE_MS);
   
    smbus_info_t * smbus_info_rotary = smbus_malloc();
    ESP_ERROR_CHECK(smbus_init(smbus_info_rotary, i2c_num, 0x3F));
    ESP_ERROR_CHECK(smbus_set_timeout(smbus_info_rotary, 1000 / portTICK_RATE_MS));
    smbus_write_byte(smbus_info_rotary, 0x0D, 255);
    smbus_write_byte(smbus_info_rotary, 0x0E, 255);
    smbus_write_byte(smbus_info_rotary, 0x0F, 255);

    //smbus_read_byte(smbus_info_rotary, );

    
    vTaskDelete(NULL);
}


void app_main()
{
    i2c_master_init();
    i2c_num = I2C_MASTER_NUM;
    xTaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
    xTaskCreate(&rotary_test_task, "rotary_test_task", 4096, NULL, 5, NULL);
}

