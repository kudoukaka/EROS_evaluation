//#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>
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
#include "TTree.h"
#include "TMath.h"
#include "TStyle.h"
#include <string>

int CHNUM = 18;
Float_t wf[18][1024]={0};
std::string FileDir = "anadata/";

const static option options[] = {
    {"FileName",        required_argument, NULL, 'f'},
    {0,0,0,0}
};

int main(int argc, char* argv[])
{
	//file name input
	int ii, index;
	std::string fname;
	std::string baseline_fname;
	while( (ii = getopt_long(argc, argv, "f:b:", options, &index)) !=-1 ){
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

	TApplication* app = new TApplication("app",&argc,argv);
	char hist_name[18];
	TH1F *hist[18];
	for(int j = 0; j < 18; j++){
		sprintf(hist_name, "%d",  j);
		hist[j] = new TH1F(hist_name, hist_name, 80, -10, 30);
	}

	//CrossTalk
	float max=0;
	float min=0;
	float wf_base[200];

	for(int events=0; events<nentries; events++){
		tree->GetEntry(events);	
		for(int ch = 0; ch < CHNUM; ch++){
			//for Ext. trig
			float wf_ptr[100];
			for(int gate=500;gate<700;gate++){
				wf_base[gate-500]=wf[ch][gate];
			}
			float noise=TMath::Mean(200,wf_base)+TMath::RMS(200,wf_base);;
			for(int gate=800;gate<900;gate++){
				wf_ptr[gate-800]=wf[ch][gate];
				//hist[ch]->Fill(gate, wf[ch][gate]);
			}
			max=TMath::MaxElement(100,wf_ptr);
			min=TMath::MinElement(100,wf_ptr);
			if(max>noise)hist[ch]->Fill(max-min);
			//hist[ch]->Fill(b);
		}
	}

    TF1* func_gaus = new TF1("func_gaus", "gaus");

    TCanvas *Canvas = new TCanvas("canvas", "canvas", 100, 100, 1000, 1000);
    Canvas->cd();
    Canvas->Divide(4,4);

    FILE *w_fp;
    w_fp = fopen("crosstalk.txt", "w");
    if(w_fp == NULL){
        printf("crosstalk.txt write err\n");
        return -1;
    }


		fprintf(w_fp, "ch, mean, sigma, sigmaErr, Chisquar, RMS, RMSErr\n");
    for(int ch = 0; ch < 16; ch++){
        Canvas->cd(ch+1);
        hist[ch]->Draw();
        hist[ch]->Fit(func_gaus,"q","",0,10);
        float mean = (float)func_gaus->GetParameter(1);
        float sigma = (float)func_gaus->GetParameter(2);
        float sigmaErr = (float)func_gaus->GetParError(2);
        float Chisquar = (float)func_gaus->GetChisquare();


        float RMS = (float)hist[ch]->GetRMS();
        float RMSErr = (float)hist[ch]->GetRMSError();
        fprintf(w_fp, "%d %f %f %f %f %f %f\n", ch, mean, sigma, sigmaErr, Chisquar, RMS, RMSErr);
    }
		Canvas->SaveAs("crosstalk.png");

    return 0;
}    
