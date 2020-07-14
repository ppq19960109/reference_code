1、文件系统的制作，见/pack目录下的readme


2、XM530_SIMPLE SDK 安装、升级(系统环境的搭建)
	1)、SPI FLASH 地址空间说明，以8M为例
		|    256K   |   1536K  |   2560K  |  2816K   |  1024K  |
        	|-----------|----------|----------|----------|---------|
        	|    boot   |  kernel  |   romfs  |   user   |   mtd   |
	2)、安装和升级说明
	    A)、uboot的烧写，使用os/tools/pctools提供的FLASH烧写器，将uboot烧写0-0x30000地址。
	    B)、烧写总文件系统filesystem.img
		将映像文件烧写到开发板上的方法有多种，这里介绍两种方式，X/YMODEM协议烧写和TFTP网络烧写，建议使用后者，其烧写速度非常快。
		①通过X/Ymodem协议烧写：在一般的远程登录工具都有该协议，烧写步骤如下：
  		  a). 启动开发板，Ctrl+C进入uboot模式
		  b). 输入loadx 或者loady命令(二选一)
		  c).远程工具选择：文件->传输->X/YMODEM->发送->选择:filesystem.img
		  d). flwrite
		  e). reset
		②通过TFTP网络烧写
		  a). 启动开发板，Ctrl+C进入uboot模式
		  b).设置网络参数：设置serverip(tftp服务器的ip)、ipaddr(单板ip)和ethaddr(单板的MAC地址)。
			setenv serverip xx.xx.xx.xx
    			setenv ipaddr xx.xx.xx.xx 
    			setenv ethaddr xx:xx:xx:xx:xx:xx
    			setenv netmask xx.xx.xx.xx
    			setenv gatewayip xx.xx.xx.xx
			save				#保存设置
    			ping serverip			#确保网络畅通。	
		  c). 将filesystem.img文件放入TFTP服务器目录下
		  d). tftp filesystem.img
		  e). flwrite 
		  f). reset	
	   C)、修改uboot环境变量.
		  a). 启动开发板，Ctrl+C进入uboot模式
		  b). setenv xmuart nfsc 0/1 	#开启/关闭串口打印
		  c). setenv bootargs mem=32M console=ttyAMA0,115200 root=/dev/mtdblock1 rootfstype=cramfs mtdparts=xm_sfc:256K(boot),4096K(romfs),2816K(user),1024K(mtd)    #设置启动参数
		  d). save
		  e). reset


3、simple SDK使用说明
	    A). 用户登录
   		    root(用户名): root
		    password(密码)：空
	    B). 运行例程
	        a). cd /var
			b). sample_venc +参数  或者  sample_audio +参数
									  如:  sample_venc 0/1/2
			c). Ctrl+c 停止程序


