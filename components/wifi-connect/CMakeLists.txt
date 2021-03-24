set(COMPONENT_ADD_INCLUDEDIRS include )
set(COMPONENT_SRCS "wifi-connect.c")
set(COMPONENT_REQUIRES  esp_common 
                        esp_netif
                        )
register_component()


