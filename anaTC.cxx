#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <getopt.h>
#include <string.h>
#include <chrono>
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

//Set Global Parameters
Float_t period=62.5;//16MHz sine wave
Float_t pre_dt=0.938;//nanosecond
Int_t events=0;
Int_t stopcapacitor[2]={0};
Float_t wf[18][1024]={0};
Float_t dt[2][1024]={0};
Float_t dt_fall[2][1024]={0};//Local TC falling edge
Float_t dt_rise[2][1024]={0};//Local TC rising edge
Int_t count_fall[2][1024]={0};//Local TC falling count
Int_t count_rise[2][1024]={0};//Local TC rising count
Float_t dt_cnt[2][1024]={0};//Local TC count
Int_t checked_local_capa[2][16][1024]={0};//check every capacitor for only 10 times in fall/rise edge, respectively
Float_t factor_pdt=0;//Global TC factor for every capa
Float_t factor_cdt[2][1024]={0};//Global TC capa count
Int_t checked_global_capa[16][1024]={0};//check every capacitor for only once in global TC
Float_t wftime[2][1024]={0};
TF1 *flocal=NULL;
TGraph *gtpl[2];//waveform template
TGraph *gspike=new TGraph();
float ptpl[1024]={0};
float ttpl[1024]={0};
int pcnt[1024]={0};
//auto gwf=new TGraph();

const static option options[] = {
	{"datafile",        required_argument, NULL, 'f'},
  {0,0,0,0}
};

//================================================================
//fitting func for Local TC
using FitFunction=double (*)(double*, double*);
double my_func1(double *x, double *par){
	float func=par[0]*(par[2]+gtpl[0]->Eval(x[0]-par[1]));
	return func;
}
double my_func2(double *x, double *par){
	float func=par[0]*(par[2]+gtpl[1]->Eval(x[0]-par[1]));
	return func;
}


//================================================================
//Check intersection point
bool intersection(int ch, int sample, float thr=0, int gradient=-1){
	if(gradient==-1 && wf[ch][sample]<=thr && wf[ch][sample-1]>thr && wf[ch][sample+1]<thr)return true;
	if(gradient==1 && wf[ch][sample]>=thr && wf[ch][sample-1]<thr && wf[ch][sample+1]>thr)return true;
	else return false;
}
//float fitser(float *x, float *par){
//	return TMath::Abs(flocal->Eval(x[0])-thr);
//}
//float intersection(int start, int end){
//	TF1 *fits=new TF1("fits",fitser,start,end,0);
//	float xits=fits->GetMinimumX();
//	delete fits;
//	return xits;
//}


//================================================================
//Initilize pattern and wftime
void MakePattern(int chips, int ch=0, int cidx=0){
	wftime[chips][0]=0.;
	for(int sample=1;sample<1024;sample++){
		wftime[chips][sample]=wftime[chips][sample-1]+dt[chips][(sample-1+cidx)%1024];
	}
//	for(int sample=0;sample<1024;sample++){
//		gwf->SetPoint(sample,wftime[chips][sample],wf[ch][sample]);
//	}
}


//================================================================
//Create pattern for local TC
void WaveTemplate(int chips=0, int cidx=0){
	int i=0;
	int checkpoint[2]={0};
	for(int ch=chips*8;ch<chips*8+8;ch++){
		MakePattern(chips,ch,cidx);
		for(int sample=1;sample<200;sample++){
			if(i<2 && wf[ch][sample-1]>0 && wf[ch][sample]<=0 && wf[ch][sample+1]<0){
				checkpoint[i]=sample;
				i++;
			}
		}
		for(int sample=0;checkpoint[0]+sample<1024;sample++){
			ttpl[sample]+=wftime[chips][sample];
			ptpl[sample]+=wf[ch][checkpoint[1]+sample-25];//Use the second point crossing 0 on falling edge as reference point
			pcnt[sample]++;
		}
	}
}


