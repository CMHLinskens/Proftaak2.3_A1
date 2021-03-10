#ifndef clock_sync_H
#define clock_sync_H

void obtain_time(void);
void initialize_sntp(void);
void clock_task(void*pvParameter);
char *clock_getTimeString();
#endif
