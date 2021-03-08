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
#include "i2c-lcd1602.h"

//button
#include "esp_peripherals.h"
#include "periph_adc_button.h"

//#include "touch_pad.h"

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

//Start touch pad buttons
#define AudioSetPin 32
#define AudioPlayPin 33
#define AudioVolPlus 27
#define AudioVolMinus 13
//End

#define MAINTAG "menu"
#define BUTTONTAG "button"

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

// static uint8_t _wait_for_user(void)
// {
//     uint8_t c = 0;
// #ifdef USE_STDIN
//     while (!c)
//     {
//        STATUS s = uart_rx_one_char(&c);
//        if (s == OK) {
//           printf("%c", c);
//        }
//     }
// #else
//     vTaskDelay(1000 / portTICK_RATE_MS);
// #endif
//     return c;
// }

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
    
    while(1)
    {
        // menu_handleKeyEvent(menu, MENU_KEY_OK);
        // vTaskDelay(2500 / portTICK_RATE_MS);
        menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
        vTaskDelay(2500 / portTICK_RATE_MS);
    }

    menu_freeMenu(menu);
    vTaskDelete(NULL);
}

// void app_main()
// {
//     xTaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
// }


/* Check operation of ADC button peripheral in ESP32-LyraTD-MSC board

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



//static const char *BUTTONTAG = "CHECK_MSC_ADC_BUTTON";


void app_main(void)
{
    ESP_LOGI(BUTTONTAG, "[ 1 ] Initialize peripherals");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    ESP_LOGI(BUTTONTAG, "[1.1] Initialize ADC Button peripheral");
    periph_adc_button_cfg_t adc_button_cfg = PERIPH_ADC_BUTTON_DEFAULT_CONFIG();
    adc_arr_t adc_btn_tag = ADC_DEFAULT_ARR();
    adc_button_cfg.arr = &adc_btn_tag;
    adc_button_cfg.arr_size = 1;

    ESP_LOGI(BUTTONTAG, "[1.2] Start ADC Button peripheral");
    esp_periph_handle_t adc_button_periph = periph_adc_button_init(&adc_button_cfg);
    esp_periph_start(set, adc_button_periph);

    ESP_LOGI(BUTTONTAG, "[ 2 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    ESP_LOGW(BUTTONTAG, "[ 3 ] Waiting for a button to be pressed ...");


    while (1) {
        char *btn_states[] = {"idle", "click", "click released", "press", "press released"};

        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(BUTTONTAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == PERIPH_ID_ADC_BTN) {
            int button_id = (int)msg.data;  // button id is sent as data_len
            int state     = msg.cmd;       // button state is sent as cmd
            switch (button_id) {
                case USER_KEY_ID0:
                    ESP_LOGI(BUTTONTAG, "[ * ] Button SET %s", btn_states[state]);
                    break;
                case USER_KEY_ID1:
                    ESP_LOGI(BUTTONTAG, "[ * ] Button PLAY %s", btn_states[state]);
                    break;
                case USER_KEY_ID2:
                    ESP_LOGI(BUTTONTAG, "[ * ] Button REC %s", btn_states[state]);
                    break;
                case USER_KEY_ID3:
                    ESP_LOGI(BUTTONTAG, "[ * ] Button MODE %s", btn_states[state]);
                    break;
                case USER_KEY_ID4:
                    ESP_LOGI(BUTTONTAG, "[ * ] Button VOL- %s", btn_states[state]);
                    break;
                case USER_KEY_ID5:
                    ESP_LOGI(BUTTONTAG, "[ * ] Button VOL+ %s", btn_states[state]);
                    break;
                default:
                    ESP_LOGE(BUTTONTAG, "[ * ] Not supported button id: %d)", button_id);
            }
        }
    }

    ESP_LOGI(BUTTONTAG, "[ 4 ] Stop & destroy all peripherals and event interface");
    esp_periph_set_destroy(set);
    audio_event_iface_destroy(evt);
}

