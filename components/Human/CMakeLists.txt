idf_component_register(SRCS ./Human.c
                       INCLUDE_DIRS include                       # Edit following two lines to set component requirements (see docs)
                    #    REQUIRES )
                       PRIV_REQUIRES Json_parse E2prom Led Smartconfig Http ServerTimer)

