/**
 * Author: Cairo Caplan <cairo@cbpf.br> in 2015
 * License: GPLv2
 *
 * This is a simple Graphical User Interface built using of ROOT to demonstrate
 * some capabilities of the FMC ADC project, where you can handle the acquision
 * window, trigger and voltage range parameters. It shows a data acquisition
 * graphic and also a fft of the acquired signal
 * 
 * This software is based on the constributions made for the lab 8 of the ISOTDAQ
 * school during its 2015 edition 
 *
 *
 */

#ifndef ROOT_TGDockableFrame
#include "TGDockableFrame.h"
#endif
#ifndef ROOT_TGMenu
#include "TGMenu.h"
#endif
#ifndef ROOT_TGMdiDecorFrame
#include "TGMdiDecorFrame.h"
#endif
#ifndef ROOT_TG3DLine
#include "TG3DLine.h"
#endif
#ifndef ROOT_TGMdiFrame
#include "TGMdiFrame.h"
#endif
#ifndef ROOT_TGMdiMainFrame
#include "TGMdiMainFrame.h"
#endif
#ifndef ROOT_TGMdiMenu
#include "TGMdiMenu.h"
#endif
#ifndef ROOT_TGListBox
#include "TGListBox.h"
#endif
#ifndef ROOT_TGNumberEntry
#include "TGNumberEntry.h"
#endif
#ifndef ROOT_TGScrollBar
#include "TGScrollBar.h"
#endif
#ifndef ROOT_TGComboBox
#include "TGComboBox.h"
#endif
/*#ifndef ROOT_TGuiBldHintsEditor
#include "TGuiBldHintsEditor.h"
#endif*/
/*
#ifndef ROOT_TGuiBldNameFrame
#include "TGuiBldNameFrame.h"
#endif
*/
#ifndef ROOT_TGFrame
#include "TGFrame.h"
#endif
#ifndef ROOT_TGFileDialog
#include "TGFileDialog.h"
#endif
#ifndef ROOT_TGShutter
#include "TGShutter.h"
#endif
#ifndef ROOT_TGButtonGroup
#include "TGButtonGroup.h"
#endif
#ifndef ROOT_TGCanvas
#include "TGCanvas.h"
#endif
#ifndef ROOT_TGFSContainer
#include "TGFSContainer.h"
#endif
/*
#ifndef ROOT_TGuiBldEditor
#include "TGuiBldEditor.h"
#endif
*/
#ifndef ROOT_TGColorSelect
#include "TGColorSelect.h"
#endif
#ifndef ROOT_TGButton
#include "TGButton.h"
#endif
#ifndef ROOT_TGFSComboBox
#include "TGFSComboBox.h"
#endif
#ifndef ROOT_TGLabel
#include "TGLabel.h"
#endif
#ifndef ROOT_TGMsgBox
#include "TGMsgBox.h"
#endif
/*
#ifndef ROOT_TRootGuiBuilder
#include "TRootGuiBuilder.h"
#endif
*/
#ifndef ROOT_TGTab
#include "TGTab.h"
#endif
#ifndef ROOT_TGListView
#include "TGListView.h"
#endif
#ifndef ROOT_TGSplitter
#include "TGSplitter.h"
#endif
#ifndef ROOT_TGStatusBar
#include "TGStatusBar.h"
#endif
#ifndef ROOT_TGListTree
#include "TGListTree.h"
#endif
/*
#ifndef ROOT_TGuiBldGeometryFrame
#include "TGuiBldGeometryFrame.h"
#endif
*/
#ifndef ROOT_TGToolTip
#include "TGToolTip.h"
#endif
#ifndef ROOT_TGToolBar
#include "TGToolBar.h"
#endif
#ifndef ROOT_TRootEmbeddedCanvas
#include "TRootEmbeddedCanvas.h"
#endif
#ifndef ROOT_TCanvas
#include "TCanvas.h"
#endif
/*
#ifndef ROOT_TGuiBldDragManager
#include "TGuiBldDragManager.h"
#endif
*/

#include "TGNumberEntry.h"
#include "RQ_OBJECT.h"

#include "Riostream.h"

#include "TSystem.h"

#include "TSysEvtHandler.h"

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

class AdcGui{
  RQ_OBJECT("AdcGui")
  
private:
  void BuildGui();
  bool Running;
  void CreateGraph();
  void UpdateGraph();
  void ToggleGraphState();
  
