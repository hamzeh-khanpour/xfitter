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

  ///Get evolution by its name, creating it if needed
  //If name=="", returns default evolution
  BaseEvolution* get_evolution(std::string name="");  
  ///Get decomposition by its name, creating it if needed
  BasePdfDecomposition* get_pdfDecomposition(std::string name="");
  BasePdfParam*getParameterisation(const std::string&name="");
  ///Get minimizer
  BaseMinimizer*get_minimizer();

  //Call atConfiguration change for all evolutions and decompositions
  //Call this after changing some of configuration parameters
  //Used by Profiler
  void updateAtConfigurationChange();

  //When fortran code accesses pdfs, it accesses this default evolution
  extern BaseEvolution*defaultEvolution;
}