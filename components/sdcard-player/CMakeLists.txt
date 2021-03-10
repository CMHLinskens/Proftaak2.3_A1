set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "sdcard_player.c")
set(COMPONENT_PRIV_INCLUDEDIRS $($ENV{ADF_PATH}/components/audio_pipeline/include))
register_component()
