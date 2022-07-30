#include "Pion_FF_PdfParam.h"
#include "xfitter_cpp_base.h"
#include <cmath>
#include <cassert>
#include <iostream>
using namespace std;


namespace xfitter{
//for dynamic loading
extern"C" Pion_FF_PdfParam*create(const char*name){
  return new Pion_FF_PdfParam(name);
}
void Pion_FF_PdfParam::atStart(){
  using namespace std;
  BasePdfParam::atStart();
  const size_t n=getNPar();
  if(n<3){
    cerr<<"[ERROR] Too few parameters given to parameterisation \""<<_name<<"\", expected at least 3, got "<<n<<endl;
    hf_errlog(18120700,"F: Wrong number of parameters for a parameterisation, see stderr");
  }
  atIteration();
}
void Pion_FF_PdfParam::atIteration(){
//      cout<<"########hello hamed PDF_par01#############";
//  updateNormalization();
}
///void Pion_FF_PdfParam::updateNormalization(){
  //norm=A/beta(B+1,C+1)
///  const double b=*pars[1]+2;
 /// const double c=*pars[2]+1;
 /// const double d=*pars[3];
 /// const double e=*pars[2] + *pars[4]+1;
  //cout<<"##hello hamed PDF_par01##b="<< *pars[1] <<"##c="<< *pars[2] <<"##d="<< *pars[3]<< "##e="<< e<<"\n";

   //  cout<<"########hello hamed PDF_par1########"<< *pars[2] <<"##"<< *pars[4] <<"##"<< d;
 /// norm=*pars[0]/(exp(lgamma(b)+lgamma(c)-lgamma(b+c)) + d*exp(+lgamma(b)+lgamma(e)-lgamma(b+e)));
  //norm = 1;
  //cout<<"########NORM PDF_par1#######A0="<< *pars[0]<<"norm"<< norm<<"***\n";

///}
double Pion_FF_PdfParam::operator()(double x)const{
  const unsigned int npar=getNPar();
  const double b=*pars[1]+2;
  const double c=*pars[2]+1;
  const double d=*pars[3];
  const double e=*pars[2] + *pars[4]+1;
  //cout<<"##hello hamed PDF_par01##b="<< *pars[1] <<"##c="<< *pars[2] <<"##d="<< *pars[3]<< "##e="<< e<<"\n";

   //  cout<<"########hello hamed PDF_par1########"<< *pars[2] <<"##"<< *pars[4] <<"##"<< d;
  double norm = *pars[0]/(exp(lgamma(b)+lgamma(c)-lgamma(b+c)) + d*exp(lgamma(b)+lgamma(e)-lgamma(b+e)));

  double power= x*pow(x,(*pars[1]))*pow((1-x),(*pars[2]));
  double poly = 1;
  double xx = 1;
  //cout<<"#####power PDF_par1######x== "<< x << " A0= " << *pars[0]<<" power "<< power<<" ***\n";

//  cout<<"########hello hamed PDF_par2##########"<< poly<<"***\n";

//  for (unsigned int i = 3; i<npar; i++) {
   // xx *= x;
//    poly+=(*pars[i])*xx;
    poly= 1 + (*pars[3])*pow((1-x) , (*pars[4])) ;
    //cout<<"########hello hamed PDF_par1#######x== "<< x << " A0= "<< *pars[0]<<" poly "<< poly<<" ***\n";

    //poly = 1;
    //cout<<"########hello hamed PDF_par2##########"<< norm<<"###"<<poly<<"###"<<power<<"***";
    //cout<<"########hello hamed PDF_par1#######x== "<< x << " A0= "<< *pars[0]<<" pdf "<< norm*power*poly<<" ***\n";



//  }
  return norm*power*poly;
}
//double Pion_FF_PdfParam::moment(int n)const{
// ;
//}
}
