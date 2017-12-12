
#pragma once

#include "ReactionBaseHVQMNR.h"

/**
  @class' ReactionHVQMNR_LHCb_7TeV_charm

  @brief A wrapper class for HVQMNR_LHCb_7TeV_charm reaction 

  Based on the ReactionTheory class. Reads options produces 3d cross section.
  * 
  Derived from ReactionBaseHVQMNR where basic stuff for HVQMNR calculation is implemented.
  This class implements calculation for LHCb charm measurement at 7 TeV 
  [Nucl. Phys. B 871 (2013), 1] [arXiv:1302.2864]

  @version 0.1
  @date 2017-01-02
  */

class ReactionHVQMNR_LHCb_7TeV_charm : public ReactionBaseHVQMNR
{
  public:
    ReactionHVQMNR_LHCb_7TeV_charm(){};

  public:
    virtual string getReactionName() const { return  "HVQMNR_LHCb_7TeV_charm" ;};
    virtual int initAtStart(const string &); 
    virtual void initAtIteration();
    virtual int compute(int dataSetID, valarray<double> &val, map<string, valarray<double> > &err);
  protected:
    virtual int parseOptions(){ return 0;};
};

