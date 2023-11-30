//#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <getopt.h>
#include <string.h>
#include <arpa/inet.h>

#include "TApplication.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TH2F.h"
#include "TF1.h"
#include "TMath.h"
#include "TStyle.h"
#include <string>
#include "TFile.h"
#include "TSpline.h"

int HEADERSIZE = 24;
int FOOTERSIZE = 8;
int CHNUM = 18;
int SAMPLEMAX = 1024;
int CH_HEADERSIZE = 8;
int CH_FOOTERSIZE = 4;



int CH_NUM = 18;
int SAMLE_NUM = 1024;
int board_num_MAX = 10;
int StopCapacitor_NUM = 32;
int BoardNum = 1;

std::string FileDir = "rawdata/";

const static option options[] = {
    {"FileName",        required_argument, NULL, 'f'},
    {"baseline_fname",  required_argument, NULL, 'b'},
    {"Ref_ch",  required_argument, NULL, 'r'},
    {0,0,0,0}
};

int main(int argc, char* argv[])
{

//    gStyle->SetOptStat(0);

    int ii, index;
    int check_ch = 0;
		double dummy;
		double wftime[1024]={0};
		double dt_tmp[1024]={0};


    //file name input
    std::string fname;
    std::string baseline_fname;
		int ref_ch;
    while( (ii = getopt_long(argc, argv, "f:b:r:", options, &index)) !=-1 ){
        switch(ii){
            case 'f':
                fname = optarg;
                break;
            case 'b':
                baseline_fname = optarg;
                break;
            case 'r':
                ref_ch = std::stoi(optarg);
                break;
            default :
                break;
        }
    }

		//read time information
		std::ifstream time_input("TC.dat");

		if(!time_input.is_open()){
			std::cout<<"TC data do not exist, assume 1ns frequency"<<std::endl;
			for(int i=0;i<1024;i++){
				wftime[i]=(i+1)*0.938;
			}
		}

		if(time_input.is_open()){
			int row=0;
			std::string time_data;
			time_input.clear();
			time_input.seekg(0,std::ios::beg);
			while(getline(time_input,time_data)){
				std::istringstream iss(time_data);
				iss >> dummy >> dt_tmp[row];
				row++;
			}
			wftime[0]=dt_tmp[0];
			for(int k=1;k<1024;k++){
				wftime[k]=wftime[k-1]+dt_tmp[k];
			}
		}


    //read baseline file
		std::string baselinefilepath_string = ("/home/david/Documents/COMET/ECAL/EROS_DAQ/DAQ/baselinefile/" + baseline_fname);
		const char* baselinefilepath = baselinefilepath_string.c_str();
    FILE *b_fp;
    b_fp = fopen(baselinefilepath, "r");
		std::cerr<<baselinefilepath;
		if(b_fp == NULL){
        printf("baselinefile read err\n");
        return -1;
    }    

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

    while( ( ret = fscanf(b_fp, "%d,%d,%d,%f", &stopcapacitor, &base_ch, &base_samplenum, &value ) ) != EOF ){
      pedestal[stopcapacitor][0][base_ch][base_samplenum] = value;
    }

    //read data file
		std::string filepath_string = (FileDir + fname);
    const char* filepath = filepath_string.c_str();
    printf("%s\n", filepath);

    FILE *fp;
    fp = fopen(filepath, "rb");
    if(fp == NULL){
        printf("file read err\n");
        return -1;
    }        

    unsigned char header[HEADERSIZE];
    unsigned char ch_header[CH_HEADERSIZE];
    unsigned char ch_footer[CH_FOOTERSIZE];
    unsigned char readdata[CHNUM * SAMLE_NUM];
    unsigned char SEM_Message[100000];
    unsigned char footer[FOOTERSIZE];
    unsigned int readnum = 0;
    double waveform[CHNUM][SAMLE_NUM];


    bool first_flag[2];
    first_flag[0] = true;
    first_flag[1] = true;
    unsigned int EventNumber[2];
    EventNumber[0] = 0;
    EventNumber[1] = 1;



    TApplication* app = new TApplication("app",&argc,argv);
    TGraph *graph=new TGraph();
		float ptmp[1024]={0};
		float count[1024]={0};

    //Read Start
    for(;;){
        //Header read
        readnum = fread(header, 1, HEADERSIZE, fp);
        if(readnum != (unsigned int)HEADERSIZE){
            break;
        }
       
       /*
        unsigned int *datalength_f = (unsigned int *)&header[8];
        unsigned int datalength = ntohl(*datalength_f);
        unsigned int *eventnumber_f = (unsigned int *)&header[12];
        unsigned int get_eventnumber = ntohl(*eventnumber_f);
        unsigned int *sem_message_size_f = (unsigned short *)&header[16];
        unsigned short sem_message_size = ntosl(*sem_message_size_f);
       */

        unsigned int *datalength_f = (unsigned int *)&header[8];
        unsigned int datalength = *datalength_f;
        unsigned int *eventnumber_f = (unsigned int *)&header[12];
        unsigned int get_eventnumber = *eventnumber_f;
        unsigned short *sem_message_size_f = (unsigned short *)&header[20];
        unsigned short sem_message_size = *sem_message_size_f;


        //data read

        int get_data[18][1024];
        float get_data2[18][1024];
        float get_data_final[18][1024];

        for(int ch = 0; ch < CHNUM; ch++){
            //ch header read
            readnum = fread(ch_header, 1, CH_HEADERSIZE, fp);
            if(readnum != (unsigned int)CH_HEADERSIZE){
                break;
            }

            unsigned char chNumber = ch_header[3];
            unsigned short *StopNumber_f = (unsigned short *)&ch_header[4];
            unsigned short StopNumber = *StopNumber_f;
            unsigned short *DataLength_f = (unsigned short *)&ch_header[6];
            unsigned short DataLength = *DataLength_f;
            
            int StopNumber32 = (int)StopNumber / 32;

            //get data;
            if(DataLength != 0){
                readnum = fread(readdata, 1, DataLength, fp);
                if(readnum != (unsigned short)DataLength){
                    break;
                }

                for(int sample = 0; sample < 1024; sample++){
                    int capacitor_number = (sample + StopNumber) % 1024;
                    unsigned short *data_f = (unsigned short *)&readdata[sample*2];
                    unsigned short data = *data_f;
                    get_data[ch][sample] = (int)(0x0fff & data);
                    get_data2[ch][sample] = (float)get_data[ch][sample] - pedestal[StopNumber32][0][ch][capacitor_number];
                    get_data_final[ch][sample] = get_data2[ch][sample] / 4096. * 1000; //ADC:12bit 2mVpp
                }
            }



            //get footer;
            readnum = fread(ch_footer, 1, CH_FOOTERSIZE, fp);
            if(readnum != (unsigned int)CH_FOOTERSIZE){
                break;
            }
        }

				//fitting range: 50 ~ -25
				int checkpoint[16];
				float dt[1024];
				int i=0;
				for(int sample=1;sample<1024;sample++){
					if(i<16 && get_data_final[ref_ch][sample-1] >= 0. && get_data_final[ref_ch][sample] <= 0. && get_data_final[ref_ch][sample+1] <= 0.){
						checkpoint[i]=sample;
						i++;
					}
				}
				for(int sample=0;checkpoint[0]+sample<1024;sample++){
					ptmp[sample]+=get_data_final[ref_ch][checkpoint[1]+sample-20];
					count[sample]+=1;
				}

        //read SEM message
        if(sem_message_size != 0){
            readnum = fread(SEM_Message, 1, sem_message_size, fp);
            if(readnum != (unsigned int)sem_message_size){
                break;
            }
        }
    

        //Footer footer
        readnum = fread(footer, 1, FOOTERSIZE, fp);
        if(readnum != (unsigned int)FOOTERSIZE){
            break;
        }



    }

    fclose(fp);

    TCanvas *Canvas = new TCanvas("canvas", "canvas", 600, 600);
		float eventnum=10000.;

		for(int sample=0;sample<80;sample++){
			graph->SetPoint(sample,wftime[sample],ptmp[sample]/count[sample]);
		}
		graph->Draw("apl");
		graph->SetName("template");
		graph->SetMarkerSize(1);
		graph->GetXaxis()->SetRangeUser(0,1000);
		graph->SetMarkerStyle(kStar);
		Canvas->SaveAs("template.png");
//    Canvas->Update();

		//Get & Save Waveform Template
		TFile *ftmp=new TFile("template.root","recreate");
//		TSpline3 *sp3=new TSpline3("sp3",graph);
//		sp3->Write();
		graph->Write();
		ftmp->Close();

    return 0;
}    
