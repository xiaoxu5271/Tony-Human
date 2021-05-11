idf_component_register(SRCS ./w5500_driver.c
                       INCLUDE_DIRS include                       # Edit following two lines to set component requirements (see docs)
                    #    REQUIRES )
                       PRIV_REQUIRES W5500_LIB Smartconfig my_spi LAN_MQTT My_Mqtt E2prom Json_parse Http Led )

