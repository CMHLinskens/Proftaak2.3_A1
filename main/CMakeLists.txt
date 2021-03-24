set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_REQUIRES audio_pipeline 
                       audio_stream 
                       esp_peripherals 
                       input_key_service 
                       playlist 
                       nvs_flash 
                       esp32-smbus 
                       esp32-i2c-lcd1602 
                       esp32-i2c-rotaryEncoder 
                       lcd-menu 
                       wifi_connect 
                       clock-sync 
                       http_request
                       json
                       audio-board
                       goertzel
                       microphone
                       alarm)
register_component()