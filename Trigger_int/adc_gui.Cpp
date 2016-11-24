// Mainframe macro generated from application: /opt/root/bin/root.exe
// By ROOT version 5.34/24 on 2015-01-23 16:09:32


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>


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
#ifndef ROOT_TGuiBldHintsEditor
#include "TGuiBldHintsEditor.h"
#endif
#ifndef ROOT_TGuiBldNameFrame
#include "TGuiBldNameFrame.h"
#endif
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
#ifndef ROOT_TGuiBldEditor
#include "TGuiBldEditor.h"
#endif
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
#ifndef ROOT_TRootGuiBuilder
#include "TRootGuiBuilder.h"
#endif
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
#ifndef ROOT_TGuiBldGeometryFrame
#include "TGuiBldGeometryFrame.h"
#endif
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
#ifndef ROOT_TGuiBldDragManager
#include "TGuiBldDragManager.h"
#endif

#include "TGNumberEntry.h"
#include "RQ_OBJECT.h"

#include "Riostream.h"


class AdcGui{
  RQ_OBJECT("AdcGui")
  
private:
  void BuildGui();
  bool Running;
  void DoAcquisition();
  void CreateGraph();
  void UpdateGraph();
  void ToggleGraphState();
  TCanvas *c1;
  
  bool newAcqAvailable;
  
//
  long NrSamples;
public:
   TGNumberEntry *neNrSamples;
  void SetNrSamples();
  void Gui();
  void ~Gui();
};

void adc_gui(){
  AdcGui *app = new AdcGui();
}


void AdcGui::AdcGui()
{
    this->Running = false;
    this->BuildGui();
}



