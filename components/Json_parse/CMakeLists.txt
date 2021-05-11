idf_component_register(SRCS ./Json_parse.c
                       INCLUDE_DIRS include                       # Edit following two lines to set component requirements (see docs)
                    #    REQUIRES )
                    REQUIRES Smartconfig json E2prom Http Bluetooth ota Led my_base64 Human W5500_APP sound )

