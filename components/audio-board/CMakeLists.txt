set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "audio-board.c")
set(COMPONENT_REQUIRES  audio_hal
                        audio_pipeline
                        esp_peripherals 
                        audio-board
                        nvs_flash
                        audio_stream
                        esp-adf-libs
                        input_key_service 
                        playlist
                        goertzel
                        clock-sync)
register_component()