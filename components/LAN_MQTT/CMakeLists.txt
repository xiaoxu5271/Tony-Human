idf_component_register(SRCS ./lan_mqtt.c
                       INCLUDE_DIRS include                       # Edit following two lines to set component requirements (see docs)
                    #    REQUIRES )
                    REQUIRES W5500_APP W5500_LIB Smartconfig Json_parse json E2prom Http Bluetooth My_Mqtt )

