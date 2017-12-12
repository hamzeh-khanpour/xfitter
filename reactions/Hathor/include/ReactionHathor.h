
#pragma once

#include "ReactionTheory.h"

/**
  @class' ReactionHathor

  @brief A wrapper class for Hathor reaction 

  Based on the ReactionTheory class. Reads options produces 3d cross section.

  @version 0.1
  @date 2017-08-07
  */

class Hathor;
class HathorPdfxFitter;

class ReactionHathor : public ReactionTheory
{
  public:
    ReactionHathor();

    ~ReactionHathor();

//    ~ReactionHathor(){};
//    ~ReactionHathor(const ReactionHathor &){};
//    ReactionHathor & operator =(const ReactionAHathor &r){return *(new ReactionHathor(r));};

  public:
    virtual string getReactionName() const { return  "Hathor" ;};
    virtual int initAtStart(const string &);
    virtual void setDatasetParamters(int dataSetID, map<string,string> pars, map<string,double> dsPars) override;
    virtual int compute(int dataSetID, valarray<double> &val, map<string, valarray<double> > &err);
  protected:
    virtual int parseOptions(){ return 0;};

    // this is map of key = dataset, value = pointer to Hathor instances,
    // one instance per one dataset
    std::map<int, Hathor*> _hathorArray;

    HathorPdfxFitter* _pdf;
    int* _rndStore;
    int _scheme;
    double _mtop;
    double _mr;
    double _mf;
};

