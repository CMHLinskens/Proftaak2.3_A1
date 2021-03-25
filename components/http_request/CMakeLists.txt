set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "http_request.c")
set(COMPONENT_PRIV_REQUIRES lwip
                            json
                            )
register_component()
