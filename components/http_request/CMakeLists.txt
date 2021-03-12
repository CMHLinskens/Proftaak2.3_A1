set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "http_request.c")
set(COMPONENT_PRIV_REQUIRES "esp32-i2c-lcd1602" "esp32-smbus" "clock-sync" "sdcard-player")
register_component()
