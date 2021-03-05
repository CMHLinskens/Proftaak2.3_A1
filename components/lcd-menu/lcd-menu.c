#include "lcd-menu.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define MENUTAG "menu"

menu_t *menu_createMenu(i2c_lcd1602_info_t *lcd_info)
{
    menu_t *menuPointer = malloc(sizeof(*menuPointer));
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
            {MENU_MAIN_ID_0, {MENU_MAIN_ID_0, MENU_MAIN_ID_2, MENU_MAIN_ID_1}, {"MAIN MENU"}},
            {MENU_MAIN_ID_1, {MENU_MAIN_ID_1, MENU_MAIN_ID_0, MENU_MAIN_ID_2}, {"RADIO"}},
            {MENU_MAIN_ID_2, {MENU_MAIN_ID_2, MENU_MAIN_ID_1, MENU_MAIN_ID_0}, {"SETTINGS"}}
        };
    if(menuPointer != NULL)
    {
        memset(menuPointer, 0, sizeof(*menuPointer));
        
        menu_t menu = { 
            lcd_info,
            menuItems,
            menuItems[0]
        };

        *menuPointer = menu;
        ESP_LOGD(MENUTAG, "malloc menu_t %p", menuPointer);
    }
    else 
    {
        ESP_LOGD(MENUTAG, "malloc menu_t failed");
    }
    return menuPointer;
}

void menu_displayWelcomeMessage(menu_t *menu)
{
    i2c_lcd1602_set_cursor(menu->lcd_info, false);
    i2c_lcd1602_move_cursor(menu->lcd_info, 6, 1);

    i2c_lcd1602_write_string(menu->lcd_info, "Welcome");
    i2c_lcd1602_move_cursor(menu->lcd_info, 8, 2);
    i2c_lcd1602_write_string(menu->lcd_info, "User");

    vTaskDelay(2500 / portTICK_RATE_MS);
    i2c_lcd1602_clear(menu->lcd_info);
}

// void menu_handleKeyEvent(menu_t *menu, int key)
// {

// }