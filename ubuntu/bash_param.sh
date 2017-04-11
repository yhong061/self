#!/system/bin/sh

#$#:传递给程序的总的参数数目
#$n:表示第几个参数，$1 表示第一个参数，$2 表示第二个参数 ... 　　
#$0:当前程序的名称
#$*:传递给程序的所有参数组成的字符串
#$@:以"参数1" "参数2" ... 形式保存所有参数


NAME=bash.sh
APP=/system/bin/./test_mem

AE_func()
{

  val=0x70
  case "$1" in
    "0" )
      val=0x0
    ;;
    "1" )
      val=0x10
    ;;
    "2" )
      val=0x20
    ;;
    "3" )
      val=0x40
    ;;
    "4" )
      val=0x60
    ;;
    "5" )
      val=0x80
    ;;
    "6" )
      val=0xa0
    ;;
    "7" )
      val=0xff
    ;;
    *)
      echo "warning: AE level is 0 to 7, use nonarl mode level"
    ;;
  esac

  echo AE Level param is "$1" , val is ${val}
  ${APP} reg w 0xfffd 0x80
  ${APP} reg w 0xfffe 0x14
  ${APP} reg w 0x003d ${val}
}


AG_func()
{

  val=0x78
  case "$1" in
    "0" )
      val=0x0
    ;;
    "1" )
      val=0x10
    ;;
    "2" )
      val=0x20
    ;;
    "3" )
      val=0x30
    ;;
    "4" )
      val=0x40
    ;;
    "5" )
      val=0x68
    ;;
    "6" )
      val=0x70
    ;;
    "7" )
      val=0x80
    ;;
    *)
      echo "warning: AG level is 0 to 7, use nonarl mode level"
    ;;
  esac

  echo AE Level param is "$1" , val is ${val}
  ${APP} reg w 0xfffd 0x80
  ${APP} reg w 0xfffe 0x14
  ${APP} reg w 0x0037 ${val}
}


if [ "$1" = "help" ];then
	echo "sh $NAME ae value  //value is : 0 to 7 "
	echo "sh $NAME ag value  //value is : 0 to 7 "
elif [ "$1" = "ae" ];then
	AE_func $2
elif [ "$1" = "ag" ];then
	AG_func $2
else
	echo $1 not match "abcde" or "none"
fi

