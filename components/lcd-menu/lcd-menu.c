#include "lcd-menu.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdio.h>

#define MENUTAG "menu"

// menu_item_t *menu_createMenuItem(unsigned int id, unsigned int otherIds[], char* menuText[])
// {
//     menu_item_t *menuItem = malloc(sizeof(menu_item_t));
//     menuItem.id = id;
//     menuItem.otherIds = otherIds;
//     menuItem.menuText = menuText;
//     return menuItem;
// }

menu_t *menu_createMenu(i2c_lcd1602_info_t *lcd_info)
{
    menu_t *menuPointer = malloc(sizeof(menu_t));

    // Temporary array of menu items to copy from
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
        {MENU_MAIN_ID_0, {MENU_MAIN_ID_0, MENU_MAIN_ID_2, MENU_MAIN_ID_1}, {"MAIN MENU"}},
        {MENU_MAIN_ID_1, {MENU_MAIN_ID_1, MENU_MAIN_ID_0, MENU_MAIN_ID_2}, {"RADIO"}},
        {MENU_MAIN_ID_2, {MENU_MAIN_ID_2, MENU_MAIN_ID_1, MENU_MAIN_ID_0}, {"SETTINGS"}}
    };
    
    if(menuPointer != NULL)
    {
        // Initialize menu with values
        menuPointer->lcd_info = lcd_info;
        menuPointer->menuItems = calloc(MAX_MENU_ITEMS, sizeof(menu_item_t));
        memcpy(menuPointer->menuItems, menuItems, MAX_MENU_ITEMS * sizeof(menu_item_t));
        menuPointer->currentMenuItemId = MENU_MAIN_ID_0;

        ESP_LOGI(MENUTAG, "malloc menu_t %p", menuPointer);
        ESP_LOGI(MENUTAG, "currentMenuItemId: %d", menuPointer->currentMenuItemId);
        ESP_LOGI(MENUTAG, "stored ids: %u, %u, %u", menuPointer->menuItems[menuPointer->currentMenuItemId].otherIds[MENU_KEY_OK], menuPointer->menuItems[menuPointer->currentMenuItemId].otherIds[MENU_KEY_LEFT], menuPointer->menuItems[menuPointer->currentMenuItemId].otherIds[MENU_KEY_RIGHT]);
        ESP_LOGI(MENUTAG, "menuItems address: %p", menuPointer->menuItems);
        ESP_LOGI(MENUTAG, "menuItem Ids: %u, %u, %u", menuPointer->menuItems[MENU_MAIN_ID_0].id, menuPointer->menuItems[MENU_MAIN_ID_1].id, menuPointer->menuItems[MENU_MAIN_ID_2].id);
    }
    else 
    {
        ESP_LOGI(MENUTAG, "malloc menu_t failed");
    }
    return menuPointer;
}

void menu_freeMenu(menu_t *menu)
{
    free(menu->lcd_info);
    menu->lcd_info = NULL;
    free(menu->menuItems);
    menu->menuItems = NULL;
    free(menu);
    menu = NULL;
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
    ESP_LOGI(MENUTAG, "current id: %u", menu->currentMenuItemId);
    ESP_LOGI(MENUTAG, "stored ids: %u, %u, %u", menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_OK], menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT], menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]);
    ESP_LOGI(MENUTAG, "stored id: %p", &menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]);
    ESP_LOGI(MENUTAG, "menuItems address: %p", menu->menuItems);
    ESP_LOGI(MENUTAG, "menuItem Ids: %u, %u, %u", menu->menuItems[MENU_MAIN_ID_0].id, menu->menuItems[MENU_MAIN_ID_1].id, menu->menuItems[MENU_MAIN_ID_2].id);

    switch(key){
        case MENU_KEY_OK:
        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_OK];
        break;
        case MENU_KEY_LEFT:
        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT];
        break;
        case MENU_KEY_RIGHT:
        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT];
        break;
    }
    ESP_LOGI(MENUTAG, "new id: %u", menu->currentMenuItemId);
    menu_displayMenuItem(menu, menu->currentMenuItemId);
}