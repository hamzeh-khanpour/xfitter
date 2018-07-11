#include "BasePdfParam.h"
#include <cmath>

/// Implement numeric integration
double BasePdfParam::moment( double const* pars, int const iMoment) {
  /// Simple rule, split log/lin spacing at xsplit=0.1

  const double xsplit = 0.1;
  const double xmin = 1e-6;
  const double xminlog = log10(xmin);
  const double xmaxlog = log10(xsplit);
  const int nlog = 100;
  const int nlin = 100;

  const double xsteplog = (xmaxlog-xminlog)/nlog;
  const double xsteplin = (1.0 - xsplit)/nlin;
  
  double sum = 0.;
  double x = xmin;

  // log x part:
  for (int i=0; i<nlog; i++) {
    double dx = pow(10,xminlog+(i+1)*xsteplog) - pow(10,xminlog+i*xsteplog);
    double val = compute(x+dx/2.,pars)*pow(x+dx/2.,iMoment);
    x += dx;
    sum += dx*val;
  }
  // lin x part:
  for (int i=0; i<nlin; i++) {
    double dx = xsteplin;
    double val = compute(x+dx/2.,pars)*pow(x+dx/2.,iMoment);
    x += dx;
    sum += dx*val;
  }
  
  return sum;
}



