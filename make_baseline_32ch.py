#!/usr/bin/python
# -*- conding: utf-8 -*
import sys
import operator
import ROOT
import struct
import os
import socket
import math

correct_total_ch = 18
correct_MagicWord = ['0x89', '0xab', '0xcd', '0xef']
correct_TYPE = ['0xff', '0x0']
correct_BoardNumber = 4
#correct_Length = 512
correct_Length = 37080
correct_DataPacketWord = ['0x12', '0x34']
correct_BoardNumber2 = 4
correct_StopNumber = 123
correct_DataLength = 2048
correct_DataPacketFooter = ['0x56', '0x78']
correct_KeyWord = ['0xa']
correct_checksum = ['0x0']
correct_Footer = ['0xfe', '0xdc', '0xba', '0x98']

def get_data(num):
    num_B = ""
    for var in range(0, num):
        num_B = num_B + "B"

    buf = f.read(num)
    val = struct.unpack(num_B, buf)

    val2 = []
    for var in range(0, num):
        val2.append(hex(val[var]))
    if num == 2:
        val3 = [val2[1], val2[0]]
    elif num == 4:
        val3 = [val2[3], val2[2], val2[1], val2[0]]
    else:
        val3 = val2

    return val3



def get_data_num1():
    buf = f.read(1)
    val = struct.unpack("B", buf)
    return val[0]



def get_data_num2():
    buf = f.read(2)
    val = struct.unpack(">H", buf)
    val2 = socket.ntohs(val[0])
    return val2


def get_data_num4():
    buf = f.read(4)
    val = struct.unpack(">I", buf)
    val2 = socket.ntohl(val[0])
    return val2


def failed_repoet(item, ch):
    print "failed by " + item
    print "EventNumber = " + str(EventNumber)
    print "ch = " + str(ch)
    sys.exit()

