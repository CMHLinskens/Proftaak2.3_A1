// #include "board.h"
// #include "audio_hal.h"
// #include "audio_pipeline.h"
// #include "audio_element.h"
// #include "mp3_decoder.h"
// #include <stdbool.h>

// #define AUDIOBOARDTAG "Audio board tag"

// audio_board_handle_t board_handle;
// audio_pipeline_handle_t pipeline_handle;
// audio_element_handle_t i2s_stream_writer, mp3_decoder;
// bool registerCheck = false;

// void audio_board_handle_init(audio_board_handle_t *new_board_handle)
// {
//     ESP_LOGI(AUDIOBOARDTAG, "Start codec chip");

//     if(board_handle != NULL){
//         new_board_handle = board_handle;
//         return;
//     }

//     board_handle = audio_board_init();
//     audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
//     new_board_handle = board_handle;
// }

// void audio_pipeline_handle_init(audio_pipeline_handle_t *new_pipeline_handle)
// {
//     ESP_LOGI(AUDIOBOARDTAG, "Create audio pipeline for playback");

//      if(pipeline_handle != NULL){
//         new_pipeline_handle = pipeline_handle;
//         return;
//     }

//     audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
//     pipeline_handle = audio_pipeline_init(&pipeline_cfg);
//     new_pipeline_handle = pipeline_handle;
// }

// void i2s_stream_handle_init(audio_element_handle_t *new_i2s_stream_writer)
// {
//     ESP_LOGI(AUDIOBOARDTAG, "Create i2s stream to write data to codec chip");

//     if(i2s_stream_writer != NULL){
//         new_i2s_stream_writer = i2s_stream_writer;
//         return;
//     }

//     i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
//     i2s_cfg.type = AUDIO_STREAM_WRITER;
//     i2s_stream_writer = i2s_stream_init(&i2s_cfg);
//     new_i2s_stream_writer = i2s_stream_writer;
// }

// void mp3_decoder_handle_init(audio_element_handle_t *new_mp3_decoder)
// {

//     ESP_LOGI(AUDIOBOARDTAG, "Create mp3 decoder to decode mp3 file");

//     if(mp3_decoder != NULL){
//         new_mp3_decoder = mp3_decoder;
//         return;
//     }

//     mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
//     mp3_decoder = mp3_decoder_init(&mp3_cfg);
// }

// void register_duplicate_elements()
// {
//     if(!registerCheck){
    
//     ESP_LOGI(AUDIOBOARDTAG, "Register mp3 and i2s elements to audio pipeline");

//     audio_pipeline_register(pipeline_handle, mp3_decoder, "mp3");
//     audio_pipeline_register(pipeline_handle, i2s_stream_writer, "i2s");
//     registerCheck = true;
//     }
// }
