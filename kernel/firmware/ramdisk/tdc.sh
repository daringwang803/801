while :
do
        date
        cat /sys/class/thermal/thermal_zone0/temp
        sleep 1
done
