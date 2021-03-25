#ifndef audio_board_H
#define audio_board_H

#include "sdcard_list.h"
#include "sdcard_scan.h"

#include "http_stream.h"
#include "audio_pipeline.h"

#define AMOUNT_OF_RADIO_CHANNELS 3

char *radioChannelNames[AMOUNT_OF_RADIO_CHANNELS + 1];

/**
 * @brief Gets the pipeline object.
 * @return pipeline
 */
audio_pipeline_handle_t get_pipeline(void);


/**
 * @brief Sets a new volume.
 * @param newVolume volume to be set
 */
void set_volume(int newVolume);

/**
 * @brief Gets the current volume.
 * @return volume
 */
int get_volume(void);

/**
 * @brief Gets the current array size.
 * @return arrayIndex
 */
int get_array_size(void);

// SD Card
/**
 * @brief  Plays audio with the given ID/URL/save file location.
 * @param  url: the file location.
 * @param  type: the type of audio file (c for clock, m for music).
 */
void play_song_with_ID(char* url, char *type);

/**
 * @brief  Makes an array of songs on the sd.
 * @param  dir: the type of audio file (c for clock, m for music).
 */
void get_all_songs_from_SDcard(char * dir);

/**
 * @brief  Starts the audio board and deinitalizes all components required.
 */
void audio_start(void);

/**
 * @brief  Stops the audio board and deinitalizes all components required.
 */
void stop_audio(void);

/**
 * @brief  Returns all the songs on the SD card
 * @return song_list 
 */
char **get_song_list(void);

// Radio
/**
 * @brief  Starts the radio.
 * @param  channel: the chosen radio channel 
 */
void play_radio(int channel);

/**
 * @brief  Stops the radio and changes the pipeline link to sd card.
 */
void stop_radio(void);

#endif