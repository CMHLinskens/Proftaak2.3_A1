#ifndef clock_sync_H
#define clock_sync_H

//Sets the time to the current time
void obtain_time(void);

void initialize_sntp(void);

//Clock task
void clock_task(void*pvParameter);

//Get the time in char*
char *clock_getTimeString();

//Get the time in int*
int *clock_getCurrentTime();

//Say the current time out loud
void sayTime(void);

#endif
