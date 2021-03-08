#ifndef lcd_menu_H
#define lcd_menu_H

#include "i2c-lcd1602.h"

// Menu size settings
#define MAX_MENU_ITEMS 4
#define MAX_MENU_KEYS 3
#define MAX_LCD_CHARS 80

// Possible key presses
#define MENU_KEY_OK 0
#define MENU_KEY_LEFT 1
#define MENU_KEY_RIGHT 2

// Menu screen IDs
#define MENU_MAIN_ID_0 0
#define MENU_MAIN_ID_1 1
#define MENU_MAIN_ID_2 2
#define MENU_MAIN_ID_3 3
#define MENU_MAIN_ID_4 4
#define MENU_MAIN_ID_5 5

#define MENU_RADIO_ID_0 6
#define MENU_RADIO_ID_1 7
#define MENU_RADIO_ID_2 8

#define MENU_LIGHTS_ID_0 9
#define MENU_LIGHTS_ID_1 10
#define MENU_LIGHTS_ID_2 11

#define MENU_AGENDA_ID_0 12
#define MENU_AGENDA_ID_1 13
#define MENU_AGENDA_ID_2 14

#define MENU_SETTINGS_ID_0 15
#define MENU_SETTINGS_ID_1 16
#define MENU_SETTINGS_ID_2 17


typedef struct {
    unsigned int id;
    unsigned int otherIds[MAX_MENU_KEYS];
    char *menuText[MAX_LCD_CHARS];
    void (*fpOnKeyEvent[MAX_MENU_KEYS])(void);
    void (*fpOnMenuEntryEvent)(void);
    void (*fpOnMenuExitEvent)(void);
} menu_item_t;

typedef struct {
    i2c_lcd1602_info_t *lcd_info;
    menu_item_t *menuItems;
    unsigned int currentMenuItemId;
} menu_t;

menu_t *menu_createMenu(i2c_lcd1602_info_t *lcd_info);
void menu_freeMenu(menu_t *menu);
void menu_displayWelcomeMessage(menu_t *menu);
void menu_displayScrollMenu(menu_t *menu);
void menu_displayMenuItem(menu_t *menu, int menuItemId);
void menu_handleKeyEvent(menu_t *menu, int key);

#endif // lcd-menu