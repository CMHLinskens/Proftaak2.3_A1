#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "clock-sync.h"

#include "esp_log.h"

#define ALARMTAG "alarm"

struct alarmtime{
    char** songs;
    int* time;
};


void alarm_task(void*pvParameter){

    struct alarmtime alarmtimes[10];

    alarmtimes[0].songs = NULL;
    alarmtimes[0].time = clock_getCurrentTime();
    while (1)
    {
        for(int i=0; i<sizeof(alarmtimes)/sizeof(alarmtimes[0]); i++){
          if(alarmtimes[i].time == clock_getCurrentTime()){
               ESP_LOGI(ALARMTAG, "alarm going off");
          }
        }
        vTaskDelay(10000 / portTICK_RATE_MS);
    }
    
}