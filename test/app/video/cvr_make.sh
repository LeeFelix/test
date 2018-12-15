#! /bin/bash

#get config
if [[ $enable_gb28181 =~ "yes" ]];then
	echo "rk_gb28181 is enable in video app"
	export RK_PROTOCOL_GB28181=YES
fi

if [[ $enable_fwk_msg =~ "yes" ]];then
	echo "fwk_msg is enable in video app"
	export RK_FWK_MSG=YES
fi

#get parameter for "-j2~8 and clean"
result=$(echo "$1" | grep -Eo '*clean')
if [ "$result" = "" ];then
	mulcore_cmd=$(echo "$1" | grep '^-j[0-9]\{1,2\}$')
elif [ "$1" = "clean" ];then
	make clean
elif [ "$1" = "distclean" ];then
	make clean
fi

outresPath="../../out/system/share/minigui/res/images/"
if [ ! -d $outresPath ];
then
  mkdir ../../out/system/share/minigui/res/images
fi

make clean
make $mulcore_cmd
cp video ../../out/system/bin

#call just for buid_all.sh
if [ "$1" = "cleanthen" ] || [ "$2" = "cleanthen" ];then
	make clean
fi

