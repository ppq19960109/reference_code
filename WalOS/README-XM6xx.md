# EMMC存储文件系统
 挂载路径：`/var/sd`

## 目录结构
	app
	├── bin
	├── data
	│   ├── classifier
	│   ├── db
	│   ├── fonts
	│   ├── images
	│   └── users
	├── tmp
	├── upgrade
	└── usr

## 目录说明

### bin
 通用的工具，包括脚本、第三方应用程序等.

### usr
 应用程序home目录

### upgrade
 上传升级文件

### data
 数据类相关.

 - classifier: 算法分类器
 - db: 数据库
 - ecc: 密钥
 - fonts: 字体
 - images: 抓取的图片
 - users: 用户相关的图片

### tmp
 临时文件，可以清空

# 升级流程
 - http上传升级文件到指定目录/var/sd/upgrade，升级文件重命名upgrade.bin
 - 系统启动检测是否存在upgrade.bin文件。如果有，则校验并解压
 - 解压之后，必须包含upgrade.sh脚本，如有则执行upgrade.sh脚本。如无，则清空upgrade目录，升级失败
 - 启动应用程序
 
## 打包方法
	sha256sum upgrade.tar.gz | awk '{printf $1}' > hash.txt
	cat hash.txt upgrade.tar.gz > upgrade.bin

## 解包方法
	参考/var/sd/app/bin/upgrade.sh

## device-id
 - 生成规则(eg: 01138d122ye19)


		01       13  8d     12  2ye     19
		工厂编号  年  随机数  月  随机数  日
