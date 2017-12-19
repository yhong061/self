#!/system/bin/sh
echo start

i=1
addr=0x20100000
#addr=$1
while((i<=0x10))
do
	busybox devmem $addr 32
	let "addr += 0x4"
	let "i += 1"
done

