#ifndef lcd_menu_H
#define lcd_menu_H

#include "i2c-lcd1602.h"

// Menu size settings
#define MAX_MENU_ITEMS 3
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

typedef struct {
    unsigned int id;
    unsigned int otherIds[MAX_MENU_KEYS];
    char *menuText[MAX_LCD_CHARS];
} menu_item_t;

typedef struct {
    i2c_lcd1602_info_t *lcd_info;
    menu_item_t *menuItems;
    // menu_item_t currentMenuItem;
    unsigned int currentMenuItemId;
} menu_t;

menu_t *menu_createMenu(i2c_lcd1602_info_t *lcd_info);
void menu_freeMenu(menu_t *menu);
void menu_displayWelcomeMessage(menu_t *menu);//menu_t *menu);
void menu_displayMenuItem(menu_t *menu, int menuItemId);
void menu_handleKeyEvent(menu_t *menu, int key);

#endif // lcd-menu