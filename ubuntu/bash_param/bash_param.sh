#!/bin/sh

#$#:传递给程序的总的参数数目
#$n:表示第几个参数，$1 表示第一个参数，$2 表示第二个参数 ... 　　
#$0:当前程序的名称
#$*:传递给程序的所有参数组成的字符串
#$@:以"参数1" "参数2" ... 形式保存所有参数


#compare strings
if [ "$1" = "abcde" ];then 
	echo $1 match "abcde"
elif [ "$1" = "none" ];then
	echo $1 match "none"
else
	echo $1 not match "abcde" or "none"
fi



