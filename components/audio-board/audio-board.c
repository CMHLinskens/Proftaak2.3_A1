#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_element.h"
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

#include "freertos/event_groups.h"
#include "http_stream.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "periph_wifi.h"

#include "audio-board.h"

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#else
#define ESP_IDF_VERSION_VAL(major, minor, patch) 1
#endif

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif

#define AUDIOBOARDTAG "AudioBoard"

typedef struct sdcard_list {
    char *save_file_name;                // Name of file to save URLs
    char *offset_file_name;              // Name of file to save offset
    FILE *save_file;                     // File to save urls
    FILE *offset_file;                   // File to save offset of urls
    char *cur_url;                       // Point to current URL
    uint16_t url_num;                    // Number of URLs
    uint16_t cur_url_id;                 // Current url ID
    uint32_t total_size_save_file;       // Size of file to save URLs
    uint32_t total_size_offset_file;     // Size of file to save offset
} sdcard_list_t;

char** songList = NULL;
audio_pipeline_handle_t pipeline;
audio_element_handle_t http_stream_reader, i2s_stream_writer, aac_decoder, mp3_decoder, fatfs_stream_reader, rsp_handle;
playlist_operator_handle_t sdcard_list_handle = NULL;
esp_periph_set_handle_t set;
audio_event_iface_handle_t evt;
periph_service_handle_t input_ser;
bool playingRadio = false;

char *radioChannels[AMOUNT_OF_RADIO_CHANNELS] = {
                        "https://22533.live.streamtheworld.com/SKYRADIO.mp3",
                        "https://icecast.omroep.nl/radio2-bb-mp3",
                        "https://icecast-qmusicnl-cdp.triple-it.nl/Qmusic_nl_live_96.mp3"
                        };

int _http_stream_event_handle(http_stream_event_msg_t *msg)
{
    if (msg->event_id == HTTP_STREAM_RESOLVE_ALL_TRACKS) {
        return ESP_OK;
    }

    if (msg->event_id == HTTP_STREAM_FINISH_TRACK) {
        return http_stream_next_track(msg->el);
    }
    if (msg->event_id == HTTP_STREAM_FINISH_PLAYLIST) {
        return http_stream_fetch_again(msg->el);
    }
    return ESP_OK;
}

audio_pipeline_handle_t getPipeline(){
    return pipeline;
}

//Handles touchpad events
static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx){

    //Touch pad init
    audio_board_handle_t board_handle = (audio_board_handle_t) ctx;
    int player_volume;
    audio_hal_get_volume(board_handle->audio_hal, &player_volume);

    //Touchpad event started
    if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE){
        ESP_LOGI(AUDIOBOARDTAG, "input key id is %d", (int)evt->data);

        //Switch case with the event data
        switch ((int)evt->data){

            //Play button
            case INPUT_KEY_USER_ID_PLAY:
                ;
                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                //Looks at the state of the play button
                switch (el_state) {
                    case AEL_STATE_INIT :
                        ESP_LOGI(AUDIOBOARDTAG, "Starting audio");
                        audio_pipeline_run(pipeline);
                        break;
                    case AEL_STATE_RUNNING :
                        ESP_LOGI(AUDIOBOARDTAG, "Pausing audio");
                        audio_pipeline_pause(pipeline);
                        break;
                    case AEL_STATE_PAUSED :
                        ESP_LOGI(AUDIOBOARDTAG, "Resuming audio");
                        audio_pipeline_resume(pipeline);
                        break;
                    default :
                        ESP_LOGI(AUDIOBOARDTAG, "Not supported state %d", el_state);
                }
                break;

            //Set button is pressed
            case INPUT_KEY_USER_ID_SET:
                ESP_LOGI(AUDIOBOARDTAG, "Stopped, advancing to the next song");
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

            //Volume up button is pressed
            case INPUT_KEY_USER_ID_VOLUP:
                //Sets volume up
                ESP_LOGI(AUDIOBOARDTAG, "Volume went up");
                player_volume += 10;
                if (player_volume > 100) {
                    player_volume = 100;
                }

                audio_hal_set_volume(board_handle->audio_hal, player_volume);
                ESP_LOGI(AUDIOBOARDTAG, "Volume set to %d %%", player_volume);
                break;

            //Volume down button is pressed
            case INPUT_KEY_USER_ID_VOLDOWN:
                //Sets volume down
                ESP_LOGI(AUDIOBOARDTAG, "Volume went down");
                player_volume -= 10;
                if (player_volume < 0) {
                    player_volume = 0;
                }

                audio_hal_set_volume(board_handle->audio_hal, player_volume);
                ESP_LOGI(AUDIOBOARDTAG, "Volume set to %d %%", player_volume);
                break;
        }
    }
    return ESP_OK;
}

