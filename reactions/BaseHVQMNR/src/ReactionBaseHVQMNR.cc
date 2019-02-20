#include "xfitter_cpp.h"
#include <TMath.h>
#include <string>

/*
   @file ReactionBaseHVQMNR.cc
   @date 2017-01-02
   @author Oleksandr Zenaiev (oleksandr.zenaiev@desy.de)
   Created by  AddReaction.py on 2017-01-02
   *
   This is abstract class from which implementations of HVQMNR
   calculations for particular datasets should be derived
*/

#include "ReactionBaseHVQMNR.h"

// the class factories
extern "C" ReactionBaseHVQMNR* create() {
  // this is abstract class, no instance should be created
  //return new ReactionBaseHVQMNR();
  hf_errlog(16123000, "F: can not create ReactionBaseHVQMNR instance: you should implement calculation in a derived class");
  return NULL;
}


// pass to MNR pointer to instance inherited from ReactionTheory to allow access to alphas and PDF routines
ReactionBaseHVQMNR::ReactionBaseHVQMNR() : _mnr(MNR::MNR(this))
{
  //printf("OZ ReactionBaseHVQMNR::ReactionBaseHVQMNR()\n");
  // set initialisation status flag
  _isInitAtStart = false;
  // set debugging flag
  _debug = steering_.ldebug;
  //_debug = 1;
}


ReactionBaseHVQMNR::~ReactionBaseHVQMNR()
{
  //printf("OZ ReactionBaseHVQMNR::~ReactionBaseHVQMNR()\n");
  for(unsigned int i = 0; i < _hCalculatedXSec.size(); i++)
    delete _hCalculatedXSec[i];
}


void ReactionBaseHVQMNR::setDatasetParameters(int dataSetID, map<string,string> pars, map<string,double> dsPars)
{
  // add new dataset
  DataSet dataSet;
  std::pair<std::map<int, DataSet>::iterator, bool> ret = _dataSets.insert(std::pair<int, DataSet>(dataSetID, dataSet));
  // check if dataset with provided ID already exists
  if(!ret.second)
    hf_errlog(16123001, "F: dataset with id = " + std::to_string(dataSetID) + " already exists");

  // set parameters for new dataset
  DataSet& ds = ret.first->second;
  // mandatory "FinalState="
  map<string,string>::iterator it = pars.find("FinalState");
  if(it == pars.end())
    hf_errlog(16123002, "F: TermInfo must contain FinalState entry");
  else
    ds.FinalState = it->second;

  // optional "NormY="
  it = pars.find("NormY");
  if(it == pars.end())
    ds.NormY = 0; // default value is unnormalised absolute cross section
  else
    ds.NormY = atoi(it->second.c_str());

  // optional "FragFrac=" (must be provided for absolute cross section)
  it = pars.find("FragFrac");
  if(it == pars.end())
  {
    if(ds.NormY == 0)
      hf_errlog(16123003, "F: for absolute cross section TermInfo must contain FragFrac entry");
  }
  else
  {
    ds.FragFraction = atof(it->second.c_str());
    if(ds.NormY != 0)
      printf("Warning: FragFrac=%f will be ignored for normalised cross sections\n", ds.FragFraction);
  }

  // set binning
  ds.BinsYMin  = GetBinValues(dataSetID, "ymin");
  ds.BinsYMax  = GetBinValues(dataSetID, "ymax");
  ds.BinsPtMin = GetBinValues(dataSetID, "pTmin");
  ds.BinsPtMax = GetBinValues(dataSetID, "pTmax");
  //if (ds.BinsYMin == NULL || ds.BinsYMax == NULL || ds.BinsPtMin == NULL || ds.BinsPtMax == NULL )
  //  hf_errlog(16123004, "F: No bins ymin or ymax or ptmin or ptmax");
  // set reference y bins if needed
  if(ds.NormY == 1)
  {
    ds.BinsYMinRef = GetBinValues(dataSetID, "yminREF");
    ds.BinsYMaxRef = GetBinValues(dataSetID, "ymaxREF");
    if(ds.BinsYMinRef == NULL || ds.BinsYMaxRef == NULL)
      hf_errlog(16123005, "F: No bins yminREF or ymaxREF for normalised cross section");
  }

  if(_debug)
    printf("Added dataset: FinalState = %s  NormY = %d  FragFraction = %f\n", ds.FinalState.c_str(), ds.NormY, ds.FragFraction);
}


