#ifndef sdcard_player.h
#define sdcard_player.h

#include "sdcard_list.h"
#include "sdcard_scan.h"

void sdcard_url_save_cb(void *user_data, char *url);
void pause();
void resume();
void play_song_with_ID(char* url);
void get_all_songs_from_SDcard(char** song_list);
void sdcard_start(void);

#endif
