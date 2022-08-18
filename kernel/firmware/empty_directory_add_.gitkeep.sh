#!/bin/bash
if [ -z "$1" ]; then
	echo Usage: $0 \<Target directory\>
	DIR=ramdisk
else
	echo "DIR:$1"
	DIR=$1
fi

echo -e Null directory list: 

for i in $(find $DIR -type d -print)
do
	TMP=$(ls $i)
	if [ "$TMP" = "\." ]; then
		continue
	elif [ -z "$TMP" ]; then
		touch $i"/.gitkeep"
 		chmod 444 $i"/.gitkeep"
	fi
done