// ********************************
// ***** utility routines *********
// ********************************

// check equality of float numbers with tolerance
bool ReactionBaseHVQMNR::IsEqual(const double val1, const double val2, const double eps/* = 1e-6*/)
{
  return (TMath::Abs(val1 - val2) < eps || TMath::Abs((val1 - val2) / val1) < eps);
}

// initialise calculation with default parameters
void ReactionBaseHVQMNR::DefaultInit(const Steering& steer, const double mq, MNR::MNR& mnr, MNR::Frag& frag, MNR::Grid& grid, MNR::Grid& grid_smoothed)
{
  // MNR parton level cross sections, quark-antiquark contributions
  mnr.bFS_Q = steer.q;
  mnr.bFS_A = steer.a;
  // number of light flavours
  mnr.fC_nl = steer.nf;
  // x3 and x4 binning
  mnr.fBn_x3 = steer.nx3;
  mnr.fBn_x4 = steer.nx4;
  mnr.fSF_nb = steer.nsfnb;
  // PDF range
  mnr.fSF_min_x = steer.xmin;
  mnr.fSF_max_x = steer.xmax;
  mnr.fSF_min_mf2 = steer.mf2min;
  mnr.fSF_max_mf2 = steer.mf2max;
  // precalculation (memory allocation etc.)
  mnr.CalcBinning();
  // Parton level pT-y grids
  grid.SetL(steer.npt, steer.ptmin, steer.ptmax, mq);
  grid.SetY(steer.ny, steer.ymin, steer.ymax);
  grid.SetW(1);
  grid_smoothed.SetL(steer.nptsm, steer.ptmin, steer.ptmax, mq);
  grid_smoothed.SetY(steer.ny, steer.ymin, steer.ymax);
  grid_smoothed.SetW(1);
  // Fragmentation
  frag.SetNz(steer.nbz);
}

// return cross section in provided pT-y bin
double ReactionBaseHVQMNR::FindXSecPtYBin(const TH2* histXSec, const double ymin, const double ymax, const double ptmin, const double ptmax, const bool diff_pt, const bool diff_y)
{
  // **************************************************************************
  // Input parameters:
  //   histXSec: 2D historam with calculated cross section (x axis pT, y axis rapidity)
  //   ymin: min rapidity of required bin
  //   ymax: max rapidity of required bin
  //   ptmin: min pT of required bin
  //   ptmax: max pT of required bin
  //   d_pt: if TRUE, cross section is divided by pT bin width
  //   d_y: if TRUE, cross section is divided by rapidity bin width
  // **************************************************************************
  //printf("FindXSecPtYBin: y %f %f pT %f %f\n", ymin, ymax, ptmin, ptmax);
  //histXSec->Print("all");
  for(int bpt = 0; bpt < histXSec->GetNbinsX(); bpt++)
  {
    //printf("bpt = %d\n", bpt);
    for(int by = 0; by < histXSec->GetNbinsY(); by++)
    {
      //printf("by = %d\n", by);
      if(!IsEqual(ymin, histXSec->GetYaxis()->GetBinLowEdge(by + 1))) continue;
      if(!IsEqual(ymax, histXSec->GetYaxis()->GetBinUpEdge(by + 1))) continue;
      if(!IsEqual(ptmin, histXSec->GetXaxis()->GetBinLowEdge(bpt + 1))) continue;
      if(!IsEqual(ptmax, histXSec->GetXaxis()->GetBinUpEdge(bpt + 1))) continue;
      double val = histXSec->GetBinContent(bpt + 1, by + 1);
      // check if XS is not nan
      if(val != val)
      {
        printf("Warning: nan cross section in bin y: %f %f, pt: %f %f, resetting to -1000000000.0\n", ymin, ymax, ptmin, ptmax);
        val = -1000000000.0;
      }
      // divide by bin width if needed
      if(diff_pt)
        val = val / (ptmax - ptmin);
      if(diff_y)
        val = val / (ymax - ymin);
      return val;
    }
  }
  hf_errlog(16123006, "F: ERROR in FindXSPtY(): bin not found y: " + std::to_string(ymin) + " " + std::to_string(ymax) +
            " , pt: " + std::to_string(ptmin) + " " + std::to_string(ptmax));
  return -1.0;
}

