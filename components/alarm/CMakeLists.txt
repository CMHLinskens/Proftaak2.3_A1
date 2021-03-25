set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "alarm.c")
set(COMPONENT_PRIV_REQUIRES clock-sync 
                            audio-board
                            )
register_component()
