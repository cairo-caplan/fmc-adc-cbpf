void fft()
{

  TCanvas *c = new TCanvas("c","c");
  c->SetLogy(1);
  c->Divide(1,2);
  FILE *filin = fopen("./fmcadc.0x0100.ch1.dat","r");
  FILE *filin2 = fopen("./fmcadc.0x0200.ch1.dat","r");
  //  TH1 *h:// = new TH1("h","h",1000,1500,5500);
  TH1D *hfft = new TH1D("hfft","hfft",1000,0,1e-6);
  TH1D *hfft2 = new TH1D("hfft2","hfft2",2000,0,2e-6);
  Double_t tmp, tmp2;
  for(Int_t ibin = 0; ibin<2000; ibin++) {
  if(ibin<1000) {
    fscanf(filin,"%*d %lf",&tmp);
    hfft->SetBinContent(ibin+1,12*tmp);
  }
    fscanf(filin2,"%*d %lf",&tmp2);
    hfft2->SetBinContent(ibin+1,tmp2);
  }
  //Compute the transform and look at the magnitude of the output
  TH1 *hm =0;
  TH1 *hm2 =0;
  TVirtualFFT::SetTransform(0);
  hm = hfft->FFT(hm, "MAG");
  hm->SetTitle("Magnitude of the 1st transform");
  hm->GetXaxis()->SetRangeUser(0,100);
  hm->SetLineWidth(2);
  c->cd(1);
  hfft->Draw();
  hfft2->SetLineColor(2);
  hfft->GetYaxis()->SetRangeUser(-1.2,1.2);
  hfft2->Draw("same");
  //  c->cd(1)->SetLogy(1);
  c->cd(2);
  hm->Draw();
  c->cd(2)->SetLogy(1);
  hm2 = hfft2->FFT(hm2, "MAG");
  hm2->SetTitle("Magnitude of the 1st transform");
  hm2->GetXaxis()->SetRangeUser(0,100);
  hm2->SetLineWidth(2);
  hm2->SetLineColor(2);
  hm2->Draw("same");
}