// check if appropriate heavy-flavour scheme is used
void ReactionBaseHVQMNR::CheckHFScheme()
{
  // check HF scheme
  if(steering_.hfscheme != 3 && steering_.hfscheme != 4)
    hf_errlog(16123007, "S: calculation does not support HFSCHEME = " + std::to_string(steering_.hfscheme) + " (only 3, 4 supported)");
}

// read parameters for perturbative scales from MINUIT extra parameters
void ReactionBaseHVQMNR::GetMuPar(const char mu, const char q, double& A, double& B, double& C, const map<std::string, std::string> pars)
{
  // ***********************************************************************************************
  // Scales for charm and beauty production are parametrised as:
  // mu_f(c)^2 = MNRmf_A_c * pT_c^2 + MNRmf_B_c * m_c^2 + MNRmf_C_c
  // mu_r(c)^2 = MNRmr_A_c * pT_c^2 + MNRmr_B_c * m_c^2 + MNRmr_C_c
  // mu_f(b)^2 = MNRmf_A_b * pT_b^2 + MNRmf_B_b * m_b^2 + MNRmf_C_b
  // mu_r(b)^2 = MNRmr_A_b * pT_b^2 + MNRmr_B_b * m_b^2 + MNRmr_C_b
  // where mu_f(c), mu_r(c), mu_f(b), mu_r(b) are factorisation and renormalisation
  // scales for charm and beauty production, respectively, pT is transverse momentum
  // and m_c, m_b are charm and beauty quark masses.
  //
  // In total, one can provide all 12 parameters (MNRmf_A_c, MNRmf_B_c, MNRmf_C_c,
  // MNRmr_A_c, MNRmr_B_c, MNRmr_C_c, MNRmf_A_b, MNRmf_B_b, MNRmf_C_b,
  // MNRmr_A_b, MNRmr_B_b, MNRmr_C_b), however there are the foolowing rules:
  // 1) if suffix _c (_b) at the end of variable name is omitted, the parameter is applied
  //    for both charm and beauty production
  // 2) instead of providing e.g. MNRmr_A_c and MNRmr_B_c the user can provide one
  //    parameter MNRmr_AB_c so then MNRmr_A_c = MNRmr_B_c = MNRmr_AB_c
  // 3) if parameters *_C_* are not provided, they are set to 0
  // So e.g. for charm production only it is enough to provide just the following two parameters
  // MNRmr_AB and MNRmf_AB.
  // input variables:
  //   mu should be either 'f' or 'r' (for factorisation or renormalisation scale, respectively)
  //   q should be either 'c' or 'b' (for charm or beauty, respectively)
  // ***********************************************************************************************

  // ***************************
  const double defA = 1.0;
  const double defB = 1.0;
  const double defC = 0.0;
  // ***************************
  std::string baseParameterName = "MNRm" + std::string(1, mu);

  // A and B parameters
  if(GetParamInPriority(baseParameterName + "_AB", pars))
    A = B = GetParamInPriority(baseParameterName + "_AB", pars);
  else
  {
    if(checkParamInPriority(baseParameterName + "_A", pars) && checkParamInPriority(baseParameterName + "_B", pars))
    {
      A = GetParamInPriority(baseParameterName + "_A", pars);
      B = GetParamInPriority(baseParameterName + "_B", pars);
    }
    else
    {
      if(checkParamInPriority(baseParameterName + "_AB_" + std::string(1, q), pars))
        A = B = GetParamInPriority(baseParameterName + "_AB_" + std::string(1, q), pars);
      else
      {
        if(checkParamInPriority(baseParameterName + "_A_" + std::string(1, q), pars))
          A = GetParamInPriority(baseParameterName + "_A_" + std::string(1, q), pars);
        else
          A = defA;
        if(checkParamInPriority(baseParameterName + "_B_" + std::string(1, q), pars))
          B = GetParamInPriority(baseParameterName + "_B_" + std::string(1, q), pars);
        else
          B = defB;
      }
    }
  }
  // C parameter
  if(checkParamInPriority(baseParameterName + "_C", pars))
    C = GetParamInPriority(baseParameterName + "_C", pars);
  else
  {
    if(checkParamInPriority(baseParameterName + "_C_" + std::string(1, q), pars))
      C = GetParamInPriority(baseParameterName + "_C_" + std::string(1, q), pars);
    else
      C = defC;
  }
}