//================================================================
//LocalCalibration function
void LocalCalibration(int chips=0, int cidx=0){
//	FitFunction my_func[2] = {my_func1, my_func2};
//	flocal=new TF1("flocal",my_func[chips],0,1024,3);
	for(int ch=chips*8;ch<chips*8+8;ch++){
		MakePattern(chips,ch,cidx);
		int ii=0;//falling edge
		int jj=0;//rising edge
		int checkpoint[4][20]={0};//60 to -10
		for(int sample=50;sample<1000;sample++){
			if(intersection(ch,sample,60,-1)==true){
				checkpoint[0][ii]=sample;
			}
			if(intersection(ch,sample,-10,-1)==true && checkpoint[0][ii]!=0){
				checkpoint[1][ii]=sample;
				ii++;
			}
			if(intersection(ch,sample,-10,1)==true){
				checkpoint[2][jj]=sample;
			}
			if(intersection(ch,sample,60,1)==true && checkpoint[2][jj]!=0){
				checkpoint[3][jj]=sample;
				jj++;
			}
		}
		//falling edge calibration
		for(int point=0;point<ii;point++){
			if(checked_local_capa[0][ch][(checkpoint[0][point]+cidx)%1024]>10)continue;
			if(checkpoint[0][point]==0 || checkpoint[1][point]==0)continue;
			//check smoothness (ignore waveform with spikes)
			bool spike=false;
			float spikes=0;
			for(int k=checkpoint[0][point];k<checkpoint[1][point];k++){
				if(wf[ch][k]<wf[ch][k+1]){
					spikes=wf[ch][k+1]-wf[ch][k];
					spike=true;
				}
			}
			if(spike){
				if(spikes>6)for(int sample=0;sample<1024;sample++){
					gspike->SetPoint(sample, sample, wf[chips][sample]);
				}
				continue;
			}
			//Start Calibration
			float du=wf[ch][checkpoint[1][point]]-wf[ch][checkpoint[0][point]];
			float dt_local=wftime[chips][checkpoint[1][point]]-wftime[chips][checkpoint[0][point]];
			for(int k=checkpoint[0][point];k<checkpoint[1][point];k++){
				float dv=wf[ch][k+1]-wf[ch][k];
				float cor=dt_local*dv/du;
				dt_fall[chips][(k+cidx)%1024]+=cor;
				count_fall[chips][(k+cidx)%1024]++;
			}
			checked_local_capa[0][ch][(checkpoint[0][point]+cidx)%1024]++;
		}
		//rising edge calibration
		for(int point=0;point<jj;point++){
			if(checked_local_capa[1][ch][(checkpoint[2][point]+cidx)%1024]>10)continue;
			if(checkpoint[2][point]==0 || checkpoint[3][point]==0)continue;
			//check smoothness (ignore waveform with spikes)
			bool spike=false;
			for(int k=checkpoint[2][point];k<checkpoint[3][point];k++){
				if(wf[ch][k]>wf[ch][k+1])spike=true;
			}
			if(spike)continue;
			//Start Calibration
			float du=wf[ch][checkpoint[3][point]]-wf[ch][checkpoint[2][point]];
			float dt_local=wftime[chips][checkpoint[3][point]]-wftime[chips][checkpoint[2][point]];
			for(int k=checkpoint[2][point];k<checkpoint[3][point];k++){
				float dv=wf[ch][k+1]-wf[ch][k];
				float cor=dt_local*dv/du;
				dt_rise[chips][(k+cidx)%1024]+=cor;
				count_rise[chips][(k+cidx)%1024]++;
			}
			checked_local_capa[1][ch][(checkpoint[0][point]+cidx)%1024]++;
		}
	}
}