  TGMainFrame *frmMain;
  TRootEmbeddedCanvas *fRootEmbeddedCanvas696;
  TCanvas *c1;
  TGComboBox *cbScale;
  TGComboBox *cbTrigger;
  TGNumberEntry *neNrPosSamples;
  TGNumberEntry *neNrPreSamples;
  TGTextButton *btnStartStop;
  TGNumberEntry *neThreshold;
  TGStatusBar *statBar;
  TTimer *timer;

  FILE *test;
  bool WaitingTrigger;

  
  TH1D *hfft;
  TH1 *hm;
  
//
  long NrPosSamples;
  long NrPreSamples;
  
public:
   
  void SetNrPosSamples();
  void SetNrPreSamples();
  void Gui();
  void ~Gui();
  
  AdcGui();
  ~AdcGui();
};

void adc_gui(){
  AdcGui *app = new AdcGui();
}

void AdcGui::AdcGui()
{
    this->Running = false;
    this->WaitingTrigger = false;
    this->BuildGui();
}

AdcGui::~AdcGui()
{
    timer->Stop();
    delete timer;
    cout << "bye" << endl;
    this->Running = false;
    

    delete c1;
    delete cbScale;
    delete cbTrigger;
    delete neNrPosSamples;
    delete neNrPreSamples;
    delete btnStartStop;
    delete neThreshold;
    delete statBar;
    
    
    fRootEmbeddedCanvas696->Cleanup();
    delete fRootEmbeddedCanvas696;
    
    
   frmMain->Cleanup();
   frmMain->CloseWindow();
   delete frmMain;
}




