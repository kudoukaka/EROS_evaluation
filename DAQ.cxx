#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <stdlib.h>

#include "TCanvas.h"
#include "TApplication.h"
#include "TGraph.h"
#include "TH2F.h"
#include "TStyle.h"
#include "setup.h"
#include "DAQ.h"

#define SERVPORT 24
#define SiTCP_IP "192.168.10.25"

int BoardMaxNum = 10;
int CH_NUM = 18;
int SAMLE_NUM = 1024;
int select_ch = 0;
int board_num_MAX = 10;
int StopCapacitor_NUM = 32;

const static option options[] = {
    {"BoardNum",        required_argument, NULL, 'n'},
    {"trigger",         required_argument, NULL, 't'},
    {"threshold_upp",   required_argument, NULL, 'A'},
    {"threshold_low",   required_argument, NULL, 'a'},
    {"hist_freq",       required_argument, NULL, 'h'},
    {"zero_suppress_on",no_argument,       NULL, 'z'},
    {"decision_on",     no_argument,       NULL, 'd'},
    {"write",           no_argument,       NULL, 'w'},
    {"event_finish",    no_argument,       NULL, 'e'},
    {0,0,0,0}
};


int main(int argc, char* argv[]){

    int ii, index;

    int BoardNum;
    std::string trigger;
    std::string threshold_upp;
    std::string threshold_low;
    bool zero_suppress_on;
    bool decision_on;
    int hist_freq;

    //default
    BoardNum = 1;
    trigger = "normal";
    threshold_upp = "08";
    threshold_low = "00";
    zero_suppress_on = false;
    decision_on = false;
    hist_freq = 400;
    bool write_on = false;
    int event_finish = 0;

    while( (ii = getopt_long(argc, argv, "n:t:A:a:f:zdwe:", options, &index)) !=-1 ){
        switch(ii){
            case 'n':
                BoardNum = atoi(optarg);
                break;
            case 't':
                trigger = optarg;
                break;
            case 'A':
                threshold_upp = optarg;
                break;
            case 'a':
                threshold_low = optarg;
                break;
            case 'f':
                hist_freq = atoi(optarg);
                break;
            case 'z':
                zero_suppress_on = true;
                break;
            case 'd':
                decision_on = true;
                break;
            case 'w':
                write_on = true;
                break;
            case 'e':
                event_finish = atoi(optarg);
                break;
        }
    }


    //argument check
    if(BoardNum <= 0 || BoardNum >= BoardMaxNum){
        fprintf(stderr, "-n %d is strange\n", BoardNum);
        exit(1);
    }

    if(trigger != "off" &&
        trigger != "self" && 
        trigger != "base" && 
        trigger != "normal"){
        fprintf(stderr, "-t %s is strange\n", trigger.c_str());
        exit(1);
    }

    //IP read
    FILE *IP_file;
    char name_IP_file[256] = "./SETUP_FILE/IP.txt";
    IP_file = fopen(name_IP_file, "r");
    if(IP_file == NULL ){
        fprintf( stderr, "%s can not read file ¥n", name_IP_file);
        exit(1);
    }

    int c;
    int IP_count = 0;
    int All_IP_num = 0;
    while( ( c = fscanf(IP_file, "%d", &IP_array[IP_count] ) ) != EOF ){
        printf("IP = %d \n", IP_array[IP_count]);
        if(IP_array[IP_count] < 16 || IP_array[IP_count] > 79){
            fprintf(stderr, "IP address is strange\n");
            exit(1);
        }

        IP_count++;
    }
    All_IP_num = IP_count;

    if(BoardNum > All_IP_num){
        fprintf( stderr, "IP address file is strange\n");
        exit(1);
    }


    //baseline correction file read
    FILE *baseline_file;
    char name_baseline_file[256] = "./SETUP_FILE/baseline.txt";
    baseline_file = fopen(name_baseline_file, "r");
    if(baseline_file == NULL ){
        fprintf(stderr, "%s can not read file ¥n", name_baseline_file);
        exit(1);
    }

    int baseline_count = 0;
    int defile_baseline_num = 0;
    char baselinefile_pass[BoardMaxNum][256];
    int All_baseline_num = 0;
    
    while( ( c = fscanf(baseline_file, "%s", baselinefile_pass[baseline_count] ) ) != EOF ){
        printf("base = %s \n", baselinefile_pass[baseline_count]);
        baseline_count++;
    }
    All_baseline_num = baseline_count;
    
    
    if(BoardNum > All_baseline_num){
        fprintf( stderr, "baseline file is strange\n");
        exit(1);
    }

    //setup
    printf("setup start\n");
    setup(BoardNum, trigger.c_str(), threshold_upp.c_str(), threshold_low.c_str(), zero_suppress_on, decision_on);


    gStyle->SetOptStat(0);
    
    int sock;
    struct sockaddr_in servaddr;

    FILE* fp;
    fp = fopen("receive.dat", "wb");
    if(fp == NULL){
        fprintf(stderr, "err fopen\n");
        exit(1);
    }



    TApplication* app = new TApplication("app",&argc,argv);
    TCanvas *Canvas[4];
    TGraph *graph[4][CH_NUM];
    TH2F *waku = new TH2F("waku", "waku", 1024, 0, 1024, 1000, -500, 500); 
    //TH2F *waku = new TH2F("waku", "waku", 1024, 0, 1024, 40, -10, 10); 
    char canvas_name[255];
    char graph_name[255];
    for(int i = 0; i < BoardNum; i++){
        sprintf(canvas_name, "BoardNumber%d", i);
        Canvas[i] = new TCanvas(canvas_name, canvas_name, 100,500, 1200, 1000);
        Canvas[i]->Divide(5,4);
        for(int j = 0; j < CH_NUM; j++){
            Canvas[i]->cd(j + 1);
            waku->Draw();
            sprintf(graph_name, "Graph%d_%d", i, j);
            graph[i][j] = new TGraph();
            graph[i][j]->SetName(graph_name);
        }

    }

    TH1F* histRMS;
    histRMS = new TH1F("histRMS", "histRMS", 4096, -2048, 2048);

    //baseline file setup
    FILE* baselinefile[BoardMaxNum];

    float pedestal[StopCapacitor_NUM][BoardNum][CH_NUM][SAMLE_NUM];
    for(int stopcapacitor = 0; stopcapacitor < StopCapacitor_NUM; stopcapacitor++){
        for(int Board = 0; Board < BoardNum; Board++){
            for(int ch = 0; ch < CH_NUM; ch++){
                for(int sample = 0; sample < SAMLE_NUM; sample++){
                    pedestal[stopcapacitor][Board][ch][sample] = 0.;
                }
            }
        }
    }


    int ret;
    int base_ch;
    int base_samplenum;
    float value;
    int stopcapacitor;

    for(int i = 0; i < BoardNum; i++){
    printf("%s\n", baselinefile_pass[i]);
        baselinefile[i] = fopen(baselinefile_pass[i], "r");
        while( ( ret = fscanf(baselinefile[i], "%d,%d,%d,%f", &stopcapacitor, &base_ch, &base_samplenum, &value ) ) != EOF ){
          pedestal[stopcapacitor][i][base_ch][base_samplenum] = value;
        }
    }

    //DAQ
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVPORT);
    if(inet_pton(AF_INET, SiTCP_IP, &servaddr.sin_addr) <= 0){
        fprintf(stderr, "err inet_pton\n");
        exit(1);
    }


    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "err socket\n");
        exit(1);
    }

    //increase send buffer
    int sendbuffer_size = 33554432;
    if ( setsockopt (sock, SOL_SOCKET, SO_RCVBUF, &sendbuffer_size, sizeof(sendbuffer_size)) < 0){
        fprintf(stderr, "setsockopt failed\n");
        exit(1);
    }



    if(connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        fprintf(stderr, "err connetct \n");
				exit(1);
    }

    sleep(1);

    int first_event[10];
    unsigned int pre_event_number[10];
    int i;
    for(i = 0; i < 10; i++){
        first_event[i] = 1;
        pre_event_number[1] = 0;
    }
    int n;
    char recvbuff[1024];
    char recv_header[24];
    printf("TCP connect\n");

    int HEADERSIZE = 24;
    int FOTTERSIZE = 8;
    int data_read_size = 0;
    int SEUinfo_read_size = 0;

    union EventNumber_f{
        char EventNumber_char[4];
        unsigned EventNumber_int;
    }EventNumber;

    union Length_f{
        char Length_char[4];
        unsigned Length_int;
    }Length;

    union SEUinfoSize_f{
        char SEUinfoSize_char[2];
        unsigned short SEUinfoSize_int;
    }SEUinfoSize;

    
    union StopNumber_f{
        char StopNumber_char[2];
        unsigned short StopNumber_short;
    }StopNumber;

    union DataLength_f{
        char DataLength_char[2];
        unsigned short DataLength_short;
    }DataLength;


    char recvbuff_fotter[1024];

    int event_counter[10];
    for(int i = 0; i < 10; i++){
        event_counter[i] = 0;
    }

    unsigned char TCP_data[37112];
    union f_data{
        unsigned char data_char[2];
        unsigned short data_short;
    }data;

    //Time
    struct timeval gettime, starttime;
    gettimeofday(&starttime, NULL);
    gettimeofday(&gettime, NULL);
    double previous_time = (double)gettime.tv_sec + (double)gettime.tv_usec/1000/1000;
    double starttime_double = (double)starttime.tv_sec + (double)starttime.tv_usec/1000/1000;
 


    //read start
    for(;;){
        int header_read = 0;
        while( (n = recv(sock, recv_header + header_read, HEADERSIZE - header_read, 0)) > 0){
          if(write_on)fwrite(recv_header + header_read, 1, n, fp);
            header_read = header_read + n;
            if(header_read == HEADERSIZE)break;
        }

        memcpy(&EventNumber.EventNumber_char[0], &recv_header[12], 4);
        memcpy(&Length.Length_char[0], &recv_header[8], 4);
        memcpy(&SEUinfoSize.SEUinfoSize_char[0], &recv_header[20], 2);

        int board_IP = (int)recv_header[6];
        int board_number = -1;
        for(int i = 0; i < board_num_MAX; i++){
            if(board_IP == IP_array[i]){
                board_number = i;
                break;
            }
        }

        //check IP of header
        if(board_number == -1){
            fprintf(stderr, "IP of header is strange\n");
            exit(1);
        }


        //read data
        int data_read = 0;
        for(;;){
            if((Length.Length_int - data_read) < 1024){
                data_read_size = Length.Length_int - data_read;
            }
            else{
                data_read_size = 1024;
            }
            while( (n = recv(sock, recvbuff, data_read_size, 0)) > 0){
              if(write_on)fwrite(recvbuff, 1, n, fp);
                memcpy(TCP_data + data_read, recvbuff, n);
                data_read = data_read + n;

                //change read size
                if((Length.Length_int - data_read) <= 0){
                    break;
                }
                else if((Length.Length_int - data_read) < 1024){
                    data_read_size = Length.Length_int - data_read;
                }
                else{
                    data_read_size = 1024;
                }
            }

            if(data_read ==  (int)Length.Length_int)break;
        }

        


        //read SEU info
        int SEUinfo_read = 0;
        for(;;){
            if((SEUinfoSize.SEUinfoSize_int - SEUinfo_read) < 1024){
                SEUinfo_read_size = SEUinfoSize.SEUinfoSize_int - SEUinfo_read;
            }
            else{
                SEUinfo_read_size = 1024;
            }
            while( (n = recv(sock, recvbuff, SEUinfo_read_size, 0)) > 0){
              if(write_on)fwrite(recvbuff, 1, n, fp);
                SEUinfo_read = SEUinfo_read + n;

                //change read size
                if((SEUinfoSize.SEUinfoSize_int - SEUinfo_read) <= 0){
                    break;
                }
                else if((SEUinfoSize.SEUinfoSize_int - SEUinfo_read) < 1024){
                    SEUinfo_read_size = SEUinfoSize.SEUinfoSize_int - SEUinfo_read;
                }
                else{
                    SEUinfo_read_size = 1024;
                }
            }
            if(SEUinfo_read ==  SEUinfoSize.SEUinfoSize_int)break;
        }

        //read fotter
        int fotter_read = 0;
        int break_flag = 0;

        int read_fotter_n = 0;
        while( (n = recv(sock, recvbuff, FOTTERSIZE - fotter_read, 0)) > 0){
          if(write_on)fwrite(recvbuff, 1, n, fp);
            fotter_read = fotter_read + n;

            memcpy(recvbuff_fotter + read_fotter_n, recvbuff, n); 
            read_fotter_n = read_fotter_n + n;
            if(read_fotter_n == FOTTERSIZE){
                if(fotter_read == FOTTERSIZE){
                   
                    if((unsigned char)recvbuff_fotter[0] != 0x98 || 
                        (unsigned char)recvbuff_fotter[1] != 0xba || 
                        (unsigned char)recvbuff_fotter[2] != 0xdc || 
                        (unsigned char)recvbuff_fotter[3] != 0xfe){
                        printf("footer err\n");
                        printf("n = %d, %x, %x, %x, %x,\n", n, (unsigned char)recvbuff[0], (unsigned char)recvbuff[1], (unsigned char)recvbuff[2], (unsigned char)recvbuff[3]);
                        break_flag = 1;
                    }
                    break;
                }
            }
        }

        int read_locatation = 0;

        if((event_counter[board_number] % hist_freq) == 1){
            histRMS->Delete();
            histRMS = new TH1F("histRMS", "histRMS", 4096, -2048, 2048);
            for(int ch = 0; ch < CH_NUM; ch++){

                //get stop capacitor number
                memcpy(&StopNumber, TCP_data + read_locatation + 4, 2);   //Stop Capacitor Number of 1ch 

                //get data length
                memcpy(&DataLength, TCP_data + read_locatation + 6, 2);  

                if(DataLength.DataLength_short == 0){   //zero suppress
                    for(int sample = 0; sample < 1024; sample++){
                        graph[board_number][ch]->SetPoint(sample, sample, -2000);
                    }
                    read_locatation += 12; // +12 -> Data Packet header + Data Packet footer
                }
                else{   //not suppress
                    for(int sample = 0; sample < 1024; sample++){
                        memcpy(data.data_char, TCP_data + (read_locatation + 8 + sample * 2 ), 2);
                        int capacitor_number = (sample + StopNumber.StopNumber_short) % 1024;
                        int get_stopcapacitor32 = (int)StopNumber.StopNumber_short / 32;
                        int data_calib_level1 = (int)(data.data_short & 0x0fff);
                        float data_calib_level2 = data_calib_level1 - pedestal[get_stopcapacitor32][board_number][ch][capacitor_number];
                        float data_calib_level3 = (float)data_calib_level2 / 4096. * 1000.;
                        graph[board_number][ch]->SetPoint(sample, sample, data_calib_level3);

                        if(ch == select_ch && sample >= 50 && sample <=950){
                            histRMS->Fill(data_calib_level3);
                        }
                    }
                        
                    read_locatation += 12 + 1024 * 2; // -> Data Packet header + Data Packet footer + 1ch Data

                    if(ch == select_ch){
                        float RMS = histRMS->GetRMS();
                        printf("board = %d, RMS = %f\n", board_number, RMS);
                    }
                }

                Canvas[board_number]->cd(ch + 1);
                graph[board_number][ch]->Draw("PLsame");
            }

            Canvas[board_number]->Update();
        }


        int event_counter_all = 0;
        for(int i = 0; i < BoardNum; i++){
            event_counter_all += event_counter[i];
        }

        if((event_counter_all % hist_freq) == 0){
            gettimeofday(&gettime, NULL);
            double gettime_double = (double)gettime.tv_sec + (double)gettime.tv_usec/1000/1000;
            double time_interval = gettime_double - previous_time;
            previous_time = gettime_double;
            double speed = (double)hist_freq / time_interval;
            //printf("%lf s, %lf Hz \n", gettime_double - starttime_double,  speed);
            //printf("%lf s, %lf Hz,%lf,%lf \n", gettime_double - starttime_double, time_interval, speed, (double)hist_freq , time_interval);
        }


        event_counter[board_number]++;
        if(break_flag == 1 || event_counter[board_number] == event_finish)break;
    }

    printf("close\n");

    if(close(sock) < 0){
        fprintf(stderr, "err close\n");
        exit(1);
    }

    return 0;
}
