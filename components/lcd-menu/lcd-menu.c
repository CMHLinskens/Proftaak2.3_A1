#include "lcd-menu.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdio.h>

#define MENUTAG "menu"

// Menu event functions
void enterMenuItem(void);
void exitMenuItem(void);
void enterRadioVolume(void);
void enterRadioChannel(void);
void increaseVolume(void);
void decreaseVolume(void);
void increaseChannel(void);
void decreaseChannel(void);

// Placeholder variables
static int volume = 0;
static int channel = 0;

static i2c_lcd1602_info_t *_lcd_info;

menu_t *menu_createMenu(i2c_lcd1602_info_t *lcd_info)
{
    menu_t *menuPointer = malloc(sizeof(menu_t));

    // Temporary array of menu items to copy from
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
        {MENU_MAIN_ID_0, {MENU_RADIO_ID_0, MENU_MAIN_ID_3, MENU_MAIN_ID_1}, {"MAIN MENU", "Radio"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_MAIN_ID_1, {MENU_MAIN_ID_1, MENU_MAIN_ID_0, MENU_MAIN_ID_2}, {"MAIN MENU", "Lights"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_MAIN_ID_2, {MENU_MAIN_ID_2, MENU_MAIN_ID_1, MENU_MAIN_ID_3}, {"MAIN MENU", "Agenda"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_MAIN_ID_3, {MENU_MAIN_ID_3, MENU_MAIN_ID_2, MENU_MAIN_ID_0}, {"MAIN MENU", "Settings"}, {NULL, NULL, NULL}, NULL, NULL},
        
        {MENU_RADIO_ID_0, {MENU_RADIO_ID_4, MENU_RADIO_ID_2, MENU_RADIO_ID_1}, {"RADIO", "Channel"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_RADIO_ID_1, {MENU_RADIO_ID_3, MENU_RADIO_ID_0, MENU_RADIO_ID_2}, {"RADIO", "Volume"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_RADIO_ID_2, {MENU_MAIN_ID_0, MENU_RADIO_ID_1, MENU_RADIO_ID_0}, {"RADIO", "Back"}, {NULL, NULL, NULL}, NULL, NULL},
        
        {MENU_RADIO_ID_3, {MENU_RADIO_ID_1, MENU_RADIO_ID_3, MENU_RADIO_ID_3}, {"VOLUME", " ", "", ""}, {NULL, decreaseVolume, increaseVolume}, enterRadioVolume, NULL},
        {MENU_RADIO_ID_4, {MENU_RADIO_ID_0, MENU_RADIO_ID_4, MENU_RADIO_ID_4}, {"CHANNEL", " ", "", ""}, {NULL, decreaseChannel, increaseChannel}, enterRadioChannel, NULL}
    };
    
    _lcd_info = lcd_info;

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
    i2c_lcd1602_clear(menu->lcd_info);

    for (size_t line = 0; line < MAX_LCD_LINES; line++) {
        const char* menuText = menu->menuItems[menuItemId].menuText[line];
        int textPosition = 10 - ((strlen(menuText) + 1) / 2);
        i2c_lcd1602_move_cursor(menu->lcd_info, textPosition, line);
        i2c_lcd1602_write_string(menu->lcd_info, menuText);
    }
}

void menu_writeScrollMenuItem(i2c_lcd1602_info_t *lcd_info, char* text, int line)
{
    int textPosition = 10 - ((strlen(text) + 1) / 2);
    i2c_lcd1602_move_cursor(lcd_info, textPosition, line);
    i2c_lcd1602_write_string(lcd_info, text);
}

void menu_displayScrollMenu(menu_t *menu)
{
    i2c_lcd1602_clear(menu->lcd_info);

    char *menuText = menu->menuItems[menu->currentMenuItemId].menuText[0];
    menu_writeScrollMenuItem(menu->lcd_info, menuText, 0);

    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT]].menuText[1];
    menu_writeScrollMenuItem(menu->lcd_info, menuText, 1);

    menuText = menu->menuItems[menu->currentMenuItemId].menuText[1];
    menu_writeScrollMenuItem(menu->lcd_info, menuText, 2);

    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]].menuText[1];
    menu_writeScrollMenuItem(menu->lcd_info, menuText, 3);

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

        // Display menu on LCD
        if(strcmp(menu->menuItems[menu->currentMenuItemId].menuText[1], " ") == 0) {
            menu_displayMenuItem(menu, menu->currentMenuItemId);
        } else {
            menu_displayScrollMenu(menu);
        }

        // Call the onMenuEntry event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent)();
        }
    }
}

// Defining menu event functions
void enterRadioVolume(void){
    char volumeStr[3];
    sprintf(volumeStr, "%d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);
}
void increaseVolume(void){
    volume++;
    if(volume > 100) volume = 100;
    char volumeStr[3];
    sprintf(volumeStr, "%d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);
}
void decreaseVolume(void){
    volume--;
    if(volume < 0) volume = 0;
    char volumeStr[3];
    sprintf(volumeStr, "%d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);
}

void enterRadioChannel(void){
    char channelStr[3];
    sprintf(channelStr, "%d", channel);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, channelStr);
}
void increaseChannel(void){
    channel++;
    if(channel > 100) channel = 100;
    char channelStr[3];
    sprintf(channelStr, "%d", channel);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, channelStr);
}
void decreaseChannel(void){
    channel--;
    if(channel < 0) channel = 0;
    char channelStr[3];
    sprintf(channelStr, "%d", channel);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, channelStr);
}
