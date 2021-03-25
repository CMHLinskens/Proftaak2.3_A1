#include "alarm.h"

#include <stdlib.h>
#include <string.h>

Node head, node;

//Creates an alarm node 
Node CreateAlarm(int* time, char* song)
{
    struct ALARM *alarm;
    alarm = (Node) malloc(sizeof(struct ALARM));
    if(alarm == NULL){
        ESP_LOGE(ALARMTAG, "Error: not enough memory.");
        return NULL;
    }
    alarm->song = calloc(1, 30);
    memcpy(alarm->song, song, 30);
    alarm->time = calloc(2, 2 * sizeof(int));
    memcpy(alarm->time, time, 2 * sizeof(int));
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
void Remove(Node *head, Node node){
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
        char* song = node->song;
        ESP_LOGI(ALARMTAG, "time = %d:%d song = %s", time[0], time[1], song);

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
                    play_song_with_ID(tmp->song, "m");
                    Remove(&head, tmp);
                    ESP_LOGI(ALARMTAG, "Removed element");
                }
                tmp = tmp->next;
            }
        }
        vTaskDelay(30000/ portTICK_RATE_MS);
    }
    
}

//Method to add an alarm node to the list
void alarm_add(int* time, char* song){
    Node newNode = CreateAlarm(time, song);
    Prepend(&head, newNode);
}

//Clears the global head node in this file 
void clear_global_list(){
    Clear(head);
    head = NULL;
}
void print_global_list(){
    PrintList(head);
}