set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "microphone.c")
set(COMPONENT_REQUIRES audio_pipeline audio_stream esp_peripherals audio_board log goertzel)
register_component()
