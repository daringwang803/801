#! /bin/sh

set -e

red='\033[1;31m'       # 红
green='\033[1;32m'     # 绿
yellow='\033[1;33m'    # 黄
blue='\033[1;34m'      # 蓝
pink='\033[1;35m'      # 粉红
res='\033[0m'          # 清除颜色



seek_part=0            #1M for partition  
seek_kernel=2048       #16M for kernel
#seek_dtb=32768         #16M for dtb
seek_rootfs=67584      



echo "${red}make scm801 image....${res}"
cd ./temp
rm * -f
cd ../output
rm * -f
cd ../

cp ./img/SPLImage.bin ./temp/bootimg_AutoBoot.bin -f
cp ./img/u-boot.bin ./temp/u-boot_AutoBoot.bin -f
cp ./img/.PartList.cfg ./temp/PartList.cfg  -f


cd ./fit
#if [ -f kernel.img ]; then
#	rm kernel.img -f
#fi
./mkimage -f kernel.its kernel.img
mv kernel.img ../img/kernel.img -f


cd ../temp

#gpt     1M    1024*1024*512=2048   
#kernel  32M 	32*1024*1024/512 =65536
#ubuntu  1G	1024*1024*1024/512=2097152
#count = 2048+65536+2097152=2164736


echo "\n${red}make linux_burn_img...${res}"
dd if=/dev/zero of=linux_burn_image bs=512 count=2164736


echo "\n${red}make partition into linux_burn_image...${res}"
dd if=../img/part_ubuntu.bin of=linux_burn_image bs=512 seek=${seek_part}

#echo "\n${red}make kernel into linux_burn_image...${res} "
#dd if=../img/zImage of=linux_burn_image bs=512 seek=${seek_kernel}

echo "\n${red}make kernel into linux_burn_image...${res} "
dd if=../img/kernel.img of=linux_burn_image bs=512 seek=${seek_kernel}


#echo "\n${red}make dtb into linux_burn_image...${res}"
#dd if=../img/faraday-leo.dtb of=linux_burn_image bs=512 seek=${seek_dtb}

echo "\n${red}make rootfs into linux_burn_image...${res}"
dd if=../img/linux-ubuntu-rootfs.img of=linux_burn_image bs=512 seek=${seek_rootfs}

 
echo "\n${red}zip scm801_ubuntu18.04.img...${res}"
zip  ../output/scm801_ubuntu18.04.img ./*


rm * -f

cd ../


echo "\n${red}scm801 image OK !!${res}"
