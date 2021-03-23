#ifndef audio_board_H
#define audio_board_H

#include "sdcard_list.h"
#include "sdcard_scan.h"

#include "http_stream.h"
#include "audio_pipeline.h"

#define AMOUNT_OF_RADIO_CHANNELS 3

char *radioChannelNames[AMOUNT_OF_RADIO_CHANNELS + 1];

audio_pipeline_handle_t getPipeline(void);
int get_volume(void);
int get_array_size(void);

// SD Card
void sdcard_url_save_cb(void *user_data, char *url);
void pause_sound(void);
void resume_sound(void);
void play_song_with_ID(char* url);
void get_all_songs_from_SDcard(char * dir);
void audio_start(void);
void stop_audio(void);
char **get_song_list(void);

// Radio
int http_stream_event_handle(http_stream_event_msg_t *msg);
void radio_init(void);
void play_radio(int channel);
void stop_radio(void);

#endif