//Saves data to SD with given url
void sdcard_url_save_cb(void *user_data, char *url){

    playlist_operator_handle_t sdcard_handle = (playlist_operator_handle_t)user_data;
    esp_err_t ret = sdcard_list_save(sdcard_handle, url);

    //Failed to save
    if (ret != ESP_OK){
        ESP_LOGE(AUDIOBOARDTAG, "Fail to save sdcard url to sdcard playlist");
    }
}

//Pauses audio
void pauseSound(){

    audio_pipeline_pause(pipeline);
    ESP_LOGI(AUDIOBOARDTAG, "Paused");
}

//Resumes audio
void resumeSound(){

    audio_pipeline_resume(pipeline);
    ESP_LOGI(AUDIOBOARDTAG, "Resumed");
}

//Plays audio with given ID/URL
void play_song_with_ID(char* url){
    //Extends the URL so the SD card can find it
    char extendedUrl[80];
    strcpy(extendedUrl, "file://sdcard/");
    strcat(extendedUrl, url);
    strcat(extendedUrl, ".mp3");
    
    if(playingRadio){
        stop_radio();
    } else {
        //Stops audio, terminates current pipeline
        audio_pipeline_stop(pipeline);
        audio_pipeline_wait_for_stop(pipeline);
        audio_pipeline_terminate(pipeline);
    }

    //Resets pipeline and starts the new audio file
    audio_element_set_uri(fatfs_stream_reader, extendedUrl);
    audio_pipeline_reset_ringbuffer(pipeline);
    audio_pipeline_reset_elements(pipeline);
    audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
    audio_pipeline_run(pipeline);

    ESP_LOGI(AUDIOBOARDTAG, "Song %s is playing", extendedUrl);
}

//Makes array of songs on the sd
void get_all_songs_from_SDcard(){
    
    sdcard_list_t *playlist = sdcard_list_handle->playlist;
    uint32_t pos = 0;
    uint16_t  size = 0;
    char* url = calloc(1, 2048);
    songList = calloc(playlist->url_num, 80);

    fseek(playlist->save_file, 0, SEEK_SET);
    fseek(playlist->offset_file, 0, SEEK_SET);
    
    for(int i = 0; i < playlist->url_num; i++){
        //Gets songs from the SD, these lines are copied from sdcard_list_show
        memset(url, 0, 2048);
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
        //Adds 0 character to mark end of the string
        temp_url[length-4] = '\0';

        //Adds url to array
        songList[i] = temp_url;
    }
    free(url);
}

//Returns all the songs on the SD card
char** getSongList(){
    if(songList == NULL){
        ESP_LOGE(AUDIOBOARDTAG, "Songlist not created yet!");
        //Songlist doesn't exist/have any songs so we return an array with a warning.
        songList = calloc(1, 19);
        songList[0] = "No songs in sdcard";
        return songList;
    } else {
        return songList;
    }
}

