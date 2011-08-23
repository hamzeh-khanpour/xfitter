#include <stdlib.h>
#include <iostream>
#include <TSystem.h>
#include <TString.h>
#include "TFile.h"
#include "TGraphAsymmErrors.h"
#include "PdfTable.h"
#include <getopt.h>
#include "TAxis.h"
using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char **argv) {

  TString OutputPath("output/");

  int c; 
  Int_t nBands = 0;

  while (1)
    {
      static struct option long_options[] =
	{
	  {"bands",  required_argument, 0, 'b'},
	};
      /* getopt_long stores the option index here. */
      int option_index = 0;
      
      c = getopt_long (argc, argv, "bands:",
		       long_options, &option_index);
      /* Detect the end of the options. */
      if (c == -1)
	break;
      switch (c) 
	{
	case 'b':
	  nBands = atoi(optarg); 
	  break;
	case '?':
	  printf("program usage: StoreGraphs --bands=n [dir] \n");
	  exit(1);
	  
	}

    }
  
  printf ("NBands=%i\n",nBands);
 
  if ( optind < argc) {

    printf ("%i %i %s\n",optind, argc, argv[optind]);
    if(argc-optind == 1 ) {
      OutputPath.Form(argv[optind]);
    }
  }

  // Create graphs over all PDFs 
  TFile output(OutputPath+"/Graphs.root","RECREATE");
  for (Int_t iq2=0; iq2<20; iq2++) {
    TString filename("");
    filename.Form("%s/pdfs_q2val_%02d.txt",OutputPath.Data(), iq2+1);

    PdfTable* table = (nBands == 0 )? new PdfTable(filename.Data()) : new PdfErrorTables(OutputPath.Data(),iq2+1,nBands);

    if (table->GetNx() == 0) {
      break;
    }    
    Int_t npdf  = table->GetNPdfs();
    Double_t Q2 = table->GetQ2();

    cout << "Q2="<<Q2<<" npdf="<<npdf<<endl;
    
    for (Int_t i=1; i<npdf; i++) {
      string name = table->GetColumnName(i);
      TGraph *graph = table->GetPDFGraph(name);
      TString GraphName;
      GraphName.Form("%s_vs_x_for_Q^{2}=%g",name.c_str(),Q2);
      TString GraphTitle;
      GraphTitle.Form("%s vs x for Q^{2}=%g GeV^{2}",name.c_str(),Q2);

      graph->SetName(GraphName);
      graph->SetTitle(GraphTitle);     

      graph->GetXaxis()->SetTitle("x");
      graph->GetYaxis()->SetTitle(name.c_str());

      graph->Write();
    }

  }
  output.Write();
  output.Close();


  return 0;
}
