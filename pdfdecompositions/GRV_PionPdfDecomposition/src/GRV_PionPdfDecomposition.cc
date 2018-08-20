/*
   @file GRV_PionPdfDecomposition.cc
   @date 2018-08-07
   @author  AddPdfDecomposition.py
   Created by  AddPdfDecomposition.py on GRV_Pion
*/
#include"GRV_PionPdfDecomposition.h"
#include"PolySqrtPdfParam.h" //This is for hacks, remove later
#include"xfitter_pars.h"
using uint=unsigned int;
using namespace std;
namespace xfitter{
//For dynamic loading:
extern "C" GRV_PionPdfDecomposition*create(){
	return new GRV_PionPdfDecomposition();
}
GRV_PionPdfDecomposition::GRV_PionPdfDecomposition():BasePdfDecomposition("GRV_Pion"){
	par_v   =nullptr;
	par_qbar=nullptr;
	par_g   =nullptr;
}
GRV_PionPdfDecomposition::GRV_PionPdfDecomposition(const std::string& inName):BasePdfDecomposition(inName){}
GRV_PionPdfDecomposition::~GRV_PionPdfDecomposition(){
	if(par_v   )delete par_v;
	if(par_qbar)delete par_qbar;
	if(par_g   )delete par_g;
}
// Init at start:
void GRV_PionPdfDecomposition::initAtStart(const std::string & pars){
	//HARDCODE copied from UvDvUbarDbarS, then modified
	//The following is not very nice: Decomposition should not create or initialize parameterisations
	//Create and initialize paramterisations
	for(const auto&node:XFITTER_PARS::gParametersY.at("GRV_PionPdfDecomposition")["HARDWIRED_PolySqrt"]){
		const string prmzName=node.first.as<string>();//Name of parameterisation
		BasePdfParam*pParam=new PolySqrtPdfParam(prmzName);
		pParam->initFromYaml(node.second);
		addParameterisation(prmzName,pParam);
	}
	par_v   =getPdfParam("v");
	par_qbar=getPdfParam("qbar");
	par_g   =getPdfParam("g");
}
void GRV_PionPdfDecomposition::initAtIteration() {
	//Enforce sum rules
	double fs=*XFITTER_PARS::gParameters.at("fs");
	//Valence sum
	par_v->setMoment(-1,2);
	//Momentum sum
	par_g->setMoment(0,1-4*par_qbar->moment(0)/(1-fs)-par_v->moment(0));
}
// Returns a LHAPDF-style function, that returns PDFs in a physical basis for given x
std::function<std::map<int,double>(const double& x)>GRV_PionPdfDecomposition::f0()const{
	return [=](double const& x)->std::map<int, double>{
		double fs=*XFITTER_PARS::gParameters.at("fs");
		double v   =(*par_v)(x);
		double qbar=(*par_qbar)(x);
		double g   =(*par_g)(x);
		double u=qbar;
		double d=qbar+v/2;
		double s=2*fs/(1-fs)*qbar;
		std::map<int,double>res_={
			{-6,0},
			{-5,0},
			{-4,0},
			{-3,s},//sbar
			{-2,d},//ubar
			{-1,u},//dbar
			{ 1,d},
			{ 2,u},
			{ 3,s},
			{ 4,0},
			{ 5,0},
			{ 6,0},
			{21,g}
		};
		return res_;
	};
}
}
