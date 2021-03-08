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
#include "lcd-menu.h"

//Button 2
#include "esp_peripherals.h"
#include "input_key_service.h"
#include "periph_adc_button.h"

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
#define BUTTONTAG "touchpad button"

char *btn_states[] = {  "INPUT_KEY_SERVICE_ACTION_UNKNOWN",        /*!< unknown action id */
                        "INPUT_KEY_SERVICE_ACTION_CLICK",          /*!< click action id */
                        "INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE",  /*!< click release action id */
                        "INPUT_KEY_SERVICE_ACTION_PRESS",          /*!< long press action id */
                        "INPUT_KEY_SERVICE_ACTION_PRESS_RELEASE"   /*!< long press release id */
                      };
                      
menu_t *menu;

//touch pad init.
void touch_pad_buttons_init();
//button handler/callback.
static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx);


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

void touch_pad_buttons_init(){

    // step 1. Initalize the peripherals set.
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    periph_cfg.extern_stack = true;
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // step 2. Setup the input key service
    audio_board_key_init(set);
    input_key_service_info_t input_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t key_serv_info = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    key_serv_info.based_cfg.extern_stack = false;
    key_serv_info.handle = set;
    periph_service_handle_t input_key_handle = input_key_service_create(&key_serv_info);
    AUDIO_NULL_CHECK(BUTTONTAG, input_key_handle, return);
    input_key_service_add_key(input_key_handle, input_info, INPUT_KEY_NUM); //INPUT_KEY_NUM = 4 (the touch pads on ESP32-LyraT board)
    periph_service_set_callback(input_key_handle, input_key_service_cb, NULL);
}

//callback for button presses.
static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    ESP_LOGD(BUTTONTAG, "type=%d, source=%d, data=%d, len=%d", evt->type, (int)evt->source, (int)evt->data, evt->len);
    switch ((int)evt->data) {
        case INPUT_KEY_USER_ID_PLAY:
            //if (evt->type == INPUT_KEY_SERVICE_ACTION_PRESS) {
            //ESP_LOGI(BUTTONTAG, "[ * ] [play] %s",btn_states[evt->type]);
            //}
            ESP_LOGI(BUTTONTAG, "[ * ] [set] %s",btn_states[evt->type]);
            break;
        case INPUT_KEY_USER_ID_SET:
            // if (evt->type == INPUT_KEY_SERVICE_ACTION_PRESS) {
            //     ESP_LOGI(BUTTONTAG, "[ * ] [ID SET] %d",evt->type);
            // }
            ESP_LOGI(BUTTONTAG, "[ * ] [set] %s",btn_states[evt->type]);
            break;
        case INPUT_KEY_USER_ID_VOLDOWN:
            if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK) {
                menu_handleKeyEvent(menu, MENU_KEY_LEFT);
            }
            ESP_LOGI(BUTTONTAG, "[ * ] [volume down] %s",btn_states[evt->type]);
            break;
        case INPUT_KEY_USER_ID_VOLUP:
            if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK) {
                menu_handleKeyEvent(menu, MENU_KEY_RIGHT);
            }
            ESP_LOGI(BUTTONTAG, "[ * ] [volume up] %s",btn_states[evt->type]);
        default:
            break;
    }
    return ESP_OK;
}


void app_main()
{
 
    i2c_master_init();
    menu = menu_createMenu(lcd_init());

    menu_displayWelcomeMessage(menu);
    menu_displayScrollMenu(menu);
    vTaskDelay(2500 / portTICK_RATE_MS);

    touch_pad_buttons_init();

}