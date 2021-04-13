#include "lcd-menu.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdio.h>
#include "cJSON.h"

#include "clock-sync.h"
#include "http_request.h"
#include "audio-board.h"
#include "alarm.h"

#define MENUTAG "menu"

// Default menu event functions
void enter_menu_item(void);
void exit_menu_item(void);

// Radio menu event functions
void enter_radio_channel(void);
void next_channel(void);
void previous_channel(void);
void ok_press_radio_channel(void);

// SD menu event functions
void enter_SD_play(void);
void next_song(void);
void previous_song(void);
void ok_press_SD_play(void);

// Agenda menu event functions
void enter_agenda_new_time(void);
void decrease_agenda_new_time(void);
void increase_agenda_new_time(void);
void enter_agenda_new_sound(void);
void previous_agenda_new_sound(void);
void next_agenda_new_sound(void);
void ok_press_agenda_new_sound(void);
void leave_agenda_new_menu(void);
void add_agenda_new_menu(void);
void clear_agenda(void);

// Settings menu event funtions
void enter_settings_volume(void);
void increase_volume(void);
void decrease_volume(void);

// Helper functions
void create_custom_characters(void);
void goto_menu_item(int menuItem);
void reset_temp_agenda_variables(void);

void menu_display_mic();
void menu_clear_mic();

// Variables for Settings menu
static int volume = 0;

// Variables for SD menu 
char** songList;
int songIndex = 0;

// Variables for Radio menu
int channelIndex = 0;

// Variables for Agenda menu
int *tempSelectedTime;
char *tempSelectedSong;

