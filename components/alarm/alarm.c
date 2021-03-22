#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "clock-sync.h"
#include <stdio.h>
#include <stdlib.h>

#include "esp_log.h"

#define ALARMTAG "alarm"

//Struct for all the alarm nodes
//songs is what will be playes when the alarm goes off
// time is the time when the alarm goes off
struct ALARM{
    char** songs;
    int* time;
    struct ALARM *next;
};

typedef struct ALARM *Node;

Node head, node;

//Creates an alarm node 
Node CreateAlarm(int* time, char** songs)
{
    struct ALARM *alarm;
    alarm = (Node) malloc(sizeof(struct ALARM));
    if(alarm == NULL){
        ESP_LOGE(ALARMTAG, "Error: not enough memory.");
        return NULL;
    }
    alarm->songs = songs;
    alarm->time = time;
    alarm->next = NULL;
    return alarm;
};

//Free the node
void FreeNode(Node node){
    if(node){
        free(node);
    }
}

//Place the node at the beginning of the list
void Prepend(Node *head, Node node){
    node->next = *head;
    *head = node;
}

//Place the node at the end of the list
void Append(Node *head, Node node){
    Node tmp = *head;
    if(*head == NULL){
        *head = node;
    }
    else{
        while(tmp->next){
            tmp = tmp->next;
        }
        tmp->next = node;
    }
}

//Removes a node
Node Remove(Node *head, Node node){
    Node tmp = *head;
    if(*head == NULL){
        return;
    }
    else if(*head == node){
        *head = (*head)->next;
        FreeNode(node);
    }
    else{
        while(tmp->next == node){
            tmp->next = tmp->next->next;
            FreeNode(node);
            return;
        }
        tmp = tmp->next;
    }
}

//Clear the list
void Clear(Node head){
    Node node;

    while(head){
        node = head;
        head = head->next;
        FreeNode(node);
    }
}

//Print a node
void Print(Node node){
    if(node){
        int* time = node->time;
        ESP_LOGI(ALARMTAG, "time = %d", time[1]);
    }
}

//Print the entire list
void PrintList(Node head){
    while(head){
        Print(head);
        head = head->next;
    }
}


void alarm_task(void*pvParameter){
    head = NULL;

    while(1){
        if(clock_getCurrentTime() != NULL){
            int* current = clock_getCurrentTime(); //Update current time
            Node tmp = head;
            while(tmp){
                int *alarmTime = tmp->time;
                //When the hour and minute are the same the alarm should go off 
                //alarmTime[0] = hour
                //alarmTime[1] = minute
                if(alarmTime[0] == current[0] && alarmTime[1] == current[1]){
                    ESP_LOGI(ALARMTAG, "Alarm going off %d, %d", alarmTime[1], current[1]);
                    //Do stuff when alarm goes off
                    alarm_going();
                    Remove(&head, tmp);
                    ESP_LOGI(ALARMTAG, "Removed element");
                }
                tmp = tmp->next;
            }
        }
        PrintList(head);
        vTaskDelay(5000/ portTICK_RATE_MS);
    }
    
}

//Method to add an alarm node to the list
void alarm_add(int* time, char** songs){
    Node newNode = CreateAlarm(time, songs);
    Prepend(&head, newNode);

}

void alarm_going(){
    //TODO play music from sd
}