//================================================================
//GlobalCalibration function
void GlobalCalibration(int chips=0, int cidx=0){
	for(int ch=chips*8;ch<chips*8+8;ch++){
		MakePattern(chips,ch,cidx);
		int ncp[3]={0};
		int checkpoint[3][20]={0};//rising 0, falling 0 & rising 30
		for(int sample=50;sample<1000;sample++){
			if(intersection(ch,sample,0,1)==true){
				checkpoint[0][ncp[0]]=sample;
				ncp[0]++;
			}
			if(intersection(ch,sample,0,-1)==true){
				checkpoint[1][ncp[1]]=sample;
				ncp[1]++;
			}
			if(intersection(ch,sample,30,1)==true){
				checkpoint[2][ncp[2]]=sample;
				ncp[2]++;
			}
		}
		for(int i=0;i<3;i++){
			for(int point=0;point<ncp[i]-1;point++){
				if(checked_global_capa[ch][(checkpoint[i][point]+cidx)%1024]>1)continue;
				float dt_global=wftime[chips][checkpoint[i][point+1]-1]-wftime[chips][checkpoint[i][point]];
				float dtk=wftime[chips][checkpoint[i][point]]-wftime[chips][checkpoint[i][point]-1];
				float dtq=wftime[chips][checkpoint[i][point+1]-1]-wftime[chips][checkpoint[i][point+1]-2];
				float uk=wf[ch][checkpoint[i][point]];
				float duk=wf[ch][checkpoint[i][point]]-wf[ch][checkpoint[i][point]-1];
				float uq=wf[ch][checkpoint[i][point+1]-1];
				float duq=wf[ch][checkpoint[i][point+1]-1]-wf[ch][checkpoint[i][point+1]-2];
				factor_pdt=period/(dt_global+(dtk*uk/duk)-(dtq*uq/duq));
				if(factor_pdt>1.1 || factor_pdt<0.9)continue;
				for(int idx=checkpoint[i][point];idx<checkpoint[i][point+1]-2;idx++){
					dt[chips][(idx+cidx)%1024]*=factor_pdt;
					factor_cdt[chips][(idx+cidx)%1024]++;
				}
				checked_global_capa[ch][(checkpoint[i][point]+cidx)%1024]++;
			}
		}
	}
}


