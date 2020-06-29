# Peter-Human

任务名                 优先级 
vTask_view_Work	        5         	    
Uart0_Task          	9
Human_Task          	4
Sht30_Task      	    3
Led_Task       	        2
lan_mqtt_task  	        10
RJ45_Check_Task	        7	
http_get_task  	        6
user_key_cd_task	    8

注意：
1，OTAs升级需要，menuconfig 中修改 HTTP CLIENT --->  Enable https  打开



2019-05-24 18:42:05 优化网络切换，根据解析DNS的情况来切换网络。 优化上传WIFI状态。

2019-05-24 18:42:22 增加metadata 更新设置，更新温湿度采集频率，数据上传频率

2019-05-27 17:17:54 修改SHT30读取模式为 single shot ，但数字为0时，不上传，规避错误值。

2019-05-28 15:49:53 增加metadata设置网络选择模式，net_mode: 0自动，1有线网，2无线网。 开关LED指示灯，cg_data_led ： 0关闭，1打开

2019-05-29 11:08:59 增加MQTT设置WIFI，修复 tcpuart 重置wifi误清零的BUG。

2019-05-29 18:47:15 增加MQTT配置有线网，可选DHCP模式，增加url上传当前有线网ip，当使用WIFI时，不上传

2019-06-05 18:42:36 修改lan MQTT 接收方式，增加容错机制，断开后自动重连。  修改解析DNS的socket 与 lan发送的socket 不同。

2019-06-06 18:06:28 增加 lan OTA 测试，此版本不可用

2019-06-10 20:45:31 修改lan入网初始化方式，修改解析DNS函数，修改DHCP函数，增加超时退出，有线网OTA可以接受数据流 数据。

2019-06-14 18:50:32 增加 有线网OTA 功能，请求数据，需要用GET ，Connection:close 不可以设置成永久连接 该参数需要关闭。本版本OTA地址是                     写死的，暂未加入接口。

2019-06-17 13:37:48 有线网OTA 增加MQTT请求。

2019-06-17 18:34:08 修改W5500域名解析BUG ，#define DNS_WAIT_TIME 1000 ///< Wait response time. unit 1s. 设置过小会导致域名解析不成功

2019-06-19 15:28:21 1，修改ap模式下与STA模式下等待信号量 为不同。 1，修改有人时上传数据方式。3，修改激活不成功也返回成功的BUG

2019-06-19 16:40:58 修复有线网进入 SOCK_CLOSE_WAIT 中不退出BUG

2019-06-21 16:47:52 修改wifi OTA 方式，屏蔽OTA失败重试部分。

2019-06-21 19:01:32 修改 数据上传方式，采用信号量的方式  need_send 

2019-06-24 14:15:21 测试中发现lan MQTT 中 lan_connet 会出现卡死现象，修改该函数，在函数中的while()中加入 循环打印、延时

2019-06-24 14:26:05 修复LAN OTA 从EEROM 读取ota url bug 

2019-07-04 14:18:16 修复 新设备 新EEPROM，导致DHCP模式选择出现错误， 改为不从 EEPROM 中读取，上电默认为1，开启DHCP。

2020-01-02 18:15:36 修改蓝牙配网bug,目前存在一个问题，当把正常连接的wifi配置成错误的时候，系统会卡在奇怪的地方，目前解决办法在wifi even handle 中重启设备

2020-01-14 17:14:53 修改上传MAC为设备MAC，优化网络指示灯，正常工作，绿灯常亮，网络问题，红灯闪烁，平台返回数据错误，蓝灯闪烁

2020-06-29 17:39:53 HUM1-V0.2.12 发布测试  增加 W5500 mqtt publish ,增加检测指令，修改 回滚固件指示灯