#include "lcd-menu.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdio.h>

#define MENUTAG "menu"

menu_t *menu_createMenu(i2c_lcd1602_info_t *lcd_info)
{
    menu_t *menuPointer = malloc(sizeof(*menuPointer));
    ESP_LOGI(MENUTAG, "malloc menu_t %p", menuPointer);
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
            // menuItems[MENU_MAIN_ID_0]
            MENU_MAIN_ID_0
        };

        *menuPointer = menu;

        ESP_LOGI(MENUTAG, "malloc menu_t %p", menuPointer);
        ESP_LOGI(MENUTAG, "currentMenuItemId: %d", menuPointer->currentMenuItemId);
    }
    else 
    {
        ESP_LOGI(MENUTAG, "malloc menu_t failed");
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

void menu_displayMenuItem(menu_t *menu, int menuItemId)
{
    const char* menuText = *(menu->menuItems[menuItemId]).menuText;
    i2c_lcd1602_clear(menu->lcd_info);
    i2c_lcd1602_move_cursor(menu->lcd_info, 0, 0);
    i2c_lcd1602_write_string(menu->lcd_info, menuText);
}

void menu_handleKeyEvent(menu_t *menu, int key)
{
    ESP_LOGI(MENUTAG, "handleKeyEvent()");
    switch(key){
        case MENU_KEY_OK:
        // menu->currentMenuItem = menu->menuItems[menu->currentMenuItem.otherIds[MENU_KEY_OK]];
        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_OK];
        break;
        case MENU_KEY_LEFT:
        // menu->currentMenuItem = menu->menuItems[menu->currentMenuItem.otherIds[MENU_KEY_LEFT]];
        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT];
        break;
        case MENU_KEY_RIGHT:
        // menu->currentMenuItem = menu->menuItems[menu->currentMenuItem.otherIds[MENU_KEY_RIGHT]];
        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT];
        break;
    }
    ESP_LOGI(MENUTAG, "new id: %u", menu->currentMenuItemId);
    menu_displayMenuItem(menu, menu->currentMenuItemId);
}