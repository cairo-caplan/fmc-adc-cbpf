#include "TCanvas.h"
#include "TGraph.h"
#include "TSystem.h"
#include "TAxis.h"
#include "TFrame.h"
#include "TTimer.h"
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <time.h>

void V_t_continuous_grp7()
{
	TCanvas *c1 = new TCanvas("c1", "Vxt", 800, 600);
	TGraph *gr;
	
	//TCanvas *c1 = new TCanvas("c1","A Simple Graph Example",200,10,700,500);

	c1->SetFillColor(42);
	c1->SetGrid();

	const Int_t n = 20;
	Double_t x[n], y[n];
	//10 for -5 to +5V, 1 to -500mV to + 500 mV, 100 to -50 mv to + 50 mV
	int scale = 1;
	char FileName[200];
	char ErrFile[200];
	char Command[300];
	
	//string err;

	// Sweep timer:
	time_t start;
	bool started;

	while(1)
	{
		//Need to be fixed to use the driver example program we adapted
	        //sprintf(Command,"./fald-acq -a 400000 -b 0 -n 1 -l 1 -g 1 -e -t 10 0100 > Data.txt");
		
		
		sprintf(FileName,"%s/Data.txt",gSystem->pwd());
		sprintf(ErrFile,"%s/Err.txt",gSystem->pwd());
		gSystem->Exec("echo 0 > /sys/devices/hw-adc-100m14b-0100/adc-100m14b-0100/enable");
		gSystem->Exec("echo 1 > /sys/devices/hw-adc-100m14b-0100/adc-100m14b-0100/enable");
		//sleep(1);
		
		//system("program args 1>/tmp/program.stdout 2>/tmp/program.stderr as in
		//http://docstore.mik.ua/orelly/perl/cookbook/ch16_08.htm
		//sprintf(Command,"%s/fald-acq -a 1000 -b 0 -n 1 -l 1 -g 1 -e -r 10 0100 1> %s 2> %s",gSystem->pwd(), FileName,ErrFile );
		//sprintf(Command,"./fald-acq -a 100 -b 0 -n 1 -l 1 -g 1 -c 1 -t 10000 -r 10 0100 > %s",FileName);
	    	//sprintf(Command,"%s/fald-acq -a 1000 -b 0 -n 1 -l 1 -g 1 -e -r 10 0100",gSystem->pwd());
		//gSystem->Exec(Command);
		
		sprintf(Command,"%s/fald-acq -a 500 -b 0 -n 1 -l 1 -g 1 -e -r 1 0100 1> %s 2> %s",gSystem->pwd(), FileName,ErrFile );
		FILE *test = gSystem->OpenPipe(Command,"r");
		gSystem->ClosePipe(test);
		
		//gr = new TGraph("/tmp/fmcadc.0x0100.ch1.dat", "%lg %lg %*lg %*lg %*lg");

		
		
		
		//gSystem->Sleep(100);
		
		
		
		//if (TFile::SizeOf(FileName)<=0) break;
		//cin >> err;
		//cout << err << endl;
		//strcmp(err,"./fald-acq: cannot start acquisition: Input/output error");
		//cout <<"running" <<endl;
		//cout << "TGraph *G1 = new TGraph(FileName = " << FileName << ") , loop i = " << i << endl;
		//See the columns configuration in FileName
		
	
		gr = new TGraph("/tmp/fmcadc.0x0100.ch1.dat", "%lg %lg");
		//c1->cd();

		// Get very first point of graph:
		#define THRESHOLD 0.1
		Double_t sample, value1, value2;
		gr->GetPoint(0,sample,value1);
		gr->GetPoint(3,sample,value2);
		if(value1 < 0 && value2 > 0) {
		  std::cout << "Triggered!" << std::endl;
		  if(!started) {
		    // FIXME: use nicer timing function!
		    start = time(NULL);
		    started = true;
		  }
		  else {
		    double diff = time(NULL) - start;
		    started = false;
		    std::cout << "dT: " << diff << " / " << (1/diff) << "Hz" << std::endl;
		  }
		}
		
		gSystem->ProcessEvents();
		//TGraph *G2 = new TGraph();
		
		//cout <<"2" <<endl;
		
		gr->SetLineColor(2);
		gr->SetLineWidth(4);
		gr->SetMarkerColor(4);
		gr->SetMarkerStyle(21);
		//cout <<"a simple graph" <<endl;
		gr->SetTitle("Vxt");
		gr->GetXaxis()->SetTitle("time sample");
		gr->GetYaxis()->SetTitle("Volts");
		//cout <<"3" <<endl;
		
		gr->Draw("ACP");

		gSystem->ProcessEvents();
		//cout << "Draw" << endl;
		//cout <<"4" <<endl;
		// TCanvas::Update() draws the frame, after which one can change it
		c1->Update();
		c1->GetFrame()->SetFillColor(21);
		c1->GetFrame()->SetBorderSize(12);
		c1->Modified();
		//cout <<"5" <<endl;
		
		gSystem->ProcessEvents();
		
		
		
	}
	 //gr = new TGraph(n,x,y);
	  delete gr;
	   cout << "Bye" << endl;
	 delete c1;
	 //
	return;
}