// Starts the audio board
void audio_start(void * pvParameter){

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
    ESP_ERROR_CHECK(esp_netif_init());
#else
    tcpip_adapter_init();
#endif

    //Start configuration

    esp_log_level_set(AUDIOBOARDTAG, ESP_LOG_INFO);

    ESP_LOGI(AUDIOBOARDTAG, "[1.0] Initialize peripherals management");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    set = esp_periph_set_init(&periph_cfg);

    ESP_LOGI(AUDIOBOARDTAG, "[1.1] Initialize and start peripherals");
    audio_board_key_init(set);
    audio_board_sdcard_init(set, SD_MODE_1_LINE);

    ESP_LOGI(AUDIOBOARDTAG, "[1.2] Set up a sdcard playlist and scan sdcard music save to it");
    sdcard_list_create(&sdcard_list_handle);
    sdcard_scan(sdcard_url_save_cb, "/sdcard", 0, (const char *[]) {"mp3"}, 1, sdcard_list_handle);

    ESP_LOGI(AUDIOBOARDTAG, "[ 2 ] Start codec chip");
     //comment again
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    // audio_board_handle_t board_handle;
    // audio_board_handle_init(board_handle);

    ESP_LOGI(AUDIOBOARDTAG, "[ 3 ] Create and start input key service");
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set;
    input_ser = input_key_service_create(&input_cfg);
    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, (void *)board_handle);

    ESP_LOGI(AUDIOBOARDTAG, "[4.0] Create audio pipeline for playback");
     //comment again
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    
    // audio_pipeline_handle_init(pipeline);
    mem_assert(pipeline);

    ESP_LOGI(AUDIOBOARDTAG, "[4.1] Create i2s stream to write data to codec chip");
    //comment again
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_config.sample_rate = 48000;
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    
    // i2s_stream_handle_init(i2s_stream_writer);

     ESP_LOGI(AUDIOBOARDTAG, "[4.2] Create http stream to read data");
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_cfg.event_handle = _http_stream_event_handle;
    http_cfg.type = AUDIO_STREAM_READER;
    http_cfg.enable_playlist_parser = true;
    http_stream_reader = http_stream_init(&http_cfg);

    ESP_LOGI(AUDIOBOARDTAG, "[4.3] Create mp3 decoder to decode mp3 file");
    //comment again
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);

    // mp3_decoder_handle_init(mp3_decoder_handle_init);

    ESP_LOGI(AUDIOBOARDTAG, "[4.4] Create resample filter");
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_handle = rsp_filter_init(&rsp_cfg);

    ESP_LOGI(AUDIOBOARDTAG, "[4.5] Create fatfs stream to read data from sdcard");
    char *url = NULL;
    sdcard_list_current(sdcard_list_handle, &url);
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_READER;
    fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);
    audio_element_set_uri(fatfs_stream_reader, url);

    ESP_LOGI(AUDIOBOARDTAG, "[4.6] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
    audio_pipeline_register(pipeline, rsp_handle, "filter");
    
    //comment again
    audio_pipeline_register(pipeline, mp3_decoder, "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");
    
    audio_pipeline_register(pipeline, http_stream_reader, "http");

    //register_duplicate_elements();

    ESP_LOGI(AUDIOBOARDTAG, "[4.7] Link it together [sdcard]-->fatfs_stream-->mp3_decoder-->resample-->i2s_stream-->[codec_chip]");
    
     //TODO needs to be called only when you want to use the sdcard. So only when in the sd menu.
    const char *link_tag[4] = {"file", "mp3", "filter", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 4);

    // ESP_LOGI(AUDIOBOARDTAG, "[4.7] Link it together http_stream-->mp3_decoder-->i2s_stream-->[codec_chip]");
    
    // //TODO needs to be called only when you want to use the radio. So only when in the radio menu. 
    // const char *link_tag[3] = {"http", "mp3", "i2s"};
    // audio_pipeline_link(pipeline, &link_tag[0], 3);

    ESP_LOGI(AUDIOBOARDTAG, "[5.0] Set up event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(AUDIOBOARDTAG, "[5.1] Listen for all pipeline events");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(AUDIOBOARDTAG, "[6.0] Creating songList");
    get_all_songs_from_SDcard();
    
    //End configuration

    radioChannelNames[0] = "Back";
    radioChannelNames[1] = "Sky Radio";
    radioChannelNames[2] = "NPO Radio 2";
    radioChannelNames[3] = "Qmusic";

    //Main loop, waits for functions to be called
    while (1) {
        //Error check
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(AUDIOBOARDTAG, "Event interface error : %d", ret);
        }
    }
}

void stop_audio(void){
    ESP_LOGI(AUDIOBOARDTAG, "Stop audio_pipeline");
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

//Starts task to start the sdcard
void start_audio_task(){
    xTaskCreate(&audio_start, "audio start", 4096, NULL, 5, NULL);
}

void play_radio(int radioChannel){
    if(radioChannel < 0 || radioChannel > AMOUNT_OF_RADIO_CHANNELS) {
        ESP_LOGE(AUDIOBOARDTAG, "Invalid radio channel in play_radio()");
        return;
    }

    // Stop playing audio
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    
    if(playingRadio == false) {      
        // Change linkage to radio
        const char *link_tag[3] = {"http", "mp3", "i2s"};
        audio_pipeline_link(pipeline, &link_tag[0], 3);
    }

    playingRadio = true;

    // Start radio
    audio_element_set_uri(http_stream_reader, radioChannels[radioChannel]);
    audio_pipeline_reset_ringbuffer(pipeline);
    audio_pipeline_reset_elements(pipeline);
    audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
    audio_pipeline_run(pipeline);
}

void stop_radio(){
    playingRadio = false;

    // Stop playing 
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    // Change linkage to SD
    const char *link_tag[4] = {"file", "mp3", "filter", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 4);
}