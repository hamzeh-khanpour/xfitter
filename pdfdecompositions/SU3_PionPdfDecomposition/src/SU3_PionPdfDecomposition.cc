/*
   @file SU3_PionPdfDecomposition.cc
   @date 2018-08-07
   @author  AddPdfDecomposition.py
   Created by  AddPdfDecomposition.py on SU3_Pion
*/
#include"xfitter_cpp_base.h"
#include<iostream>
#include"SU3_PionPdfDecomposition.h"
#include"xfitter_pars.h"
#include"xfitter_steer.h"
using uint=unsigned int;
using namespace std;
//For dynamic loading:
namespace xfitter{
extern "C" SU3_PionPdfDecomposition*create(const char*name){
  return new SU3_PionPdfDecomposition(name);
}
SU3_PionPdfDecomposition::SU3_PionPdfDecomposition(const char*name):BasePdfDecomposition{name}{}
const char*SU3_PionPdfDecomposition::getClassName()const{return"SU3_Pion";}
BasePdfParam*getParam(const BasePdfDecomposition*self,const YAML::Node&node,const char*s){
  try{
    return getParameterisation(node[s].as<string>());
  }catch(const YAML::InvalidNode&ex){
    if(node[s].IsNull()){
      cerr<<"[ERROR] No \""<<s<<"\" parameterisation given for decomposition \""<<self->_name<<"\""<<endl;
      hf_errlog(18092410,"F: Error in decomposition parameters, details written to stderr");
    }else throw ex;
  }catch(const YAML::BadConversion&ex){
    cerr<<"[ERROR] Bad parameter \""<<s<<"\" given for decomposition \""<<self->_name<<"\""<<endl;
    hf_errlog(18092410,"F: Error in decomposition parameters, details written to stderr");
  }
  return nullptr;//unreachable, suppress warning
}
// Init at start:
void SU3_PionPdfDecomposition::atStart(){
  const YAML::Node node=XFITTER_PARS::getDecompositionNode(_name);
  //get parameterisation usually doesn't throw
  par_v=getParam(this,node,"valence");
  par_S=getParam(this,node,"sea");
  par_g=getParam(this,node,"gluon");
}
void SU3_PionPdfDecomposition::atIteration() {
  //Enforce sum rules
  //Valence sum
  par_v->setMoment(-1,1);
  //Momentum sum
  par_g->setMoment(0,1-6*par_S->moment(0)-2*par_v->moment(0));
}
// Returns a LHAPDF-style function, that returns PDFs in a physical basis for given x
std::function<std::map<int,double>(const double& x)>SU3_PionPdfDecomposition::f0()const{
  return [=](double const& x)->std::map<int, double>{
    double v=(*par_v)(x);
    double S=(*par_S)(x);
    double g=(*par_g)(x);
    double d=S+v;
    std::map<int,double>res_={
      {-6,0},
      {-5,0},
      {-4,0},
      {-3,S},//sbar
      {-2,d},//ubar
      {-1,S},//dbar
      { 1,d},//d
      { 2,S},//u
      { 3,S},//s
      { 4,0},
      { 5,0},
      { 6,0},
      {21,g}
    };
    return res_;
  };
}
}