//================================================================
//Main function
Int_t main(Int_t argc, Char_t* argv[]){

	auto start_time = std::chrono::high_resolution_clock::now();

	int ii, index;
	//file name input
	std::string fname;
	while( (ii = getopt_long(argc, argv, "f:t:", options, &index)) !=-1 ){
		switch(ii){
			case 'f':
				fname = optarg;
				break;
		}
	}

	//read data file
	std::cout<<"Reading data file..."<<std::endl;

	auto fdata = TFile::Open(("anadata/"+fname).c_str(),"READ");

	if(!fdata || fdata->IsZombie()){
		std::cerr<<"Failed to open root file '"<<fname<<"'"<<std::endl;
		delete fdata;
		return -1;
	} 

	auto tree=(TTree*)fdata->Get("datatree");
	Int_t nentries=tree->GetEntries();
	tree->SetBranchAddress("wf",wf);
	tree->SetBranchAddress("cidx",stopcapacitor);


	//Initilization
	for(int chips=0;chips<2;chips++){
		for(int sample=0;sample<1024;sample++){
			dt[chips][sample]=pre_dt;
		}
	}

	//Local Calibration
	std::cout<<"Now Performing Local Calibration..."<<std::endl;

	//Make waveform template for Local TC
//	for(int i=0;i<2;i++){
//		gtpl[i]=new TGraph();
//	}
//	std::cout<<"Creating waveform template..."<<std::endl;
//	for(int chips=0;chips<2;chips++){
//		for(events=0; events<nentries; events++){
//			tree->GetEntry(events);
//			WaveTemplate(chips,stopcapacitor[chips]);
//		}
//		for(int sample=0;sample<80;sample++){
//			gtpl[chips]->SetPoint(sample,ttpl[sample]/pcnt[sample],ptpl[sample]/pcnt[sample]);
//			ttpl[sample]=0; pcnt[sample]=0; ptpl[sample]=0;
//		}
//		TCanvas *Canvas = new TCanvas("canvas", "canvas", 600, 600);
//		gtpl[chips]->Draw("apl");
//		gtpl[chips]->SetMarkerSize(1);
//		gtpl[chips]->SetMarkerStyle(kStar);
//		if(chips==0){
//			gtpl[chips]->SetName("template1");
//			Canvas->SaveAs("template1.png");
//		}
//		if(chips==1){
//			gtpl[chips]->SetName("template2");
//			Canvas->SaveAs("template2.png");
//		}
//	}


	//Locat TC start
	for(events=0; events<nentries; events++){
		tree->GetEntry(events);
		if((events+1)%1000==0)std::cout<<"Now Processing events: "<<events+1<<std::endl;
		for(int chips=0;chips<2;chips++){
			LocalCalibration(chips,stopcapacitor[chips]);//Calibration done by seperating rising edge and falling edge
		}
	}
	for(int chips=0;chips<2;chips++){
		for(int capa=0;capa<1024;capa++){
			dt_fall[chips][capa]/=count_fall[chips][capa];
			dt_rise[chips][capa]/=count_rise[chips][capa];
			dt_cnt[chips][capa]=count_fall[chips][capa]+count_rise[chips][capa];
			dt[chips][capa]=(dt_fall[chips][capa]+dt_rise[chips][capa])/2;
		}
	}
	std::cout<<"Local Calibration Finished!"<<std::endl;


	//Global Calibration
	std::cout<<"Now Performing Global Calibration..."<<std::endl;

	for(events=0; events<nentries; events++){
		tree->GetEntry(events);
		if((events+1)%1000==0)std::cout<<"Now Processing events: "<<events+1<<std::endl;
		for(int chips=0;chips<2;chips++){
			GlobalCalibration(chips,stopcapacitor[chips]);
		}
	}
	std::cout<<"Global Calibration Finished!"<<std::endl;


	//draw counted capacitor(are they all calibrated?)
	TCanvas *clocal=new TCanvas("cloacl","clocal",1000,800);
	clocal->Divide(1,2);
	TGraph *glocal_cnt[2];
	for(int chips=0;chips<2;chips++){
		clocal->cd(chips+1);
		glocal_cnt[chips]=new TGraph();
		for(int capa=0;capa<1024;capa++){
			glocal_cnt[chips]->SetPoint(capa,capa,dt_cnt[chips][capa]);
		}
		glocal_cnt[chips]->Draw();
		glocal_cnt[chips]->GetXaxis()->SetTitle("Capacitor Number");
		glocal_cnt[chips]->GetYaxis()->SetTitle("Count");
	}
	clocal->SaveAs("local_count.png");

	//draw counted capacitor in global TC
	TCanvas *cglobal=new TCanvas("cglobal","cglobal",1000,800);
	cglobal->Divide(1,2);
	TGraph *gglobal_cnt[2];
	float global_count[2][1024]={0};
	for(int chips=0;chips<2;chips++){
		cglobal->cd(chips+1);
		gglobal_cnt[chips]=new TGraph();
		for(int capa=0;capa<1024;capa++){
			for(int ch=chips*8;ch<chips*8+8;ch++){
				global_count[chips][capa]+=checked_global_capa[ch][capa];
			}
			gglobal_cnt[chips]->SetPoint(capa,capa,factor_cdt[chips][capa]);
		}
		gglobal_cnt[chips]->Draw();
		gglobal_cnt[chips]->GetXaxis()->SetTitle("Capacitor Number");
		gglobal_cnt[chips]->GetYaxis()->SetTitle("Count");
	}
	cglobal->SaveAs("global_count.png");

	//Draw waveform with spike
	TCanvas *cspike=new TCanvas("cspike","cspike",1000,600);
	cspike->cd();
	gspike->Draw("apl");
	gspike->GetXaxis()->SetRangeUser(200,400);
	gspike->SetMarkerSize(0.6);
	gspike->SetMarkerStyle(kStar);
	cspike->SaveAs("spike.png");

	//output TC data (x:capacitor number, y:calibrated timing)
	FILE *w_fp[2];
	w_fp[0] = fopen("chip1.dat", "w");
	w_fp[1] = fopen("chip2.dat", "w");
	if(w_fp[0] == NULL || w_fp[1] == NULL){
		printf("TC.dat write err\n");
		return -1;
	}
	for(int chips=0;chips<2;chips++){
		for(int sample=0;sample<1024;sample++){
			fprintf(w_fp[chips], "%d %f\n", sample, dt[chips][sample]);
		}
	}
	fdata->Close();

	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
	std::cout << "Run time: " << duration.count() << " ms" << std::endl;

	return 0;
}
