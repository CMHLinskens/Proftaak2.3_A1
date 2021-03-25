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

void create_custom_characters(void);
void goto_menu_item(int menuItem);
void reset_temp_agenda_variables(void);

void menu_display_mic();
void menu_clear_mic();

// Placeholder variables
static int volume = 0;

// Variables for SD menu 
char** songList;
int songIndex = 0;

// Variables for Radio menu
int channelIndex = 0;

// Variables for Agenda menu
int *tempSelectedTime;
char *tempSelectedSong;

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
menu_t *menu_createMenu()
{
    menu_t *menuPointer = malloc(sizeof(menu_t));

    // Temporary array of menu items to copy from
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
        {MENU_MAIN_ID_0, {MENU_RADIO_ID_0, MENU_MAIN_ID_3, MENU_MAIN_ID_1}, {"MAIN MENU", "Radio"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_MAIN_ID_1, {MENU_SD_ID_0, MENU_MAIN_ID_0, MENU_MAIN_ID_2}, {"MAIN MENU", "SD"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_MAIN_ID_2, {MENU_AGENDA_ID_0, MENU_MAIN_ID_1, MENU_MAIN_ID_3}, {"MAIN MENU", "Agenda"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_MAIN_ID_3, {MENU_SETTINGS_ID_0, MENU_MAIN_ID_2, MENU_MAIN_ID_0}, {"MAIN MENU", "Settings"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        
        {MENU_RADIO_ID_0, {MENU_RADIO_ID_2, MENU_RADIO_ID_1, MENU_RADIO_ID_1}, {"RADIO", "Channel"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_RADIO_ID_1, {MENU_MAIN_ID_0, MENU_RADIO_ID_0, MENU_RADIO_ID_0}, {"RADIO", "Back"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        
        {MENU_RADIO_ID_2, {MENU_RADIO_ID_2, MENU_RADIO_ID_2, MENU_RADIO_ID_2}, {"CHANNEL", " ", "", ""}, {okPressRadioChannel, previousChannel, nextChannel}, enterRadioChannel, NULL},

        {MENU_SD_ID_0, {MENU_SD_ID_2, MENU_SD_ID_1, MENU_SD_ID_1}, {"SD", "Play"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_SD_ID_1, {MENU_MAIN_ID_1, MENU_SD_ID_0, MENU_SD_ID_0}, {"SD", "Back"}, {NULL, NULL, NULL}, enterMenuItem, NULL},

        {MENU_SD_ID_2, {MENU_SD_ID_2, MENU_SD_ID_2, MENU_SD_ID_2}, {"SD", " ", "", ""}, {okPressSDPlay, previousSong, nextSong}, enterSDPlay, NULL},
    
        {MENU_AGENDA_ID_0, {MENU_AGENDA_ID_3, MENU_AGENDA_ID_2, MENU_AGENDA_ID_1}, {"AGENDA", "New"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_AGENDA_ID_1, {MENU_AGENDA_ID_1, MENU_AGENDA_ID_0, MENU_AGENDA_ID_2}, {"AGENDA", "Clear"}, {ClearAgenda, NULL, NULL}, enterMenuItem, NULL},
        {MENU_AGENDA_ID_2, {MENU_MAIN_ID_2, MENU_AGENDA_ID_1, MENU_AGENDA_ID_0}, {"AGENDA", "Back"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        
        {MENU_AGENDA_ID_3, {MENU_AGENDA_ID_7, MENU_AGENDA_ID_6, MENU_AGENDA_ID_4}, {"NEW", "Time"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_AGENDA_ID_4, {MENU_AGENDA_ID_8, MENU_AGENDA_ID_3, MENU_AGENDA_ID_5}, {"NEW", "Sound"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_AGENDA_ID_5, {MENU_AGENDA_ID_5, MENU_AGENDA_ID_4, MENU_AGENDA_ID_6}, {"NEW", "Add"}, {AddAgendaNewMenu, NULL, NULL}, enterMenuItem, NULL},
        {MENU_AGENDA_ID_6, {MENU_AGENDA_ID_6, MENU_AGENDA_ID_5, MENU_AGENDA_ID_3}, {"NEW", "Back"}, {leaveAgendaNewMenu, NULL, NULL}, enterMenuItem, NULL},

        {MENU_AGENDA_ID_7, {MENU_AGENDA_ID_3, MENU_AGENDA_ID_7, MENU_AGENDA_ID_7}, {"TIME", " ", "", ""}, {NULL, decreaseAgendaNewTime, increaseAgendaNewTime}, enterAgendaNewTime, NULL},
        {MENU_AGENDA_ID_8, {MENU_AGENDA_ID_8, MENU_AGENDA_ID_8, MENU_AGENDA_ID_8}, {"SOUND", " ", "", ""}, {okPressAgendaNewSound, previousAgendaNewSound, nextAgendaNewSound}, enterAgendaNewSound, NULL},
    
        {MENU_SETTINGS_ID_0, {MENU_SETTINGS_ID_2, MENU_SETTINGS_ID_1, MENU_SETTINGS_ID_1}, {"SETTINGS", "Volume"}, {NULL, NULL, NULL}, enterMenuItem, NULL},
        {MENU_SETTINGS_ID_1, {MENU_MAIN_ID_3, MENU_SETTINGS_ID_0, MENU_SETTINGS_ID_0}, {"SETTINGS", "Back"}, {NULL, NULL, NULL}, enterMenuItem, NULL},

        {MENU_SETTINGS_ID_2, {MENU_SETTINGS_ID_0, MENU_SETTINGS_ID_2, MENU_SETTINGS_ID_2}, {"VOLUME", " ", "", ""}, {NULL, decreaseVolume, increaseVolume}, enterSettingsVolume, NULL}
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

    create_custom_characters();

    return menuPointer;
}

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
void displaySongs(char *titleText){
    i2c_lcd1602_clear(_lcd_info);

    // Display scroll menu title
    char *menuText = titleText;
    menu_writeScrollMenuItem(_lcd_info, menuText, 0);

    // Loop back around if songIndex exeeds songList size
    if(songIndex + 1 > get_array_size() + 1){
        songIndex = 0;
    } else if (songIndex - 1 < -1) {
        songIndex = get_array_size();
    }

    // Get the song index before current selected song
    // if below 0 loop back to top
    int previousSongIndex = songIndex - 1 < 0? get_array_size() : songIndex - 1;
    menuText = songList[previousSongIndex];
    menu_writeScrollMenuItem(_lcd_info, menuText, 1);

    menuText = songList[songIndex];
    menu_writeScrollMenuItem(_lcd_info, menuText, 2);

    // Get the song index after current selected song
    // if after 24 loop back to beginning
    int nextSongIndex = songIndex + 1 > get_array_size()? 0 : songIndex + 1;
    menuText = songList[nextSongIndex];
    menu_writeScrollMenuItem(_lcd_info, menuText, 3);  
    
    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(_lcd_info, 17, 2);
    i2c_lcd1602_write_string(_lcd_info, cursor);

    enterMenuItem();
}

void displayRadioChannels(){
    i2c_lcd1602_clear(_lcd_info);

    // Display scroll menu title
    char *menuText = "CHANNEL";
    menu_writeScrollMenuItem(_lcd_info, menuText, 0);

    // Loop back around if radioIndex exeeds AMOUNT_OF_RADIO_CHANNELS size
    if(channelIndex + 1 > AMOUNT_OF_RADIO_CHANNELS + 1){
        channelIndex = 0;
    } else if (channelIndex - 1 < -1) {
        channelIndex = AMOUNT_OF_RADIO_CHANNELS;
    }

    // Get the channel index before current selected song
    // if below 0 loop back to top
    int previousChannelIndex = channelIndex - 1 < 0? AMOUNT_OF_RADIO_CHANNELS : channelIndex - 1;
    menuText = radioChannelNames[previousChannelIndex];
    menu_writeScrollMenuItem(_lcd_info, menuText, 1);

    menuText = radioChannelNames[channelIndex];
    menu_writeScrollMenuItem(_lcd_info, menuText, 2);

    // Get the channel index after current selected song
    // if after AMOUNT_OF_RADIO_CHANNELS loop back to beginning
    int nextChannelIndex = channelIndex + 1 > AMOUNT_OF_RADIO_CHANNELS? 0 : channelIndex + 1;
    menuText = radioChannelNames[nextChannelIndex];
    menu_writeScrollMenuItem(_lcd_info, menuText, 3);  
    
    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(_lcd_info, 17, 2);
    i2c_lcd1602_write_string(_lcd_info, cursor);

    enterMenuItem();
}

// Default enter event, displays time
void enterMenuItem(void) {
    if(isListening)
        menu_displayMic();

    // menu_displayTemperature(http_request_get_response());
    menu_displayTime(clock_getTimeString());
}

// Radio channel enter event
void enterRadioChannel(void){
    enterMenuItem();

    displayRadioChannels();
}
// Radio channel right press event
void  nextChannel(void){
    channelIndex++;
    displayRadioChannels();
}
// Radio channel left press event
void previousChannel(void){
    channelIndex--;
    displayRadioChannels();
}
void okPressRadioChannel(){
    // User pressed back
    if(channelIndex == 0){
        // Switch back to radio menu
        _menu->currentMenuItemId = MENU_RADIO_ID_0;

        menu_displayScrollMenu(_menu);

        if(_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
            (*_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent)();
        }

    } else {
        // else play selected radio channel
        play_radio(channelIndex - 1);
    }
}

// SD play enter event
void enterSDPlay(void){
    enterMenuItem();

    displaySongs("SD");
}
// SD play right press event 
void nextSong(void){
    songIndex++;
    displaySongs("SD");
}
// SD play left press event
void previousSong(void){
    songIndex--;
    displaySongs("SD");
}
// SD play ok press event
void okPressSDPlay(){
    // If pressed Back, go back to SD scroll menu
    if(strcmp(songList[songIndex], "Back") == 0){
       gotoMenuItem(MENU_SD_ID_0);
    } else {
        // else play selected song
        play_song_with_ID(songList[songIndex], "m");
    }
}

void enterAgendaNewTime(void){
    enterMenuItem();

    tempSelectedTime = clock_getCurrentTime();

    i2c_lcd1602_move_cursor(_lcd_info, 8, 1);
    i2c_lcd1602_write_string(_lcd_info, clock_getTimeString());
}
void decreaseAgendaNewTime(void){
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
}
void increaseAgendaNewTime(void){
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
}

void enterAgendaNewSound(void){
    enterMenuItem();

    displaySongs("SOUND");
}
void previousAgendaNewSound(void){
    songIndex--;
    displaySongs("SOUND");
}
void nextAgendaNewSound(void){
    songIndex++;
    displaySongs("SOUND");
}
void okPressAgendaNewSound(void){
    // If a song was pressed set this as the selected song
    if(strcmp(songList[songIndex], "Back") != 0){
        tempSelectedSong = songList[songIndex];
    }

    // Return to New Agenda menu
    gotoMenuItem(MENU_AGENDA_ID_4);
}
void leaveAgendaNewMenu(void){
    // Reset temp variables
    resetTempAgendaVariables();

    // Return to Agenda menu
    gotoMenuItem(MENU_AGENDA_ID_0);
}
void AddAgendaNewMenu(void){
    // Add alarm to list
    alarm_add(tempSelectedTime, tempSelectedSong);

    // Reset temp variables
    resetTempAgendaVariables();

    // Return to Agenda menu
    gotoMenuItem(MENU_AGENDA_ID_0);
}
void ClearAgenda(void){
    // Clear agenda list
    clear_global_list();
}

void resetTempAgendaVariables(void){
    tempSelectedTime[0] = 0;
    tempSelectedTime[1] = 0;
}

// Goes to given menu item
void gotoMenuItem(int menuItem){
    _menu->currentMenuItemId = menuItem;

    menu_displayScrollMenu(_menu);

    if(_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
        (*_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent)();
    }
}

// Settings volume enter event
void enterSettingsVolume(void){
    enterMenuItem();

    char volumeStr[5];
    sprintf(volumeStr, "%03d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);
}
// Volume right press event
void increaseVolume(void){
    volume++;
    if(volume > 100) volume = 100;
    char volumeStr[5];
    sprintf(volumeStr, "%03d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);

    setVolume(volume);
}
// Volume left press event
void decreaseVolume(void){
    volume--;
    if(volume < 0) volume = 0;
    char volumeStr[5];
    sprintf(volumeStr, "%03d", volume);

    i2c_lcd1602_move_cursor(_lcd_info, 9, 1);
    i2c_lcd1602_write_string(_lcd_info, volumeStr);

    setVolume(volume);
}

void menu_displayMic(void){
    i2c_lcd1602_move_cursor(_lcd_info, 0, 1);
    i2c_lcd1602_write_custom_char(_lcd_info, I2C_LCD1602_INDEX_CUSTOM_0); //Mic character
}

void menu_clearMic(void){
    i2c_lcd1602_move_cursor(_lcd_info, 0, 1);
    i2c_lcd1602_write_char(_lcd_info, ' '); // Clear mic character
}

void menu_mic(bool listening){
    if(listening)
        menu_displayMic();
    else
        menu_clearMic();

    isListening = listening;
}