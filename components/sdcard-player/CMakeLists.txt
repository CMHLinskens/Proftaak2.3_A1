set(COMPONENT_ADD_INCLUDEDIRS include )
set(COMPONENT_SRCS "sdcard_player.c")
set(COMPONENT_REQUIRES audio_pipeline 
                       audio_stream 
                       esp_peripherals 
                       input_key_service 
                       playlist 
                       nvs_flash
                       audio-board)
register_component()


