idf_component_register(SRCS ./MQTTConnectClient.c 
./MQTTConnectServer.c 
./MQTTDeserializePublish.c 
./MQTTFormat.c 
./MQTTPacket.c 
./MQTTSerializePublish.c 
./MQTTSubscribeClient.c 
./MQTTSubscribeServer.c 
./MQTTUnsubscribeClient.c 
./MQTTUnsubscribeServer.c 
./w5500.c ./w5500_dhcp.c 
./w5500_dns.c 
./w5500_socket.c 
./wizchip_conf.c
                       INCLUDE_DIRS include                       # Edit following two lines to set component requirements (see docs)
                       REQUIRES 
                       PRIV_REQUIRES )

