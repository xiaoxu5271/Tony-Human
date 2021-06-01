file(GLOB SRC_LIST "*.c")
idf_component_register(SRCS ${SRC_LIST}
                       INCLUDE_DIRS include                       # Edit following two lines to set component requirements (see docs)
                       REQUIRES 
                       PRIV_REQUIRES )

