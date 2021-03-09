set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "clock-sync.c")
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
register_component()
