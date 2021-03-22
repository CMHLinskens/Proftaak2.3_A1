#include "lcd-menu.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdio.h>
#include "cJSON.h"

#include "clock-sync.h"
#include "http_request.h"
#include "sdcard_player.h"

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
void enterSDPlay(void);
void nextSong(void);
void previousSong(void);
void okPressSDPlay(void);

// Placeholder variables
static int volume = 0;
static int channel = 0;

// Variables for SD menu 
char** songList;
int songIndex = 0;

static i2c_lcd1602_info_t *_lcd_info;
static menu_t *_menu;

// Inits the lcd and returns struct with lcd info for lcd usage
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
    ESP_LOGI(MENUTAG, "backlight off");
    i2c_lcd1602_set_backlight(lcd_info, false);

    // turn on backlight
    ESP_LOGI(MENUTAG, "backlight on");
    i2c_lcd1602_set_backlight(lcd_info, true);

    // turn on cursor 
    ESP_LOGI(MENUTAG, "cursor on");
    i2c_lcd1602_set_cursor(lcd_info, true);

    return lcd_info;
}

// Creates and returnes a pointer to the menu 
menu_t *menu_createMenu()
{
    menu_t *menuPointer = malloc(sizeof(menu_t));

    // Temporary array of menu items to copy from
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
        {MENU_MAIN_ID_0, {MENU_RADIO_ID_0, MENU_MAIN_ID_4, MENU_MAIN_ID_1}, {"MAIN MENU", "Radio"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_MAIN_ID_1, {MENU_SD_ID_0, MENU_MAIN_ID_0, MENU_MAIN_ID_2}, {"MAIN MENU", "SD"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_MAIN_ID_2, {MENU_MAIN_ID_2, MENU_MAIN_ID_1, MENU_MAIN_ID_3}, {"MAIN MENU", "Lights"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_MAIN_ID_3, {MENU_MAIN_ID_3, MENU_MAIN_ID_2, MENU_MAIN_ID_4}, {"MAIN MENU", "Agenda"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_MAIN_ID_4, {MENU_MAIN_ID_4, MENU_MAIN_ID_3, MENU_MAIN_ID_0}, {"MAIN MENU", "Settings"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        
        {MENU_RADIO_ID_0, {MENU_RADIO_ID_4, MENU_RADIO_ID_2, MENU_RADIO_ID_1}, {"RADIO", "Channel"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_RADIO_ID_1, {MENU_RADIO_ID_3, MENU_RADIO_ID_0, MENU_RADIO_ID_2}, {"RADIO", "Volume"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_RADIO_ID_2, {MENU_MAIN_ID_0, MENU_RADIO_ID_1, MENU_RADIO_ID_0}, {"RADIO", "Back"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        
        {MENU_RADIO_ID_3, {MENU_RADIO_ID_1, MENU_RADIO_ID_3, MENU_RADIO_ID_3}, {"VOLUME", " ", "", ""}, {NULL, decreaseVolume, increaseVolume}, enterRadioVolume, NULL},
        {MENU_RADIO_ID_4, {MENU_RADIO_ID_0, MENU_RADIO_ID_4, MENU_RADIO_ID_4}, {"CHANNEL", " ", "", ""}, {NULL, decreaseChannel, increaseChannel}, enterRadioChannel, NULL},

        {MENU_SD_ID_0, {MENU_SD_ID_2, MENU_SD_ID_1, MENU_SD_ID_1}, {"SD", "Play"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_SD_ID_1, {MENU_MAIN_ID_1, MENU_SD_ID_0, MENU_SD_ID_0}, {"SD", "Back"}, {NULL, NULL, NULL}, enterMenuItem, NULL},

        {MENU_SD_ID_2, {MENU_SD_ID_2, MENU_SD_ID_2, MENU_SD_ID_2}, {"SD", " ", "", ""}, {okPressSDPlay, previousSong, nextSong}, enterSDPlay, NULL}
    };
    
    _lcd_info = lcd_init();
    
    // If allocation is succesful, set all values
    if(menuPointer != NULL)
    {
        // Initialize menu with values
        menuPointer->lcd_info = _lcd_info;
        menuPointer->menuItems = calloc(MAX_MENU_ITEMS, sizeof(menu_item_t));
        memcpy(menuPointer->menuItems, menuItems, MAX_MENU_ITEMS * sizeof(menu_item_t));
        menuPointer->currentMenuItemId = MENU_MAIN_ID_0;

        _menu = menuPointer;

        ESP_LOGD(MENUTAG, "malloc menu_t %p", menuPointer);
    }
    else 
    {
        ESP_LOGD(MENUTAG, "malloc menu_t failed");
    }

    // Retrieve all songs from sdcard-player
    char **tempSongList = getSongList();
    songList = calloc(25, 80);
    songList[0] = "Back";
    for(int i = 1; i < 25; i++){
        songList[i] = tempSongList[i - 1];
    }

    return menuPointer;
}

// Frees memory used by menu pointer
void menu_freeMenu(menu_t *menu)
{
    free(menu->lcd_info);
    menu->lcd_info = NULL;
    free(menu->menuItems);
    menu->menuItems = NULL;
    free(menu);
    menu = NULL;
}

// Displays a welcome message on lcd
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

// Displays time in top right corner of lcd
void menu_displayTime(char *time)
{
    i2c_lcd1602_move_cursor(_lcd_info, 15, 0);
    i2c_lcd1602_write_string(_lcd_info, time);
}

void menu_displayTemperature(char* response){

    cJSON *root = cJSON_Parse(response);
    cJSON *maan = cJSON_GetObjectItem(root, "main");
    double temp = cJSON_GetObjectItem(maan,"temp")->valuedouble;
    int temp_in_c = (int) temp;
    temp_in_c -= 272;

    char temp_in_string[50] = {0};
    sprintf(temp_in_string,"%dC",temp_in_c);

    i2c_lcd1602_move_cursor(_lcd_info, 0, 0);
    i2c_lcd1602_write_string(_lcd_info, &temp_in_string[0]);
}

// Displays menu all lines from menu item on lcd 
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

// Writes an menu item on given line
void menu_writeScrollMenuItem(i2c_lcd1602_info_t *lcd_info, char* text, int line)
{
    int textPosition = 10 - ((strlen(text) + 1) / 2);
    i2c_lcd1602_move_cursor(lcd_info, textPosition, line);
    i2c_lcd1602_write_string(lcd_info, text);
}

// Displays menu scroll menu on lcd
// by writing currentMenuItem and the item before and after
void menu_displayScrollMenu(menu_t *menu)
{
    i2c_lcd1602_clear(menu->lcd_info);

    // Gets title of scroll menu
    char *menuText = menu->menuItems[menu->currentMenuItemId].menuText[0];
    menu_writeScrollMenuItem(menu->lcd_info, menuText, 0);

    // Get item before currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT]].menuText[1];
    menu_writeScrollMenuItem(menu->lcd_info, menuText, 1);

    // Get currentMenuItem
    menuText = menu->menuItems[menu->currentMenuItemId].menuText[1];
    menu_writeScrollMenuItem(menu->lcd_info, menuText, 2);

    // Get item after currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]].menuText[1];
    menu_writeScrollMenuItem(menu->lcd_info, menuText, 3);

    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(menu->lcd_info, 17, 2);
    i2c_lcd1602_write_string(menu->lcd_info, cursor);
}

// Handles key press by switching to new item or doing an onKeyEvent
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

// Displays songsList in a scroll menu form
void displaySongs(){
    i2c_lcd1602_clear(_lcd_info);

    // Display scroll menu title
    char *menuText = "SD";
    menu_writeScrollMenuItem(_lcd_info, menuText, 0);

    // Loop back around if songIndex exeeds songList size
    if(songIndex + 1 > 25){
        songIndex = 0;
    } else if (songIndex - 1 < -1) {
        songIndex = 24;
    }

    // Get the song index before current selected song
    // if below 0 loop back to top
    int previousSongIndex = songIndex - 1 < 0? 24 : songIndex - 1;
    menuText = songList[previousSongIndex];
    menu_writeScrollMenuItem(_lcd_info, menuText, 1);

    menuText = songList[songIndex];
    menu_writeScrollMenuItem(_lcd_info, menuText, 2);

    // Get the song index after current selected song
    // if after 24 loop back to beginning
    int nextSongIndex = songIndex + 1 > 24? 0 : songIndex + 1;
    menuText = songList[nextSongIndex];
    menu_writeScrollMenuItem(_lcd_info, menuText, 3);  
    
    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(_lcd_info, 17, 2);
    i2c_lcd1602_write_string(_lcd_info, cursor);

    menu_displayTime(clock_getTimeString());
}

// Default enter event, displays time
void enterMenuItem(void) {
    menu_displayTemperature(http_request_get_response());
    menu_displayTime(clock_getTimeString());
}
// Radio volume enter event
void enterRadioVolume(void){
    enterMenuItem();

    char volumeStr[3];
    sprintf(volumeStr, "%d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);
}
// Radio volume right press event
void increaseVolume(void){
    volume++;
    if(volume > 100) volume = 100;
    char volumeStr[3];
    sprintf(volumeStr, "%d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);
}
// Radio volume left press event
void decreaseVolume(void){
    volume--;
    if(volume < 0) volume = 0;
    char volumeStr[3];
    sprintf(volumeStr, "%d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);
}

// Radio channel enter event
void enterRadioChannel(void){
    enterMenuItem();

    char channelStr[3];
    sprintf(channelStr, "%d", channel);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, channelStr);
}
// Radio channel right press event
void increaseChannel(void){
    channel++;
    if(channel > 100) channel = 100;
    char channelStr[3];
    sprintf(channelStr, "%d", channel);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, channelStr);
}
// Radio channel left press event
void decreaseChannel(void){
    channel--;
    if(channel < 0) channel = 0;
    char channelStr[3];
    sprintf(channelStr, "%d", channel);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, channelStr);
}

// SD play enter event
void enterSDPlay(void){
    enterMenuItem();

    displaySongs();
}
// SD play right press event 
void nextSong(void){
    songIndex++;
    displaySongs();
}
// SD play left press event
void previousSong(void){
    songIndex--;
    displaySongs();
}
// SD play ok press event
void okPressSDPlay(){
    // If pressed Back, go back to SD scroll menu
    if(strcmp(songList[songIndex], "Back") == 0){
        _menu->currentMenuItemId = MENU_SD_ID_0;

        menu_displayScrollMenu(_menu);

        if(_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
            (*_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent)();
        }

    } else {
        // else play selected song
        play_song_with_ID(songList[songIndex]);
    }
}
