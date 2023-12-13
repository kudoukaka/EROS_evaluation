//#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <getopt.h>
#include <string.h>
#include <arpa/inet.h>

#include "TApplication.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TH2F.h"
#include "TF1.h"
#include "TMath.h"
#include "TFile.h"
#include "TTree.h"
#include "TStyle.h"
#include <string>

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
    {0,0,0,0}
};

int main(int argc, char* argv[])
{

//    gStyle->SetOptStat(0);

    int ii, index;
    int check_ch = 0;
    

    //file name input
    std::string fname;
    std::string baseline_fname;
    while( (ii = getopt_long(argc, argv, "f:b:", options, &index)) !=-1 ){
        switch(ii){
            case 'f':
                fname = optarg;
                break;
            case 'b':
                baseline_fname = optarg;
                break;
            default :
                break;
        }
    }

		//OutputRootFile
		std::string rootname = fname;
		size_t pos = rootname.find(".dat");
		if (pos != std::string::npos) {
			rootname.replace(pos, 4, ".root");
		} else{
			rootname = "anadata/result.root";
		}

		auto fout=new TFile(("anadata/"+rootname).c_str(),"RECREATE");
		auto datatree = new TTree("datatree", "waveforms");
		int sc[2]={0};
		float wf[18][1024]={0};
		datatree->Branch("wf",wf,"wf[18][1024]/F");
		datatree->Branch("cidx",sc,"cidx[2]/I");

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
		int events=0;



    TApplication* app = new TApplication("app",&argc,argv);
    char hist_name[18];
    TH1F *hist[CHNUM];
    for(int j = 0; j < CHNUM; j++){
        sprintf(hist_name, "%d",  j);
        hist[j] = new TH1F(hist_name, hist_name, 400, -50, 50);
    }



    //Read Start
    for(;;){
				if((events+1)%1000==0)std::cout<<"Processing events: "<<events+1<<std::endl;
				events++;
				
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

								if(ch==0)sc[0]=StopNumber;
								if(ch==8)sc[1]=StopNumber;
                for(int sample = 0; sample < 1024; sample++){
                    int capacitor_number = (sample + StopNumber) % 1024;
                    unsigned short *data_f = (unsigned short *)&readdata[sample*2];
                    unsigned short data = *data_f;
                    get_data[ch][sample] = (int)(0x0fff & data);
                    get_data2[ch][sample] = (float)get_data[ch][sample] - pedestal[StopNumber32][0][ch][capacitor_number]; 
                    get_data_final[ch][sample] = get_data2[ch][sample] / 4096. * 1000; //ADC:12bit 2mVpp
										wf[ch][sample]=get_data_final[ch][sample];
                }
            }



            //get footer;
            readnum = fread(ch_footer, 1, CH_FOOTERSIZE, fp);
            if(readnum != (unsigned int)CH_FOOTERSIZE){
                break;
            }
        }
				datatree->Fill();

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

		datatree->Write("", TObject::kOverwrite);
		fout->Close();

    return 0;
}    
