 
/*
   @file MINUITMinimizer.cc
   @date 2018-08-17
   @author  AddMinimizer.py
   Created by  AddMinimizer.py on 2018-08-17
*/

#include "MINUITMinimizer.h"
#include "FTNFitPars.h"          // tools to handle fortran minuit
#include "xfitter_pars.h"

/// Fortran interfaces:
extern "C" {
  void generate_io_filenames_();
  void minuit_ini_();
  void minuit_(void fcn(const int&, const double&, double&, const double*, const int& , const double& ), int const& i);
  // FCN
  void fcn_(const int& npar, const double& dummy, double& chi2out, const double* pars, const int& iflag, const double& dummy2);  

  /// Interface to minuit parameters
  void addexternalparam_(const char name[],  const double &val, 
                         const double  &step,
                         const double &min, const double &max, 
                         const double &prior, const double &priorUnc,
                         const int &add, 
                         map<std::string,double*> *map,
                         int len);


}

namespace xfitter {
  
/// the class factories, for dynamic loading
extern "C" MINUITMinimizer* create() {
    return new MINUITMinimizer();
}


// Constructor
MINUITMinimizer::MINUITMinimizer() : BaseMinimizer("MINUIT") {  
}

// Constructor
MINUITMinimizer::MINUITMinimizer(const std::string& inName) : BaseMinimizer(inName) 
{  
}

// Init at start:
void MINUITMinimizer::initAtStart() {
  // Call fortran interface
  generate_io_filenames_();
  minuit_ini_();
  return;
}

/// Miniminzation loop
void MINUITMinimizer::doMimimization() 
{
  minuit_(fcn_,0);  // let fortran run ...
  return;
}

/// Action at last iteration 
void MINUITMinimizer::actionAtFCN3() 
{
  double**p = getPars();
  for (size_t i=0; i<getNpars(); i++) {
    std::cout << i << " " << *p[i] << "\n"; 
  }
  //  exit(0);
  return;
}

/// Error analysis
void MINUITMinimizer::errorAnalysis() 
{
    return;
}

/// parameters  
void MINUITMinimizer::addParameter(double par, std::string const &name, double step, double const* bounds , double  const* priors  )
{
  BaseMinimizer::addParameter(par,name,step,bounds,priors);
  double minv    =0;
  double maxv    =0;
  double priorVal=0;
  double priorUnc=0;
  int add = true;

  if ( bounds != nullptr ) {
    minv = bounds[0];
    maxv = bounds[1];
  }
  
  addexternalparam_(name.c_str(),par,step,minv,maxv,priorVal,priorUnc,add,&XFITTER_PARS::gParameters,name.size());
  
  return;
}

  
} //namespace xfitter