// read fragmentation parameter from MINUIT extra parameters
double ReactionBaseHVQMNR::GetFragPar(const char q, const map<string,string> pars)
{
  // *********************************************************************
  // Parameters for non-perturbative fragmentation can be provided
  // as MINUIT extra parameters MNRfrag_c or MNRfrag_b
  // for charm and beauty production, respectively.
  // q should be either 'c' or 'b' (for charm or beauty, respectively)
  // *********************************************************************

  // ***************************
  const double defFFc = 4.4;
  const double defFFb = 11.0;
  // ***************************
  double parvalue = NAN;
  char parname[16];
  sprintf(parname, "MNRfrag_%c", q);
  if(!checkParamInPriority(parname, pars))
  {
    // parameter not in ExtraParamMinuit -> using default value
    if(q == 'c')
      parvalue = defFFc;
    else if(q == 'b')
      parvalue = defFFb;
    else
      hf_errlog(17102103, "F: no default value for q = " + std::string(1, q) + " in ReactionBaseHVQMNR::GetFragPar()");
  }
  else
    parvalue = GetParamInPriority(parname, pars);

  // TODO check below whether it is still relevant
  /*      ! parameter in ExtraParamMinuit, but not in MINUIT: this happens, if we are not in 'Fit' mode -> using default value
        if(st.lt.0) then
          if(q.eq.'c') then
            FFpar=defFFc
          else if(q.eq.'b') then
            FFpar=defFFb
          else
            write(*,*)'Warning in GetFPar(): no default value for q = ',q
            call makenan(FFpar)
          endif
        endif
      endif
      end*/
  return parvalue;
}

// read and update theory parameters
void ReactionBaseHVQMNR::UpdateParameters()
{
  // heavy-quark masses
  _pars.mc = fermion_masses_.mch;
  _pars.mb = fermion_masses_.mbt;
  // scale parameters
  GetMuPar('f', 'c', _pars.mf_A_c, _pars.mf_B_c, _pars.mf_C_c);
  GetMuPar('r', 'c', _pars.mr_A_c, _pars.mr_B_c, _pars.mr_C_c);
  GetMuPar('f', 'b', _pars.mf_A_b, _pars.mf_B_b, _pars.mf_C_b);
  GetMuPar('r', 'b', _pars.mr_A_b, _pars.mr_B_b, _pars.mr_C_b);
  // fragmentation parameters
  _pars.fragpar_c = GetFragPar('c');
  _pars.fragpar_b = GetFragPar('b');
}

// print theory parameters
void ReactionBaseHVQMNR::PrintParameters(Parameters const* pars) const
{
  if(pars == NULL)
    pars = &(this->_pars);
  printf("MNR scale parameters:\n");
  printf("%f  %f  %f\n", pars->mf_A_c, pars->mf_B_c, pars->mf_C_c);
  printf("%f  %f  %f\n", pars->mr_A_c, pars->mr_B_c, pars->mr_C_c);
  printf("%f  %f  %f\n", pars->mf_A_b, pars->mf_B_b, pars->mf_C_b);
  printf("%f  %f  %f\n", pars->mr_A_b, pars->mr_B_b, pars->mr_C_b);
  printf("MNR masses:\n");
  printf("mc = %f  mb = %f\n", pars->mc, pars->mb);
  printf("MNR fragmentation parameters:\n");
  printf("fragpar_c = %f  fragpar_b = %f\n", pars->fragpar_c, pars->fragpar_b);
}
