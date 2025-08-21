# EROS_evaluation

This is a package for EROS performance check. Including pattern correction, binary data decoding, Noise level, linearity, signal-to-noise ratio, crosstalk, timing resolution & calibration.

## How to Install
```
cd EROS_evaluation
make -j4
```
It will also create 'anadata/', 'rawdata/' and 'reasult/' for futher analysis.

## How to Start DAQ
### Initial Setup

Chang IP address in 'SETUP_FILE/IP.txt', change baseline file in 'SETUP_FILE/baseline.txt'

### DAQ
```
./DAQ
```
It will start DAQ with external trigger mode. The options are described as the following:

<br>

-t: Trigger mode. "normal" for external trigger, "base" for base mode, "self" for self trigger, "off" for trigger off.

-n: How many board will be communicated. Default is 1.

-f: Frequency to update the graph (-f 500 to update the graph after every 500 events).

-z: Turn on zero suppress.

-d: Combine decision trigger & fast trigger.

-w: Save data in "receive.dat".

-e: Set event number.

## How to Perform Evaluation:

### Pattern correction:
1. Take 35000 data with base mode.
```
./DAQ -t base -w -e 35000
```
2. Generate pattern data
```
python make_baseline_32ch.py receive.dat
```
3. Copy the baseline data to baselinesfile/
```
cp baseline.txt baselinefile/baseline_<EROSName>.txt
```
Then you can remove the baseline.root and receive.dat.
### Decode:

```
./eros_d2r -f <raw_data_file_name> -b <baseline_file_name> -t <tc_file_name>
```
IMPORTANT: Please put your raw data file in 'rawdata/', 'baseline_\<EROSName\>.txt' in 'baselinefile/', and timing calibration file in 'TCdata/' (Not necessary. If no timing calibration files were found, it will run with default daltaT), the decoded data will be stored in 'anadata/'.

### Noise Level:
1. Take data in trigger mode, 1000 events will be enough.
```
./DAQ -t normal -w -e 1000
```
2. Move the data into 'rawdata/'.
```
mv receive.dat rawdata/<name> (e.g. 20231212_A608_noise.dat)
```
3. Decode the data and run noise analysis.
```
./eros_d2r -f <raw_data_file_name> -b <baseline_file_name> -t <tc_file_name>
./Noise -f <data_file_name>
```
The fitting result will be saved in 'noise.txt', and the figure will be saved as 'noise_level.png'.
### Linearity:
1. Take data by inputing a pulse from 0 to 3500mV to only one channel.
2. Move the data into 'rawdata/' and decode.
3. Run Linearity analysis.
```
./Linearity -f <data_file_name>
```
The fitting result will be saved in 'linearity.txt', and the figure will be saved as 'linearity.png'.

Please Check the fitting result carefully, and change the fitting region if needed. Same in the following analysis.
### Signal-to-noise Ratio:
1. Using the same data taken in Linearity check, and run S/R analysis
```
./SNR -f <data_file_name>
```
The fitting result will be saved in 'snr.txt', and the figure will be saved as 'signal_noise.png'.
### Crosstalk:
1. Using the same data taken in Linearity check, and run crosstalk analysis
```
./CrossT -f <data_file_name>
```
The fitting result will be saved in 'crosstalk.txt', and the figure will be saved as 'crosstalk.png'.
### Timing Resolution & Calibration:
1. Take waveform data with appropriate sine wave (Exp. 20MHz 200mV Vpp). Evaluate the timing resolution before calibration.
```
./TRes -f <data_file_name>
```
The resolution will be saved in 'tres.txt', the figure will be save in 'chip1_tres.png' and 'chip2_tres.png'.
2. Perform timing calibration.
```
./TCal -f <data_file_name>
```
The calibration data will be save in 'chip1.dat' and 'chip2.dat'. Then move the calibration data into 'TCdata/'.
```
mv chip*.dat TCdata/<EROSName>
```
3. Evaluate the timing resolution after calibration.
```
./TRes -f <data_file_name> -t TCdata/<EROSName>/
```
