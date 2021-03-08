#include "lcd-menu.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdio.h>

#define MENUTAG "menu"

// Menu events functions
void enterMenuItem(void);
void exitMenuItem(void);

menu_t *menu_createMenu(i2c_lcd1602_info_t *lcd_info)
{
    menu_t *menuPointer = malloc(sizeof(menu_t));

    // Temporary array of menu items to copy from
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
        {MENU_MAIN_ID_0, {MENU_MAIN_ID_0, MENU_MAIN_ID_3, MENU_MAIN_ID_1}, {"MAIN MENU", "Radio"}, {NULL, NULL, NULL}, enterMenuItem, exitMenuItem},
        {MENU_MAIN_ID_1, {MENU_MAIN_ID_1, MENU_MAIN_ID_0, MENU_MAIN_ID_2}, {"MAIN MENU", "Lights"}, {NULL, NULL, NULL}, enterMenuItem, exitMenuItem},
        {MENU_MAIN_ID_2, {MENU_MAIN_ID_2, MENU_MAIN_ID_1, MENU_MAIN_ID_3}, {"MAIN MENU", "Agenda"}, {NULL, NULL, NULL}, enterMenuItem, exitMenuItem},
        {MENU_MAIN_ID_3, {MENU_MAIN_ID_3, MENU_MAIN_ID_2, MENU_MAIN_ID_0}, {"MAIN MENU", "Settings"}, {NULL, NULL, NULL}, enterMenuItem, exitMenuItem}
    };
    
    if(menuPointer != NULL)
    {
        // Initialize menu with values
        menuPointer->lcd_info = lcd_info;
        menuPointer->menuItems = calloc(MAX_MENU_ITEMS, sizeof(menu_item_t));
        memcpy(menuPointer->menuItems, menuItems, MAX_MENU_ITEMS * sizeof(menu_item_t));
        menuPointer->currentMenuItemId = MENU_MAIN_ID_0;

        ESP_LOGD(MENUTAG, "malloc menu_t %p", menuPointer);
    }
    else 
    {
        ESP_LOGD(MENUTAG, "malloc menu_t failed");
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

void menu_displayScrollMenu(menu_t *menu)
{
    i2c_lcd1602_clear(menu->lcd_info);
    char *menuText = menu->menuItems[menu->currentMenuItemId].menuText[0];
    int textPosition = 10 - ((strlen(menuText) + 1) / 2);
    i2c_lcd1602_move_cursor(menu->lcd_info, textPosition, 0);
    i2c_lcd1602_write_string(menu->lcd_info, menuText);

    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT]].menuText[1];
    textPosition = 10 - ((strlen(menuText) + 1) / 2);
    i2c_lcd1602_move_cursor(menu->lcd_info, textPosition, 1);
    i2c_lcd1602_write_string(menu->lcd_info, menuText);

    menuText = menu->menuItems[menu->currentMenuItemId].menuText[1];
    textPosition = 10 - ((strlen(menuText) + 1) / 2);
    i2c_lcd1602_move_cursor(menu->lcd_info, textPosition, 2);
    i2c_lcd1602_write_string(menu->lcd_info, menuText);

    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]].menuText[1];
    textPosition = 10 - ((strlen(menuText) + 1) / 2);
    i2c_lcd1602_move_cursor(menu->lcd_info, textPosition, 3);
    i2c_lcd1602_write_string(menu->lcd_info, menuText);

    const char *cursor = "<";
    i2c_lcd1602_move_cursor(menu->lcd_info, 17, 2);
    i2c_lcd1602_write_string(menu->lcd_info, cursor);

}

void menu_handleKeyEvent(menu_t *menu, int key)
{
    // If key press leads to the same ID as the currentMenuItemId
    // do not switch to a new menu item, instead call the onKey event
    if(menu->menuItems[menu->currentMenuItemId].otherIds[key] == menu->currentMenuItemId){
        // Call the onKey event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnKeyEvent[key] != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnKeyEvent[key])();
        }
    } else {
        // Call the onMenuExit event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnMenuExitEvent != NULL) {
        (*menu->menuItems[menu->currentMenuItemId].fpOnMenuExitEvent)();
        }

        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[key];
        ESP_LOGI(MENUTAG, "new id: %u", menu->currentMenuItemId);

        // Call the onMenuEntry event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent)();
        }

        // Display menu on LCD
        // menu_displayMenuItem(menu, menu->currentMenuItemId);
        menu_displayScrollMenu(menu);
    }
}

// Defining menu events functions
void enterMenuItem(void) {
    ESP_LOGI(MENUTAG, "Entered a menu item");
}
void exitMenuItem(void){
    ESP_LOGI(MENUTAG, "Exited a menu item");
}
void okPressRadioEvent(void){
    ESP_LOGI(MENUTAG, "Ok press on radio");
}
void okPressSettingsEvent(void){
    ESP_LOGI(MENUTAG, "Ok press on settings");
}
void leftPressEvent(void){
    ESP_LOGI(MENUTAG, "Left key pressed");
}
void rightPressEvent(void){
    ESP_LOGI(MENUTAG, "Right key pressed");
}