if __name__ == "__main__":

    argvs = sys.argv
    argc = len(argvs)
    if(argc != 2):
        print "argvs failed"

    print "start"

    #define list for baseline
    baseline = [[[0 for j in range(1024)] for i in range(correct_total_ch)] for k in range(32)]

    canvas_16ch = ROOT.TCanvas("canvas_16ch", "canvas_16ch")
    canvas_16ch.cd()
    canvas_16ch.Divide(4,4)
    canvas_2ch  = ROOT.TCanvas("canvas_2ch", "canvas_2ch")
    canvas_2ch.Divide(1,2)
    canvas_2ch.cd()

    f = open(argvs[1], 'rb')

    first_event_flag = 1
    read_event_num = 32000
    #read_event_num = 320

    read_event = [[0 for j in range(32)] for i in range(correct_total_ch)]

    for  event_num in range(0, read_event_num):
        if (first_event_flag == 1):
            first_event_flag = 0
            continue

        if (event_num % 1000) == 1:
            print "event_num = " + str(event_num)

        #if(event_num % 10) != 1:
        #    continue

        #Event
        MagicWord = get_data(4)
        TYPE = get_data(2)
        BoardNumber = get_data_num2()
        Length = get_data_num4()
        EventNumber = get_data_num4()
        COUNT_SEU_ERR_ENABLE = get_data_num2()
        COUNT_SEU_ERR_DISABLE = get_data_num2()
        SEU_OUTPUT_SIZE = get_data_num2()
        RESERVED = get_data_num2()

        if MagicWord != correct_MagicWord:
            failed_repoet("MagicWord", 0)
        if TYPE != correct_TYPE:
            failed_repoet("TYPE", 0)
        #if BoardNumber != correct_BoardNumber:
        #    failed_repoet("BoardNumber", 0)
        #if Length != correct_Length:
        #    failed_repoet("Length", 0)
        #print MagicWord
        #print TYPE
        #print BoardNumber
        #print Length
        #print EventNumber


        for ch_number in range(0, correct_total_ch):
            #ch
            DataPacketWord = get_data(2)
            BoardNumber2 = get_data_num1()
            chNumber = get_data_num1()
            StopNumber = get_data_num2()
            DataLength = get_data_num2()
            
            if DataPacketWord != correct_DataPacketWord:
                failed_repoet("DataPacketWord", ch_number)
            #if BoardNumber2 != correct_BoardNumber2:
            #    failed_repoet("BoardNumber2", ch_number)
            #if StopNumber != correct_StopNumber:
            #    failed_repoet("StopNumber", ch_number)
            if DataLength != correct_DataLength:
                failed_repoet("DataLength", ch_number)
            #print DataPacketWord
            #print chNumber
            #print "stopnumber = " + str(StopNumber)
            #print "datalength = " + str(DataLength)

            StopNumber32 = StopNumber / 32;
            read_event[ch_number][StopNumber32] = read_event[ch_number][StopNumber32] + 1    


            for var in range(0, DataLength / 2):
                data = get_data_num2()
                data = data & 0x0fff;

                capacitor = (StopNumber + var) % 1024
                baseline[StopNumber32][ch_number][capacitor] = baseline[StopNumber32][ch_number][capacitor] + data            

            DataPacketFooter = get_data(2)
            KeyWord = get_data(1)
            checksum = get_data(1)

            if DataPacketFooter != correct_DataPacketFooter:
                failed_repoet("DataPacketFooter", ch_number)
            #if KeyWord != correct_KeyWord:
            #    failed_repoet("KeyWord", ch_number)
            if checksum != correct_checksum:
                failed_repoet("checksum", ch_number)
            #print DataPacketFooter
            #print KeyWord
            #print checksum

        for var in range(0, SEU_OUTPUT_SIZE):
            OUTPUT_DATA = get_data(1)
            OUTPUT_DATA = int(str(OUTPUT_DATA[0]), 16)
            #fw.write(str(OUTPUT_DATA) + " ")
        Footer = get_data(4)
        CRC32 = get_data(4)

        if Footer != correct_Footer:
            failed_repoet("Footer", ch_number)


    f=open("baseline.txt", 'w') 
    graph_ch0  = ROOT.TGraph()
    graph_ch1  = ROOT.TGraph()
    graph_ch2  = ROOT.TGraph()
    graph_ch3  = ROOT.TGraph()
    graph_ch4  = ROOT.TGraph()
    graph_ch5  = ROOT.TGraph()
    graph_ch6  = ROOT.TGraph()
    graph_ch7  = ROOT.TGraph()
    graph_ch8  = ROOT.TGraph()
    graph_ch9  = ROOT.TGraph()
    graph_ch10 = ROOT.TGraph()
    graph_ch11 = ROOT.TGraph()
    graph_ch12 = ROOT.TGraph()
    graph_ch13 = ROOT.TGraph()
    graph_ch14 = ROOT.TGraph()
    graph_ch15 = ROOT.TGraph()
    graph_ch16 = ROOT.TGraph()
    graph_ch17 = ROOT.TGraph()

    graph_ch0.SetName("ch0") 
    graph_ch1.SetName("ch1")  
    graph_ch2.SetName("ch2")
    graph_ch3.SetName("ch3")
    graph_ch4.SetName("ch4")
    graph_ch5.SetName("ch5")
    graph_ch6.SetName("ch6")
    graph_ch7.SetName("ch7")
    graph_ch8.SetName("ch8")
    graph_ch9.SetName("ch9")
    graph_ch10.SetName("ch10")
    graph_ch11.SetName("ch11")
    graph_ch12.SetName("ch12")
    graph_ch13.SetName("ch13")
    graph_ch14.SetName("ch14")
    graph_ch15.SetName("ch15")
    graph_ch16.SetName("ch16")
    graph_ch17.SetName("ch17")



    for ch_number in range(0, correct_total_ch):
        for capacitor in range(0, 1024):
            for StopNumber32 in range(0, 32):
                baseline[StopNumber32][ch_number][capacitor] = float(baseline[StopNumber32][ch_number][capacitor]) / float(read_event[ch_number][StopNumber32])
                final_baseline = math.floor(baseline[StopNumber32][ch_number][capacitor]*(10**2)) / (10**2) 
                f.write(str(StopNumber32) + "," + str(ch_number) + "," + str(capacitor) + "," + str(final_baseline)+"\n")               

                if(ch_number == 0):
                    graph_ch0.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 1):
                    graph_ch1.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 2):
                    graph_ch2.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 3):
                    graph_ch3.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 4):
                    graph_ch4.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 5):
                    graph_ch5.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 6):
                    graph_ch6.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 7):
                    graph_ch7.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 8):
                    graph_ch8.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 9):
                    graph_ch9.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 10):
                    graph_ch10.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 11):
                    graph_ch11.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 12):
                    graph_ch12.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 13):
                    graph_ch13.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 14):
                    graph_ch14.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 15):
                    graph_ch15.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 16):
                    graph_ch16.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])
                if(ch_number == 17):
                    graph_ch17.SetPoint(capacitor, capacitor, baseline[0][ch_number][capacitor])

    #graph_ch0.SetMinimum(2000)
    #graph_ch1.SetMinimum(2000)
    #graph_ch2.SetMinimum(2000)
    #graph_ch3.SetMinimum(2000)
    #graph_ch4.SetMinimum(2000)
    #graph_ch5.SetMinimum(2000)
    #graph_ch6.SetMinimum(2000)
    #graph_ch7.SetMinimum(2000)
    #graph_ch8.SetMinimum(2000)
    #graph_ch9.SetMinimum(2000)
    #graph_ch10.SetMinimum(2000)
    #graph_ch11.SetMinimum(2000)
    #graph_ch12.SetMinimum(2000)
    #graph_ch13.SetMinimum(2000)
    #graph_ch14.SetMinimum(2000)
    #graph_ch15.SetMinimum(2000)
    #graph_ch16.SetMinimum(2000)
    #graph_ch17.SetMinimum(2000)
    #graph_ch0.SetMaximum(2000)
    #graph_ch1.SetMaximum(2000)
    #graph_ch2.SetMaximum(2000)
    #graph_ch3.SetMaximum(2000)
    #graph_ch4.SetMaximum(2000)
    #graph_ch5.SetMaximum(2000)
    #graph_ch6.SetMaximum(2000)
    #graph_ch7.SetMaximum(2000)
    #graph_ch8.SetMaximum(2000)
    #graph_ch9.SetMaximum(2000)
    #graph_ch10.SetMaximum(2000)
    #graph_ch11.SetMaximum(2000)
    #graph_ch12.SetMaximum(2000)
    #graph_ch13.SetMaximum(2000)
    #graph_ch14.SetMaximum(2000)
    #graph_ch15.SetMaximum(2000)
    #graph_ch16.SetMaximum(2000)
    #graph_ch17.SetMaximum(2000)


    canvas_16ch.cd(1)
    graph_ch0.Draw("APL")
    canvas_16ch.cd(2)
    graph_ch1.Draw("APL")
    canvas_16ch.cd(3)
    graph_ch2.Draw("APL")
    canvas_16ch.cd(4)
    graph_ch3.Draw("APL")
    canvas_16ch.cd(5)
    graph_ch4.Draw("APL")
    canvas_16ch.cd(6)
    graph_ch5.Draw("APL")
    canvas_16ch.cd(7)
    graph_ch6.Draw("APL")
    canvas_16ch.cd(8)
    graph_ch7.Draw("APL")
    canvas_16ch.cd(9)
    graph_ch8.Draw("APL")
    canvas_16ch.cd(10)
    graph_ch9.Draw("APL")
    canvas_16ch.cd(11)
    graph_ch10.Draw("APL")
    canvas_16ch.cd(12)
    graph_ch11.Draw("APL")
    canvas_16ch.cd(13)
    graph_ch12.Draw("APL")
    canvas_16ch.cd(14)
    graph_ch13.Draw("APL")
    canvas_16ch.cd(15)
    graph_ch14.Draw("APL")
    canvas_16ch.cd(16)
    graph_ch15.Draw("APL")
    canvas_2ch.cd(1)
    graph_ch16.Draw("APL")
    canvas_2ch.cd(2)
    graph_ch17.Draw("APL")


    result_root = ROOT.TFile("baseline.root", "RECREATE")
    result_root.cd()
    graph_ch0.Write()
    graph_ch1.Write()
    graph_ch2.Write()
    graph_ch3.Write()
    graph_ch4.Write()
    graph_ch5.Write()
    graph_ch6.Write()
    graph_ch7.Write()
    graph_ch8.Write()
    graph_ch9.Write()
    graph_ch10.Write()
    graph_ch11.Write()
    graph_ch12.Write()
    graph_ch13.Write()
    graph_ch14.Write()
    graph_ch15.Write()
    graph_ch16.Write()
    graph_ch17.Write()
    canvas_16ch.Write()
    canvas_2ch.Write()
    result_root.Close()
    f.close()

