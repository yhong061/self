操作步骤：
0.  将需要的驱动文件，copy 到目录/system/modules （只要执行 一次即可）
	1） 串口执行命令：mount -o remount,rw /system
	2） 将 xc6130.ko 复制到目录 /system/modules 
		  将 reg_xc6130.ko 复制到目录 /system/modules
	3）将可执行文件test_mem 复制到目录 /system/bin
	4）将调试脚本 insmod_xc6130.sh 复制到目录 /sdcard/
	5） 串口执行命令： sync
	6）串口执行命令：chmod 777 /system/bin/test_mem

1.  直接验证噪点问题
	直接打开相机应用，验证测试噪点问题

2.  自行配置寄存器，调试噪点问题
	1) 每次上电，都加载驱动，串口执行脚本： sh /sdcard/insmod_xc6130.sh
	2) 开始调试，直接输入命令可查看使用方法：/system/bin/./test_mem
				===============================
				sample: 
				./test_mem wb 0             : 配置旧的白平衡参数
				./test_mem wb 1             : 配置新的白平衡参数
				./test_mem reg r 0xreg      : 读指定寄存器0xreg，寄存器必须以0x开头
				./test_mem reg w 0xreg 0xval: 写指定寄存器0xreg，值为0xval, 寄存器和值必须以0x开头
				./test_mem nr 0             : 配置旧的降噪参数
				./test_mem nr 1             : 配置新的降噪参数
				===============================	
		示例:
		  1. 配置旧的白平衡参数:
			# /system/modules/./test_mem wb 0 
			[main:384] cmd= wb 0
			------------wb_array_old-----------

		  2. 配置新的降噪参数:
			# /system/modules/./test_mem nr 1                            
			[main:401] cmd= nr 1
			------------nr_array_new-----------
			
			3. 读寄存器0x2的值，返回值为0x43
			# /system/modules/./test_mem reg r 0x2                           
			[main:365] cmd= reg r 0x2
			[xc6130_reg_read]reg_num = 0x2 value = 0x43			

	
