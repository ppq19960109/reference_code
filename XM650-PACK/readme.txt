1���ļ�ϵͳ����������/packĿ¼�µ�readme


2��XM530_SIMPLE SDK ��װ������(ϵͳ�����Ĵ)
	1)��SPI FLASH ��ַ�ռ�˵������8MΪ��
		|    256K   |   1536K  |   2560K  |  2816K   |  1024K  |
        	|-----------|----------|----------|----------|---------|
        	|    boot   |  kernel  |   romfs  |   user   |   mtd   |
	2)����װ������˵��
	    A)��uboot����д��ʹ��os/tools/pctools�ṩ��FLASH��д������uboot��д0-0x30000��ַ��
	    B)����д���ļ�ϵͳfilesystem.img
		��ӳ���ļ���д���������ϵķ����ж��֣�����������ַ�ʽ��X/YMODEMЭ����д��TFTP������д������ʹ�ú��ߣ�����д�ٶȷǳ��졣
		��ͨ��X/YmodemЭ����д����һ���Զ�̵�¼���߶��и�Э�飬��д�������£�
  		  a). ���������壬Ctrl+C����ubootģʽ
		  b). ����loadx ����loady����(��ѡһ)
		  c).Զ�̹���ѡ���ļ�->����->X/YMODEM->����->ѡ��:filesystem.img
		  d). flwrite
		  e). reset
		��ͨ��TFTP������д
		  a). ���������壬Ctrl+C����ubootģʽ
		  b).�����������������serverip(tftp��������ip)��ipaddr(����ip)��ethaddr(�����MAC��ַ)��
			setenv serverip xx.xx.xx.xx
    			setenv ipaddr xx.xx.xx.xx 
    			setenv ethaddr xx:xx:xx:xx:xx:xx
    			setenv netmask xx.xx.xx.xx
    			setenv gatewayip xx.xx.xx.xx
			save				#��������
    			ping serverip			#ȷ�����糩ͨ��	
		  c). ��filesystem.img�ļ�����TFTP������Ŀ¼��
		  d). tftp filesystem.img
		  e). flwrite 
		  f). reset	
	   C)���޸�uboot��������.
		  a). ���������壬Ctrl+C����ubootģʽ
		  b). setenv xmuart nfsc 0/1 	#����/�رմ��ڴ�ӡ
		  c). setenv bootargs mem=32M console=ttyAMA0,115200 root=/dev/mtdblock1 rootfstype=cramfs mtdparts=xm_sfc:256K(boot),4096K(romfs),2816K(user),1024K(mtd)    #������������
		  d). save
		  e). reset


3��simple SDKʹ��˵��
	    A). �û���¼
   		    root(�û���): root
		    password(����)����
	    B). ��������
	        a). cd /var
			b). sample_venc +����  ����  sample_audio +����
									  ��:  sample_venc 0/1/2
			c). Ctrl+c ֹͣ����


