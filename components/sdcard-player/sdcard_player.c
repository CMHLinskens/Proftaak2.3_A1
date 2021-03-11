/* Control with a touch pad playing MP3 files from SD Card

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "filter_resample.h"

#include "esp_peripherals.h"
#include "periph_sdcard.h"
#include "periph_touch.h"
#include "periph_button.h"
#include "input_key_service.h"
#include "periph_adc_button.h"
#include "board.h"

#include "sdcard_list.h"
#include "sdcard_scan.h"

typedef struct sdcard_list {
    char *save_file_name;                /*!< Name of file to save URLs */
    char *offset_file_name;              /*!< Name of file to save offset */
    FILE *save_file;                     /*!< File to save urls */
    FILE *offset_file;                   /*!< File to save offset of urls */
    char *cur_url;                       /*!< Point to current URL */
    uint16_t url_num;                    /*!< Number of URLs */
    uint16_t cur_url_id;                 /*!< Current url ID */
    uint32_t total_size_save_file;       /*!< Size of file to save URLs */
    uint32_t total_size_offset_file;     /*!< Size of file to save offset */
} sdcard_list_t;


static const char *TAG = "SDCard";
static audio_board_handle_t board_handle;

audio_pipeline_handle_t pipeline;
audio_element_handle_t i2s_stream_writer, mp3_decoder, fatfs_stream_reader, rsp_handle;
playlist_operator_handle_t sdcard_list_handle = NULL;

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    //Handle touch pad events to start, pause, resume, finish current song and adjust volume
    board_handle = (audio_board_handle_t) ctx;
    int player_volume;
    audio_hal_get_volume(board_handle->audio_hal, &player_volume);

    if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE) {
        ESP_LOGI(TAG, "input key id is %d", (int)evt->data);

        switch ((int)evt->data) {

            //Play button
            case INPUT_KEY_USER_ID_PLAY:
                ;
                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                //Looks at the state of the play button
                switch (el_state) {
                    case AEL_STATE_INIT :
                        ESP_LOGI(TAG, "Starting audio");
                        audio_pipeline_run(pipeline);
                        break;
                    case AEL_STATE_RUNNING :
                        ESP_LOGI(TAG, "Pausing audio");
                        audio_pipeline_pause(pipeline);
                        break;
                    case AEL_STATE_PAUSED :
                        ESP_LOGI(TAG, "Resuming audio");
                        audio_pipeline_resume(pipeline);
                        break;
                    default :
                        ESP_LOGI(TAG, "Not supported state %d", el_state);
                }
                break;

            case INPUT_KEY_USER_ID_SET:
                ESP_LOGI(TAG, "Stopped, advancing to the next song");
                char *url = NULL;

                //Stops music, terminates current pipeline and looks up the next audio file on the SD card
                audio_pipeline_stop(pipeline);
                audio_pipeline_wait_for_stop(pipeline);
                audio_pipeline_terminate(pipeline);
                sdcard_list_next(sdcard_list_handle, 1, &url);

                //Resets pipeline and starts the new audio file
                audio_element_set_uri(fatfs_stream_reader, url);
                audio_pipeline_reset_ringbuffer(pipeline);
                audio_pipeline_reset_elements(pipeline);
                audio_pipeline_run(pipeline);
                break;

            case INPUT_KEY_USER_ID_VOLUP:
                //Sets volume up
                ESP_LOGI(TAG, "Volume went up");
                player_volume += 10;
                if (player_volume > 100) {
                    player_volume = 100;
                }

                audio_hal_set_volume(board_handle->audio_hal, player_volume);
                ESP_LOGI(TAG, "Volume set to %d", player_volume);
                break;

            case INPUT_KEY_USER_ID_VOLDOWN:
                //Sets volume down
                ESP_LOGI(TAG, "Volume went down");
                player_volume -= 10;
                if (player_volume < 0) {
                    player_volume = 0;
                }

                audio_hal_set_volume(board_handle->audio_hal, player_volume);
                ESP_LOGI(TAG, "Volume set to %d %%", player_volume);
                break;
        }
    }
    return ESP_OK;
}

void sdcard_url_save_cb(void *user_data, char *url)
{
    playlist_operator_handle_t sdcard_handle = (playlist_operator_handle_t)user_data;
    esp_err_t ret = sdcard_list_save(sdcard_handle, url);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fail to save sdcard url to sdcard playlist");
    }
}

void pause(){
    audio_pipeline_pause(pipeline);
    ESP_LOGI(TAG, "Paused");
}

void resume(){
    audio_pipeline_resume(pipeline);
    ESP_LOGI(TAG, "Resumed");
}

//Plays song with given ID/URL
void play_song_with_ID(char* url){

    //Extends the URL so the SD card can find it
    char extendedUrl[100];
    strcpy(extendedUrl, "file://sdcard/");
    strcat(extendedUrl, url);
    strcat(extendedUrl, ".mp3");
    
    //Stops music, terminates current pipeline
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    //Resets pipeline and starts the new audio file
    audio_element_set_uri(fatfs_stream_reader, extendedUrl);
    audio_pipeline_reset_ringbuffer(pipeline);
    audio_pipeline_reset_elements(pipeline);
    audio_pipeline_run(pipeline);

    ESP_LOGI(TAG, "Song %s is playing", extendedUrl);
    
}

