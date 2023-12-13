#!/bin/sh

FILE_NAME="SN_20210520_A8_ch0"

for amp in `seq 50 10 190`
do    
    echo ${amp}
    mini_amp=`expr ${amp} / 10`

    /home/hamada/work/control_AFG/vxi11_1.10_mod/vxi11_cmd 192.168.10.5 "source2:VOLTage:AMPLitude  ${amp}e-3"

    ./DAQ -t normal -w -e 2000
    mv receive.dat ../rawdata/${FILE_NAME}_${mini_amp}.dat

done

for amp in `seq 200 50 450`
do    
    echo ${amp}
    mini_amp=`expr ${amp} / 10`

    /home/hamada/work/control_AFG/vxi11_1.10_mod/vxi11_cmd 192.168.10.5 "source2:VOLTage:AMPLitude  ${amp}e-3"

    ./DAQ -t normal -w -e 2000
    mv receive.dat ../rawdata/${FILE_NAME}_${mini_amp}.dat

done

