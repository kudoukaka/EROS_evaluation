#!/bin/sh

FILE_NAME="SN_20220621_69F4_ch0"

#for amp in `seq 50 50 350`
#do    
#    echo ${amp}
#    /home/hamada/work/control_AFG/vxi11_1.10_mod/vxi11_cmd 192.168.10.5 "source2:VOLTage:AMPLitude  ${amp}e-3"
#
#    ./DAQ -t normal -w -e 2000
#    mv receive.dat ../rawdata/${FILE_NAME}_${amp}.dat
#
#done


for amp in `seq 100 100 1000`
do    
    echo ${amp}
    /home/hamada/work/control_AFG/vxi11_1.10_mod/vxi11_cmd 192.168.10.5 "source2:VOLTage:AMPLitude  ${amp}e-3"

    ./DAQ -t normal -w -e 2000
    mv receive.dat ../rawdata/${FILE_NAME}_${amp}.dat

done

