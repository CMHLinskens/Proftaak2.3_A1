set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "clock-sync.c")
set(COMPONENT_PRIV_REQUIRES lcd-menu 
                            esp32-i2c-lcd1602 
                            esp32-smbus
                            audio-board
                            )
register_component()