// Variables for all menu's
bool isListening = false;
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
menu_t *menu_create_menu()
{
    // Allocate the memory for the menu_t struct
    menu_t *menuPointer = malloc(sizeof(menu_t));

    // We build the array of menu items with a pointer on the stack
    // so we can later copy this to the menu_t struct
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
        {MENU_MAIN_ID_0, {MENU_RADIO_ID_0, MENU_MAIN_ID_3, MENU_MAIN_ID_1}, {"MAIN MENU", "Radio"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_MAIN_ID_1, {MENU_SD_ID_0, MENU_MAIN_ID_0, MENU_MAIN_ID_2}, {"MAIN MENU", "SD"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_MAIN_ID_2, {MENU_AGENDA_ID_0, MENU_MAIN_ID_1, MENU_MAIN_ID_3}, {"MAIN MENU", "Agenda"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_MAIN_ID_3, {MENU_SETTINGS_ID_0, MENU_MAIN_ID_2, MENU_MAIN_ID_0}, {"MAIN MENU", "Settings"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        
        {MENU_RADIO_ID_0, {MENU_RADIO_ID_2, MENU_RADIO_ID_1, MENU_RADIO_ID_1}, {"RADIO", "Channel"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_RADIO_ID_1, {MENU_MAIN_ID_0, MENU_RADIO_ID_0, MENU_RADIO_ID_0}, {"RADIO", "Back"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        
        {MENU_RADIO_ID_2, {MENU_RADIO_ID_2, MENU_RADIO_ID_2, MENU_RADIO_ID_2}, {"CHANNEL", " ", "", ""}, {ok_press_radio_channel, previous_channel, next_channel}, enter_radio_channel, NULL},

        {MENU_SD_ID_0, {MENU_SD_ID_2, MENU_SD_ID_1, MENU_SD_ID_1}, {"SD", "Play"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_SD_ID_1, {MENU_MAIN_ID_1, MENU_SD_ID_0, MENU_SD_ID_0}, {"SD", "Back"}, {NULL, NULL, NULL}, enter_menu_item, NULL},

        {MENU_SD_ID_2, {MENU_SD_ID_2, MENU_SD_ID_2, MENU_SD_ID_2}, {"SD", " ", "", ""}, {ok_press_SD_play, previous_song, next_song}, enter_SD_play, NULL},
    
        {MENU_AGENDA_ID_0, {MENU_AGENDA_ID_3, MENU_AGENDA_ID_2, MENU_AGENDA_ID_1}, {"AGENDA", "New"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_AGENDA_ID_1, {MENU_AGENDA_ID_1, MENU_AGENDA_ID_0, MENU_AGENDA_ID_2}, {"AGENDA", "Clear"}, {clear_agenda, NULL, NULL}, enter_menu_item, NULL},
        {MENU_AGENDA_ID_2, {MENU_MAIN_ID_2, MENU_AGENDA_ID_1, MENU_AGENDA_ID_0}, {"AGENDA", "Back"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        
        {MENU_AGENDA_ID_3, {MENU_AGENDA_ID_7, MENU_AGENDA_ID_6, MENU_AGENDA_ID_4}, {"NEW", "Time"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_AGENDA_ID_4, {MENU_AGENDA_ID_8, MENU_AGENDA_ID_3, MENU_AGENDA_ID_5}, {"NEW", "Sound"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_AGENDA_ID_5, {MENU_AGENDA_ID_5, MENU_AGENDA_ID_4, MENU_AGENDA_ID_6}, {"NEW", "Add"}, {add_agenda_new_menu, NULL, NULL}, enter_menu_item, NULL},
        {MENU_AGENDA_ID_6, {MENU_AGENDA_ID_6, MENU_AGENDA_ID_5, MENU_AGENDA_ID_3}, {"NEW", "Back"}, {leave_agenda_new_menu, NULL, NULL}, enter_menu_item, NULL},

        {MENU_AGENDA_ID_7, {MENU_AGENDA_ID_3, MENU_AGENDA_ID_7, MENU_AGENDA_ID_7}, {"TIME", " ", "", ""}, {NULL, decrease_agenda_new_time, increase_agenda_new_time}, enter_agenda_new_time, NULL},
        {MENU_AGENDA_ID_8, {MENU_AGENDA_ID_8, MENU_AGENDA_ID_8, MENU_AGENDA_ID_8}, {"SOUND", " ", "", ""}, {ok_press_agenda_new_sound, previous_agenda_new_sound, next_agenda_new_sound}, enter_agenda_new_sound, NULL},
    
        {MENU_SETTINGS_ID_0, {MENU_SETTINGS_ID_2, MENU_SETTINGS_ID_1, MENU_SETTINGS_ID_1}, {"SETTINGS", "Volume"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_SETTINGS_ID_1, {MENU_MAIN_ID_3, MENU_SETTINGS_ID_0, MENU_SETTINGS_ID_0}, {"SETTINGS", "Back"}, {NULL, NULL, NULL}, enter_menu_item, NULL},

        {MENU_SETTINGS_ID_2, {MENU_SETTINGS_ID_0, MENU_SETTINGS_ID_2, MENU_SETTINGS_ID_2}, {"VOLUME", " ", "", ""}, {NULL, decrease_volume, increase_volume}, enter_settings_volume, NULL}
    };
    
    // Init lcd
    _lcd_info = lcd_init();
    
    // If allocation is succesful, set all values
    if(menuPointer != NULL)
    {
        // Initialize menu with values
        menuPointer->lcd_info = _lcd_info;
        menuPointer->menuItems = calloc(MAX_MENU_ITEMS, sizeof(menu_item_t));
        memcpy(menuPointer->menuItems, menuItems, MAX_MENU_ITEMS * sizeof(menu_item_t));
        menuPointer->currentMenuItemId = MENU_MAIN_ID_0;
        
        // Set global variable
        _menu = menuPointer;

        ESP_LOGD(MENUTAG, "malloc menu_t %p", menuPointer);
    }
    else 
    {
        ESP_LOGD(MENUTAG, "malloc menu_t failed");
    }

    // Retrieve all songs from sdcard-player
    get_all_songs_from_SDcard("m");
    char **tempSongList = get_song_list();
    songList = calloc(get_array_size() + 1, 80);
    songList[0] = "Back";
    for(int i = 1; i < get_array_size() + 1; i++){
        songList[i] = tempSongList[i - 1];
    }

    // Init agenda temp variables
    tempSelectedSong = calloc(1, 30);
    tempSelectedTime = calloc(2, sizeof(int));
    tempSelectedTime[0] = 0;
    tempSelectedTime[1] = 0;

    // Add custom character to the lcd
    create_custom_characters();

    return menuPointer;
}

// Creates a custom mic icon for the lcd menu
void create_custom_characters(){

  uint8_t mic[8] = {
    0B00000,
    0B00000,
    0B01110,
    0B01110,
    0B01110,
    0B01110,
    0B00100,
    0B01110
    };

    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_0, mic);
}

// Frees memory used by menu pointer
void menu_free_menu(menu_t *menu)
{
    // Free menu
    free(menu->lcd_info);
    menu->lcd_info = NULL;
    free(menu->menuItems);
    menu->menuItems = NULL;
    free(menu);
    menu = NULL;

    // Free song list
    free(songList);
    songList = NULL;

    // Free temp variables
    free(tempSelectedSong);
    tempSelectedSong = NULL;
    free(tempSelectedTime);
    tempSelectedTime = NULL;
}

// Displays a welcome message on lcd
void menu_display_welcome_message(menu_t *menu)
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
void menu_display_time(char *time)
{
    i2c_lcd1602_move_cursor(_lcd_info, 15, 0);
    i2c_lcd1602_write_string(_lcd_info, time);
}

// Displays temperature in top left corner of lcd
void menu_display_temperature(char* response){
    if(response == NULL)
        return;

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
void menu_display_menu_item(menu_t *menu, int menuItemId)
{
    i2c_lcd1602_clear(menu->lcd_info);

    // Dump all text of the menu item underneath each other
    for (size_t line = 0; line < MAX_LCD_LINES; line++) {
        const char* menuText = menu->menuItems[menuItemId].menuText[line];
        int textPosition = 10 - ((strlen(menuText) + 1) / 2);
        i2c_lcd1602_move_cursor(menu->lcd_info, textPosition, line);
        i2c_lcd1602_write_string(menu->lcd_info, menuText);
    }
}

// Writes an menu item on given line in the center of the lcd
void menu_write_scroll_menu_item(i2c_lcd1602_info_t *lcd_info, char* text, int line)
{
    // Get center position
    int textPosition = 10 - ((strlen(text) + 1) / 2);

    // Write text
    i2c_lcd1602_move_cursor(lcd_info, textPosition, line);
    i2c_lcd1602_write_string(lcd_info, text);
}

// Displays menu scroll menu on lcd
// by writing currentMenuItem and the item before and after
void menu_display_scroll_menu(menu_t *menu)
{
    i2c_lcd1602_clear(menu->lcd_info);
    
    // Gets title of scroll menu
    char *menuText = menu->menuItems[menu->currentMenuItemId].menuText[0];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 0);

    // Get item before currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT]].menuText[1];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 1);

    // Get currentMenuItem
    menuText = menu->menuItems[menu->currentMenuItemId].menuText[1];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 2);

    // Get item after currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]].menuText[1];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 3);

    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(menu->lcd_info, 17, 2);
    i2c_lcd1602_write_string(menu->lcd_info, cursor);
}

// Handles key press by switching to new item or doing an onKeyEvent
void menu_handle_key_event(menu_t *menu, int key)
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
            menu_display_menu_item(menu, menu->currentMenuItemId);
        } else {
            menu_display_scroll_menu(menu);
        }

        // Call the onMenuEntry event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent)();
        }
    }
}

// Displays a list of items and updates the index 
void display_selectable_menu_items(char* menuHeader, int* index, char** itemList, int itemlistSize){
    i2c_lcd1602_clear(_lcd_info);

    // Display scroll menu title
    menu_write_scroll_menu_item(_lcd_info, menuHeader, 0);

    // Loop back around if radioIndex exeeds AMOUNT_OF_RADIO_CHANNELS size
    if(*index + 1 > itemlistSize + 1){
        *index = 0;
    } else if (*index - 1 < -1) {
        *index = itemlistSize;
    }

    // Get the item index before current selected song
    // if below 0 loop back to top
    int previousItemIndex = *index - 1 < 0? itemlistSize : *index - 1;
    char *menuText = itemList[previousItemIndex];
    // Display item with index - 1
    menu_write_scroll_menu_item(_lcd_info, menuText, 1);

    // Display item with index
    menuText = itemList[*index];
    menu_write_scroll_menu_item(_lcd_info, menuText, 2);

    // Get the item index after current selected song
    // if after itemListSize loop back to beginning
    int nextItemIndex = *index + 1 > itemlistSize? 0 : *index + 1;
    menuText = itemList[nextItemIndex];
    // Display item with index + 1
    menu_write_scroll_menu_item(_lcd_info, menuText, 3);  
    
    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(_lcd_info, 17, 2);
    i2c_lcd1602_write_string(_lcd_info, cursor);

    // Default menu enter event
    enter_menu_item();
}

// Default enter event, displays time
void enter_menu_item(void) {
    if(isListening)
        menu_display_mic();

    menu_display_time(clock_getTimeString());
}

// Radio channel enter event
void enter_radio_channel(void){
    enter_menu_item();

    display_selectable_menu_items("CHANNEL", &channelIndex, radioChannelNames, AMOUNT_OF_RADIO_CHANNELS);
}
// Radio channel right press event
void next_channel(void){
    channelIndex++;
    display_selectable_menu_items("CHANNEL", &channelIndex, radioChannelNames, AMOUNT_OF_RADIO_CHANNELS);
}
// Radio channel left press event
void previous_channel(void){
    channelIndex--;
    display_selectable_menu_items("CHANNEL", &channelIndex, radioChannelNames, AMOUNT_OF_RADIO_CHANNELS);
}
void ok_press_radio_channel(){
    // User pressed back
    if(channelIndex == 0){
        // Switch back to radio menu
        _menu->currentMenuItemId = MENU_RADIO_ID_0;

        menu_display_scroll_menu(_menu);

        if(_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
            (*_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent)();
        }

    } else {
        // else play selected radio channel
        play_radio(channelIndex - 1);
    }
}

// SD play enter event
void enter_SD_play(void){
    enter_menu_item();

    display_selectable_menu_items("SD", &songIndex, songList, get_array_size());
}
// SD play right press event 
void next_song(void){
    songIndex++;
    display_selectable_menu_items("SD", &songIndex, songList, get_array_size());
}
// SD play left press event
void previous_song(void){
    songIndex--;
    display_selectable_menu_items("SD", &songIndex, songList, get_array_size());
}
// SD play ok press event
void ok_press_SD_play(){
    // If pressed Back, go back to SD scroll menu
    if(strcmp(songList[songIndex], "Back") == 0){
       goto_menu_item(MENU_SD_ID_0);
    } else {
        // else play selected song
        play_song_with_ID(songList[songIndex], "m");
    }
}

// Agenda new time enter event
void enter_agenda_new_time(void){
    enter_menu_item();

    tempSelectedTime = clock_getCurrentTime();

    i2c_lcd1602_move_cursor(_lcd_info, 8, 1);
    i2c_lcd1602_write_string(_lcd_info, clock_getTimeString());
}
// Agenda new time left event
void decrease_agenda_new_time(void){
    // Remove a minute and remove an hour if necessary
    tempSelectedTime[1]--;
    if(tempSelectedTime[1] < 0) {
        tempSelectedTime[0]--;
        tempSelectedTime[1] = 59;
        if (tempSelectedTime[0] < 0){
            tempSelectedTime[0] = 23;
        }
    }

    char *selectedTimeString = calloc(1, 6);
    sprintf(selectedTimeString, "%02d:%02d", tempSelectedTime[0], tempSelectedTime[1]);

    i2c_lcd1602_move_cursor(_lcd_info, 8, 1);
    i2c_lcd1602_write_string(_lcd_info, selectedTimeString);

    free(selectedTimeString);
}
// Agenda new time right event
void increase_agenda_new_time(void){
    // Add a minute and add an hour if necessary 
    tempSelectedTime[1]++;
    if(tempSelectedTime[1] > 59) {
        tempSelectedTime[0]++;
        tempSelectedTime[1] = 0;
        if (tempSelectedTime[0] > 23){
            tempSelectedTime[0] = 0;
        }
    }

    char *selectedTimeString = calloc(1, 6);
    sprintf(selectedTimeString, "%02d:%02d", tempSelectedTime[0], tempSelectedTime[1]);

    i2c_lcd1602_move_cursor(_lcd_info, 8, 1);
    i2c_lcd1602_write_string(_lcd_info, selectedTimeString);

    free(selectedTimeString);
}
// Agenda new sound enter event
void enter_agenda_new_sound(void){
    enter_menu_item();

    display_selectable_menu_items("SOUND", &songIndex, songList, get_array_size());
}
// Agenda new sound left event
void previous_agenda_new_sound(void){
    songIndex--;
    display_selectable_menu_items("SOUND", &songIndex, songList, get_array_size());
}
// Agenda new sound right event
void next_agenda_new_sound(void){
    songIndex++;
    display_selectable_menu_items("SOUND", &songIndex, songList, get_array_size());
}
// Agenda new sound ok press event
void ok_press_agenda_new_sound(void){
    // If a song was pressed set this as the selected song
    if(strcmp(songList[songIndex], "Back") != 0){
        tempSelectedSong = songList[songIndex];
    }

    // Return to New Agenda menu
    goto_menu_item(MENU_AGENDA_ID_4);
}
// Agenda new leave event
void leave_agenda_new_menu(void){
    // Reset temp variables
    reset_temp_agenda_variables();

    // Return to Agenda menu
    goto_menu_item(MENU_AGENDA_ID_0);
}
// Agenda add ok press event
void add_agenda_new_menu(void){
    // Add alarm to list
    alarm_add(tempSelectedTime, tempSelectedSong);

    // Reset temp variables
    reset_temp_agenda_variables();

    // Return to Agenda menu
    goto_menu_item(MENU_AGENDA_ID_0);
}
// Clears list in agenda.c
void clear_agenda(void){
    clear_global_list();
}

// Resets temp agenda variables for adding new alarms
void reset_temp_agenda_variables(void){
    tempSelectedTime[0] = 0;
    tempSelectedTime[1] = 0;
}

// Goes to given menu item
void goto_menu_item(int menuItem){
    _menu->currentMenuItemId = menuItem;

    menu_display_scroll_menu(_menu);

    // Call entry event
    if(_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
        (*_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent)();
    }
}

// Settings volume enter event
void enter_settings_volume(void){
    enter_menu_item();

    char volumeStr[5];
    sprintf(volumeStr, "%03d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);
}
// Volume right press event
void increase_volume(void){
    volume++;
    if(volume > 100) volume = 100;
    char volumeStr[5];
    sprintf(volumeStr, "%03d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);

    set_volume(volume);
}
// Volume left press event
void decrease_volume(void){
    volume--;
    if(volume < 0) volume = 0;
    char volumeStr[5];
    sprintf(volumeStr, "%03d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);

    set_volume(volume);
}

// Displays mic icon on screen
void menu_display_mic(void){
    i2c_lcd1602_move_cursor(_lcd_info, 0, 1);
    i2c_lcd1602_write_custom_char(_lcd_info, I2C_LCD1602_INDEX_CUSTOM_0); //Mic character
}

// Removes mic icon from screen
void menu_clear_mic(void){
    i2c_lcd1602_move_cursor(_lcd_info, 0, 1);
    i2c_lcd1602_write_char(_lcd_info, ' '); // Clear mic character
}

// Notifies this file that the board has started or stopped listening
void menu_mic(bool listening){
    if(listening)
        menu_display_mic();
    else
        menu_clear_mic();

    isListening = listening;
}