## 硬件测试项目
 1. 继电器控制
 2. 按键
 3. 补光灯
 4. 双摄
 5. lcd
 6. 网络通讯
 7. 喇叭
 8. RTC
 9. RS485(暂时不用)

## 硬件测试方法
 - 通过按键来控制补光灯开关，双摄的切换。 测试项：2、3、4、5、6
 - 程序运行播放语音，看是否有“你好欢迎光临”的声音播放。 测试项：7
 - 程序运行获取rtc时间。 如果rtc获取成功，继电器开，否则继电器关。测试项:1、6、8
 

## 软件测试项目
	kill -9 `ps -ef | grep fdrserver | grep -v grep |awk '{print $1}'`
	kill -9 `ps -ef | grep isp | grep -v grep |awk '{print $1}'`
	
	mount -t nfs -o nolock -o tcp -o rsize=32768,wsize=32768 192.168.20.196:/home/heyan /var/nfs
	
	cd /var/sd/usr
	./fdr_test
	
	
	/var/sd/bin/busybox vi /var/sd/bin/startup.sh
	
	/var/sd/bin/ntpdate.sh