void AdcGui::BuildGui()
{

   // main frame
   frmMain = new TGMainFrame(gClient->GetRoot(),10,10,kMainFrame | kVerticalFrame);
   frmMain->SetName("frmMain");
   frmMain->SetLayoutBroken(kTRUE);

   // embedded canvas
   fRootEmbeddedCanvas696 = new TRootEmbeddedCanvas(0,frmMain,800,600);
   fRootEmbeddedCanvas696->SetName("fRootEmbeddedCanvas696");
   Int_t wfRootEmbeddedCanvas696 = fRootEmbeddedCanvas696->GetCanvasWindowId();
   this->c1 = new TCanvas("c1", 10, 10, wfRootEmbeddedCanvas696);
   fRootEmbeddedCanvas696->AdoptCanvas(this->c1);
   frmMain->AddFrame(fRootEmbeddedCanvas696, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fRootEmbeddedCanvas696->MoveResize(0,8,800,600);
   TGLabel *lblNrPosSamples = new TGLabel(frmMain,"Number of Samples");
   lblNrPosSamples->SetTextJustify(36);
   lblNrPosSamples->SetMargins(0,0,0,0);
   lblNrPosSamples->SetWrapLength(-1);
   frmMain->AddFrame(lblNrPosSamples, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   lblNrPosSamples->MoveResize(816,20,200,16);

   ULong_t ucolor;        // will reflect user color changes
   gClient->GetColorByName("#ffffff",ucolor);

   // combo box Scale/Range
   cbScale = new TGComboBox(frmMain,-1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   cbScale->SetName("cbScale");
   cbScale->AddEntry("100 mV ",0);
   cbScale->AddEntry("1 V ",1);
   cbScale->AddEntry("10 V ",2);
   cbScale->Resize(176,20);
   cbScale->Select(1);
   frmMain->AddFrame(cbScale, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   cbScale->MoveResize(808,128,176,20);
   TGLabel *lblScale = new TGLabel(frmMain,"ADC voltage scale ");
   lblScale->SetTextJustify(36);
   lblScale->SetMargins(0,0,0,0);
   lblScale->SetWrapLength(-1);
   frmMain->AddFrame(lblScale, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   lblScale->MoveResize(816,104,160,16);
   
    // combo box Trigger mode
   cbTrigger = new TGComboBox(frmMain,-1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   cbTrigger->SetName("cbTrigger");
   cbTrigger->AddEntry("Internal ",0);
   cbTrigger->AddEntry("External ",1);
   cbTrigger->Resize(176,20);
   cbTrigger->Select(0);
   frmMain->AddFrame(cbTrigger, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   cbTrigger->MoveResize(808,178,176,20);
   TGLabel *lblTrigger = new TGLabel(frmMain,"Trigger mode ");
   lblTrigger->SetTextJustify(36);
   lblTrigger->SetMargins(0,0,0,0);
   lblTrigger->SetWrapLength(-1);
   frmMain->AddFrame(lblTrigger, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   lblTrigger->MoveResize(816,154,160,16);
   
   
   
   
   TGLabel *lblNrPreSamples = new TGLabel(frmMain,"Pre");
   lblNrPreSamples->SetTextJustify(36);
   lblNrPreSamples->SetMargins(0,0,0,0);
   lblNrPreSamples->SetWrapLength(-1);
   frmMain->AddFrame(lblNrPreSamples, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   lblNrPreSamples->MoveResize(808,46,50,20);
   
   neNrPreSamples = new TGNumberEntry(frmMain, (Double_t) 0,25,-1,(TGNumberFormat::EStyle) 5);
   neNrPreSamples->SetName("neNrPreSamples");
   frmMain->AddFrame(neNrPreSamples, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   //this->neNrPreSamples->MoveResize(808,46,168,20);
   this->neNrPreSamples->MoveResize(858,46,118,20);
   this->neNrPreSamples->Connect("ValueSet(Long_t)","AdcGui",this, "SetNrPreSamples()");
   
   
   TGLabel *lblNrPosSamples = new TGLabel(frmMain,"Pos");
   lblNrPosSamples->SetTextJustify(36);
   lblNrPosSamples->SetMargins(0,0,0,0);
   lblNrPosSamples->SetWrapLength(-1);
   frmMain->AddFrame(lblNrPosSamples, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   lblNrPosSamples->MoveResize(808,76,50,20);
   
   neNrPosSamples = new TGNumberEntry(frmMain, (Double_t) 0,25,-1,(TGNumberFormat::EStyle) 5);
   neNrPosSamples->SetName("neNrPosSamples");
   frmMain->AddFrame(neNrPosSamples, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   this->neNrPosSamples->MoveResize(858,76,118,20);
   
   this->neNrPreSamples->SetFormat( TGNumberFormat::kNESInteger);
   //this->neNrPreSamples->SetLimitValues(0,20000);
   cout << this->neNrPreSamples->GetNumMin() << endl;
   
   this->neNrPosSamples->SetFormat( TGNumberFormat::kNESInteger);
   //this->neNrPosSamples->SetLimitValues(0,20000);
   
   // status bar
   statBar = new TGStatusBar(frmMain);
   statBar->SetName("statBar");
   frmMain->AddFrame(statBar, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   statBar->MoveResize(0,610,1000,20);
   statBar->SetText("Idle");
   
   //(0,8,800,600);
   
   
   this->neNrPreSamples->Connect("ValueSet(Long_t)","AdcGui",this, "SetNrPreSamples()");
   this->neNrPosSamples->Connect("ValueSet(Long_t)","AdcGui",this, "SetNrPosSamples()");
   
   
   
   
   
   
   
   
   
   TGLabel *lblThreshold = new TGLabel(frmMain,"Trigger threshold value");
   lblThreshold->SetTextJustify(36);
   lblThreshold->SetMargins(0,0,0,0);
   lblThreshold->SetWrapLength(-1);
   frmMain->AddFrame(lblThreshold, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   lblThreshold->MoveResize(816,300,160,16);
   
   neThreshold = new TGNumberEntry(frmMain, (Double_t) 0,25,-1,(TGNumberFormat::EStyle) 5);
   neThreshold->SetName("neThreshold");
   frmMain->AddFrame(neThreshold, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   this->neThreshold->MoveResize(808,320,160,16);
   neThreshold->SetNumber(-5000);


   
   btnStartStop = new TGTextButton(frmMain,"Start Acquisitions");
   btnStartStop->SetTextJustify(36);
   btnStartStop->SetMargins(0,0,0,0);
   btnStartStop->SetWrapLength(-1);
   btnStartStop->Resize(104,24);
   frmMain->AddFrame(btnStartStop, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   btnStartStop->MoveResize(808,550,168,20);

   btnStartStop->Connect("Clicked()", "AdcGui", this, "ToggleGraphState()");
   


   frmMain->SetMWMHints(kMWMDecorAll,
                        kMWMFuncAll,
                        kMWMInputModeless);
   frmMain->MapSubwindows();

   frmMain->Resize(frmMain->GetDefaultSize());
   frmMain->MapWindow();
   frmMain->Resize(1003,631);
   
   //this->Connect("UpdateGraph()","AdcGui",this,"UpdateGraph()");

   this->NrPreSamples = 100;
   this->NrPosSamples = 100;
   
   neNrPreSamples->SetNumber(this->NrPreSamples);
   neNrPosSamples->SetNumber(this->NrPosSamples);

	timer = new TTimer();//( 500, kTRUE);	
	timer->Connect("Timeout()", "AdcGui",
		       this, "UpdateGraph()");
	//timer->Start(500, kTRUE);   // 0.5 seconds single-shot
   	//timer->Start(0, kFALSE);
	//timer->Start(500, kFALSE);
   
   
   
   
   
   this->CreateGraph();
   
}  

void AdcGui::SetNrPosSamples(){ this->NrPosSamples = this->neNrPosSamples->GetNumberEntry()->GetIntNumber();}
void AdcGui::SetNrPreSamples(){ this->NrPreSamples = this->neNrPreSamples->GetNumberEntry()->GetIntNumber();}


void AdcGui::CreateGraph(){
	this->c1->SetFillColor(42);
	this->c1->SetGrid();
	//this->c1->Divide(1,2);
	this->c1->Divide(1,1);
	this->c1->cd(2)->SetLogy(1);
	
}

void AdcGui::ToggleGraphState(){
	if (this->Running==false){ 
	  this->Running=true;
          timer->Start(100, kFALSE);
	  btnStartStop->SetText("Stop Acquisitions");
	  statBar->SetText("Acquiring Data");
	cout << "this->Running=true;" << endl;
	  //Emit("UpdateGraph()", NULL);
	}
	else{
	  this->Running=false;
          timer->Stop();
	cout << "this->Running=false;" << endl;
	  btnStartStop->SetText("Start Acquisitions");
	  statBar->SetText("Idle");
	  if (this->WaitingTrigger== true){
	 	this->WaitingTrigger= false;
		gSystem->Exec("pkill acq-program");
	  }
	}
}




void AdcGui::UpdateGraph(){ 
	  const Int_t n = 20;
	  Double_t x[n], y[n];
	  //10 for -5 to +5V, 1 to -500mV to + 500 mV, 100 to -50 mv to + 50 mV
	  int scale = 10;
	  char Command[300];
      
      TGraph *gr;
      if  (this->Running==true){
	  //sleeptime_t st_atime_prev = 0;(1);ToggleGraphState()

	  //system("program args 1>/tmp/program.stdout 2>/tmp/program.stderr as in
	  //http://docstore.mik.ua/orelly/perl/cookbook/ch16_08.htm
	  //sprintf(Command,"%s/fald-acq -a 1000 -b 0 -n 1 -l 1 -g 1 -e -r 10 0100 1> %s 2> %s",gSystem->pwd(), FileName,ErrFile );
	  //sprintf(Command,"./fald-acq -a 100 -b 0 -n 1 -l 1 -g 1 -c 1 -t 10000 -r 10 0100 > %s",FileName);
	  //sprintf(Command,"%s/fald-acq -a 1000 -b 0 -n 1 -l 1 -g 1 -e -r 10 0100",gSystem->pwd());
	  //gSystem->Exec(Command);


	  //sprintf(Command,"source %s/continuous_exec_script.sh",gSystem->pwd() );

	  ///sprintf(Command,"./fald-simple-acq -a 100 -b 100 -n 1 -t 0 -c 0 0x100 > /tmp/fmcadc.0x0100.ch1.dat.INT");
	  int scale=0;
	  
	  switch(this->cbScale->GetSelected()){
	    case 0:
		scale = 100;
		break;
	    case 1:
	      scale = 1;
	      break;
	    default:
	      scale = 10;
	      break;
	  }
	  

	  char trigger_options[15];
	  switch (this->cbTrigger->GetSelected()){
	    case 0:
		sprintf(trigger_options,"-c 1 -t %ld",this->neThreshold->GetIntNumber());
		break;
	    default:
		sprintf(trigger_options,"-e");
		break;
	  }
	  
	  /// 1> Data_output_file.dat 2> Error_file.txt
	  sprintf(Command,"./acq-program -a %ld -b %ld  -n 1 %s -r %d --negative-edge 0x200 " \
	  "1> /dev/shm/fmcadc.0x0200.dat 2> /dev/shm/fmcadc.err.txt ; echo done ", \
	  this->NrPosSamples,this->NrPreSamples,trigger_options,scale);

	///sprintf(Command,"acq-program -a %ld -b %ld  -n 1 -r 1 -c 1 -t -5000  --negative-edge 0x200 " \
	  "1> /dev/shm/fmcadc.0x0200.dat 2> /dev/shm/fmcadc.err.txt ", \
	  this->NrPosSamples,this->NrPreSamples);
	if (this->WaitingTrigger==false){
		char isItDone[10];
		int ret = 0;
                //puts("OpenPipe");
		test = gSystem->OpenPipe(Command,"r"); 
                //puts("Pipe opened");
	}
	else{
	}
        
        struct stat st; 
        
        int ret =stat("/dev/shm/fmcadc.0x0200.dat", &st);


        
        //puts("Check if done");
	//if (strlen(isItDone)>0){
        if (ret==0 && st.st_size>0){
                //puts("Yes, Done");
		this->WaitingTrigger= false;
		gSystem->ClosePipe(test); /// BLOCKS HERE
		gr = new TGraph("/dev/shm/fmcadc.0x0200.dat", "%lg %lg %*lg %*lg %*lg");
		  // Signal 
		  (gr->GetYaxis())->SetRangeUser(-33000,33000);
		  // Noise floor
		  //(gr->GetYaxis())->SetRangeUser(-150,30);		
		  //
		  //TH1D *hfft = new TH1D("hfft","hfft",2000,0,2e-6);
		  //for(Int_t ibin = 0; ibin<2000; ibin++) {
		  //  hfft->SetBinContent(ibin+1, 
		  //
		  //gSystem->Sleep(100);

		  gr->SetLineColor(2);
		  gr->SetLineWidth(4);
		  gr->SetMarkerColor(4);
		  gr->SetMarkerStyle(21);
		  //cout <<"a simple graph" <<endl;
		  gr->SetTitle("Vxt");
		  gr->GetXaxis()->SetTitle("time sample");
		  gr->GetYaxis()->SetTitle("ADC reading");
		  //cout <<"3" <<endl;
		  
		  this->c1->cd(1);

		  gr->Draw("ACP");
		  
		  int N_Samples = this->NrPosSamples + this->NrPreSamples;
	}
	else{
		//gSystem->Sleep(100);
                //puts("Not done");
                //gSystem->ProcessEvents();
		this->WaitingTrigger= true;
	}
	  

/*
	  delete hfft;
	  hfft = new TH1D("hfft","hfft",N_Samples,0,1e-6);
	  
	  Int_t tmp_i_0,tmp_i_1,tmp_i_2,tmp_i_3,tmp_i_4;
	  char str0[50],str1[50],str2[50],str3[50],str4[50];
	  Double_t tmp,mult;
	  if (scale=1) mult=0.5;
	  if (scale=10) mult=5.0;
	  if (scale=100) mult=0.05;
	  
	   FILE *filin = fopen("/dev/shm/fmcadc.0x0200.dat","r");
	   
	  for(Int_t ibin = 0; ibin<N_Samples; ibin++) {
	    //fscanf(filin,"%s %s %s %s %s\n",str0,str1,str2,str3,str4);
	    //cout<< str0 << " " << str1 << " "<< str2 << " "<< str3 << " " << str4 << endl;
	      fscanf(filin,"%*s %d %*s %*s %*s\n",&tmp_i_0);//,&tmp_i_1,&tmp_i_2,&tmp_i_3,&tmp_i_4);
	      //cout << "tmp_i " << tmp_i_0<<" "<< tmp_i_1<<" "<< tmp_i_2<<" "<< tmp_i_3<<" "<< tmp_i_4 << endl;
	      tmp=((double)tmp_i_0/(pow(2,16-2)))*scale;
	      //cout << "tmp" << tmp << endl;
	      hfft->SetBinContent(ibin+1,12*tmp);
	  }
	  fclose(filin);
	  //Compute the transform and look at the magnitude of the output
	  //hm =0;


	  delete hm;
	  hm =0;
	  TVirtualFFT::SetTransform(0);
	  
	  hm = hfft->FFT(hm, "MAG");
	  hm->SetTitle("Magnitude of the 1st transform");
	  hm->GetXaxis()->SetRangeUser(0,100);
	  hm->SetLineWidth(2);
	  
	  c1->cd(2);
	  hm->Draw();
	  c1->cd(2)->SetLogy(1);
	  
	  */
	  
	  

	  //gSystem->ProcessEvents();
	  // TCanvas::Update() draws the frame, after which one can change it
	
	  this->c1->Update();
	  this->c1->GetFrame()->SetFillColor(21);
	  this->c1->GetFrame()->SetBorderSize(12);
	  this->c1->Modified();
      }
      gSystem->ProcessEvents();
}
