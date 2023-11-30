//#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <sstream>
#include <fstream>
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
int events = 1;

std::string FileDir = "rawdata/";
double fitser(double *x, double *par);
double intersection(int start, int end);
auto fth=new TF1("fth","[0]",0,1024);
auto graph=new TGraph();

const static option options[] = {
    {"FileName",        required_argument, NULL, 'f'},
    {"baseline_fname",  required_argument, NULL, 'b'},
    {"Ref_ch",  required_argument, NULL, 'r'},
    {0,0,0,0}
};

int main(int argc, char* argv[])
{
		std::cout<<"Now performing Global TC..."<<std::endl;

//    gStyle->SetOptStat(0);

    int ii, index;
    int check_ch = 0;
    

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
		int row=0;
		int count[1024];
		double dummy;
		double dt[1024];
		double dt_cortmp[1024];
		double dt_cor[1024];
		double wftime[1024];
		int capa[8][1024]={0};
		int checked_capa[8][1024]={0};

		if(time_input.is_open()){
			std::string time_data;
			time_input.clear();
			time_input.seekg(0,std::ios::beg);
			while(getline(time_input,time_data)){
				std::istringstream iss(time_data);
				iss >> dummy >> dt[row];
				row++;
			}
			wftime[0]=dt[0];
			for(int k=1;k<1024;k++){
				wftime[k]=wftime[k-1]+dt[k];
			}
		}

		if(!time_input.is_open()){
			std::cout<<"No TC data is found, now creating"<<std::endl;
			for(int i=0;i<1024;i++){
				dt[i]=0.938;
				wftime[i]=(i+1)*0.938;
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

    TApplication* appd = new TApplication("app",&argc,argv);

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

				for(int ch=ref_ch;ch<ref_ch+2;ch++){
					int nA=0;
					int nB=0;
					int nC=0;
					int checkpointA[20]={0};
					int checkpointB[20]={0};
					int checkpointC[20]={0};
					double intersecA[20]={0};
					double intersecB[20]={0};
					double intersecC[20]={0};
					for(int sample=0;sample<1024;sample++){
						graph->SetPoint(sample,wftime[sample],get_data_final[ch][sample]);
					}
					for(int sample=1;sample<1023;sample++){
						if(get_data_final[ch][sample]>=0. && get_data_final[ch][sample-1]<0. && get_data_final[ch][sample+1]>0.){
							fth->FixParameter(0,0);
							intersecA[nA]=intersection(wftime[sample-1],wftime[sample+1]);
							checkpointA[nA]=sample;
							nA++;
							checked_capa[ch][sample]++;
						}
						if(get_data_final[ch][sample]<=0. && get_data_final[ch][sample-1]>0. && get_data_final[ch][sample+1]<0.){
							fth->FixParameter(0,0);
							intersecB[nB]=intersection(wftime[sample-1],wftime[sample+1]);
							checkpointB[nB]=sample;
							nB++;
							checked_capa[ch][sample]++;
						}
						if(get_data_final[ch][sample]>=50. && get_data_final[ch][sample-1]<50. && get_data_final[ch][sample+1]>50.){
							fth->FixParameter(0,-20);
							intersecC[nC]=intersection(wftime[sample-1],wftime[sample+1]);
							checkpointC[nC]=sample;
							nC++;
							checked_capa[ch][sample]++;
						}
					}
	
					for(int point=0;point<nA-1;){
						float pA=intersecA[point+1]-intersecA[point];
						if(pA>55 && pA<70 && capa[ch][checkpointA[point]]<10){
							dt_cortmp[checkpointA[point]]=62.5/pA;
							for(int cor=checkpointA[point];cor<checkpointA[point+1]-1;cor++){
								dt_cor[cor]+=dt_cortmp[checkpointA[point]]*dt[cor];
								count[cor]++;
							}
							capa[ch][checkpointA[point]]++;
							point++;
						}
						else point+=2;
					}
	
					for(int point=0;point<nB-1;){
						float pB=intersecB[point+1]-intersecB[point];
						if(pB>55 && pB<70 && capa[ch][checkpointB[point]]<10){
							dt_cortmp[checkpointB[point]]=62.5/pB;
							for(int cor=checkpointB[point];cor<checkpointB[point+1]-1;cor++){
								dt_cor[cor]+=dt_cortmp[checkpointB[point]]*dt[cor];
								count[cor]++;
							}
							capa[ch][checkpointB[point]]++;
							point++;
						}
						else point+=2;
					}
	
					for(int point=0;point<nC-1;){
						float pC=intersecC[point+1]-intersecC[point];
						if(pC>55 && pC<70 && capa[ch][checkpointC[point]]<10){
							dt_cortmp[checkpointC[point]]=62.5/pC;
							for(int cor=checkpointC[point];cor<checkpointC[point+1]-1;cor++){
								dt_cor[cor]+=dt_cortmp[checkpointC[point]]*dt[cor];
								count[cor]++;
							}
							capa[ch][checkpointC[point]]++;
							point++;
						}
						else point+=2;
					}
				}

				if(events%1000==0)std::cout<<"Processing events: "<<events<<std::endl;
				events++;


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

    FILE *w_fp;
    w_fp = fopen("TC.dat", "w");
    if(w_fp == NULL){
        printf("TC.dat write err\n");
        return -1;
    }

		int sum[1024]={0};
		auto c=new TCanvas();
		auto gtest=new TGraph();
    for(int ch=ref_ch;ch<ref_ch+8;ch++){
      for(int sample=0;sample<1024;sample++){
        sum[sample]+=checked_capa[ch][sample];
      }
    }

		for(int sample=0;sample<1024;sample++){
			gtest->SetPoint(sample,sample,sum[sample]);
			if(count[sample]==0)count[sample]=1;
			if(dt_cor[sample]==0)dt_cor[sample]=0.938;
			if(dt_cor[sample]/count[sample]!=0)dt[sample]=dt_cor[sample]/count[sample];
			fprintf(w_fp, "%d %f\n", sample, dt[sample]);
		}
		gtest->Draw();
		c->SaveAs("GTCcount.png");

    return 0;
}  

double fitser(double *x, double *par){
	return TMath::Abs(graph->Eval(x[0])-fth->EvalPar(x,par));
}

double intersection(int start, int end){
	TF1 *fits=new TF1("fits",fitser,start,end,0);
	double xits=fits->GetMinimumX();
	delete fits;
	return xits;
}
