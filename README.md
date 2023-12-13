# EROS_evaluation

This is a package for EROS performance check. Including pattern correction, binary data decoding, Noise level, linearity, signal-to-noise ratio, crosstalk, timing resolution & calibration.

## How to Install

Simple Makefile, simple "make". It will also create 'anadata/', 'rawdata/' and 'reasult/' for futher analysis.

## How to Start DAQ
### Initial Setup

Chang IP address in 'SETUP_FILE/IP.txt', change baseline file in 'SETUP_FILE/baseline.txt'

### DAQ option
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

-e: Event number limit.

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
### Decode:

```
./eros_d2r -f <raw_data_file_name> -b <baseline_file_name>
```
Please put your raw data file in 'rawdata/' and baseline file in 'baselinefile/'!!!

### Noise Level Check:

### Linearity Check:

### Signal-to-noise Ratio:

### Crosstalk:

### Timing Resolution & Calibration:
