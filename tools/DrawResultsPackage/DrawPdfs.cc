#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <TError.h>
#include <TCanvas.h>
#include <TFile.h>

#include <Output.h>
#include <PdfsPainter.h>
#include <CommandParser.h>
#include <DataPainter.h>

using namespace std;

int main(int argc, char **argv) 
{
  //parse command line arguments
  opts = CommandParser(argc, argv);

  gErrorIgnoreLevel=1001;

  //read output directory
  vector <Output*> info_output;
  for (vector<string>::iterator it = opts.dirs.begin(); it != opts.dirs.end(); it++)
    {
      Output* out = new Output((*it).c_str());
      if (out != 0)
      	info_output.push_back(out);
    }

  //--------------------------------------------------
  //Pdf plots
  typedef map <int, vector<gstruct> > pdfmap;

  map <float, pdfmap> q2list;
  vector<string>::iterator itn = opts.labels.begin();
  for (unsigned int o = 0; o < info_output.size(); o++)
    {
      //check if errors are symmetric or asymmetric hessian
      string option = "";
      TString filename("");
	
      //asymmetric hessian
      filename.Form("%s/pdfs_q2val_s%02dm_%02d.txt", info_output[o]->GetName()->Data(), 1, 1);
      ifstream asfile(filename.Data());
      if (asfile.is_open())
	{
	  if (opts.asymbands)
	    option = "a";
	  else
	    option = "b";
	  asfile.close();
	}

      //symmetric hessian errors
      filename.Form("%s/pdfs_q2val_s%02ds_%02d.txt",info_output[o]->GetName()->Data(), 1, 1);
      ifstream sfile(filename.Data());
      if (sfile.is_open())
	{
	  option = "s";
	  sfile.close();
	}

      //MC errors
      filename.Form("%s/pdfs_q2val_mc%03ds_%02d.txt",info_output[o]->GetName()->Data(), 1, 1);
      ifstream mcfile(filename.Data());
      if (mcfile.is_open())
	{
	  option = "mc";
	  mcfile.close();
	}

      info_output[o]->Prepare(opts.dobands, option);
      //loop on Q2 bins
      for (int nq2 = 0; nq2 < (info_output[o]->GetNQ2Files()); nq2++)
	{
	  pdfmap pmap;
	  if (q2list.size() > nq2)
	    pmap = q2list[info_output[o]->GetQ2Value(nq2)];

	  //loop on pdf types
	  for (unsigned int ipdf = 0; ipdf < pdflabels.size(); ipdf++)
	    {
	      //	      TGraphAsymmErrors* pdf = info_output[o]->GetPdf((Output::pdf)ipdf, nq2);
	      gstruct gs;
	      gs.graph = info_output[o]->GetPdf((Output::pdf)ipdf, nq2);
	      gs.label = (*itn);
	      pmap[ipdf].push_back(gs);
	    }
	  q2list[info_output[o]->GetQ2Value(nq2)] = pmap;
	}
      itn++;
    }

  vector <TCanvas*> pdfscanvaslist, pdfscanvasratiolist;
  for (map <float, pdfmap>::iterator qit = q2list.begin(); qit != q2list.end(); qit++)
    for (pdfmap::iterator pdfit = (*qit).second.begin(); pdfit != (*qit).second.end(); pdfit++)
      {
	pdfscanvaslist.push_back(PdfsPainter((*qit).first, (*pdfit).first, (*pdfit).second));
	pdfscanvasratiolist.push_back(PdfsRatioPainter((*qit).first, (*pdfit).first, (*pdfit).second));
      }


  //--------------------------------------------------
  //Data pulls plots
  map <int, vector<dataseth> > datamap;
  itn = opts.labels.begin(); //vector<string>::iterator 
  for (unsigned int o = 0; o < info_output.size(); o++)
    {
      for (unsigned int d = 0; d < info_output[o]->GetNsets(); d++)
	{
	  //if (info_output[o]->GetSet(d)->GetNSubPlots() == 1)
	  for (unsigned int p = 0; p < info_output[o]->GetSet(d)->GetNSubPlots(); p++)
	    {
	      info_output[o]->GetSet(d)->GetHistogram(p, false);
	      //	      int id = info_output[o]->GetSet(d)->GetSetId();
	      int id = info_output[o]->GetSet(d)->GetSetId() * 100 + p;
	      //check bins sanity
	      vector <float> b1 = info_output[o]->GetSet(d)->getbins1(p);
	      vector <float> b2 = info_output[o]->GetSet(d)->getbins2(p);
	      vector<float>::iterator it1 = b1.begin();
	      vector<float>::iterator it2 = b2.begin();
	      if (b1.size() < 1)
		{
		  cout << "zero bins for dataset: " << info_output[o]->GetSet(d)->GetName() << " Subplot " << p <<  endl;
		  cout << "Cannot plot data pulls, skipping" << endl;
		  continue;
		}
	      bool skip = false;
	      for (; (it1+1) != b1.end(); it1++, it2++)
		if (*(it1+1) < *it2 || *it1 >= *(it1+1))
		  skip = true;
	      if (skip)
		{
		  cout << "bin inconsistency for dataset: " << info_output[o]->GetSet(d)->GetName() << " Subplot " << p << endl;
		  cout << "Cannot plot data pulls, skipping" << endl;
		  continue;
		}
	      string dtname = (string)info_output[o]->GetSet(d)->GetName();
	      if (p > 0)
		{
		  char nump[5];
		  sprintf(nump, "%d", p);
		  dtname = dtname + " - Subplot " + nump;
		}
	      dataseth dt = dataseth(dtname,
				     //				     info_output[o]->GetSet(d)->GetName(),
				     info_output[o]->GetName()->Data(),
				     (*itn),
				     info_output[o]->GetSet(d)->getbins1(p),
				     info_output[o]->GetSet(d)->getbins2(p),
				     info_output[o]->GetSet(d)->getdata(p),
				     info_output[o]->GetSet(d)->getuncor(p),
				     info_output[o]->GetSet(d)->gettoterr(p),
				     info_output[o]->GetSet(d)->gettheory(p),
				     info_output[o]->GetSet(d)->gettheoryshifted(p),
				     info_output[o]->GetSet(d)->gettherrup(p),
				     info_output[o]->GetSet(d)->gettherrdown(p),
				     info_output[o]->GetSet(d)->getpulls(p),
				     info_output[o]->GetSet(d)->GetXlog(p),
				     info_output[o]->GetSet(d)->GetYlog(p),
				     info_output[o]->GetSet(d)->GetXmin(p),
				     info_output[o]->GetSet(d)->GetXmax(p),
				     info_output[o]->GetSet(d)->GetXTitle(p),
				     info_output[o]->GetSet(d)->GetYTitle(p));
	      datamap[id].push_back(dt);
	    }
	}
      itn++;
    }

  vector <TCanvas*> datapullscanvaslist;
  for (map <int, vector <dataseth> >::iterator it = datamap.begin(); it != datamap.end(); it++)
    datapullscanvaslist.push_back(DataPainter((*it).first, (*it).second));


  //Save plots
  system(((string)"mkdir -p " + opts.outdir).c_str());
  //open the file
  vector <TCanvas*>::iterator  it = pdfscanvaslist.begin();
  (*it)->Print((opts.outdir + "Plots.eps[").c_str());
  for (vector <TCanvas*>::iterator it = pdfscanvaslist.begin(); it != pdfscanvaslist.end();)
    {
      char numb[15];
      sprintf(numb, "%d", it - pdfscanvaslist.begin());
      TCanvas * pagecnv = new TCanvas(numb, "", 0, 0, opts.resolution * 3, opts.resolution * 3);
      pagecnv->Divide(3, 3);
      for (int i = 1; i <= 9; i++)
	{
	  pagecnv->cd(i);
	  (*it)->DrawClonePad();
	  it++;
	}
      pagecnv->Print((opts.outdir + "Plots.eps").c_str());

      sprintf(numb, "%d", it - pdfscanvaslist.begin());
      TCanvas * pagecnv2 = new TCanvas(numb, "", 0, 0, opts.resolution * 3, opts.resolution * 3);
      pagecnv2->Divide(3, 3);
      for (int i = 10; i <= pdflabels.size(); i++)
	{
	  pagecnv2->cd(i - 9);
	  (*it)->DrawClonePad();
	  it++;
	}
      pagecnv2->Print((opts.outdir + "Plots.eps").c_str());
    }
  for (vector <TCanvas*>::iterator it = pdfscanvasratiolist.begin(); it != pdfscanvasratiolist.end();)
    {
      char numb[15];
      sprintf(numb, "ratio_%d", it - pdfscanvasratiolist.begin());
      TCanvas * pagecnv = new TCanvas(numb, "", 0, 0, opts.resolution * 3, opts.resolution * 3);
      pagecnv->Divide(3, 3);
      for (int i = 1; i <= 9; i++)
	{
	  pagecnv->cd(i);
	  (*it)->DrawClonePad();
	  it++;
	}
      pagecnv->Print((opts.outdir + "Plots.eps").c_str());

      sprintf(numb, "ratio_%d", it - pdfscanvasratiolist.begin());
      TCanvas * pagecnv2 = new TCanvas(numb, "", 0, 0, opts.resolution * 3, opts.resolution * 3);
      pagecnv2->Divide(3, 3);
      for (int i = 10; i <= pdflabels.size(); i++)
	{
	  pagecnv2->cd(i - 9);
	  (*it)->DrawClonePad();
	  it++;
	}
      pagecnv2->Print((opts.outdir + "Plots.eps").c_str());
    }
  
  for (vector <TCanvas*>::iterator it = datapullscanvaslist.begin(); it != datapullscanvaslist.end();)
    {
      char numb[15];
      sprintf(numb, "data_%d", it - datapullscanvaslist.begin());
      TCanvas * pagecnv = new TCanvas(numb, "", 0, 0, opts.resolution * 2, opts.resolution * 2);
      pagecnv->Divide(1, 2);
      for (int i = 1; i <= 2; i++)
	if (it != datapullscanvaslist.end())
	  {
	    pagecnv->cd(i);
	    (*it)->DrawClonePad();
	    it++;
	  }
      pagecnv->Print((opts.outdir + "Plots.eps").c_str());
    }
  
  //close the file
  it = pdfscanvaslist.begin();
  (*it)->Print((opts.outdir + "Plots.eps]").c_str());

  if (opts.pdf)
    {
      cout << "Converting to pdf format..." << endl;
      system(((string)"ps2pdf " + opts.outdir + "Plots.eps " + opts.outdir + "Plots.pdf").c_str());
    }

  if (opts.splitplots)
    {
      for (vector <TCanvas*>::iterator it = pdfscanvaslist.begin(); it != pdfscanvaslist.end(); it++)
	(*it)->Print((opts.outdir + (*it)->GetName() + ".eps").c_str());
      for (vector <TCanvas*>::iterator it = pdfscanvasratiolist.begin(); it != pdfscanvasratiolist.end(); it++)
	(*it)->Print((opts.outdir + (*it)->GetName() + ".eps").c_str());
      for (vector <TCanvas*>::iterator it = datapullscanvaslist.begin(); it != datapullscanvaslist.end(); it++)
	(*it)->Print((opts.outdir + (*it)->GetName() + ".eps").c_str());
    }

  //Save all plots in a root file
  TFile * f = new TFile((opts.outdir + "plots.root").c_str(), "recreate");
  for (vector <TCanvas*>::iterator it = pdfscanvaslist.begin(); it != pdfscanvaslist.end(); it++)
    (*it)->Write();
  for (vector <TCanvas*>::iterator it = pdfscanvasratiolist.begin(); it != pdfscanvasratiolist.end(); it++)
    (*it)->Write();
  for (vector <TCanvas*>::iterator it = datapullscanvaslist.begin(); it != datapullscanvaslist.end(); it++)
    (*it)->Write();
  f->Close();

  return 0;
}
