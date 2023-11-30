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
#include "TFile.h"
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
double my_func(double *x, double *par);
double fitser(double *x, double *par);
double intersection(int start, int end);
auto ftmp=TFile::Open("template.root","read");
auto gtmp=(TGraph*)ftmp->Get("template;1");
auto f1=new TF1("f1",my_func,0,1024,3);
float thr=0;
//auto f2=new TF1("f2","[0]",0,1024);

const static option options[] = {
    {"FileName",        required_argument, NULL, 'f'},
    {"baseline_fname",  required_argument, NULL, 'b'},
    {"Ref_ch",  required_argument, NULL, 'r'},
    {0,0,0,0}
};

int main(int argc, char* argv[])
{
		std::cout<<"Now performing Local TC..."<<std::endl;

//    gStyle->SetOptStat(0);

    int ii, index;
    int check_ch = 0;
		float dt[1024]={0};
		float count[1024]={0};
		int capa[8][1024]={0};

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
		double wftime[1024]={0};
		std::ifstream time_input("TC.dat");

		if(time_input.is_open()){
			std::string time_data;
			int row=0;
			double dummy;
			double dt_tmp[1024];
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

    if(!time_input.is_open()){
      std::cout<<"No TC data is found, now creating"<<std::endl;
      for(int i=0;i<1024;i++){
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
		auto graph=new TGraph();

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

				for(int ch=ref_ch;ch<ref_ch+8;ch++){
					int i=0;
					float t1, t2;
					int checkpoint[20]={0};
					int checkpointA[20]={0};
					int checkpointB[20]={0};
					int checkpointC[20]={0};
					int checkpointD[20]={0};
					for(int sample=1;sample<1024;sample++){
						if(sample<1000 & get_data_final[ch][sample]<=10. && get_data_final[ch][sample-1]>10. && get_data_final[ch][sample+1]<10.){
							checkpointA[i]=sample;
						}
						if(sample<1000 && get_data_final[ch][sample]<=0. && get_data_final[ch][sample-1]>0. && get_data_final[ch][sample+1]<0.){
							checkpoint[i]=sample;
							capa[ch][sample]++;
						}
						if(sample<1023 && checkpoint[i]>0 && get_data_final[ch][sample]<=-10. && get_data_final[ch][sample-1]>-10. && get_data_final[ch][sample+1]<-10.){
							checkpointB[i]=sample;
						}
						if(sample<1023 && checkpoint[i]>0 && get_data_final[ch][sample]>=-10. && get_data_final[ch][sample-1]<-10. && get_data_final[ch][sample+1]>-10.){
							checkpointC[i]=sample;
						}
						if(sample<1023 && checkpoint[i]>0 && get_data_final[ch][sample]>=10. && get_data_final[ch][sample-1]<10. && get_data_final[ch][sample+1]>10.){
							checkpointD[i]=sample;
							i++;
						}
						if(sample==1023 && checkpointD[i]==0 && checkpointC[i]!=0 && checkpointB[i]!=0 && checkpointA[i]!=0 && checkpoint[i]!=0){
							checkpointD[i]=sample;
							i++;
						}
					}

					f1->SetParameter(0,1);
					f1->SetParameter(2,-1);
					for(int point=0;point<i;point++){
						if(capa[ch][checkpoint[point]]>10)continue;
						for(int sample=0;sample<1024;sample++){
							graph->SetPoint(sample,wftime[sample],get_data_final[ch][sample]);
						}
						f1->SetParameter(1,wftime[checkpoint[point]]-wftime[20]);
						graph->Fit(f1,"Q","",wftime[checkpoint[point]]-25,wftime[checkpoint[point]]+55);
						for(int j=checkpointA[point];j<checkpointB[point];j++){
							if(get_data_final[ch][j]>get_data_final[ch][j+1]){
								thr=get_data_final[ch][j];
								t1=intersection(wftime[j-2],wftime[j+2]);
								thr=get_data_final[ch][j+1];
								t2=intersection(wftime[j-1],wftime[j+3]);
								if(t2-t1>0 && t2-t1<2){
									dt[j+1]+=t2-t1;
									count[j+1]++;
								}
							}
						}
						for(int j=checkpointC[point];j<checkpointD[point];j++){
							if(get_data_final[ch][j]<get_data_final[ch][j+1]){
								thr=get_data_final[ch][j];
								t1=intersection(wftime[j-2],wftime[j+2]);
								thr=get_data_final[ch][j+1];
								t2=intersection(wftime[j-1],wftime[j+3]);
								if(t2-t1>0 && t2-t1<2){
									dt[j+1]+=t2-t1;
									count[j+1]++;
								}
							}
						}
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

		auto gcount=new TGraph();
		int sum[1024]={0};
    FILE *w_fp;
    w_fp = fopen("TC.dat", "w");
    if(w_fp == NULL){
        printf("TC.dat write err\n");
        return -1;
    }

		for(int ch=ref_ch;ch<ref_ch+8;ch++){
			for(int sample=0;sample<1024;sample++){
				sum[sample]+=capa[ch][sample];
			}
		}
		for(int sample=0;sample<1024;sample++){
			gcount->SetPoint(sample,sample,sum[sample]);
			if(count[sample]==0)count[sample]=1;
			if(dt[sample]==0)dt[sample]=0.938;
			dt[sample]/=count[sample];
			fprintf(w_fp, "%d %f\n", sample, dt[sample]);
		}

		auto c1=new TCanvas();
		c1->cd();
		graph->Draw("apl");
		graph->SetMarkerStyle(kStar);
		graph->SetMarkerSize(0.3);
		c1->SaveAs("LocalTC.png");

		auto c2=new TCanvas();
		c2->cd();
		gcount->Draw("apl");
		gcount->SetMarkerStyle(kStar);
		gcount->SetMarkerSize(0.3);
		c2->SaveAs("LTCcount.png");
		
    return 0;
}  

double my_func(double *x, double *par){
	double func=par[0]*(par[2]+gtmp->Eval(x[0]-par[1]));
	return func;
}

double fitser(double *x, double *par){
	return TMath::Abs(f1->Eval(x[0])-thr);
}

double intersection(int start, int end){
	TF1 *fits=new TF1("fits",fitser,start,end,0);
	double xits=fits->GetMinimumX();
	delete fits;
	return xits;
}
