#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "clock-sync.h"
#include <stdio.h>
#include <stdlib.h>

#include "esp_log.h"

#define ALARMTAG "alarm"

struct ALARM{
    char** songs;
    int* time;
};

struct ALARM* alarms;
int N = 0;

void alarm_add(char** songs, int* time);

void alarm_task(void*pvParameter){
    alarms = malloc(sizeof(struct ALARM));
    while (1)
    {
        alarm_add(NULL, clock_getCurrentTime());
        if(clock_getCurrentTime() != NULL){
            int* current = clock_getCurrentTime();
            alarms[0].time = clock_getCurrentTime();
            for(int i=0; i<=N; i++){
                int* alarm = alarms[i].time;
                if(alarm[0] == current[0] && alarm[1] == current[1]){
                    ESP_LOGI(ALARMTAG, "alarm going off %d, %d", alarm[1], current[1]);
                }
                else{
                    ESP_LOGI(ALARMTAG, "%d, %d", alarm[1] , current[1]);
                }
            }
        }
        else{
            ESP_LOGI(ALARMTAG,"%s", clock_getTimeString());
        }
        vTaskDelay(60000 / portTICK_RATE_MS);
    }
    
}

void alarm_add(char** songs, int* time){
    alarms = realloc(alarms, (N+1) * sizeof(alarms)/sizeof(alarms[0]));
    alarms->time  = malloc(sizeof(int));
    alarms->time = time;
    N++;
}
