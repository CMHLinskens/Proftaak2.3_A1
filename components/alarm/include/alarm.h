#ifndef alarm_H
#define alarm_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "clock-sync.h"
#include <stdio.h>
#include <stdlib.h>

#include "esp_log.h"

#include "audio-board.h"

#define ALARMTAG "alarm"

//Struct for all the alarm nodes
//songs is what will be playes when the alarm goes off
//time is the time when the alarm goes off
struct ALARM{
    char* song;
    int* time;
    struct ALARM *next;
};

typedef struct ALARM *Node;

//Alarm task keeps looping and plays a song when ALARM->time == the current time
void alarm_task(void*pvParameter);

//Method to add an alarm to the list
//time is desired time for the alarm to go off
//song is desired song to play when alarm goes off
void alarm_add(int* time, char* song);

void clear_global_list();
void print_global_list();

#endif