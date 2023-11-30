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
double fitser1(double *x, double *par);
double intersection1(double start, double end);
double fitser2(double *x, double *par);
double intersection2(double start, double end);
auto gtemp1=new TGraph();
auto gtemp2=new TGraph();
auto gcheck=new TGraph();
auto gcheck2=new TGraph();

std::string FileDir = "rawdata/";

const static option options[] = {
    {"FileName",        required_argument, NULL, 'f'},
    {"baseline_fname",  required_argument, NULL, 'b'},
		{"Ref_ch",  required_argument, NULL, 'r'},
    {0,0,0,0}
};

int main(int argc, char* argv[])
{
		std::cout<<"New calculating timing resolution"<<std::endl;

//    gStyle->SetOptStat(0);

    int ii, index;
    int check_ch = 0;
		int count[1024]={0};
		float pd[1024]={0};

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
		int row=0;
		double dummy;
		double wftime[1024]={0};
		std::ifstream time_input("TC.dat");

		if(time_input.is_open()){
			std::cout<<"TC data read successful"<<std::endl;
			std::string time_data;
			time_input.clear();
			time_input.seekg(0,std::ios::beg);
			while(getline(time_input,time_data)){
				std::istringstream iss(time_data);
				iss >> dummy >> wftime[row];
				row++;
			}
			for(int k=1;k<1024;k++){
				wftime[k]+=wftime[k-1];
			}
		}

		if(!time_input.is_open()){
			std::cout<<"No TC data is found, calculating TRes before TC"<<std::endl;
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
		float t_interval=62.5;
		auto graph=new TGraph();
		TH1F *hist[11];
		hist[0]=new TH1F("0ns","0ns",40,-10,10);
		hist[1]=new TH1F("60ns","60ns",50,t_interval*1-12.5,t_interval*1+12.5);
		hist[2]=new TH1F("120ns","120ns",50,t_interval*2-12.5,t_interval*2+12.5);
		hist[3]=new TH1F("180ns","180ns",50,t_interval*3-12.5,t_interval*3+12.5);
		hist[4]=new TH1F("240ns","240ns",50,t_interval*4-12.5,t_interval*4+12.5);
		hist[5]=new TH1F("300ns","300ns",50,t_interval*5-12.5,t_interval*5+12.5);
		hist[6]=new TH1F("360ns","360ns",50,t_interval*6-12.5,t_interval*6+12.5);
		hist[7]=new TH1F("420ns","420ns",50,t_interval*7-12.5,t_interval*7+12.5);
		hist[8]=new TH1F("480ns","480ns",50,t_interval*8-12.5,t_interval*8+12.5);
		hist[9]=new TH1F("540ns","540ns",50,t_interval*9-12.5,t_interval*9+12.5);
		hist[10]=new TH1F("600ns","600ns",50,t_interval*10-12.5,t_interval*10+12.5);


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

				int start=0;
				int checkpoint[20]={0};
				float t_tmp;
				float t[12]={0};
				int period[20]={0};
				int tdelay=1;
				for(int ch = ref_ch; ch < ref_ch+2; ch++){
					int i=0;
					for(int sample=0;sample<1024;sample++){
						if(ch==ref_ch){
							if(start==0 && sample>40 && get_data_final[ch][sample]>0. && get_data_final[ch][sample-1]<0. && get_data_final[ch][sample+1]>0.)start=sample;
							gtemp1->SetPoint(sample,wftime[sample],get_data_final[ch][sample]);
						}
						else if(ch==ref_ch+1){
							if(sample>40 && get_data_final[ch][sample]>0. && get_data_final[ch][sample-1]<0. && get_data_final[ch][sample+1]>0.){
								checkpoint[i]=sample;
								i++;
							}
							gtemp2->SetPoint(sample,wftime[sample],get_data_final[ch][sample]);
						}
					}
					if(ch==ref_ch){
						t_tmp=intersection1(wftime[start-1],wftime[start+1]);
						if(gtemp1->Eval(t_tmp+2.)>0 && gtemp1->Eval(t_tmp-2.)<0)t[0]=t_tmp;
					}
					if(ch==ref_ch+1)for(int xt=0;xt<i && tdelay<12;xt++){
						t_tmp=intersection2(wftime[checkpoint[xt]-1],wftime[checkpoint[xt]+1]);
						if(gtemp2->Eval(t_tmp+2.)>0 && gtemp2->Eval(t_tmp-2.)<0){
							t[tdelay]=t_tmp;
							tdelay++;
						}
					}
				}
				start=2;
				hist[0]->Fill(t[1]-t[0]);
				hist[1]->Fill(t[2]-t[0]);
				hist[2]->Fill(t[3]-t[0]);
				hist[3]->Fill(t[4]-t[0]);
				hist[4]->Fill(t[5]-t[0]);
				hist[5]->Fill(t[6]-t[0]);
				hist[6]->Fill(t[7]-t[0]);
				hist[7]->Fill(t[8]-t[0]);
				hist[8]->Fill(t[9]-t[0]);
				hist[9]->Fill(t[10]-t[0]);
				hist[10]->Fill(t[11]-t[0]);
				if((t[5]-t[0])>255)for(int sample=0;sample<1024;sample++){
					gcheck->SetPoint(sample,wftime[sample],get_data_final[ref_ch+1][sample]);
				}
				if((t[5]-t[0])<235.5)for(int sample=0;sample<1024;sample++){
					gcheck2->SetPoint(sample,wftime[sample],get_data_final[ref_ch+1][sample]);
				}

				int np=0;
				for(int sample=1;sample<1023;sample++){
					if(get_data_final[ref_ch][sample]>=0. && get_data_final[ref_ch][sample-1]<0. && get_data_final[ref_ch][sample+1]>0.){
						period[np]=sample;
						np++;
					}
				}
				for(int sample=1;sample<np;sample++){
					if(wftime[period[sample]]-wftime[period[sample-1]]>50 && wftime[period[sample]]-wftime[period[sample-1]]<70){
						pd[period[sample-1]]+=wftime[period[sample]]-wftime[period[sample-1]];
						pd[period[sample]]+=wftime[period[sample]]-wftime[period[sample-1]];
						count[period[sample-1]]++;
						count[period[sample]]++;
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


    TF1* func_gaus = new TF1("func_gaus", "gaus");

    TCanvas *Canvas = new TCanvas("canvas", "canvas", 1000, 1000);
    Canvas->cd();
    Canvas->Divide(4,3);

    FILE *w_fp;
    w_fp = fopen("result.txt", "w");
    if(w_fp == NULL){
        printf("result.txt write err\n");
        return -1;
    }


		fprintf(w_fp, "mean, sigma, sigmaErr, Chisquar, RMS, RMSErr\n");
		for(int delay=0;delay<11;delay++){
			Canvas->cd(delay+1);
    	hist[delay]->Draw();
    	hist[delay]->Fit(func_gaus);
    	float mean = (float)func_gaus->GetParameter(1);
    	float sigma = (float)func_gaus->GetParameter(2);
    	float sigmaErr = (float)func_gaus->GetParError(2);
    	float Chisquar = (float)func_gaus->GetChisquare();


    	float RMS = (float)hist[delay]->GetRMS();
    	float RMSErr = (float)hist[delay]->GetRMSError();
    	fprintf(w_fp, "%f %f %f %f %f %f\n", mean, sigma, sigmaErr, Chisquar, RMS, RMSErr);
		}
		Canvas->SaveAs("Time_Resolution.png");

    TCanvas *Canvas2 = new TCanvas("canvas2", "canvas2", 1000, 1000);
		Canvas2->cd();
		for(int sample=0;sample<1024;sample++){
			if(count[sample]==0)count[sample]=1;
			graph->SetPoint(sample,sample,pd[sample]/count[sample]);
		}
		graph->Draw("ap");
		graph->SetMarkerStyle(kStar);
		graph->SetMarkerSize(0.6);
		graph->GetYaxis()->SetRangeUser(50,70);
		Canvas2->SaveAs("Period.png");

		TCanvas *Canvas3 = new TCanvas("canvas3", "canvas3", 1000, 1000);
		Canvas3->cd();
		Canvas3->Divide(1,2);
		Canvas3->cd(1);
		gcheck->Draw();
		Canvas3->cd(2);
		gcheck2->Draw();
		Canvas3->SaveAs("Check.pdf");

    return 0;
}   

double fitser1(double *x, double *par){
  return TMath::Abs(gtemp1->Eval(x[0]));
}

double intersection1(double start, double end){
	TF1 *fits1=new TF1("fits1",fitser1,start,end,0);
  double xits=fits1->GetMinimumX();
	delete fits1;
  return xits;
}

double fitser2(double *x, double *par){
  return TMath::Abs(gtemp2->Eval(x[0]));
}

double intersection2(double start, double end){
	TF1 *fits2=new TF1("fits2",fitser2,start,end,0);
  double xits=fits2->GetMinimumX();
	delete fits2;
  return xits;
}
