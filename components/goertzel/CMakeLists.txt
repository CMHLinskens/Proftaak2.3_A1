set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "goertzel.c")
set(COMPONENT_REQUIRES  audio_pipeline 
                        audio_stream 
                        esp_peripherals 
                        audio_board 
                        log
                        )
register_component()
