# For IDF v4.0x:
#idf_component_register(SRCS "smbus.c" INCLUDE_DIRS include)

set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "smbus.c")
register_component()
