#!/bin/sh

filesize=8192
offset=0x7800

#write
while :
do
	./eeprom-clone -w -o $offset -l $filesize -f $1 1>/dev/null 2>/dev/null
	ret=`./eeprom-clone -c -o $offset -l $filesize -f $1 1>/dev/null 2>/dev/null`
	eq=`echo $ret | grep NOT`
	if [ "$eq" != "" ];then
		echo "write caltable fail!!! retry" 1>/dev/null 2>/dev/null
	else
		break
	fi
done

echo 0
