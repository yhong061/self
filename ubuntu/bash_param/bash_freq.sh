#!/system/bin/sh

case "$1" in

  "1" )
  freq=696000
  echo 1 : ${freq} 
  echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
  echo ${freq} > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  ;;
  "2" )
  freq=1008000
  echo 2 : ${freq} 
  echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
  echo ${freq} > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  ;;
  "3"  )
  freq=1260000
  echo 3 : ${freq} 
  echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
  echo ${freq} > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  ;;
  "4")
  freq=1404000
  echo 4 : ${freq} 
  echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
  echo ${freq} > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  ;;
  "5")
  freq=1536000
  echo 5 : ${freq} 
  echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
  echo ${freq} > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  ;;
  *)
  echo no parameters
  echo 1 : 696000 
  echo 2 : 1008000 
  echo 3 : 1260000 
  echo 4 : 1404000 
  echo 5 : 1536000
  ;;
esac



