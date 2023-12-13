#!/bin/sh

FILE_NAME="SN_20200810_22"

for amp in `seq 300 50 1000`
do    
    echo ${amp}
    /home/hamada/work/control_AFG/vxi11_1.10_mod/vxi11_cmd 192.168.10.5 "source2:VOLTage:AMPLitude  ${amp}e-3"

    ./DAQ -t base -w -e 2000
    mv receive.dat ../rawdata/${FILE_NAME}_${amp}.dat

    sleep 1

done


