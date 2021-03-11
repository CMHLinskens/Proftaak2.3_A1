#ifndef lcd_menu_H
#define lcd_menu_H

#include "i2c-lcd1602.h"

//init
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

// Menu size settings
#define MAX_MENU_ITEMS 13
#define MAX_MENU_KEYS 3
#define MAX_LCD_LINES 4

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

#define MENU_RADIO_ID_0 5
#define MENU_RADIO_ID_1 6
#define MENU_RADIO_ID_2 7
#define MENU_RADIO_ID_3 8
#define MENU_RADIO_ID_4 9

#define MENU_SD_ID_0 10
#define MENU_SD_ID_1 11
#define MENU_SD_ID_2 12

#define MENU_AGENDA_ID_0 12
#define MENU_AGENDA_ID_1 13
#define MENU_AGENDA_ID_2 14

#define MENU_SETTINGS_ID_0 15
#define MENU_SETTINGS_ID_1 16
#define MENU_SETTINGS_ID_2 17

#define MENU_LIGHTS_ID_0 18
#define MENU_LIGHTS_ID_1 19
#define MENU_LIGHTS_ID_2 20

typedef struct {
    unsigned int id;
    unsigned int otherIds[MAX_MENU_KEYS];
    char *menuText[MAX_LCD_LINES];
    void (*fpOnKeyEvent[MAX_MENU_KEYS])(void);
    void (*fpOnMenuEntryEvent)(void);
    void (*fpOnMenuExitEvent)(void);
} menu_item_t;

typedef struct {
    i2c_lcd1602_info_t *lcd_info;
    menu_item_t *menuItems;
    unsigned int currentMenuItemId;
} menu_t;

void i2c_master_init(void);
i2c_lcd1602_info_t * lcd_init();
menu_t *menu_createMenu();
void menu_freeMenu(menu_t *menu);
void menu_displayTime(char *time);
void menu_displayDateTime(menu_t *menu, char* time, char* date);
void menu_displayWelcomeMessage(menu_t *menu);
void menu_displayScrollMenu(menu_t *menu);
void menu_displayMenuItem(menu_t *menu, int menuItemId);
void menu_handleKeyEvent(menu_t *menu, int key);

#endif // lcd-menu