#include "clock-sync.h"

#include <time.h>
#include <string.h>
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "lcd-menu.h"
#include "audio-board.h"

#define CLOCKTAG "clock"

char *timeString;
char *dateString;
char **clockSounds;

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(CLOCKTAG, "Notification of a time synchronization event");
}

void obtain_time(void)
{
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    // loop to see if time is set
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        ESP_LOGI(CLOCKTAG, "Waiting for system time to be set...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    // update time
    time(&now);
    localtime_r(&now, &timeinfo);
}

void initialize_sntp(void)
{
    ESP_LOGI(CLOCKTAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    sntp_init();
}

void clock_task(void*pvParameter){
    initialize_sntp();
    while(1)
    {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        // Is time set? If not, tm_year will be (1970 - 1900).
        if (timeinfo.tm_year < (2016 - 1900)) {
            ESP_LOGI(CLOCKTAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
            obtain_time();
            // update 'now' variable with current time
            time(&now);
        }
        // update 'now' variable with current time
        time(&now);

        char strftime_buf[64];
        char strftime_buf2[64];

        // set timezone
        setenv("TZ", "CET-1", 1);
        tzset();
        localtime_r(&now, &timeinfo);

        // convert time to string
        strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &timeinfo); // time
        strftime(strftime_buf2, sizeof(strftime_buf), "%x", &timeinfo); // date

        timeString = strftime_buf;
        dateString = strftime_buf2;

        menu_display_time(timeString);

        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}

char *clock_getTimeString()
{
    if(timeString == NULL) { return "00:00"; }
    return timeString;
}

int *clock_getCurrentTime(){
    if(timeString == NULL) { return NULL; }
    int *time = calloc(2, sizeof(int));
    for(int i = 0; i < 2; i++){
        char tempString[3];
        memcpy(tempString, &timeString[3 * i], 2);
        tempString[3] = '\0';
        int tempTime = atoi(tempString);
        time[i] = tempTime;
    }
    return time;
}

char* clock_getDate(){
    if(dateString == NULL){ return NULL; }
    return dateString;
}

void sayTime(void){
    // Retrieve time
    int *time = clock_getCurrentTime();
    if(time == NULL) {
        ESP_LOGE(CLOCKTAG, "Time has not been set yet.");
        return;
    }

    // Initialize sounds list
    char *soundsToPlay[10];
    soundsToPlay[0] = "Het is nu";
    soundsToPlay[2] = "Uur";
    soundsToPlay[8] = "In de";

    // Check if we are in the morning, afternoon or evening
    if(time[0] < 12) {
        soundsToPlay[9] = "Ochtend";
    } else if (time[0] < 18) {
        soundsToPlay[9] = "Middag";
    } else {
        soundsToPlay[9] = "Avond";
    }
    
    // Remove 12 hours from time if it is after 12
    if(time[0] > 12){
        time[0] -= 12;
    }

    // Set the hour sound
    char hourString[3];
    sprintf(hourString, "%d", time[0]);
    soundsToPlay[1] = hourString;

    // Set the minute sounds
    char minuteString[3];
    sprintf(minuteString, "%d", time[1]);

    // temp variables (I dont know a better solution)
    char tempMinuteString1[2];
    char tempMinuteString2[3];

    // If we have a full hour add nothing more to the list
    if(minuteString[0] == '0'){
        soundsToPlay[3] = soundsToPlay[4] = soundsToPlay[5] = soundsToPlay[6] = soundsToPlay[7] = "";
    }
    // If we have minutes < 10 just add the string and "Minuten"
    else if(minuteString[1] == '\0'){
        soundsToPlay[3] = "En";
        soundsToPlay[4] = minuteString;
        soundsToPlay[5] = "Minuten";
        soundsToPlay[6] = soundsToPlay[7] = "";
    // Else determine the minute sound fragments
    } else {
        soundsToPlay[3] = "En";
        char minuteTens = minuteString[0];
        char minuteOnes = minuteString[1];
        switch(minuteTens){
            case '1':
                // 
                switch(minuteOnes){
                    case '0':
                        soundsToPlay[4] = "10";
                        soundsToPlay[5] = "";
                    break;
                    case '1':
                        soundsToPlay[4] = "11";
                        soundsToPlay[5] = "";
                    break;
                    case '2':
                        soundsToPlay[4] = "12";
                        soundsToPlay[5] = "";
                    break;
                    case '3':
                        soundsToPlay[4] = "13";
                        soundsToPlay[5] = "";
                    break;
                    case '4':
                        soundsToPlay[4] = "14";
                        soundsToPlay[5] = "";
                    break;
                    default:
                        tempMinuteString1[0] = minuteOnes;
                        tempMinuteString1[1] = '\0';
                        soundsToPlay[4] = tempMinuteString1;
                        tempMinuteString2[0] = minuteTens;
                        tempMinuteString2[1] = '0';
                        tempMinuteString2[2] = '\0';
                        soundsToPlay[5] = tempMinuteString2;
                    break;
                }
                soundsToPlay[6] = "Minuten";
                soundsToPlay[7] = "";
            break;
            case '2':
            case '3':
            case '4':
            case '5':
                // If we have 20, 30, 40, etc. add empty strings on 4 and 5
                // else fill them with minutes before the tens and add "En" 
                if(minuteOnes == '0'){
                    soundsToPlay[4] = "";
                    soundsToPlay[5] = "";
                } else {       
                    tempMinuteString1[0] = minuteOnes;
                    tempMinuteString1[1] = '\0';
                    soundsToPlay[4] = tempMinuteString1;
                    soundsToPlay[5] = "En";
                }
                // tempMinuteString for turning the 2, 3, 4, etc. into 20, 30, 40, etc.
                tempMinuteString2[0] = minuteTens;
                tempMinuteString2[1] = '0';
                tempMinuteString2[2] = '\0';
                soundsToPlay[6] = tempMinuteString2;
                soundsToPlay[7] = "Minuten";
            break;
            default:
                ESP_LOGE(CLOCKTAG, "Unknown minutes in playTime()");
            break;
        }
    }

    // Play all sounds
    for(int i = 0; i < 10; i++){
        if(!strcmp(soundsToPlay[i], "")) { continue; }
        play_song_with_ID(soundsToPlay[i], "c");
        audio_pipeline_wait_for_stop(get_pipeline());
    }
}