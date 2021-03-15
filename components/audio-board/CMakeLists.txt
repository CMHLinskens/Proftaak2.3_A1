set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "audio-board.c")
set(COMPONENT_REQUIRES audio_hal
                       audio_pipeline
                       esp_peripherals 
                       audio_board)
register_component()