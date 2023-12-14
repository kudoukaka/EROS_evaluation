#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <getopt.h>
#include <string.h>

#include "TApplication.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "TGraph.h"
#include "TH2F.h"
#include "TF1.h"
#include "TMath.h"
#include "TStyle.h"
#include <string>

int stopcapacitor[2]={0};
double period=62.5;
double thr=0;//threshold for intersection
double t[2][12]={0};
double dt[2][1024]={0};
double wf[18][1024]={0};
double wftime[2][1024]={0};
TH1F *hist[2][11];
char hist_name[2][11];
auto gwf=new TGraph();

const static option options[] = {
	{"datafile",        required_argument, NULL, 'f'},
	{"TC_data_folder",  required_argument, NULL, 't'},
	{0,0,0,0}
};


//================================================================
//Check intersection point
bool crossing(int ch, int sample, double cross=0, int gradient=-1){
	if(gradient==-1 && wf[ch][sample]<=cross && wf[ch][sample-1]>cross && wf[ch][sample+1]<cross)return true;
	if(gradient==1 && wf[ch][sample]>=cross && wf[ch][sample-1]<cross && wf[ch][sample+1]>cross)return true;
	else return false;
}
double fitser(double *x, double *par){
	return TMath::Abs(gwf->Eval(x[0])-thr);
}
double intersection(double start, double end){
	TF1 *fits=new TF1("fits",fitser,start,end,0);
	double xits=fits->GetMinimumX();
	delete fits;
	return xits;
}


//================================================================
//Initilize pattern and wftime
void MakePattern(int chips, int ch=0, int cidx=0){
	wftime[chips][0]=0.;
	for(int sample=1;sample<1024;sample++){
		wftime[chips][sample]=wftime[chips][sample-1]+dt[chips][(sample-1+cidx)%1024];
	}
	for(int sample=0;sample<1024;sample++){
		gwf->SetPoint(sample,wftime[chips][sample],wf[ch][sample]);
	}
}


//================================================================
//Create Histogram
void MakeHisto(){
	for(int chips=0;chips<2;chips++){
		for(int i=0;i<11;i++){
			sprintf(hist_name[chips],"%d",i+100*chips);
			if(i==0)hist[chips][i]=new TH1F(hist_name[chips],hist_name[chips],40,-2,2);
			if(i>0)hist[chips][i]=new TH1F(hist_name[chips],hist_name[chips],100,period*i-12.5,period*i+12.5);
		}
	}
}


//================================================================
//Calculate Time Resolution
void TimeResolution(int chips=0, int cidx=0){
	for(int ch=chips*8;ch<chips*8+2;ch++){
		MakePattern(chips,ch,cidx);
		int ii[2]={0};
		for(int sample=1;sample<1000;sample++){
			if(crossing(ch,sample,thr,1)==true && ii[ch%8]<12){
				t[ch%8][ii[ch%8]]=intersection(wftime[chips][sample-1],wftime[chips][sample+1]);
				ii[ch%8]++;
			}
		}
	}
	for(int figure=1;figure<12;figure++){
		hist[chips][figure-1]->Fill(t[1][figure]-t[0][1]);
	}
}


//================================================================
//Output Time REsolution Figure and Result
int Output(){
	TF1* func_gaus = new TF1("func_gaus", "gaus");
	TCanvas *Canvas[2];
	Canvas[0]	= new TCanvas("canvas1", "canvas1", 1000, 1000);
	Canvas[1]	= new TCanvas("canvas2", "canvas2", 1000, 1000);
	Canvas[0]->cd();
	Canvas[0]->Divide(4,3);
	Canvas[1]->cd();
	Canvas[1]->Divide(4,3);

	FILE *w_fp;
	w_fp = fopen("tres.txt", "w");
	if(w_fp == NULL){
		printf("tres.txt write err\n");
		return -1;
	}

	fprintf(w_fp, "mean, sigma, sigmaErr, Chisquar, RMS, RMSErr\n");
	for(int chips=0;chips<2;chips++){
		for(int delay=0;delay<11;delay++){
			Canvas[chips]->cd(delay+1);
			hist[chips][delay]->Draw();
			hist[chips][delay]->Fit(func_gaus);
			double mean = (double)func_gaus->GetParameter(1);
			double sigma = (double)func_gaus->GetParameter(2);
			double sigmaErr = (double)func_gaus->GetParError(2);
			double Chisquar = (double)func_gaus->GetChisquare();
			double RMS = (double)hist[chips][delay]->GetRMS();
			double RMSErr = (double)hist[chips][delay]->GetRMSError();
			fprintf(w_fp, "%f %f %f %f %f %f\n", mean, sigma, sigmaErr, Chisquar, RMS, RMSErr);
		}
	}
	Canvas[0]->SaveAs("chip1_tres.png");
	Canvas[1]->SaveAs("chip2_tres.png");
	return 0;
}

//================================================================
//Main function
Int_t main(Int_t argc, Char_t* argv[]){
	std::cout<<"New calculating timing resolution"<<std::endl;

	int ii, index;
	//file name input
	std::string fname;
	std::string tc;
	while( (ii = getopt_long(argc, argv, "f:t:", options, &index)) !=-1 ){
		switch(ii){
			case 'f':
				fname = optarg;
				break;
			case 't':
				tc = optarg;
				break;
		}
	}

	//read tc data
	int row=0;
	double dummy;
	std::string tc_path[2];
	tc_path[0]=(tc+"chip1.dat");
	tc_path[1]=(tc+"chip2.dat");
	std::ifstream time_input[2];
	time_input[0].open(tc_path[0].c_str());
	time_input[1].open(tc_path[1].c_str());

	if(time_input[0].is_open() && time_input[1].is_open()){
		std::cout<<"TC data read successful"<<std::endl;
		for(int i=0;i<2;i++){
			std::string time_data[2];
 			time_input[i].clear();
 			time_input[i].seekg(0,std::ios::beg);
 			while(getline(time_input[i],time_data[i])){
				std::istringstream iss(time_data[i]);
				iss >> dummy >> dt[i][row];
				row++;
			}
			row=0;
		}
	}

	if(!time_input[0].is_open() || !time_input[1].is_open()){
		std::cout<<"No TC data is found, calculating TRes before TC"<<std::endl;
		for(int chips=0;chips<2;chips++){
			for(int i=0;i<1024;i++){
				dt[chips][i]=0.938;
			}
		}
	}

	//read waveform data
	std::string datapath="anadata/"+fname;
	std::cout<<"Reading data file..."<<std::endl;
	auto fdata = TFile::Open(datapath.c_str(),"READ");

	if(!fdata || fdata->IsZombie()){
		std::cerr<<"Failed to open root file '"<<fname<<"'"<<std::endl;
		delete fdata;
		return -1;
  }

	auto tree=(TTree*)fdata->Get("datatree");
	Int_t nentries=tree->GetEntries();
	tree->SetBranchAddress("wf",wf);
	tree->SetBranchAddress("cidx",stopcapacitor);
	std::cout<<"Data file read successful!"<<std::endl;

	//Calculate Time Resolution
	MakeHisto();
	for(int events=0; events<nentries;events++){
		tree->GetEntry(events);
		if((events+1)%1000==0)std::cout<<"Now Processing events: "<<events+1<<std::endl;
		for(int chips=0;chips<2;chips++){
			TimeResolution(chips,stopcapacitor[chips]);
		}
	}

	Output();
}
