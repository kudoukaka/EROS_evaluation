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
#include "TFile.h"
#include "TTree.h"
#include "TF1.h"
#include "TMath.h"
#include "TStyle.h"
#include <string>

std::string FileDir = "anadata/";
Float_t wf[18][1024]={0};
int CHNUM = 18;

const static option options[] = {
	{"FileName",        required_argument, NULL, 'f'},
	{0,0,0,0}
};

int main(int argc, char* argv[])
{
	//file name input
	int ii, index;
	std::string fname;
	while( (ii = getopt_long(argc, argv, "f:", options, &index)) !=-1 ){
		switch(ii){
			case 'f':
				fname = optarg;
				break;
			default :
				break;
		}
	}
	
	//read data file
	std::string filepath_string = (FileDir + fname);
	const char* filepath = filepath_string.c_str();
	printf("Reading data: %s\n", filepath);
	TFile *fdata = TFile::Open(filepath,"READ");

	if(!fdata || fdata->IsZombie()){
		std::cerr<<"Failed to open root file '"<<filepath<<"'"<<std::endl;
		delete fdata;
		return -1;
	}

	auto tree=(TTree*)fdata->Get("datatree");
	Int_t nentries=tree->GetEntries();
	tree->SetBranchAddress("wf",wf);

	char hist_name[18];
	TH1F *hist[CHNUM];
	for(int j = 0; j < CHNUM; j++){
		sprintf(hist_name, "%d",  j);
		hist[j] = new TH1F(hist_name, hist_name, 400, -50, 50);
	}

	for(int events=0; events<nentries; events++){
		tree->GetEntry(events);
		for(int ch=0;ch<CHNUM;ch++){
			for(int sample=0;sample<1024;sample++){
				hist[ch]->Fill(wf[ch][sample]);
			}
		}
	}
	
	TF1* func_gaus = new TF1("func_gaus", "gaus");
	TCanvas *Canvas = new TCanvas("canvas", "canvas", 100,100, 1000, 1000);
	Canvas->cd();
	Canvas->Divide(4,4);

	FILE *w_fp;
	w_fp = fopen("noise.txt", "w");
	if(w_fp == NULL){
		printf("noise.txt write err\n");
		return -1;
	}

	fprintf(w_fp, "ch, sigma, sigmaErr, Chisquar, RMS, RMSErr\n");
	for(int ch = 0; ch < CHNUM; ch++){
		Canvas->cd(ch+1);
		hist[ch]->Fit(func_gaus,"Q");
		float sigma = (float)func_gaus->GetParameter(2);
		float sigmaErr = (float)func_gaus->GetParError(2);
		float Chisquar = (float)func_gaus->GetChisquare();
		float RMS = (float)hist[ch]->GetRMS();
		float RMSErr = (float)hist[ch]->GetRMSError();
		fprintf(w_fp, "%d %f %f %f %f %f\n", ch, sigma, sigmaErr, Chisquar, RMS, RMSErr);
	}
	Canvas->SaveAs("noise_level.png");

	return 0;
}    
