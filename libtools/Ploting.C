#include "TCanvas.h"
#include <string.h>
#include <unistd.h>

void Ploting()
{
	TCanvas *c1 = new TCanvas("c1", "Jittering Histograms", 800, 600);
	TCanvas *c2 = new TCanvas("c2", "Jittering Measurements", 800, 600);
	TH1F *h1 = new TH1F("h1", "Jittering Histogram",100, 832, 834 );

	int loop = 0;

	for(int i = 0; i < 5000; i++)
	{
		char Command[100];
		//Need to be fixed to use the driver example program we adapted
		sprintf(Command,"./fald-acq -a 400000 -b 0 -n 1 -l 1 -g 1 -c 1 -t 10 0100 > Data.txt");
	    	gSystem->Exec(Command);
		cout << "Filename Data.txt, loop i = " << i << endl;

	 	char FileName[50] = "Data.txt";
		
		cout << "TGraph *G1 = new TGraph(FileName), loop i = " << i << endl;
		//See the columns configuration in FileName
		TGraph *G1 = new TGraph(FileName, "%lg %lg %*lg %*lg %*lg");
		TGraph *G2 = new TGraph();
		
		cout << "c2->cd(), loop i = " << i << endl;
		
		c2->cd();
		
		cout << "G1->Draw, loop i = " << i << endl;
		
		G1->Draw("AC*");
		
		cout << "c2->Update(), loop i = " << i << endl;
		
		//c2->Update();
		
		cout << "int N, loop i = " << i << endl;
		
		int N = G1->GetN();
		double x = (G1->GetX())[N];
		double y = (G1->GetY())[N];
		G2->DrawGraphs(N,G1->GetX(),G1->GetY(),"");

	//Getting the signals
		double *X = G1->GetY();

	//Getting the Deltas TICKS
		double x0 = 0 ;
		double x1 = 0;
		double Threshold = -100;
		for(int j = 1; j < N; j++)
		{
			if( (X[j-1] > Threshold)&&(X[j] < Threshold)&&(j>x0) )
			{
				x1 = j;
				if(x0 > 0)
				{
					double Increase1 = X[x1] - X[x1-1];
					double Fraction1 = Threshold - X[x1-1];
					double Increase0 = X[x0] - X[x0-1];
					double Fraction0 = Threshold - X[x0-1];
					double Delta = (x1 + Fraction1/Increase1) - (x0 + Fraction0/Increase0);
					h1->Fill(Delta);
				}
				x0 = x1;
			}
		}
		c1->cd();
	
		TF1 *f1 = new TF1("f1", "gaus", 832, 834);
		h1->Fit("f1", "QR");
		f1->SetLineColor(kRed);
		h1->Draw();
		f1->Draw("same");
		c1->Update();
		//	G1->GetPoint(N, x, y);
		//	cout<<"N = "<<N<<" ,x = "<<x<<", y = "<<y<<endl;

		  
		/*TGraph(const char* filename, const char* format = "%lg %lg", Option_t* option = "")
		 
		 Graph constructor reading input from filename.
		 filename is assumed to contain at least two columns of numbers.
		 the string format is by default "%lg %lg".
		 this is a standard c formatting for scanf. If columns of numbers should be
		 skipped, a "%*lg" or "%*s" for each column can be added,
		 e.g. "%lg %*lg %lg" would read x-values from the first and y-values from
		 the third column.
		 For files separated by a specific delimiter different from ' ' and '\t' (e.g. ';' in csv files)
		 you can avoid using %*s to bypass this delimiter by explicitly specify the "option" argument,
		 e.g. option=" \t,;" for columns of figures separated by any of these characters (' ', '\t', ',', ';')
		 used once (e.g. "1;1") or in a combined way (" 1;,;;  1").
		 Note in that case, the instanciation is about 2 times slower.
		*/
		cout<<"Loop number "<<loop<<endl;
		//usleep(1*1000);
		loop+=1;
		if(loop==49) 
		{
			cout<<"Keep acquisition? (y/n)"<<endl;
			string Resp;
			cin>>Resp;
			
			if( Resp.compare("y")!=0)
			{
				cout<< "bye " <<Resp << endl;
				return;
			}
			else{		
				loop = 0;				
			}
		}
		delete G1;
		delete G2;
		
	}
	delete c1;
	delete c2;
	return;
}
