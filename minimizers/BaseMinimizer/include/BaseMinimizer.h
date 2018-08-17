#pragma once

#include <string>
#include <vector>

/**
  @class BaseMinimizer

  @brief Base class to develop interfaces to mimimizers.

  Contains methods to arrange mimimization loop (including lhapdf profiling)

  @version 0.1
  @date 2018-07-25
  */

namespace xfitter
{
  
  class BaseMinimizer {
  public:
    /// default constructor
    BaseMinimizer(const std::string& name) :        _name(name),
      _allParameterNames()
	{}

    /// Initialization
    virtual void initAtStart() = 0;

    /// Provide some information
    virtual void printInfo(){};

    /// Add parameter blocks with Npar parameters. Optional steps bounds and priors can be used for fixed/bounded parameters.
    virtual void addParameterBlock(int Npar, double const* pars
				   , std::string const* names
				   , double const* steps   = nullptr
				   , double const* const* bounds = nullptr
				   , double const* const* priors = nullptr );

    /// Add single parameter. Optional flags and bounds can be used for fixed/bounded parameters.
    virtual void addParameter(double par, std::string const &name, double step = 0.01, double const* bounds = nullptr, double  const* priors = nullptr );
    
    /// Action at each iteration
    virtual void initAtIteration(){};

    /// Miniminzation loop
    virtual void doMimimization() = 0;
    
    /// Perform optional action when equivalent to minuit fcn3 is called (normally after fit)
    virtual void actionAtFCN3(){};

    /// Perform post-fit error analysis
    virtual void errorAnalysis(){};

    /// Number of parameters:
    unsigned int getNpars() const { return _allParameterNames.size(); }

  protected:
    /// Retrieve 
    double** getPars() const ;
    
    /// name to ID minimizer
    std::string _name;

    /// names of the parameters
    std::vector<std::string> _allParameterNames;
    
  };

  /// For dynamic loader
  typedef BaseMinimizer* create_minimizer();

}
