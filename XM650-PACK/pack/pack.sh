#!/bin/sh
PWD=`pwd`
TOOLS_DIR=$PWD/tools

if [ "$1" = "rm" ]
then
	rm -f $PWD/*.jffs2
        rm -f $PWD/*.cramfs
        rm -f $PWD/*.img
        rm -f $PWD/*.bin
else
        cp $PWD/uboot/u-boot-128MB-high.bin $PWD/u-boot.bin
        $TOOLS_DIR/mkimage -A arm -O linux -T kernel -C gzip -a 0x0 \
-e 0x30000 -n linux -d $PWD/u-boot.bin  $PWD/u-boot.bin.img

        cp $PWD/uboot/uboot-128MB-env.bin $PWD/u-boot.env.bin
        $TOOLS_DIR/mkimage -A arm -O linux -T kernel -C gzip -a 0x30000 \
-e 0x40000 -n linux -d $PWD/u-boot.env.bin  $PWD/u-boot.env.img

	$TOOLS_DIR/mkimage -A arm -O linux -T kernel -C gzip -a 0x40000 \
-e 0x1e0000 -n linux -d $PWD/kernel/uImage  $PWD/kernel.img

	$TOOLS_DIR/mkcramfs $PWD/romfs/ $PWD/romfs.cramfs
	$TOOLS_DIR/mkimage -A arm -O linux -T kernel -C gzip -a 0x1e0000 \
-e 0x300000 -n linux -d $PWD/romfs.cramfs  $PWD/romfs.cramfs.img


	$TOOLS_DIR/mkcramfs $PWD/usr/ $PWD/user.cramfs
	$TOOLS_DIR/mkimage -A arm -O linux -T kernel -C gzip -a 0x300000 \
-e 0x770000 -n linux -d $PWD/user.cramfs $PWD/user.cramfs.img
	
	$TOOLS_DIR/mkcramfs $PWD/custom/ $PWD/custom.cramfs
	$TOOLS_DIR/mkimage -A arm -O linux -T kernel -C gzip -a 0x770000 \
		-e 0x7b0000 -n linux -d $PWD/custom.cramfs $PWD/custom.cramfs.img

#	$TOOLS_DIR/mkfs.jffs2 -d $PWD/custom -l -e 0x40000 -o $PWD/custom.jffs2
#	$TOOLS_DIR/mkimage -A arm -O linux -T kernel -C gzip -a 0x770000 \
#		-e 0x7b0000 -n linux -d $PWD/custom.jffs2 $PWD/custom.jffs2.img
	
	$TOOLS_DIR/mkfs.jffs2 -d $PWD/mtd -l -e 0x50000 -o $PWD/mtd.jffs2
	$TOOLS_DIR/mkimage -A arm -O linux -T kernel -C gzip -a 0x7b0000 \
		-e 0x800000 -n linux -d $PWD/mtd.jffs2 $PWD/mtd.jffs2.img

	$TOOLS_DIR/mkpkt $PWD/filesystem.img $PWD/kernel.img $PWD/romfs.cramfs.img \
$PWD/user.cramfs.img $PWD/custom.cramfs.img $PWD/mtd.jffs2.img

        $TOOLS_DIR/zip -j $PWD/sd_upgrade.bin $PWD/kernel.img $PWD/romfs.cramfs.img  \
$PWD/user.cramfs.img
 
        $TOOLS_DIR/zip -j $PWD/sd_upgrade_all.bin $PWD/u-boot.bin.img $PWD/kernel.img \
$PWD/user.cramfs.img  $PWD/romfs.cramfs.img $PWD/u-boot.env.img

        $TOOLS_DIR/mkall_8M $PWD/upall.bin  \
                $PWD/u-boot.bin       0       30000   \
                $PWD/u-boot.env.bin   30000   40000   \
                $PWD/kernel/uImage    40000   1e0000  \
                $PWD/romfs.cramfs     1e0000  300000  \
                $PWD/user.cramfs      300000  770000  \
                $PWD/custom.cramfs    770000  7b0000  \
                $PWD/mtd.jffs2        7b0000  800000  

	#cp -a $PWD/*.img $PWD/../
        #cp -a $PWD/*.bin $PWD/../
fi

