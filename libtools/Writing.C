#include <fstream>
#include <iostream>
using namespace std;

void Writing(int i)
{
  ofstream myfile;
  myfile.open ("Data.txt", std::ofstream::app);

  TRandom * RANDOM = new TRandom2(0);
  
  // write inputted data into the file.
  char Line[50];
  sprintf(Line, "%.1f %.1f",i, RANDOM->Gaus(50,10));
//  cout<<Line<<endl;
  
  myfile << Line<< endl;

  myfile.close();
  
  return;
}
