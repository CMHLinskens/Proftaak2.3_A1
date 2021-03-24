set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "mp3-radio.c")
set(COMPONENT_REQUIRES nvs_flash
                       audio_pipeline
                       audio_stream
                       esp-adf-libs
                       esp_peripherals
                       audio-board)
register_component()