void get_all_songs_from_SDcard(char** song_list){
    //Makes array of songs on the sd
    //Uses some code from sdcard_list_show in sdcard_list.c
    sdcard_list_t *playlist = sdcard_list_handle->playlist;

    uint32_t pos = 0;
    uint16_t  size = 0;
    char* url = calloc(1, 1024 * 2);

    fseek(playlist->save_file, 0, SEEK_SET);
    fseek(playlist->offset_file, 0, SEEK_SET);
    
    for(int i = 0; i < playlist->url_num; i++){
        //Gets songs from the SD
        memset(url, 0, 1024 * 2);
        fread(&pos, 1, sizeof(uint32_t), playlist->offset_file);
        fread(&size, 1, sizeof(uint16_t), playlist->offset_file);
        fseek(playlist->save_file, pos, SEEK_SET) ;
        fread(url, 1, size, playlist->save_file);

        //Copy's the url so the array doesnt point to a pointer.
        char *temp_url = calloc(1, 80);
        //Gets the 14'th char, because we dont want the file name to go with it
        strcpy(temp_url, url + 14);

        //Removes the suffix .mp3
        int length = strlen(temp_url);
        temp_url[length-4] = '\0';

        //Adds url to array
        song_list[i] = temp_url;
    }
}

void sdcard_start(void * pvParameter)
{
    //Configuration
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "[1.0] Initialize peripherals management");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    ESP_LOGI(TAG, "[1.1] Initialize and start peripherals");
    audio_board_key_init(set);
    audio_board_sdcard_init(set, SD_MODE_1_LINE);

    ESP_LOGI(TAG, "[1.2] Set up a sdcard playlist and scan sdcard music save to it");
    sdcard_list_create(&sdcard_list_handle);
    ESP_LOGI(TAG, "SDcard created!");
    sdcard_scan(sdcard_url_save_cb, "/sdcard", 0, (const char *[]) {"mp3"}, 1, sdcard_list_handle);

    ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[ 3 ] Create and start input key service");
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set;
    periph_service_handle_t input_ser = input_key_service_create(&input_cfg);
    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, (void *)board_handle);

    ESP_LOGI(TAG, "[4.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[4.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_config.sample_rate = 48000;
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[4.2] Create mp3 decoder to decode mp3 file");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);

    ESP_LOGI(TAG, "[4.3] Create resample filter");
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_handle = rsp_filter_init(&rsp_cfg);

    ESP_LOGI(TAG, "[4.4] Create fatfs stream to read data from sdcard");
    char *url = NULL;
    sdcard_list_current(sdcard_list_handle, &url);
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_READER;
    fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);
    audio_element_set_uri(fatfs_stream_reader, url);

    ESP_LOGI(TAG, "[4.5] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
    audio_pipeline_register(pipeline, mp3_decoder, "mp3");
    audio_pipeline_register(pipeline, rsp_handle, "filter");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(TAG, "[4.6] Link it together [sdcard]-->fatfs_stream-->mp3_decoder-->resample-->i2s_stream-->[codec_chip]");
    const char *link_tag[4] = {"file", "mp3", "filter", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 4);

    ESP_LOGI(TAG, "[5.0] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[5.1] Listen for all pipeline events");
    audio_pipeline_set_listener(pipeline, evt);

    //End of configuration
    
    //TEST CODE

    //Test to show all the songs on the SD card
    char** test = calloc(24, 80);
    get_all_songs_from_SDcard(test);

    //Prints all the songs on the SD card
    for(int i = 0; i < 24; i++){
        ESP_LOGE(TAG, "This is the song %d with url %s", i, test[i]);
        ESP_LOGE(TAG, "Adress of test[i]: %p", &test[i]);
    }

    //Test to play songs with ID
    char* avond = "Avond";
    play_song_with_ID(avond);
    
    //Test to pause/resume
    pause();
    resume();
    

    while (1) {

        // Handle event interface messages from pipeline to set music info and to advance to the next song
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Event interface error : %d", ret);
            continue;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) {
            // Set music info for a new song to be played
            if (msg.source == (void *) mp3_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {

                audio_element_info_t music_info = {0};
                audio_element_getinfo(mp3_decoder, &music_info);
                ESP_LOGI(TAG, "Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                         music_info.sample_rates, music_info.bits, music_info.channels);
                audio_element_setinfo(i2s_stream_writer, &music_info);
                rsp_filter_set_src_info(rsp_handle, music_info.sample_rates, music_info.channels);
                continue;

            }
            // Advance to the next song when previous finishes
            if (msg.source == (void *) i2s_stream_writer && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {

                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);

                if (el_state == AEL_STATE_FINISHED) {
                    ESP_LOGI(TAG, "Finished, advancing to the next song");
                    sdcard_list_next(sdcard_list_handle, 1, &url);
                    ESP_LOGW(TAG, "URL: %s", url);
                    /* In previous versions, audio_pipeline_terminal() was called here. It will close all the elememnt task and when use
                     * the pipeline next time, all the tasks should be restart again. It speed too much time when we switch to another music.
                     * So we use another method to acheive this as below.
                     */
                    audio_element_set_uri(fatfs_stream_reader, url);
                    audio_pipeline_reset_ringbuffer(pipeline);
                    audio_pipeline_reset_elements(pipeline);
                    audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
                    audio_pipeline_run(pipeline);
                }
                continue;
            }
        }
    }

    ESP_LOGI(TAG, "Stop audio_pipeline");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, mp3_decoder);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);
    audio_pipeline_unregister(pipeline, rsp_handle);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Stop all peripherals before removing the listener */
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(mp3_decoder);
    audio_element_deinit(rsp_handle);
    esp_periph_set_destroy(set);
    periph_service_destroy(input_ser);
}

void start_sdcard_task(){
    
    xTaskCreate(&sdcard_start, "sdcard player", 4096, NULL, 5, NULL);
}