void AdcGui::BuildGui()
{

   // main frame
   TGMainFrame *fMainFrame1308 = new TGMainFrame(gClient->GetRoot(),10,10,kMainFrame | kVerticalFrame);
   fMainFrame1308->SetName("fMainFrame1308");
   fMainFrame1308->SetLayoutBroken(kTRUE);

   // embedded canvas
   TRootEmbeddedCanvas *fRootEmbeddedCanvas696 = new TRootEmbeddedCanvas(0,fMainFrame1308,800,600);
   fRootEmbeddedCanvas696->SetName("fRootEmbeddedCanvas696");
   Int_t wfRootEmbeddedCanvas696 = fRootEmbeddedCanvas696->GetCanvasWindowId();
   this->c1 = new TCanvas("c1", 10, 10, wfRootEmbeddedCanvas696);
   fRootEmbeddedCanvas696->AdoptCanvas(this->c1);
   fMainFrame1308->AddFrame(fRootEmbeddedCanvas696, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fRootEmbeddedCanvas696->MoveResize(0,8,800,600);
   TGLabel *lblNrSamples = new TGLabel(fMainFrame1308,"Number of Samples");
   lblNrSamples->SetTextJustify(36);
   lblNrSamples->SetMargins(0,0,0,0);
   lblNrSamples->SetWrapLength(-1);
   fMainFrame1308->AddFrame(lblNrSamples, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   lblNrSamples->MoveResize(816,20,200,16);

   ULong_t ucolor;        // will reflect user color changes
   gClient->GetColorByName("#ffffff",ucolor);

   // combo box
   TGComboBox *cbScale = new TGComboBox(fMainFrame1308,-1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   cbScale->SetName("cbScale");
   cbScale->AddEntry("Entry 1 ",0);
   cbScale->AddEntry("Entry 2 ",1);
   cbScale->AddEntry("Entry 3 ",2);
   cbScale->AddEntry("Entry 4 ",3);
   cbScale->AddEntry("Entry 5 ",4);
   cbScale->AddEntry("Entry 6 ",5);
   cbScale->AddEntry("Entry 7 ",6);
   cbScale->Resize(176,20);
   cbScale->Select(0);
   fMainFrame1308->AddFrame(cbScale, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   cbScale->MoveResize(808,128,176,20);
   TGLabel *lblScale = new TGLabel(fMainFrame1308,"ADC voltage scale ");
   lblScale->SetTextJustify(36);
   lblScale->SetMargins(0,0,0,0);
   lblScale->SetWrapLength(-1);
   fMainFrame1308->AddFrame(lblScale, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   lblScale->MoveResize(816,104,160,16);
   neNrSamples = new TGNumberEntry(fMainFrame1308, (Double_t) 0,25,-1,(TGNumberFormat::EStyle) 5);
   neNrSamples->SetName("neNrSamples");
   fMainFrame1308->AddFrame(neNrSamples, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   this->neNrSamples->MoveResize(808,56,168,20);
   this->neNrSamples->Connect("ValueSet(Long_t)","AdcGui",this, "SetNrSamples()");


   TGTextButton *btnStartStop;
   btnStartStop = new TGTextButton(fMainFrame1308,"Start Acquisitions");
   btnStartStop->SetTextJustify(36);
   btnStartStop->SetMargins(0,0,0,0);
   btnStartStop->SetWrapLength(-1);
   btnStartStop->Resize(104,24);
   fMainFrame1308->AddFrame(btnStartStop, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   btnStartStop->MoveResize(808,200,168,20);

   btnStartStop->Connect("Clicked()", "AdcGui", this, "ToggleGraphState()");
   


   fMainFrame1308->SetMWMHints(kMWMDecorAll,
                        kMWMFuncAll,
                        kMWMInputModeless);
   fMainFrame1308->MapSubwindows();

   fMainFrame1308->Resize(fMainFrame1308->GetDefaultSize());
   fMainFrame1308->MapWindow();
   fMainFrame1308->Resize(1003,631);
   
   this->Connect("UpdateGraph()","AdcGui",this,"UpdateGraph()");
   this->Connect("DoAcquisition()","AdcGui",this,"DoAcquisition()");

   this->NrSamples = 100;
   neNrSamples->SetNumber(this->NrSamples);
   this->CreateGraph();
   
}  

void AdcGui::SetNrSamples(){
   this->NrSamples = this->neNrSamples->GetNumberEntry()->GetIntNumber();
}

void AdcGui::CreateGraph(){
	this->newAcqAvailable = false;
	//Emit("CreateGraph()", NULL);
	Emit("DoAcquisition()", NULL);
}

void AdcGui::ToggleGraphState(){
	if (this->Running==false) this->Running=true;
	else this->Running=false;
}

void AdcGui::DoAcquisition(){
	while (1){
	      if ((this->Running==true) && (this->newAcqAvailable = false)){
		    puts("Child process");
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

		    ///sprintf(Command,"./fald-simple-acq -a 100 -b 100 -n 1 -t 0 -c 0 0x100 > /tmp/fmcadc.0x0100.ch1.dat.INT");

		    //sprintf(Command,"./fald-simple-acq_test -a %ld -b 100 -n 1 -t 0 -r 1 -c 1 0x100 1> /tmp/fmcadc.0x0100.ch1.dat.INT 2> /tmp/err.txt ",this->NrSamples);
		    sprintf(Command,"./fald-simple-acq_test -a %ld -b 100 -n 1 -t 0 -r 1 -e 0x100 1> /tmp/fmcadc.0x0100.ch1.dat.INT 2> /tmp/err.txt ",this->NrSamples);

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
		    this->newAcqAvailable = true;
		    Emit("UpdateGraph()",NULL);
	      }
	      gSystem->ProcessEvents();
	}
  
}


void AdcGui::UpdateGraph(){ 
      
      TGraph *gr;
      //while (1){
	    if (this->newAcqAvailable = true){
	    //TCanvas *this->c1 = new TCanvas("this->c1","A Simple Graph Example",200,10,700,500);

	    this->c1->SetFillColor(42);
	    this->c1->SetGrid();

	    const Int_t n = 20;
	    Double_t x[n], y[n];
	    //10 for -5 to +5V, 1 to -500mV to + 500 mV, 100 to -50 mv to + 50 mV
	    int scale = 10;
	    char FileName[200];
	    char ErrFile[200];
	    char Command[300];
	    
	    //Parent process
	    puts("Parent process");
	    
	    //gSystem->Sleep(2000);

	    gr = new TGraph("/tmp/fmcadc.0x0100.ch1.dat.INT", "%lg %lg %*lg %*lg %*lg");
	    // Signal 
	    (gr->GetYaxis())->SetRangeUser(-32768,32768);
	    // Noise floor
	    //(gr->GetYaxis())->SetRangeUser(-150,30);		
	    /*
	    TH1D *hfft = new TH1D("hfft","hfft",2000,0,2e-6);
	    for(Int_t ibin = 0; ibin<2000; ibin++) {
	      hfft->SetBinContent(ibin+1, 
	    */
	    //gSystem->Sleep(100);

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

	    //gSystem->ProcessEvents();
	    //cout << "Draw" << endl;
	    //cout <<"4" <<endl;
	    // TCanvas::Update() draws the frame, after which one can change it
	    this->c1->Update();
	    this->c1->GetFrame()->SetFillColor(21);
	    this->c1->GetFrame()->SetBorderSize(12);
	    this->c1->Modified();
	    //cout <<"5" <<endl;
	    gSystem->ProcessEvents();
	    this->newAcqAvailable = false;
      //}
	
      //gSystem->ProcessEvents();
			     
    
}
