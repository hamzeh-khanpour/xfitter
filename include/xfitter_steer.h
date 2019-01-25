#pragma once

#include <string>
/**
     @class xfitter_steer

     @brief basic building blocks of xFitter running sequence

     @version 0.1
     @date 2018-07-13
*/

namespace xfitter
{
  class BaseEvolution;
  class BasePdfDecomposition;
  class BasePdfParam;
  class BaseMinimizer;

  /// Load named evolution code.  
  BaseEvolution* get_evolution(std::string name="");  
  /// Load named pdfDecomposition code.  
  BasePdfDecomposition* get_pdfDecomposition(std::string name="");
  BasePdfParam*getParameterisation(const std::string&name="");
  /// Load the minimizer
  BaseMinimizer* get_minimizer();

  //When fortran code accesses pdfs, it accesses this default evolution
  extern BaseEvolution*defaultEvolution;
}
