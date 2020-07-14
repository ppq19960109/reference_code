# xm630
## 准备
 - usb转ttl模块1-2个
 - RJ45接口网线1根
 - 12v电源适配器一个
 - ttl串口烧录线2根，一根烧录uboot，一根烧录isp
 - 电脑一台，并安装usb转ttl模块的驱动
 - 安装雄迈串行FLASH烧录器软件，tftp软件，终端模拟软件(putty).

## 流程
 - 烧录isp程序
 - 烧录uboot
 - 下载filesystem
 
## 烧录isp
 - 使用ttl串口烧录线(isp烧录线)一端连接usb转ttl模块，另一端连接xm630主板，注意连接线的线序。如图所示：
 - 给xm630主板提供电源
 - 打开雄迈串行FLASH烧录器软件
 - 选择正确的串口，波特率选择115200
 - 加载烧录程序，如果所示：
 - 点击烧录按钮，等待烧录成功。
 - 重复以上步骤烧录另一个主板

## 烧录uboot

 过程类似与烧录isp

 - 使用ttl串口烧录线(xm630烧录线)一端连接usb转ttl模块，另一端连接xm630主板
 - 给xm630主板提供电源
 - 打开雄迈串行FLASH烧录器软件
 - 选择正确的串口，波特率选择57600
 - 加载烧录程序，如果所示：
 - 点击烧录按钮，等待烧录成功。
 - 重复以上步骤烧录另一个主板

## 下载filesystem
 - 打开终端模拟软件，登录操作系统
 - 使用RJ45接口网线连接xm630主板和电脑
 - 输入指令

		setenv ipaddr 192.168.20.10 
		setenv serverip 192.168.20.99
		setenv ethaddr 00:01:02:03:04:00
		setenv bootargs mem=40M console=ttyAMA0,115200 root=/dev/mtdblock2 rootfstype=cramfs mtdparts=xm_sfc:256K(boot),1536K(kernel),1280K(romfs),4544K(user),256K(custom),320K(mtd)
		setenv xmuart nfsc 0
		saveenv
		tftp filesystem.img
		flwrite
		reset

## 上传程序

	mount -t nfs -o nolock -o tcp -o rsize=32768,wsize=32768 192.168.20.196:/home/heyan /var/nfs
	
	# format emmc
	cd /var/nfs/git_workspace/emmc/bin
	./mkfs.vfat /dev/mmcblk0
	cp /var/nfs/git_workspace/emmc/* /var/sd/ -r
	
	# ntpdate
	/var/sd/bin/ntpdate.sh
	
	# write prikey,pubkey,device-id
	cd /var/nfs/git_workspace/product
	./product.sh



