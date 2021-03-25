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
#define MAX_MENU_ITEMS 22
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

#define MENU_RADIO_ID_0 4
#define MENU_RADIO_ID_1 5
#define MENU_RADIO_ID_2 6

#define MENU_SD_ID_0 7
#define MENU_SD_ID_1 8
#define MENU_SD_ID_2 9

#define MENU_AGENDA_ID_0 10
#define MENU_AGENDA_ID_1 11
#define MENU_AGENDA_ID_2 12
#define MENU_AGENDA_ID_3 13
#define MENU_AGENDA_ID_4 14
#define MENU_AGENDA_ID_5 15
#define MENU_AGENDA_ID_6 16
#define MENU_AGENDA_ID_7 17
#define MENU_AGENDA_ID_8 18

#define MENU_SETTINGS_ID_0 19
#define MENU_SETTINGS_ID_1 20
#define MENU_SETTINGS_ID_2 21

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
i2c_lcd1602_info_t * lcd_init(void);
menu_t *menu_create_menu(void);
void menu_free_menu(menu_t *menu);
void menu_display_time(char *time);
void menu_display_welcome_message(menu_t *menu);
void menu_display_scroll_menu(menu_t *menu);
void menu_display_menu_item(menu_t *menu, int menuItemId);
void menu_handle_key_event(menu_t *menu, int key);

void menu_mic(bool listening);

#endif // lcd-menu