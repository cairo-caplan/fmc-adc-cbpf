#include "TCanvas.h"
#include "TGraph.h"
#include "TSystem.h"
#include "TAxis.h"
#include "TFrame.h"
#include <string.h>
#include <unistd.h>
#include <iostream>

void V_t_continuous_INT()
{
	TCanvas *c1 = new TCanvas("c1", "V vs time sample Graph", 800, 600);
	TGraph *gr;
	
	//TCanvas *c1 = new TCanvas("c1","A Simple Graph Example",200,10,700,500);

	c1->SetFillColor(42);
	c1->SetGrid();

	const Int_t n = 20;
	Double_t x[n], y[n];
	//10 for -5 to +5V, 1 to -500mV to + 500 mV, 100 to -50 mv to + 50 mV
	int scale = 10;
	char FileName[200];
	char ErrFile[200];
	char Command[300];
	


	while(1)
	{
		//Need to be fixed to use the driver example program we adapted
		//sprintf(Command,"./fald-acq -a 400000 -b 0 -n 1 -l 1 -g 1 -c 1 -t 10 0100 > Data.txt");
		
		
		sprintf(FileName,"%s/Data.txt",gSystem->pwd());
		sprintf(ErrFile,"%s/Err.txt",gSystem->pwd());
		//sleep(1);
		
		//system("program args 1>/tmp/program.stdout 2>/tmp/program.stderr as in
		//http://docstore.mik.ua/orelly/perl/cookbook/ch16_08.htm
		//sprintf(Command,"%s/fald-acq -a 1000 -b 0 -n 1 -l 1 -g 1 -e -r 10 0100 1> %s 2> %s",gSystem->pwd(), FileName,ErrFile );
		//sprintf(Command,"./fald-acq -a 100 -b 0 -n 1 -l 1 -g 1 -c 1 -t 10000 -r 10 0100 > %s",FileName);
	    	//sprintf(Command,"%s/fald-acq -a 1000 -b 0 -n 1 -l 1 -g 1 -e -r 10 0100",gSystem->pwd());
		//gSystem->Exec(Command);
		

		//sprintf(Command,"source %s/continuous_exec_script.sh",gSystem->pwd() );
		sprintf(Command,"./fald-simple-acq -a 1000 -b 1000 -n 1 -t 500 -c 0 0x100 > /tmp/fmcadc.0x0100.ch1.dat.INT");
		  //<DEVID>: hexadecimal identifier (e.g.: "0x200")
                  //-before|-b <num>        number of pre samples
                  //--after|-a <num>         n. of post samples (default: 16)
		  //--nshots|-n <num>        number of trigger shots
		  //--delay|-d <num>         delay sample after trigger
		  //--under-sample|-U <num>  pick 1 sample every <num>
		  //--threshold|-t <num>     internal trigger threshold [0 - 32000]
		  //--channel|-c <num>       channel used as trigger (0..3)
		  //--tiemout|-T <millisec>  timeout for acquisition
		  //--negative-edge          internal trigger is falling edge
		  //--binary|-B <file>       save binary to <file>
		  //--multi-binary|-M <file> save two files per shot: <file>.0000.ctrl etc
		  //--dont-read|-N           config-only, use with zio-dump
		  //--help|-h                show this help
		  // ISODTAQ board ID 0x100
		FILE *test = gSystem->OpenPipe(Command,"r");
		gSystem->ClosePipe(test);
		//gSystem->Sleep(2000);
		c1->cd();	
		gr = new TGraph("/tmp/fmcadc.0x0100.ch1.dat.INT", "%lg %lg");
		// Signal 
		//(gr->GetYaxis())->SetRangeUser(-4000,4000);
                // Noise floor
		(gr->GetYaxis())->SetRangeUser(-32000,32000);		
		/*
		TH1D *hfft = new TH1D("hfft","hfft",2000,0,2e-6);
		for(Int_t ibin = 0; ibin<2000; ibin++) {
		  hfft->SetBinContent(ibin+1, 
		*/
		//gSystem->Sleep(100);
		
		
		
		//if (TFile::SizeOf(FileName)<=0) break;
		//cin >> err;
		//cout << err << endl;
		//strcmp(err,"./fald-acq: cannot start acquisition: Input/output error");
		//cout <<"running" <<endl;
		//cout << "TGraph *G1 = new TGraph(FileName = " << FileName << ") , loop i = " << i << endl;
		//See the columns configuration in FileName
		
		
		
		//gr = new TGraph(FileName, "%lg %lg %*lg %*lg %*lg");
		
		
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
		gr->GetYaxis()->SetTitle("ADC reading");
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
