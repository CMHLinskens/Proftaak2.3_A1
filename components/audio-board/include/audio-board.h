#ifndef audio_board_H
#define audio_board_H

void audio_board_handle_init(audio_board_handle_t *new_board_handle);
void audio_pipeline_handle_init(audio_pipeline_handle_t *new_pipeline_handle);
void i2s_stream_handle_init(audio_element_handle_t *new_i2s_stream_writer);
void mp3_decoder_handle_init(audio_element_handle_t *new_mp3_decoder);
void register_duplicate_elements();

#endif